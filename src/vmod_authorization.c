#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#define MONGO_HAVE_STDINT
#include "mongo.h"
#include "vrt.h"
#include "bin/varnishd/cache.h"

#include "vcc_if.h"

typedef struct mongoConfig {
    char *host;
    int port;
    char *collection;
    mongo conn[1];
} config_t;

typedef struct authHeader {
    char *scheme;
    char *token;
    char *signature;
    char *xdate;
} auth_header;

#define LOG_E(...) fprintf(stderr, __VA_ARGS__);
#ifdef DEBUG
#   define  LOG_T(...) fprintf(stderr, __VA_ARGS__);
#else
#   define  LOG_T(...) do {} while(0);
#endif

static auth_header *
parse_header(const char* authorization_str){
    const char* delimiters = " :";
    const char* default_scheme = "NYTV";
    char *tmp_copy;
    auth_header * _header;
    _header = malloc(sizeof(auth_header));
    tmp_copy = strdup(authorization_str);
    _header->scheme = strtok(tmp_copy, delimiters);
    if(strncmp(_header->scheme, default_scheme, 100) != 0){
        return NULL; 
    }
    _header->token = strtok(NULL, delimiters);
    _header->signature = strtok(NULL, delimiters);
    return _header;
}

static config_t *
make_config(const char *host, int port, const char* collection)
{
    config_t *cfg;

    LOG_T("make_config(%s,%d,%s)\n", host, port, collection);

    cfg = malloc(sizeof(config_t));

    if(cfg == NULL){
        return NULL;
    }
        
    cfg->host = strdup(host);
    cfg->port = port;
    cfg->collection = collection;
    mongo_client(cfg->conn, cfg->host, cfg->port);
    return cfg;
}

static char * 
get_user_by_token( config_t *cfg, const char* token ) {
    bson query[1];
    mongo_cursor cursor[1];
    bson_init( query );
    bson_append_string( query, "token", token);
    bson_finish( query );

    mongo_cursor_init( cursor, cfg->conn, cfg->collection );
    mongo_cursor_set_query( cursor, query );

    while( mongo_cursor_next( cursor ) == MONGO_OK ) {
        bson_iterator iterator[1];
        if ( bson_find( iterator, mongo_cursor_bson( cursor ), "user" )) {
            return bson_iterator_string( iterator );
        }
    }

    bson_destroy( query );
    mongo_cursor_destroy( cursor );
    return NULL;
}

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
    if (priv->priv == NULL) {
        priv->priv = make_config("127.0.0.1", 27017, "test.cherry");
        priv->free = free;
    }
	return (0);
}

void 
vmod_dbconnect(struct sess *sp, struct vmod_priv *priv, const char* host, int port, const char* collection){
    config_t *old_cfg = priv->priv;

    priv->priv = make_config(host, port, collection);
    if(priv->priv && old_cfg) {
        free(old_cfg->host);
        free(old_cfg);
    }

}

const char *
vmod_get_credentials(struct sess *sp, struct vmod_priv *priv, const char *token)
{
	char *p;
	unsigned u, v;
    config_t *cfg = priv->priv;
    auth_header * _header = parse_header(token);

	u = WS_Reserve(sp->wrk->ws, 0); /* Reserve some work space */
	p = sp->wrk->ws->f;		/* Front of workspace area */
	v = snprintf(p, u, get_user_by_token( cfg, _header->token ), _header->token);

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