#ifndef _BACKEND_H
#define _BACKEND_H

/* Delegations */
typedef int (*backend_callback_connect) (backend_t *backend);
typedef int (*backend_callback_disconnect) (backend_t *backend);
typedef int (*backend_callback_connected) (backend_t *backend);
typedef int (*backend_callback_reconnect) (backend_t *backend);

/* Backend Object API */
backend_t *backend_new (void);
void backend_free (backend_t *backend);

const char *backend_get_kind (backend_t *backend);
void backend_set_kind (backend_t *backend, const char *kind);
int backend_connect (backend_t *backend);
int backend_disconnect (backend_t *backend);
int backend_connected (backend_t *backend);

/* Implementation setters */
void backend_callback_set_connect (backend_t *backend,
                                   backend_callback_connect fn);
void backend_callback_set_disconnect (backend_t *backend,
                                      backend_callback_disconnect fn);
void backend_callback_set_connected (backend_t *backend,
                                     backend_callback_connected fn);
void backend_callback_set_reconnect (backend_t *backend,
                                     backend_callback_reconnect fn);


#endif /* _BACKEND_H */
