#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
/*
 * mhash.h has a habit of pulling in assert(). Let's hope it's a define,
 * and that we can undef it, since Varnish has a better one.
 */
#include <mhash.h>
#ifdef assert
#   undef assert
#endif

#define MONGO_HAVE_STDINT
#include "mongo.h"
#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "vcc_if.h"


typedef struct mongoConfig {
    char *host;
    int port;
    char *collection;
    mongo conn[1];
    char *private_key;
    char *public_key;
} config_t;

typedef struct authHeader {
    char *scheme;
    char *token;
    char *signature;
    char *xdate;
} auth_header;

enum alphabets {
    BASE64 = 0,
    N_ALPHA
};

static struct e_alphabet {
    char *b64;
    char i64[256];
    char padding;
} alphabet[N_ALPHA];


#define LOG_ERR(sess, ...)                                                      \
    if ((sess) != NULL) {                                                       \
        WSP((sess), SLT_VCL_error, __VA_ARGS__);                                \
    } else {                                                                    \
        fprintf(stderr, __VA_ARGS__);                                           \
        fputs("\n", stderr);                                                    \
    }

static auth_header *
parse_header(const char* authorization_str){
    const char* delimiters = " :";
    const char* default_scheme = "NYTV";
    char *tmp_copy;
    auth_header * _header;
    _header = malloc(sizeof(auth_header));
    tmp_copy = strdup(authorization_str);
    _header->scheme = strtok(tmp_copy, delimiters);
    if(strncmp(_header->scheme, default_scheme, 100) != 0){
        return NULL; 
    }
    _header->token = strtok(NULL, delimiters);
    _header->signature = strtok(NULL, delimiters);
    return _header;
}

static config_t *
make_config(const char *host, int port, const char* collection)
{
    config_t *cfg;
    cfg = malloc(sizeof(config_t));

    if(cfg == NULL){
        return NULL;
    }
        
    cfg->host = strdup(host);
    cfg->port = port;
    cfg->collection = collection;
    mongo_client(cfg->conn, cfg->host, cfg->port);
    return cfg;
}

static char * 
get_user_by_token( config_t *cfg, const char* token ) {
    bson query[1];
    mongo_cursor cursor[1];
    bson_init( query );
    bson_append_string( query, cfg->public_key, token);
    bson_finish( query );
    mongo_cursor_init( cursor, cfg->conn, cfg->collection );
    mongo_cursor_set_query( cursor, query );
    while( mongo_cursor_next( cursor ) == MONGO_OK ) {
        bson_iterator iterator[1];
        if ( bson_find( iterator, mongo_cursor_bson( cursor ), cfg->private_key )) {
            return bson_iterator_string( iterator );
        }
    }

    bson_destroy( query );
    mongo_cursor_destroy( cursor );
    return NULL;
}

/*
 * Base64-encode, inspired heavily by gnulib/Simon Josefsson (as referenced in RFC4648)
 */
static size_t
base64_encode (struct e_alphabet *alpha, const char *in,
        size_t inlen, char *out, size_t outlen)
{
    size_t outlenorig = outlen;
    unsigned char tmp[3], idx;

    if (outlen<4)
        return -1;

    if (inlen == 0) {
        *out = '\0';
        return (1);
    }

    while (1) {
        assert(inlen);
        assert(outlen>3);

        tmp[0] = (unsigned char) in[0];
        tmp[1] = (unsigned char) in[1];
        tmp[2] = (unsigned char) in[2];

        *out++ = alpha->b64[(tmp[0] >> 2) & 0x3f];

        idx = (tmp[0] << 4);
        if (inlen>1)
            idx += (tmp[1] >> 4);
        idx &= 0x3f;
        *out++ = alpha->b64[idx];
            
        if (inlen>1) {
            idx = (tmp[1] << 2);
            if (inlen>2)
                idx += tmp[2] >> 6;
            idx &= 0x3f;

            *out++ = alpha->b64[idx];
        } else {
            if (alpha->padding)
                *out++ = alpha->padding;
        }

        if (inlen>2) {
            *out++ = alpha->b64[tmp[2] & 0x3f];
        } else {
            if (alpha->padding)
                *out++ = alpha->padding;
        }

        /*
         * XXX: Only consume 4 bytes, but since we need a fifth for
         * XXX: NULL later on, we might as well test here.
         */
        if (outlen<5)
            return -1;

        outlen -= 4;

        if (inlen<4)
            break;
        
        inlen -= 3;
        in += 3;
    }

    assert(outlen);
    outlen--;
    *out = '\0';
    return outlenorig-outlen;
}

