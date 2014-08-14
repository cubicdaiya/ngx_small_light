ngx_small_light
==================

`ngx_small_light` is a dynamic image transformation module for [nginx](http://nginx.org/).
And `ngx_small_light` is written for using as the same way as [mod_small_light](http://code.google.com/p/smalllight/) as possible in nginx.([mod_small_light](http://code.google.com/p/smalllight/) is an apache module)

## Dependencies

  - [PCRE](http://www.pcre.org/) (required)
  - [ImageMagick](http://www.imagemagick.org/script/index.php) (required)
  - [Imlib2](http://docs.enlightenment.org/api/imlib2/html/) (optional)
  - [GD](http://libgd.bitbucket.org/) (optional)

## Build

    cd ${ngx_small_light_src_dir}
    ./setup
    cd {$nginx_src_dir}
    ./configure --add-module=${ngx_small_light_src_dir}
    make
    make install

If you want to enable the libraries except ImageMagick in `ngx_small_light`, add following options when executing setup. (ImageMagick is always enabled)

```sh
./setup --with-imlib2           # enable ImageMagick and Imlib2
./setup --with-gd               # enable ImageMagick and GD
./setup --with-imlib2 --with-gd # enable ImageMagick and Imlib2 and GD
```

## How To

See the [configuration guide](https://github.com/cubicdaiya/ngx_small_light/wiki/Configuration).

## Configuration Example

```nginx
server {
    listen 8000;
    server_name localhost;

    small_light on;
    small_light_pattern_define msize dw=500,dh=500,da=l,q=95,e=imagemagick,jpeghint=y;
    small_light_pattern_define ssize dw=120,dh=120,da=l,q=95,e=imlib2,jpeghint=y;

    # http://localhost:8000/small_light(p=msize)/img/filename.jpg -> generate msize image
    # http://localhost:8000/small_light(p=ssize)/img/filename.jpg -> generate ssize image
    # http://localhost:8000/small_light(of=gif,q=100)/img/filename.jpg -> generate gif image which quality is 100
    location ~ small_light[^/]*/(.+)$ {
        set $file $1;
        rewrite ^ /$file;
    }
} 
```

## Optimizing Tips

When the output format is JPEG and image-converting engine is ImageMagick or Imlib2,
you may give 'y' to the parameter `jpeghint`. The speed of processing images is improved dramatically.

And when image-converting engine is ImageMagick, giving 1 to `OMP_NUM_THREADS` in `nginx.conf` is recommended strongly.
Because OpenMP is enabled in ImageMagick by default and ImageMagick enabled OpenMP is very slow on multi-process environment.

```nginx
env OMP_NUM_THREADS=1;
```

## Running Test

	perl Build.PL
	cpanm --installdeps .
	NGINX_BIN=${nginx_prefix_dir}/sbin/nginx ./Build test
	# or
	NGINX_BIN=${nginx_prefix_dir}/sbin/nginx prove t/**/*.t
