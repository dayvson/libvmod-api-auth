#include <stdlib.h>
#include "../backend.h"
#include "mongo.h"

enum error_types {
  ERR_NOMEM = -1,
  ERR_OK = 0,
  ERR_FAIL = 1
};

static int
_connect (backend_t *backend)
{
  int status;
  mongo *conn;
  const char *kind = backend_get_kind (backend);

  /* TODO: Parse the uri (inside of kind) */

  if ((conn = (mongo *) malloc (sizeof (mongo))) == NULL)
    return ERR_NOMEM;

  if ((status = mongo_client (conn, "127.0.0.1", 27017)) != MONGO_OK) {
    backend_set_data (backend, conn);
    return ERR_OK;
  }

  return ERR_FAIL;
}

static int
_disconnect (backend_t *backend)
{
  mongo_destroy (backend_get_data (backend));
  return ERR_OK;
}

static int
_connected (backend_t *backend) {
  return mongo_check_connection (backend_get_data (backend)) == MONGO_OK
    ? ERR_OK
    : ERR_FAIL;
}


void
backend_init_mongo (backend_t *backend)
{
  backend_callback_set_connect (backend, _connect);
  backend_callback_set_disconnect (backend, _disconnect);
  backend_callback_set_connected (backend, _connected);
  backend_callback_set_reconnect (backend, _reconnect);
}
