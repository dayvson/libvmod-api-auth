#ifndef _AUTHORIZATION_HELPERS_H
#define _AUTHORIZATION_HELPERS_H 1


typedef struct authHeader
{
    char *scheme;
    char *token;
    char *signature;
    char *xdate;
} auth_header;

void init_base64_alphabet();
auth_header * parse_header(const char *authorization_str);
const char * encode_base64(struct sess *sp, const char *msg);
const char * encode_hmac(struct sess *sp, const char *key, const char *msg);

#endif