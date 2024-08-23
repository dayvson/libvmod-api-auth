# Varnish Authorization Module

## Motivation
The Varnish Authorization Module was created to eliminate the need for writing authorization code within applications. This module offers flexibility and reusability, supporting multiple databases extensibility (Redis and MongoDB are available as part of the module). With this module, you can easily manage the security layer of your API without writing extensive code, while maintaining a straightforward and simple VCL configuration.

## Example (How to Use)
Below is a VCL example demonstrating how to use the authorization module:

```vcl
import authorization;

sub vcl_init {
  authorization.database("mongodb");
  authorization.database_connect("127.0.0.1", 27017, "test.cherry");
  authorization.database_scheme("token", "secretkey", "ratelimit");
}

sub vcl_recv {
  if (req.http.Authorization && req.http.X-Custom-Date) {
    if(authorization.is_valid(req.http.Authorization, req.url, req.http.X-Custom-Date)){
      return (pass);
    } else {
      error 401 "Not Authorized";
    }
  }
}
```

## Functions
The table below lists the functions available for use in VCL files:

| **Function**    | **Arguments**                               | **Return** |
|-----------------|---------------------------------------------|------------|
| database        | STRING database_name (mongodb or redis)     | VOID       |
| database_connect| STRING host, INT port, STRING database.collection | VOID |
| database_scheme | STRING public_key, STRING private_key, STRING ratelimit_key | VOID |
| is_valid        | STRING authorization_header, STRING url, STRING custom_header | BOOL |

### database
Responsible for defining the database Varnish will use (MongoDB or Redis).

#### Arguments:
- `database_name`: A database name (mongodb or redis)

### database_connect
Connects Varnish to your MongoDB/Redis instance.

#### Arguments:
- `host`: MongoDB/Redis host name
- `port`: Database port
- `database.collection`: String containing the database name and collection for MongoDB, or database number for Redis

### database_scheme
Specifies the scheme collection for Varnish to locate authorization values.

#### Arguments:
- `public_key`: Property to access the public key in your collection
- `private_key`: Property to access the private key in your collection
- `rate_limit`: Property to access the rate limit in your collection

### is_valid
Attempts to authenticate the request.

#### Arguments:
- `authorization`: Authorization header
- `url`: Requested URL
- `custom_header`: Custom header used to generate the signature

## Authorization Requests
The Authorization header value format is as follows:
```
Authorization: NYTV <TOKEN>:<Signature(Base64(HMAC-SHA512(<String>, <SecretKey>))>)
```
You must also provide the raw string used in the HMAC-SHA512 signature, added to the header as the value for 'X-Custom-Date'.

### Important Notes on Authentication
- The TOKEN is your public key exposed in the Authorization Header.
- The SecretKey is your private key not exposed in the header but used to create the signature.
- The string to sign (verb, headers, resource) must be UTF-8 encoded.
- The X-Custom-Date header value must be specified.
- The hash function for computing the signature is HMAC-SHA512 using your SecretKey as the key.

## Dependencies
The module has the following dependencies:
- MHASH 0.9.9.9
- LIBMONGO 1.6.2
- HIREDIS 0.11

## Compiling
To compile the module, use the following command:

```
./configure VARNISHSRC=DIR [VMODDIR=DIR]
```

- `VARNISHSRC` is the Varnish source tree directory for compiling your vmod.
- Optionally, set the vmod install directory by adding `VMODDIR=DIR` (defaults to the pkg-config discovered directory from your Varnish installation).

Make targets:
- `make`: Builds the vmod
- `make install`: Installs the vmod in `VMODDIR`
- `make check`: Runs unit tests in `src/tests/*.vtc`
