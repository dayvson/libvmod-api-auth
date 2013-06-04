#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "database.h"



struct database_config
{
    int  port;
    char *kind;
    char *host;
    char *private_key;
    char *public_key;
    char *ratelimit_key;
    char *table;
};

struct database
{
    void *data;
    database_cfg_t *config;
    database_callback_connect connect;
    database_callback_disconnect disconnect;
    database_callback_connected connected;
    database_callback_reconnect reconnect;
    database_callback_user_credentials credentials;
    database_callback_ratelimit check_ratelimit;
};


static pthread_mutex_t auth_mtx = PTHREAD_MUTEX_INITIALIZER;
static struct database_t **database_list;
int database_list_sz;


database_t *
database_new(database_cfg_t *cfg)
{
    database_t *database;
    database = malloc(sizeof(database_t));
    database_set_config(database, cfg);
    if (strcmp(databasecfg_get_kind(cfg), "mongodb") == 0)
    {
        database_init_mongo(database);
    }
    else if (strcmp(databasecfg_get_kind(cfg), "redis") == 0)
    {
        database_init_redis(database);
    }
    return database;
}

database_cfg_t *
databasecfg_new()
{
    database_cfg_t *cfg;
    cfg = malloc(sizeof(database_cfg_t));
    return cfg;
}

void
database_free(database_t *database)
{
    free(database);
}

void
databasecfg_free(database_cfg_t *cfg)
{
    free(cfg);
}

int
database_connected(database_t *database)
{
    return database->connected(database);
}

int
database_connect(database_t *database)
{
    return database->connect(database);
}

int
database_disconnect(database_t *database)
{
    return database->disconnect(database);
}

int
database_isratelimit_allowed(database_t *database, const char *token, const char *secretkey)
{
    return database->check_ratelimit(database, token, secretkey);
}

const char *
database_get_credentials(database_t *database, const char *token)
{
    return database->credentials(database, token);
}

void 
database_set_config(database_t *database, database_cfg_t *cfg)
{
    database->config = cfg;
}

database_cfg_t * 
database_get_config(database_t *database)
{
    return database->config;
}

const char *
databasecfg_get_kind(database_cfg_t *cfg)
{
    return (const char *) cfg->kind;
}

const char *
databasecfg_get_ratelimit_key(database_cfg_t *cfg)
{
    return (const char *) cfg->ratelimit_key;
}

void
databasecfg_set_ratelimit_key(database_cfg_t *cfg, const char *key)
{
    cfg->ratelimit_key = key;
}

void
databasecfg_set_kind(database_cfg_t *cfg, const char *kind)
{
    cfg->kind = kind;
}

const char *
databasecfg_get_host(database_cfg_t *cfg)
{
    return (const char *) cfg->host;
}

void
databasecfg_set_host(database_cfg_t *cfg, const char *host)
{
    cfg->host = host;
}

int
databasecfg_get_port(database_cfg_t *cfg)
{
    return cfg->port;
}

void
databasecfg_set_port(database_cfg_t *cfg, int port)
{
    cfg->port = port;
}

void
database_set_data(database_t *database, void *data)
{
    database->data = data;
}

void *
database_get_data(database_t *database)
{
    return database->data;
}

void
databasecfg_set_private_key(database_cfg_t *cfg, const char *key)
{
    cfg->private_key = key;
}

const char *
databasecfg_get_private_key(database_cfg_t *cfg)
{
    return cfg->private_key;
}

void
databasecfg_set_public_key(database_cfg_t *cfg, const char *key)
{
    cfg->public_key = key;
}

const char *
databasecfg_get_public_key(database_cfg_t *cfg)
{
    return cfg->public_key;
}

void
databasecfg_set_table(database_cfg_t *cfg, const char *table)
{
    cfg->table = table;
}

const char *
databasecfg_get_table(database_cfg_t *cfg)
{
    return cfg->table;
}


void 
create_database_pool(database_cfg_t *cfg)
{
    int i;
    database_list = NULL;
    database_list_sz = 50;
    database_list = malloc(sizeof(database_t) * database_list_sz);
    AZ(pthread_mutex_lock(&auth_mtx));
    for (i = 0 ; i < database_list_sz; i++) {
        database_t *database = database_new(cfg);
        database_connect(database);
        database_list[i] = database;
    }
    AZ(pthread_mutex_unlock(&auth_mtx));
}


#define _LOG_ERR(sess, ...)                                                     \
    if ((sess) != NULL) {                                                       \
        WSP((sess), SLT_VCL_error, __VA_ARGS__);                                \
    } else {                                                                    \
        fprintf(stderr, __VA_ARGS__);                                           \
        fputs("\n", stderr);                                                    \
    }

database_t *
get_database_instance(struct sess *sp)
{
    database_t *db;
    db = database_list[sp->id % database_list_sz];
    return db;
}

/* Interface setters */
void
database_callback_set_ratelimit(database_t *database, database_callback_ratelimit fn)
{
    database->check_ratelimit = fn;
}

void
database_callback_set_user_credentials(database_t *database, database_callback_user_credentials fn)
{
    database->credentials = fn;
}

void
database_callback_set_connect(database_t *database, database_callback_connect fn)
{
    database->connect = fn;
}

void
database_callback_set_disconnect(database_t *database, database_callback_disconnect fn)
{
    database->disconnect = fn;
}

void
database_callback_set_connected(database_t *database, database_callback_connected fn)
{
    database->connected = fn;
}

void
database_callback_set_reconnect(database_t *database, database_callback_reconnect fn)
{
    database->connected = fn;
}