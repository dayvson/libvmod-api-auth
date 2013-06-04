#ifndef DATABASE_H
#define DATABASE_H 1
#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "vcc_if.h"

enum status_types{
    STATUS_NOMEM = -1,
    STATUS_OK = 0,
    STATUS_FAIL = 1
};

typedef struct database database_t;

typedef struct database_config database_cfg_t;

/* Delegations */
typedef int (*database_callback_connect) (database_t *database);
typedef int (*database_callback_disconnect) (database_t *database);
typedef int (*database_callback_connected) (database_t *database);
typedef int (*database_callback_reconnect) (database_t *database);
typedef const char *(*database_callback_user_credentials) (database_t *database, const char *token);
typedef int (*database_callback_ratelimit) (database_t *database, const char *token, const char *secretkey);


/* database config API */

database_cfg_t *databasecfg_new();
void databasecfg_free(database_cfg_t *cfg);
const char *databasecfg_get_kind(database_cfg_t *cfg);
const char *databasecfg_get_private_key(database_cfg_t *cfg);
const char *databasecfg_get_public_key(database_cfg_t *cfg);
const char *databasecfg_get_ratelimit_key(database_cfg_t *cfg);
const char *databasecfg_get_table(database_cfg_t *cfg);
const char *databasecfg_get_host(database_cfg_t *cfg);
int databasecfg_get_port(database_cfg_t *cfg);

void databasecfg_set_kind (database_cfg_t *cfg, const char *kind);
void databasecfg_set_private_key(database_cfg_t *cfg, const char *key);
void databasecfg_set_public_key(database_cfg_t *cfg, const char *key);
void databasecfg_set_ratelimit_key(database_cfg_t *cfg, const char *key);
void databasecfg_set_table(database_cfg_t *cfg, const char *table);
void databasecfg_set_host(database_cfg_t *cfg, const char *host);
void databasecfg_set_port(database_cfg_t *cfg, int port);


/* database Object API */

void create_database_pool(database_cfg_t *cfg);
database_t *get_database_instance(struct sess *sp);
database_t *database_new(database_cfg_t *cfg);
void database_free(database_t *database);

const char *database_get_credentials(database_t *database, const char *token);
void *database_get_data(database_t *database);
database_cfg_t *database_get_config(database_t *database);
void database_set_data(database_t *database, void *data);
void database_set_config(database_t *database, database_cfg_t *cfg);
int database_connect(database_t *database);
int database_disconnect(database_t *database);
int database_connected(database_t *database);
int database_isratelimit_allowed(database_t *database, const char *token, const char *secretkey);
/* Implementation setters */
void database_callback_set_connect(database_t *database, database_callback_connect fn);
void database_callback_set_disconnect(database_t *database, database_callback_disconnect fn);
void database_callback_set_connected(database_t *database, database_callback_connected fn);
void database_callback_set_reconnect(database_t *database, database_callback_reconnect fn);
void database_callback_set_user_credentials(database_t *database, database_callback_user_credentials fn);
void database_callback_set_ratelimit(database_t *database, database_callback_ratelimit fn);


/* Database Initializers */
void database_init_mongo(database_t *database);
void database_init_redis(database_t *database);

#endif /* DATABASE_H */
