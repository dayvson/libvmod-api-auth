import authorization;

sub vcl_init {
    authorization.database("mongodb");
    authorization.database_connect("127.0.0.1", 27017, "database.collection");
    authorization.database_scheme("token", "secretkey", "ratelimit");
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
