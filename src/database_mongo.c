#include <stdlib.h>
#include "mongodb.h"
#include "mongo.h"

enum status_types {
  STATUS_NOMEM = -1,
  STATUS_OK = 0,
  STATUS_FAIL = 1
};

typedef struct mongoConfig {
    mongo conn[1];
} mongo_t;

static int
_connect (database_t *database)
{
    int status;
    mongo_t *mongo_config = malloc( sizeof( mongo_t ) );
    /* TODO: Parse the uri (inside of kind) */
    if(mongo_client ( mongo_config->conn, "127.0.0.1", 27017) == MONGO_OK ){
        database_set_data ( database, mongo_config );
        return STATUS_OK;
    }
    return STATUS_FAIL;
}

static int
_disconnect ( database_t *database )
{
    mongo_t *mongo_config = database_get_data ( database );
    mongo_destroy ( mongo_config->conn );
    return STATUS_OK;
}

static int
_connected ( database_t *database ) {
    mongo_t *mongo_config = database_get_data ( database );
    if(mongo_check_connection ( mongo_config->conn ) == MONGO_OK){
        return STATUS_OK;
    }
    return STATUS_FAIL;
}

const char * 
_credentials( database_t *database, const char* token ) 
{
    bson query[1];
    mongo_cursor cursor[1];
    const char* private_key; 
    if(token == NULL) {
        return NULL;
    }
    mongo_t *mongo_config = (mongo_t *) database_get_data( database );
    
    bson_init( query );
    bson_append_string( query, database_get_public_key( database ), token);
    bson_finish( query );
    mongo_cursor_init( cursor, mongo_config->conn, database_get_table( database ) );
    mongo_cursor_set_query( cursor, query );

    /* XXX: Check if we have a find_one() in the mongo API to avoid the loop */
    while ( mongo_cursor_next( cursor ) == MONGO_OK ) {
        bson_iterator iterator[1];
        if ( bson_find( iterator, mongo_cursor_bson( cursor ), database_get_private_key( database ) ) ) {
            private_key = bson_iterator_string( iterator );
            break;
        }
    }

    bson_destroy( query );
    mongo_cursor_destroy( cursor );
    return private_key;
}


void
database_init_mongo (database_t *database)
{
    database_callback_set_connect (database, _connect);
    database_callback_set_disconnect (database, _disconnect);
    database_callback_set_connected (database, _connected);
    database_callback_set_user_credentials(database, _credentials);
}