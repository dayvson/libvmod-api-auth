#include <stdlib.h>
#include <string.h>
#include "database.h"


struct database
{
    int  port;
    char *kind;
    char *host;
    char *private_key;
    char *public_key;
    char *table;
    void *data;
    database_callback_connect connect;
    database_callback_disconnect disconnect;
    database_callback_connected connected;
    database_callback_reconnect reconnect;
    database_callback_user_credentials credentials;
};

database_t *
database_new(const char *database_kind)
{
    database_t *database;
    database = malloc(sizeof(database_t));
    database_set_kind(database, database_kind);
    if (strcmp(database_kind, "mongodb") == 0)
    {
        database_init_mongo(database);
    }
    else if (strcmp(database_kind, "redis") == 0)
    {
        database_init_redis(database);
    }
    return database;
}

void
database_free(database_t *database)
{
    free(database);
}

int
database_connect(database_t *database)
{
    return database->connect(database);
}

const char *
database_get_credentials(database_t *database, const char *token)
{
    return database->credentials(database, token);
}

const char *
database_get_kind(database_t *database)
{
    return (const char *) database->kind;
}

void
database_set_kind(database_t *database, const char *kind)
{
    database->kind = kind;
}

const char *
database_get_host(database_t *database)
{
    return (const char *) database->host;
}

void
database_set_host(database_t *database, const char *host)
{
    database->host = host;
}

int
database_get_port(database_t *database)
{
    return database->port;
}

void
database_set_port(database_t *database, int port)
{
    database->port = port;
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
database_set_private_key(database_t *database, const char *key)
{
    database->private_key = key;
}

const char *
database_get_private_key(database_t *database)
{
    return database->private_key;
}

void
database_set_public_key(database_t *database, const char *key)
{
    database->public_key = key;
}

const char *
database_get_public_key(database_t *database)
{
    return database->public_key;
}

void
database_set_table(database_t *database, const char *table)
{
    database->table = table;
}

const char *
database_get_table(database_t *database)
{
    return database->table;
}

/* Interface setters */
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