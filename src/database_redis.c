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
    redis_t *redis_config = malloc(sizeof(redis_t));
    redis_config->conn = redisConnect(database_get_host(database), database_get_port(database));
    redis_config->reply = redisCommand(redis_config->conn, "SELECT %s", database_get_table(database));

    if (redis_config->conn != NULL)
    {
        database_set_data(database, redis_config);
        return STATUS_OK;
    }
    return STATUS_FAIL;
}

static int
_disconnect(database_t *database)
{
    redis_t *redis_config = database_get_data(database);
    redisFree(redis_config->conn);
    return STATUS_OK;
}

static int
_connected(database_t *database)
{
    /*XXX: I need to figure out a way to know if we still connected */
    return STATUS_OK;
}

static const char *
_credentials(database_t *database, const char *token)
{
    redis_t *redis_config = database_get_data(database);
    redis_config->reply = redisCommand(redis_config->conn, "HGET %s %s", token, database_get_private_key(database));
    if (redis_config->reply->len == 0)
        return NULL;

    const char *secretkey = strdup(redis_config->reply->str);
    freeReplyObject(redis_config->reply);
    return secretkey;
}

static int
_check_ratelimit(database_t *database, const char *token, const char *secretkey)
{
    redis_t *redis_config = database_get_data(database);
    redis_config->reply = redisCommand(redis_config->conn, "HGET %s %s", token, database_get_ratelimit_key(database));
    if (redis_config->reply->len == 0)
        return 0;

    char *sep = "/";
    char *tmp_copy = strdup(redis_config->reply->str);
    freeReplyObject(redis_config->reply);
    int time_in_seconds = 10;
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

    redis_config->reply = redisCommand(redis_config->conn, "INCR %s", secretkey);
    int current_value = redis_config->reply->integer;
    freeReplyObject(redis_config->reply);
    if (current_value ==  1)
    {
        redis_config->reply = redisCommand(redis_config->conn, "EXPIRE %s 10", secretkey);
        freeReplyObject(redis_config->reply);
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