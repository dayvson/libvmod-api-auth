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

sub vcl_deliver {
    authorization.get_authorization("SEND_A_TOKEN")
}
```

