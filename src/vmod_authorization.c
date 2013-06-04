#include <stdlib.h>
#include <stdio.h>
#import "database.h"
#import "authorization_helpers.h"


static pthread_mutex_t auth_mtx = PTHREAD_MUTEX_INITIALIZER;

#define LOG_ERR(sess, ...)                                                      \
    if ((sess) != NULL) {                                                       \
        WSP((sess), SLT_VCL_error, __VA_ARGS__);                                \
    } else {                                                                    \
        fprintf(stderr, __VA_ARGS__);                                           \
        fputs("\n", stderr);                                                    \
    }

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
   init_base64_alphabet();
   
}

void
vmod_database(struct sess *sp, struct vmod_priv *priv, const char *database_type)
{
    if (priv->priv != NULL)
        databasecfg_free (priv->priv);
    database_cfg_t *db_cfg = databasecfg_new();
    databasecfg_set_kind(db_cfg, database_type);
    priv->priv = db_cfg;
}

void
vmod_database_connect(struct sess *sp, struct vmod_priv *priv, const char *host, int port, const char *table)
{
    database_cfg_t *db_cfg;
    if (priv->priv != NULL)
        db_cfg = priv->priv;

    databasecfg_set_host(db_cfg, host);
    databasecfg_set_port(db_cfg, port);
    databasecfg_set_table(db_cfg, table);
    create_database_pool(db_cfg);
    priv->priv = db_cfg;
}

void
vmod_database_scheme(struct sess *sp, struct vmod_priv *priv, const char *public_key, const char *private_key, const char *ratelimit_key)
{
    database_cfg_t *db_cfg;
    if (priv->priv != NULL)
        db_cfg = priv->priv;
    databasecfg_set_private_key(db_cfg, private_key);
    databasecfg_set_public_key(db_cfg, public_key);
    databasecfg_set_ratelimit_key(db_cfg, ratelimit_key);
    priv->priv = db_cfg;
}

unsigned
vmod_is_valid(struct sess *sp, struct vmod_priv *priv, const char *authorization_header, const char *url, const char *custom_header)
{
    
    database_t *database;
    database_cfg_t *db_cfg;
    auth_header *_header;
    _header = parse_header(authorization_header);
    char string_to_sign[1024];
    snprintf(string_to_sign, sizeof string_to_sign, "%s-%s", url, custom_header);
    db_cfg = priv->priv;
    AZ(pthread_mutex_lock(&auth_mtx));
    database = get_database_instance(sp);
    char *secretkey = database_get_credentials(database, _header->token);
    if (secretkey == NULL)
        return 0;
   
    char *sign_hmac = encode_hmac(sp, secretkey, string_to_sign);
    char *signed_b64  = encode_base64(sp, sign_hmac);
    AZ(pthread_mutex_unlock(&auth_mtx));
    if (VRT_strcmp(_header->signature, signed_b64) == 0)
        return 1;

    return 0;
}