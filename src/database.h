#ifndef DATABASE_H
#define DATABASE_H 1

typedef struct database database_t;

/* Delegations */
typedef int (*database_callback_connect) (database_t *database);
typedef int (*database_callback_disconnect) (database_t *database);
typedef int (*database_callback_connected) (database_t *database);
typedef int (*database_callback_reconnect) (database_t *database);
typedef const char* (*database_callback_user_credentials) (database_t *database, const char* token);

/* database Object API */
database_t * database_new (const char *database_kind);
void database_free (database_t *database);

const char *database_get_kind (database_t *database);
const char *database_get_private_key (database_t *database);
const char *database_get_public_key (database_t *database);
const char *database_get_table (database_t *database);
const char *database_get_host (database_t *database);
int database_get_port (database_t *database);
void database_set_kind (database_t *database, const char *kind);
void database_set_private_key (database_t *database, const char *key);
void database_set_public_key (database_t *database, const char *key);
void database_set_table (database_t *database, const char *table);
void database_set_host (database_t *database, const char *host);
void database_set_port (database_t *database, int port);

void database_set_data(database_t *database, void *data);
void * database_get_data(database_t *database);
int database_connect (database_t *database);
int database_disconnect (database_t *database);
int database_connected (database_t *database);
const char * database_get_credentials (database_t *database, const char * token);

/* Implementation setters */
void database_callback_set_connect (database_t *database,
                                   database_callback_connect fn);
void database_callback_set_disconnect (database_t *database,
                                      database_callback_disconnect fn);
void database_callback_set_connected (database_t *database,
                                     database_callback_connected fn);
void database_callback_set_reconnect (database_t *database,
                                     database_callback_reconnect fn);
void database_callback_set_user_credentials (database_t *database,
                                     database_callback_user_credentials fn);



void database_init_mongo (database_t *database);
void database_init_redis (database_t *database);
//const char * get_user_credentials(database_t *database, const char* token);
#endif /* DATABASE_H */
