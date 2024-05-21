# nginx-uuid7-module

This project is just rework of already existing algorithms and module.<br />
See:<br />
[https://github.com/mibamur/nginx-modules-guide/](https://github.com/cybozu/nginx-uuid4-module/tree/master)<br />
[https://gist.github.com/h0nza/uuidv7.c](https://gist.github.com/h0nza/c11c6c1af9550ce5be816c10079eea0b)<br />


`nginx-uuid7-module` provides a directive to set version-7 UUIDs for variables.

This module uses unix time and getentropy() to generate UUIDS.
You can use them UUID as an unique identifier for an HTTP request.

```
uuid7 $uuid;

if ($cookie_uuid = "") {
    set $uuida_cookie "uuid=${uuid};"
}

add_header Set-Cookie $uuid_cookie;
```

In this example, `$uuid` contains a UUID string such as
`018f986b-4d34-7a3b-83d3-bdef90c12176`.


## Directives

Syntax: `uuid7 $VARIABLE`  
Context: http, server, location

Generates a version-7 UUID and assigns it to the specified variable.


## Install

Download nginx source code:
```
git clone https://github.com/1ovsss/nginx-uuid7-module.git
wget http://nginx.org/download/nginx-{$version}.tar.gz
tar -xvf nginx-{$version}.tar.gz
cd nginx-{$version}
```

Build:
```
./configure --add-module=/path/to/nginx-uuid7-module
make
make install
```

If you want to add this module as dynamic run this insead:
```
./configure --with-compat --add-dynamic-module=../nginx-uuid7-module
make modules
```