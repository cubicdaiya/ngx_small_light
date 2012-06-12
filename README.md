ngx_small_light
==================

ngx_small_light is a dynamic image transformation module for Nginx.
And ngx_small_light is written for using as the same way as [mod_small_light](http://code.google.com/p/smalllight/) as possible in Nginx.(mod_small_light is a Apache module)

## Dependencies

  - [Nginx](http://nginx.org/)
  - [PCRE](http://www.pcre.org/)
  - [ImageMagick](http://www.imagemagick.org/script/index.php)

## Build

    cd ${ngx_small_light_src_dir}
    ./setup
    cd {$nginx_src_dir}
    ./configure --with-pcre --add-module=${ngx_small_light_src_dir}
    make
    make install

## Configuration Example

    server {
        listen 8000;
        server_name localhost;

        small_light on;
        small_light_pattern_define msize dw=500,dh=500,da=l,q=95,e=imagemagick,jpeghint=y;
        small_light_pattern_define ssize dw=120,dh=120,da=l,q=95,e=imagemagick,jpeghint=y;

        # http://localhost:8000/small_light(p=msize)/img/filename.jpg -> generate msize image
        # http://localhost:8000/small_light(p=ssize)/img/filename.jpg -> generate ssize image
        # http://localhost:8000/small_light(of=gif,q=100)/img/filename.jpg -> generate gif image which quality is 100
        location ~ small_light[^/]*/(.+)$ {
            set $file $1;
            rewrite ^ /$file;
        }
    } 

## Todo

  - support Imlib2 and GD
  - add various convenient options
  - make documentations
