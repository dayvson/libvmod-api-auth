#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "database_redis.h"
#include <hiredis/hiredis.h>

typedef struct redisConfig
{
    redisContext *conn;
    redisReply *reply;
} redis_t;


static int
_connect(database_t *database)
{
    int status;
    /* FIXME: add type-casts */
    redis_t *client = (redis_t*)malloc(sizeof(redis_t));
    if (client == NULL) {
        /* FIXME: handle malloc fail. */
    };

    database_cfg_t *config = database_get_config(database);
    client->conn = redisConnect(databasecfg_get_host(config), databasecfg_get_port(config));
    client->reply = redisCommand(client->conn, "SELECT %s", databasecfg_get_table(config));
    
    if (client->conn != NULL)
    {
        database_set_data(database, client);
        return STATUS_OK;
    }
    return STATUS_FAIL;
}

static int
_disconnect(database_t *database)
{
    redis_t *client = database_get_data(database);
    redisFree(client->conn);
    return STATUS_OK;
}

static int
_connected(database_t *database)
{
    redis_t *client = database_get_data(database);
    client->reply = redisCommand(client->conn, "PING");
    if(client->reply != NULL && strcmp(client->reply->str, "PONG") == 0){
        freeReplyObject(client->reply);
        return STATUS_OK;
    }
    return STATUS_FAIL;
}

static const char *
_credentials(database_t *database, const char *token)
{
    database_cfg_t *config = database_get_config(database);
    redis_t *client = database_get_data(database);
    redisReply *reply = redisCommand(client->conn, "HGET %s %s", token, databasecfg_get_private_key(config));
    if (reply != NULL){
        return strdup(reply->str);
    }
    return NULL;
}

static int
_check_ratelimit(database_t *database, const char *token, const char *secretkey)
{
    database_cfg_t *config = database_get_config(database);
    redis_t *client = database_get_data(database);
    client->reply = redisCommand(client->conn, "HGET %s %s", token, databasecfg_get_ratelimit_key(config));
    if (client->reply->len == 0)
        return 0;

    char *sep = "/";
    char *tmp_copy = strdup(client->reply->str);
    
    if (tmp_copy == NULL) {
        /* FIXME: check for strdup failure.*/
    };

    freeReplyObject(client->reply);
    int time_in_seconds = 10;

    /* FIXME:
     * If tmp_copy is guaranteed to always contain a properly formatted decimal
     * value, this is fine. If there is a chance the limit value string could
     * come back malformed, you should use strtol instead.
     */

    /* FIXME:
     * If this function is potentially called by multiple threads, you MUST:
     * - use strtol instead of atoi
     * - use strtok_r instead of strtok
     */
    int limit_value = atoi(strtok(tmp_copy, sep));

    char *timef = strtok(NULL, sep);
    if (strcmp(timef, "s") == 0)
    {
        time_in_seconds = 1;
    }
    else if (strcmp(timef, "m") == 0)
    {
        time_in_seconds = 60;
    }
    else if (strcmp(timef, "h") == 0)
    {
        time_in_seconds = 3600;
    }

    client->reply = redisCommand(client->conn, "INCR %s", secretkey);
    int current_value = client->reply->integer;
    freeReplyObject(client->reply);
    if (current_value ==  1)
    {
        client->reply = redisCommand(client->conn, "EXPIRE %s %d", secretkey, time_in_seconds);
        freeReplyObject(client->reply);
    }
    if (current_value > limit_value)
        return 0;

    return 1;
}

void
database_init_redis(database_t *database)
{
    database_callback_set_connect(database, _connect);
    database_callback_set_disconnect(database, _disconnect);
    database_callback_set_connected(database, _connected);
    database_callback_set_user_credentials(database, _credentials);
    database_callback_set_ratelimit(database, _check_ratelimit);
}