const char *
vmod_encode_base64(struct sess *sp, const char *msg)
{
    char *p;
    int u;
    assert(msg);
    assert(BASE64<N_ALPHA);
    CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
    CHECK_OBJ_NOTNULL(sp->ws, WS_MAGIC);
    
    u = WS_Reserve(sp->ws,0);
    p = sp->ws->f;
    u = base64_encode(&alphabet[BASE64], msg, strlen(msg), p, u);
    if (u < 0) {
        WS_Release(sp->ws,0);
        return NULL;
    }
    WS_Release(sp->ws,u);
    return p;
}


const char *
vmod_encode_hmac(struct sess *sp, const char *key, const char *msg)
{
    size_t maclen = mhash_get_hash_pblock(MHASH_SHA512);
    size_t blocksize = mhash_get_block_size(MHASH_SHA512);
    unsigned char mac[blocksize];
    unsigned char *hexenc;
    unsigned char *hexptr;
    int j;
    MHASH td;

    assert(msg);
    assert(key);
    CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
    CHECK_OBJ_NOTNULL(sp->ws, WS_MAGIC);

    /*
     *If the return value is 0 you shouldn't use that algorithm in HMAC.
     */
    assert(mhash_get_hash_pblock(MHASH_SHA512) > 0);

    td = mhash_hmac_init(MHASH_SHA512, (void *) key, strlen(key),
        mhash_get_hash_pblock(MHASH_SHA512));
    mhash(td, msg, strlen(msg));
    mhash_hmac_deinit(td,mac);
    

    hexenc = WS_Alloc(sp->ws, 2*blocksize+3); // 0x, '\0' + 2 per input
    if (hexenc == NULL){
        return NULL;
    }
    hexptr = hexenc;
    for (j = 0; j < blocksize; j++) {
        sprintf(hexptr,"%.2x", mac[j]);
        hexptr+=2;
        assert((hexptr-hexenc)<(2*blocksize + 3));
    }
    *hexptr = '\0';
    return hexenc;
}


static void
digest_alphabet(struct e_alphabet *alpha)
{
    int i;
    const char *p;
    for (i = 0; i < 256; i++){
        alpha->i64[i] = -1;
    }
    
    for (p = alpha->b64, i = 0; *p; p++, i++){
        alpha->i64[(int)*p] = (char)i;
    }

    if (alpha->padding){
        alpha->i64[alpha->padding] = 0;
    }
}

void 
init_base64_alphabet()
{
    alphabet[BASE64].b64 =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
        "ghijklmnopqrstuvwxyz0123456789+/";
    alphabet[BASE64].padding = '=';
    digest_alphabet(&alphabet[BASE64]);
}

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
    if (priv->priv == NULL) {
        priv->priv = make_config("127.0.0.1", 27017, "test.cherry");
        priv->free = free;
    }
    init_base64_alphabet();
	return (0);
}

void 
vmod_dbconnect(struct sess *sp, struct vmod_priv *priv, const char* host, int port, const char* collection){
    config_t *old_cfg = priv->priv;

    priv->priv = make_config(host, port, collection);
    if(priv->priv && old_cfg) {
        free(old_cfg->host);
        free(old_cfg);
    }

}

void 
vmod_dbscheme(struct sess *sp, struct vmod_priv *priv, const char* public_key, const char* private_key){
    config_t *cfg = priv->priv;
    if(cfg){
        cfg->public_key = public_key;
        cfg->private_key = private_key;
        priv->priv = cfg;
    }
}

unsigned
vmod_is_valid(struct sess *sp, struct vmod_priv *priv, const char *authorization_header, const char *url, const char *custom_header)
{
    config_t *cfg = priv->priv;
    auth_header * _header;
    _header = parse_header(authorization_header);
    char *p;
    unsigned u, v;

    u = WS_Reserve(sp->wrk->ws, 0); /* Reserve some work space */
    p = sp->wrk->ws->f;     /* Front of workspace area */
    v = snprintf(p, u, "%s-%s", url, custom_header);

    v++;
    if (v > u) {
        /* No space, reset and leave */
        WS_Release(sp->wrk->ws, 0);
        return (NULL);
    }
    /* Update work space with what we've used */
    WS_Release(sp->wrk->ws, v);
    char * user_data = get_user_by_token( cfg, _header->token);
    char * sign_hmac = vmod_encode_hmac(sp, user_data, p);
    char * sign_b64  = vmod_encode_base64(sp, sign_hmac);

    if(VRT_strcmp(_header->signature, sign_b64) == 0){
        return (1);
    }else{
        return (0);
    }
}