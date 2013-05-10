import authorization;

sub vcl_init {
    authorization.dbconnect("127.0.0.1", 27017, "database.collection");
}

sub vcl_recv {
    if (req.http.Authorization && req.http.X-Custom-Date) {
        if(authorization.is_valid(req.http.Authorization, req.http.X-Custom-Date) == 1){
            unset req.http.Authorization;
            if(req.request != "GET"){
                return (pass);
            }else{
                return (lookup);    
            }
        }else{
            error 401 "Not Authorized";
        }
    }
}