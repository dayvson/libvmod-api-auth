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
#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "vcc_if.h"
#import "authorization_helpers.h"

/* STYLE:
 *  -add type-casts for functions returning pointers, the first is done for you.
 */

enum alphabets
{
    BASE64 = 0,
    N_ALPHA
};

static struct e_alphabet
{
    char *b64;
    char i64[256];
    char padding;
} alphabet[N_ALPHA];

/*
 * Base64-encode, inspired heavily by gnulib/Simon Josefsson (as referenced in RFC4648)
 */
static size_t
base64_encode (struct e_alphabet *alpha, const char *in, size_t inlen, char *out, size_t outlen)
{
    size_t outlenorig = outlen;
    unsigned char tmp[3], idx;

    if (outlen < 4)
        return -1;

    if (inlen == 0)
    {
        *out = '\0';
        return (1);
    }

    while (1)
    {
        assert(inlen);
        assert(outlen > 3);

        tmp[0] = (unsigned char) in[0];
        tmp[1] = (unsigned char) in[1];
        tmp[2] = (unsigned char) in[2];

        *out++ = alpha->b64[(tmp[0] >> 2) & 0x3f];

        idx = (tmp[0] << 4);
        if (inlen > 1)
            idx += (tmp[1] >> 4);
        idx &= 0x3f;
        *out++ = alpha->b64[idx];

        if (inlen > 1)
        {
            idx = (tmp[1] << 2);
            if (inlen > 2)
                idx += tmp[2] >> 6;
            idx &= 0x3f;

            *out++ = alpha->b64[idx];
        }
        else
        {
            if (alpha->padding)
                *out++ = alpha->padding;
        }

        if (inlen > 2)
        {
            *out++ = alpha->b64[tmp[2] & 0x3f];
        }
        else
        {
            if (alpha->padding)
                *out++ = alpha->padding;
        }

        /*
         * XXX: Only consume 4 bytes, but since we need a fifth for
         * XXX: NULL later on, we might as well test here.
         */
        if (outlen < 5)
            return -1;

        outlen -= 4;

        if (inlen < 4)
            break;

        inlen -= 3;
        in += 3;
    }

    assert(outlen);
    outlen--;
    *out = '\0';
    return outlenorig - outlen;
}

static void
digest_alphabet(struct e_alphabet *alpha)
{
    int i;
    const char *p;
    for (i = 0; i < 256; i++)
    {
        alpha->i64[i] = -1;
    }

    for (p = alpha->b64, i = 0; *p; p++, i++)
    {
        alpha->i64[(int)*p] = (char)i;
    }

    if (alpha->padding)
    {
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

const char *
encode_base64(struct sess *sp, const char *msg)
{
    char *encoded;
    int reserved;

    /* XXX: Rewrite those checks without using asserts */
    assert(msg);
    assert(BASE64 < N_ALPHA);
    CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
    CHECK_OBJ_NOTNULL(sp->ws, WS_MAGIC);

    /* XXX: Improve the variable names :) */
    reserved = WS_Reserve(sp->ws, 0);
    encoded = sp->ws->f;
    reserved = base64_encode(&alphabet[BASE64], msg, strlen(msg), encoded, reserved);
    if (reserved < 0)
    {
        WS_Release(sp->ws, 0);
        return NULL;
    }
    WS_Release(sp->ws, reserved);
    return encoded;
}

auth_header *
parse_header(const char *authorization_str)
{
    if (authorization_str == NULL)
        return NULL;

    const char *delimiters = " :";
    const char *default_scheme = "NYTV";
    char *tmp_copy;
    auth_header *_header;

    /* XXX: Protect the malloc call with the varnish locking system */
    /* STYLE: type-cast: */
    _header = (auth_header*)malloc(sizeof(auth_header));
    tmp_copy = strdup(authorization_str);
    if (_header == NULL || tmp_copy == NULL) {
        /* FIXME: Check for malloc fail. */
    };

    /* XXX: strtok() is not reentrant :( */
    /* FIXME: use strtok_r */
    _header->scheme = strtok(tmp_copy, delimiters);
    if (VRT_strcmp(_header->scheme, default_scheme) != 0)
        return NULL;
    _header->token = strtok(NULL, delimiters);
    _header->signature = strtok(NULL, delimiters);
    return _header;
}

const char *
encode_hmac(struct sess *sp, const char *key, const char *msg)
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
    mhash_hmac_deinit(td, mac);


    hexenc = WS_Alloc(sp->ws, 2 * blocksize + 3); // 0x, '\0' + 2 per input
    if (hexenc == NULL)
    {
        return NULL;
    }
    hexptr = hexenc;
    for (j = 0; j < blocksize; j++)
    {
        /* FIXME: use snprintf to avoid segfaults (paranoia):*/
        sprintf(hexptr, "%.2x", mac[j]);
        hexptr += 2;
        assert((hexptr - hexenc) < (2 * blocksize + 3));
    }
    *hexptr = '\0';
    return hexenc;
}
