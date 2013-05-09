#Varnish Authorization Module
-----------------------------


###Compiling:

```
 ./configure VARNISHSRC=DIR [VMODDIR=DIR]
```

`VARNISHSRC` is the directory of the Varnish source tree for which to
compile your vmod. Both the `VARNISHSRC` and `VARNISHSRC/include`
will be added to the include search paths for your module.

Optionally you can also set the vmod install directory by adding
`VMODDIR=DIR` (defaults to the pkg-config discovered directory from your
Varnish installation).

Make targets:

* make - builds the vmod
* make install - installs your vmod in `VMODDIR`
* make check - runs the unit tests in ``src/tests/*.vtc``

In your VCL you could then use this vmod along the following lines::

```
import authorization;
authorization.dbconnect("localhost", 27012, "database.yourcollection");

backend default {
 .host = "127.0.0.1";
 .port = "3000";
}

sub vcl_fetch {
    
    set beresp.ttl = 1m;
    if (req.http.Authorization && req.http.X-NYTV-Date) {
       set beresp.ttl = 60s;
       set beresp.http.vmod-secretkey = authorization.get_credentials(req.http.Authorization);
       set beresp.http.vmod-hmacsha512 = authorization.encode_hmac( authorization.get_credentials(req.http.Authorization), req.http.X-NYTV-Date);
       set beresp.http.vmod-signature = authorization.encode_base64(authorization.encode_hmac(authorization.get_credentials(req.http.Authorization), req.http.X-NYTV-Date));
       set beresp.http.vmodauthenticated = authorization.is_valid(req.http.Authorization, req.http.X-NYTV-Date);
       return (deliver);
    }
}

```

