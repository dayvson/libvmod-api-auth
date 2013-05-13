#Varnish Authorization Module
-----------------------------
###Motivation
Write me


###Example (how to use):
VCL example to show how to use authorization module::

```
import authorization;

sub vcl_init {
  authorization.dbconnect("127.0.0.1", 27017, "test.cherry");
  authorization.dbscheme("token", "secretkey");
}

sub vcl_recv {
  if (req.http.Authorization && req.http.X-Custom-Date) {
    if(authorization.is_valid(req.http.Authorization, req.url, req.http.X-Custom-Date)){
      return (pass);
    }else{
      error 401 "Not Authorized";
    }
  }
}

```

###FUNCTIONS:
<table>
  <tr>
    <td><strong>FUNCTION</strong></td>
    <td><strong>RETURN</strong></td>
  </tr>
  <tr>
    <td>dbconnect</td>
    <td>VOID</td>
  </tr>
  <tr>
    <td>dbscheme</td>
    <td>VOID</td>
  </tr>
  <tr>
    <td>is_valid</td>
    <td>BOOL</td>
  </tr>
</table>

All the functions listed here can be used VCL files

####dbconnect
It is responsible to connect VARNISH to your Mongodb instance this function returns is VOID

dbconnect(STRING host, INT port, STRING database.collection)

#####Arguments:
  host                -> Your mongodb host name
  port                -> mongodb port
  database.collection -> A string contain database name and collection

#####Example:
sub vcl_init {
    authorization.dbconnect("127.0.0.1", 27017, "test.cherry");
} 


####dbscheme
It is responsible to provide your scheme collection VARNISH know where he can find the values to authorization this function returns is VOID 

dbscheme(STRING public_key, STRING private_key)

#####Arguments:
  public_key  -> A string property to access the public_key in your collection
  private_key -> A string property to access the private_key in your collection

#####Example:
sub vcl_init {
    authorization.dbscheme("token", "secretkey");
}

####is_valid
This function is responsible to provide your scheme collection VARNISH know where he can find the values to authorization this function returns a BOOLEAN 

is_valid(STRING authorization_header, STRING url, STRING custom_header)

#####Arguments:
  authorization -> The authorization header 
  url           -> The url requested
  custom_header -> A custom header to used to generate the signature

#####Example:
<pre>
sub vcl_recv {
  if (req.http.Authorization && req.http.X-Custom-Date) {
    if(authorization.is_valid(req.http.Authorization, req.url, req.http.X-Custom-Date)){
      return (pass);
    }else{
      error 401 "Not Authorized";
    }
  }
}
</pre>

##Authenticating REST Requests

The value of the Authorization header is as follows:
```
Authorization: NYTV <TOKEN>:<Signature(Base64(HMAC-SHA512(<String>, <SecretKey>)))>
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
    <td><strong>LIBRARY</strong></td>
    <td><strong>VERSION</strong></td>
  </tr>
  <tr>
    <td>MHASH</td>
    <td>0.9.9.9</td>
  </tr>
  <tr>
    <td>LIBMONGO</td>
    <td>1.6.2</td>
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




