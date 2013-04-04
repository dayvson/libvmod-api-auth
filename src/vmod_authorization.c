#include <stdlib.h>
#include <stdio.h>
#define MONGO_HAVE_STDINT
#include "mongo.h"
#include "vrt.h"
#include "bin/varnishd/cache.h"

#include "vcc_if.h"

static char* get_user_by_token( mongo *conn, const char* token ) {
  bson query[1];
  mongo_cursor cursor[1];
  const char* COLLECTION = "test.keys";
  bson_init( query );
  bson_append_string( query, "token", token );
  bson_finish( query );

  mongo_cursor_init( cursor, conn, COLLECTION );
  mongo_cursor_set_query( cursor, query );

  while( mongo_cursor_next( cursor ) == MONGO_OK ) {
    bson_iterator iterator[1];
    if ( bson_find( iterator, mongo_cursor_bson( cursor ), "user" )) {
       return bson_iterator_string( iterator );
    }
  }

  bson_destroy( query );
  mongo_cursor_destroy( cursor );
  return "";
}

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	return (0);
}

const char *
vmod_get_authorization(struct sess *sp, const char *token)
{
	const char* HOST = "127.0.0.1";
	

	mongo conn[1];
  	int status = mongo_client( conn, HOST, 27017 );
  	
	char *p;
	unsigned u, v;

	u = WS_Reserve(sp->wrk->ws, 0); /* Reserve some work space */
	p = sp->wrk->ws->f;		/* Front of workspace area */
	v = snprintf(p, u, get_user_by_token( conn, token ), token);
	mongo_destroy( conn );
	v++;
	if (v > u) {
		/* No space, reset and leave */
		WS_Release(sp->wrk->ws, 0);
		return (NULL);
	}
	/* Update work space with what we've used */
	WS_Release(sp->wrk->ws, v);
	return (p);
}