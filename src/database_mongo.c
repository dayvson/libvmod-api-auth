#include <stdlib.h>
#include "database_mongo.h"
#include "mongo.h"

/* STYLE:
 *  -add type-casts for functions returning pointers, the first is done for you.
 */

typedef struct mongoConfig
{
    mongo conn[1];
} mongo_t;

static int
_connect(database_t *database)
{
    int status;
    /* FIXME: idiomatic C would have this type-cast as: */
    mongo_t *mongo_config = (mongo_t*)malloc(sizeof(mongo_t));
    database_cfg_t *config = (database_cfg_t*)database_get_config(database);
    if (mongo_client(mongo_config->conn, databasecfg_get_host(config), databasecfg_get_port(config)) == MONGO_OK)
    {
        database_set_data(database, mongo_config);
        return STATUS_OK;
    }
    return STATUS_FAIL;
}

static int
_disconnect(database_t *database)
{
    mongo_t *mongo_config = database_get_data(database);
    mongo_destroy(mongo_config->conn);
    return STATUS_OK;
}

static int
_connected(database_t *database)
{
    mongo_t *mongo_config = database_get_data(database);
    if ( mongo_check_connection(mongo_config->conn) == MONGO_OK)
        return STATUS_OK;

    return STATUS_FAIL;
}

static const char *
_credentials(database_t *database, const char *token)
{
    bson query[1];
    mongo_cursor cursor[1];
    const char *private_key;
    const char *search_key;
    if (token == NULL)
        return NULL;

    mongo_t *mongo_config = (mongo_t *) database_get_data(database);
    database_cfg_t *config = database_get_config(database);
    bson_init(query);
    bson_append_string(query, databasecfg_get_public_key(config), token);
    bson_finish(query);
    mongo_cursor_init(cursor, mongo_config->conn, databasecfg_get_table(config));
    mongo_cursor_set_query(cursor, query);

    /* OPTIMIZE: pull the value once and avoiding repeat function calls: */
    search_key = (const char*)databasecfg_get_private_key(config);

    /* XXX: Check if we have a find_one() in the mongo API to avoid the loop */
    while (mongo_cursor_next( cursor ) == MONGO_OK)
    {
        bson_iterator iterator[1];
        if (bson_find(iterator, mongo_cursor_bson(cursor), search_key))
        {
            private_key = bson_iterator_string(iterator);
            break;
        }
    }

    bson_destroy(query);
    mongo_cursor_destroy(cursor);
    return private_key;
}

static int
_check_ratelimit(database_t *database, const char *token, const char *secretkey)
{
    return 1; //XXX: Need implement rate-limit for mongo
}

void
database_init_mongo (database_t *database)
{
    database_callback_set_connect(database, _connect);
    database_callback_set_disconnect(database, _disconnect);
    database_callback_set_connected(database, _connected);
    database_callback_set_user_credentials(database, _credentials);
    database_callback_set_ratelimit(database, _check_ratelimit);
}
