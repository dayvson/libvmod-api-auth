import authorization;

sub vcl_init {
    authorization.database("redis");
    authorization.database_connect("127.0.0.1", 6379, "");
    authorization.database_scheme("token", "secretkey");
}

sub vcl_recv {
    if (req.http.Authorization && req.http.X-Custom-Date) {
        if(authorization.is_valid(req.http.Authorization, req.url, req.http.X-NYTV-Date)){
            return (pass);
        }else{
            error 401 "Not Authorized";
        }
    }
}