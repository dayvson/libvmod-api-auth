#include <stdlib.h>
#include <stdio.h>
#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "vcc_if.h"
#import "authorization_helpers.h"
#import "database.h"

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
    return 0;
}

void
vmod_database(struct sess *sp, struct vmod_priv *priv, const char *database_type)
{
    if (priv->priv != NULL)
        database_free (priv->priv);
    database_t *database = database_new(database_type);
    priv->priv = database;
}


void
vmod_database_connect(struct sess *sp, struct vmod_priv *priv, const char *host, int port, const char *table)
{
    database_t *database;
    if (priv->priv != NULL)
        database = priv->priv;

    database_set_host(database, host);
    database_set_port(database, port);
    database_set_table(database, table);
    database_connect(database);

    priv->priv = database;
}

void
vmod_database_scheme(struct sess *sp, struct vmod_priv *priv, const char *public_key, const char *private_key, const char *ratelimit_key)
{
    database_t *database;
    if (priv->priv != NULL)
        database = priv->priv;
    database_set_private_key(database, private_key);
    database_set_public_key(database, public_key);
    database_set_ratelimit_key(database, ratelimit_key);
    priv->priv = database;
}

unsigned
vmod_is_valid(struct sess *sp, struct vmod_priv *priv, const char *authorization_header, const char *url, const char *custom_header)
{
    database_t *database;
    auth_header *_header;
    _header = parse_header(authorization_header);
    char *string_to_sign;
    unsigned reserved, allocated;

    reserved = WS_Reserve(sp->wrk->ws, 0); /* Reserve some work space */
    string_to_sign = sp->wrk->ws->f;     /* Front of workspace area */
    allocated = snprintf(string_to_sign, reserved, "%s-%s", url, custom_header);
    allocated++;
    if (allocated > reserved)
    {
        /* No space, reset and leave */
        WS_Release(sp->wrk->ws, 0);
        return 0;
    }

    database = priv->priv;
    /* Update work space with what we've used */
    WS_Release(sp->wrk->ws, allocated);
    char *secretkey = database_get_credentials(database, _header->token);
    if (secretkey == NULL)
        return 0;
    int isok = database_isratelimit_allowed(database, _header->token, secretkey);
    LOG_ERR(sp, "######################### TOKEN %s ", _header->token);
    LOG_ERR(sp, "######################### SECRETKEY %s ", secretkey);
    LOG_ERR(sp, "######################### RATELIMIT %d ", isok);
    char *sign_hmac = encode_hmac(sp, secretkey, string_to_sign);
    char *signed_b64  = encode_base64(sp, sign_hmac);

    if (VRT_strcmp(_header->signature, signed_b64) == 0)
        return 1;
    
    return 0;
}