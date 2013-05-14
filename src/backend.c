#include <vmod-api-auth/backend.h>

#include <stdlib.h>
#include "backend.h"

struct backend
{
  char *kind;
  void *data;
  backend_callback_connect connect;
  backend_callback_disconnect disconnect;
  backend_callback_connected connected;
  backend_callback_reconnect reconnect;
};


backend_t *
backend_new (void) {
  return NULL;
}

void
backend_free (backend_t *backend)
{
  free (backend);
}

int
backend_connect (backend_t *backend)
{
  return backend->connect (backend);
}

int
backend_disconnect (backend_t *backend)
{
  return backend->disconnect (backend);
}

int
backend_connected (backend_t *backend)
{
  return backend->connected (backend);
}

const char *
backend_get_kind (backend_t *backend)
{
  return (const char *) backend->kind;
}

void
backend_set_kind (backend_t *backend, const char *kind)
{
  if (kind == NULL)
    return;

  if (backend->kind != NULL)
    free (backend->kind);

  backend->kind = strcmp (kind);
}

/* Interface setters */

void
backend_callback_set_connect (backend_t *backend,
                              backend_callback_connect fn)
{
  backend->connect = fn;
}

void
backend_callback_set_disconnect (backend_t *backend,
                                 backend_callback_disconnect fn)
{
  backend->disconnect = fn;
}

void
backend_callback_set_connected (backend_t *backend,
                                backend_callback_connected fn)
{
  backend->connected = fn;
}

void
backend_callback_set_reconnect (backend_t *backend,
                                backend_callback_reconnect fn)
{
  backend->connected = fn;
}
