#include <stdlib.h>
#include "database_redis.h"
#include <hiredis/hiredis.h>

enum status_types {
  STATUS_NOMEM = -1,
  STATUS_OK = 0,
  STATUS_FAIL = 1
};

typedef struct redisConfig {
    redisContext *conn;
    redisReply *reply;
} redis_t;


static int
_connect (database_t *database)
{
    int status;
    redis_t *redis_config = malloc( sizeof( redis_t ) );
    
    redis_config->conn = redisConnect(database_get_host ( database ), database_get_port ( database ));
    /* TODO: Parse the uri (inside of kind) */
    if (redis_config->conn != NULL) {
        database_set_data ( database, redis_config );
        return STATUS_OK;
    }
    return STATUS_FAIL;
}

static int
_disconnect ( database_t *database )
{
    redis_t *redis_config = database_get_data ( database );
    redisFree ( redis_config->conn );
    return STATUS_OK;
}

static int
_connected ( database_t *database ) {
    return STATUS_OK;
}

static const char * 
_credentials( database_t *database, const char* token ) 
{
    redis_t *redis_config = database_get_data ( database );
    redis_config->reply = redisCommand( redis_config->conn, "HGET %s %s", token, database_get_private_key( database ) );
    return redis_config->reply->str;
}

void
database_init_redis (database_t *database)
{
    database_callback_set_connect (database, _connect);
    database_callback_set_disconnect (database, _disconnect);
    database_callback_set_connected (database, _connected);
    database_callback_set_user_credentials(database, _credentials);

}

