#Varnish Authorization Module
-----------------------------
###Motivation
Write me


###Example (how to use):
VCL example to show how to use authorization module::

```
import authorization;

backend default {
 .host = "127.0.0.1";
 .port = "3000";
}

sub vcl_init {
    authorization.dbconnect("127.0.0.1", 27017, "test.cherry");
}

sub vcl_recv {
    if (req.http.Authorization && req.http.X-Custom-Date) {
        if(authorization.is_valid(req.http.Authorization, req.http.X-Custom-Date) == 1){
            unset req.http.Authorization;
            return (lookup);
        }else{
            error 401 "Not Authorized";
        }
        
    }
    
}

sub vcl_deliver {
    set resp.http.X-Hits = obj.hits;
    if (obj.hits > 0) {
        set resp.http.X-Cache = "HIT";
    } else {
        set resp.http.X-Cache = "MISS";
    }
}

```

##Authenticating REST Requests

The value of the Authorization header is as follows:
```
Authorization: NYTV <TOKEN>:<Signature(Base64(HMAC-SHA1(<String>, <SecretKey>)))>
```
You also need to provide the raw string used on HMAC-SHA512 signature. This should be added to the header as the value for 'X-Custom-Date'.
```
X-Custom-Date: String
```

####Some importants things about authentication
 * The TOKEN is your public key that will be exposed in your Authorization Header
 * The SecretKey is your private key that is not expose in your header but you have to use that to create the signature.
 * The string to sign (verb, headers, resource) must be UTF-8 encoded.  
 * The value of the X-Custom-Date header must be specified.
 * The hash function to compute the signature is HMAC-SHA512 using your SecretKey as the key.


Suppose your TOKEN ID is "9c421d03fe8562827bcf573310051844a65da0fc" and your Secret Key is "bnl0di1jaGVycnktYXBp". 
Then you could compute the signature as follows:
Node.JS example:
```
var crypto = require('crypto');
var stringToSign = '123456';
var token = "9c421d03fe8562827bcf573310051844a65da0fc";
var hMACSignature = crypto.createHmac('sha512', 'bnl0di1jaGVycnktYXBp')
                              .update(stringToSign)
                              .digest('hex');
var signature = new Buffer(hMACSignature).toString('base64');

console.log('curl -H "Authorization: NYTV ' + token + ':' + signature + '" -H "X-NYTV-Date: ' + stringToSign + '"  http://localhost:8080/foo');

```
The resulting signature would be "ZjQyNTRkZTllNGY4NTE4MTk1NmVjMzdiZjg2MWUzNDFlMmMzMzMyYQ==", and, you would add the 
Authorization header to your request to come up with the following result:
```
GET /video/1234 HTTP/1.0
Authorization: NYTV 9c421d03fe8562827bcf573310051844a65da0fc:ZjQyNTRkZTllNGY4NTE4MTk1NmVjMzdiZjg2MWUzNDFlMmMzMzMyYQ==
Content-Type: application/json
X-Custom-Date: 123456
```

Since you've signed your string for authentication you can try a request using a curl
```
curl -H "Authorization: NYTV 9c421d03fe8562827bcf573310051844a65da0fc:ZjQyNTRkZTllNGY4NTE4MTk1NmVjMzdiZjg2MWUzNDFlMmMzMzMyYQ==" -H "X-Custom-Date: 123456"  http://localhost:8080/remove-me
```


###Dependencies:
<table>
  <tr>
    <td><strong>MHASH</strong></td>
    <td><strong>0-9-9-9</strong></td>
  </tr>
  <tr>
    <td>LIBMONGO</td>
    <td>>1.6.2</td>
  </tr>
</table>


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




