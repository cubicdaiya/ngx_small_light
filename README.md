# ngx_small_light

A dynamic image transformation module for [nginx](http://nginx.org/).

# Table of contents

* [Features](#features)
* [Supported Formats](#supported-formats)
* [Dependencies](#dependencies)
* [Installation](#installation)
 * [Dynamic module](#dynamic-module)
* [Getting started](#getting-started)
* [Configuration example](#configuration-example)
* [Directives](#directives)
 * [small_light](#small_light)
 * [small_light_getparam_mode](#small_light_getparam_mode)
 * [small_light_material_dir](#small_light_material_dir)
 * [small_light_pattern_define](#small_light_pattern_define)
 * [small_light_imlib2_temp_dir](#small_light_imlib2_temp_dir)
 * [small_light_buffer](#small_light_buffer)
* [Parameters for small_light function](#parameters-for-small_light-function)
* [Named Pattern](#named-pattern)
* [Using GET parameters](#using-get-parameters)
* [Enabling WebP transformation](#enabling-webp-transformation)
* [Optimizing Tips](#optimizing-tips)
 * [JPEG hinting](#jpeg-hinting)
 * [Limit thread-number with OpenMP](#limit-thread-number-with-openmp)
* [Limitations](#limitations)
 * [Not supported features with Imlib2](#not-supported-features-with-imlib2)
 * [Not supported features with GD](#not-supported-features-with-gd)
 * [Not supported animated GIF](#not-supported-animated-gif)
* [Running Tests](#running-tests)
* [License](#license)

## Features

Supports the various image-processings below.

 * Resize
 * Rotate
 * Sharpen
 * Unsharpen
 * Blur
 * Border
 * Canvas
 * Crop
 * Composition
 * JPEG Hinting(except GD)
 * Format convert(e.g. PNG -> JPEG)
 * Color-space convert(e.g. CMYK -> sRGB)

`ngx_small_light` is developed for using as the same way as [mod_small_light](https://github.com/yamac/smalllight) as possible in nginx.

## Supported Formats

Supports the formats below.

 * JPEG
 * GIF(except Imlib2)
 * PNG
 * WebP(except Imlib2)

## Dependencies

 - [PCRE](http://www.pcre.org/) (required)
 - [ImageMagick](http://www.imagemagick.org/script/index.php) (required)
 - [Imlib2](http://docs.enlightenment.org/api/imlib2/html/) (optional)
 - [GD](http://libgd.bitbucket.org/) (optional)

## Installation

```sh
cd ${ngx_small_light_src_dir}
./setup
cd {$nginx_src_dir}
./configure --add-module=${ngx_small_light_src_dir}
make
make install
```

If you want to enable the libraries except ImageMagick in `ngx_small_light`, add the options below when executing `setup`. (ImageMagick is always enabled)

```sh
./setup                         # enable ImageMagick
./setup --with-imlib2           # enable ImageMagick and Imlib2
./setup --with-gd               # enable ImageMagick and GD
./setup --with-imlib2 --with-gd # enable ImageMagick and Imlib2 and GD
```

### Dynamic module

You can also compile `ngx_small_light` as the dynamic module with `--add-dynamic-module` in nginx-1.9.11 and ngx_small_light-0.6.15 or later.

```sh
cd ${ngx_small_light_src_dir}
./setup
cd {$nginx_src_dir}
./configure \
--modules-path=/usr/local/nginx/modules \
--add-dynamic-module=${ngx_small_light_src_dir}
make
make install
```

Add the configuration below for loading module dynamically.

```nginx
load_module /usr/local/nginx/modules/ngx_http_small_light_module.so;
```

## Getting started

Add the configuration below to some server context in nginx.conf and start nginx.

```nginx
small_light on;
location ~ small_light[^/]*/(.+)$ {
    set $file $1;
    rewrite ^ /$file;
}
```

If you can get the original image of image.jpg from the URL below,

```
http://$host:$port/img/image.jpg
```

You will be able to get the converted image of image.jpg from the URL below.

```
http://$host:$port/small_light(dw=300,dh=300)/img/image.jpg
```

The part of `small_light(...)` is called **small_light function**.

## Configuration example

There is some configuration example below.

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

## Directives

### small_light

|Syntax     |*small_light on &#124; off*|
|-----------|---------------------------|
|**Default**|off                        |
|**Context**|server, location           |

This directive sets whether image-processing with `ngx_small_light` is enabled in a server context.

### small_light_getparam_mode

|Syntax     |*small_light_getparam_mode on &#124; off*|
|-----------|-----------------------------------------|
|**Default**|off                                      |
|**Context**|server, location                         |

This directive sets whether converting-image is enabled by GET parameters 
instead of **small_light function** (e.g. `/small_light(dw=200,dh=200)`).
At the expense of it, a **small_light function** is disabled.
But you need to set both `small_light` and `small_light_getparam_mode` **on** to enable the feature of this directive.

### small_light_material_dir

|Syntax     |*small_light_material_dir path*|
|-----------|---------------------------------------------|
|**Default**|                                             |
|**Context**|server                                       |

This directive assigns the directory for embedded icon images.

### small_light_pattern_define

|Syntax     |*small_light_pattern_define pattern_name parameters*|
|-----------|----------------------------------------------------|
|**Default**|                                                    |
|**Context**|server                                              |

This directive names comma-delimited parameters.

### small_light_radius_max

|Syntax     |*small_light_radius_max number*|
|-----------|-------------------------------|
|**Default**|10                             |
|**Context**|server,location                |

This directive sets maximum radius value of geometry for `sharpen` and `unsharp` and `blur`.

### small_light_sigma_max

|Syntax     |*small_light_sigma_max number*|
|-----------|-------------------------------|
|**Default**|10                             |
|**Context**|server,location                |

This directive sets maximum sigma value of geometry for `sharpen` and `unsharp` and `blur`.

### small_light_imlib2_temp_dir

|Syntax     |*small_light_imlib2_temp_dir path* [*level1* [*level2* [*level 3* ]]]|
|-----------|---------------------------------------------------------------------|
|**Default**|small_light_imlib2_temp 1 2                                          |
|**Context**|server                                                               |

This directive assigns the directory for temporary file for Imlib2 processing.
This directive is available when Imlib2 is enabled.

### small_light_buffer

|Syntax     |*small_ligh_buffer size*|
|-----------|------------------------|
|**Default**|1m                      |
|**Context**|server                  |

This directive assigns the maximum size of the buffer used for reading images
when Content-Length is not set in response headers.

## Parameters for small_light function

|Parameter  |Type  |Default    |Description                                     |ImageMagick|Imlib2|GD |
|-----------|------|-----------|------------------------------------------------|-----------|------|---|
|p          |string|           |named pattern of comma-delimited parameters     |        :o:|   :o:|:o:|
|e          |string|imagemagick|engine name (imagemagick, imlib2, gd)           |           |      |   |
|q          |number|           |quality                                         |        :o:|   :o:|:o:|
|of         |string|           |output format (jpg, gif, png, webp)             |        :o:|   :o:|:o:|
|jpeghint   |char  |n          |enable jpeg hinting (y, n)                      |        :o:|   :o:|:x:|
|dw         |coord |sw         |destination width                               |        :o:|   :o:|:o:|
|dh         |coord |sh         |destination height                              |        :o:|   :o:|:o:|
|dx         |coord |sx         |destination x coordinate                        |        :o:|   :o:|:o:|
|dy         |coord |sy         |destination y coordinate                        |        :o:|   :o:|:o:|
|da         |char  |l          |destination aspect ratio contol (l, s, n)       |        :o:|   :o:|:o:|
|ds         |char  |n          |destination scaling control (s, n)              |        :o:|   :o:|:o:|
|cw         |number|           |canvas width                                    |        :o:|   :o:|:o:|
|ch         |number|           |canvas height                                   |        :o:|   :o:|:o:|
|cc         |color |000000     |canvas color                                    |        :o:|   :o:|:o:|
|bw         |number|           |border width                                    |        :o:|   :o:|:o:|
|bh         |number|           |border height                                   |        :o:|   :o:|:o:|
|bc         |color |000000     |border color                                    |        :o:|   :o:|:o:|
|sw         |coord |           |source witdh                                    |        :o:|   :o:|:o:|
|sh         |coord |           |source height                                   |        :o:|   :o:|:o:|
|sx         |coord |           |source x coordinate                             |        :o:|   :o:|:o:|
|sy         |coord |           |source y coordinate                             |        :o:|   :o:|:o:|
|pt         |char  |n          |pass through control (y, n)                     |        :o:|   :o:|:o:|
|sharpen    |string|           |radius,sigma (e.g. 10x5)                        |        :o:|   :o:|:o:|
|unsharp    |string|           |radius,sigma,amount,threshold (e.g 2x5+0.5+0)   |        :o:|   :x:|:x:|
|blur       |string|           |radius,sigma (e.g. 5x10)                        |        :o:|   :o:|:x:|
|embedicon  |string|           |embedded icon file in `small_light_material_dir`|        :o:|   :x:|:x:|
|ix         |number|0          |embedded icon x coordinate                      |        :o:|   :x:|:x:|
|iy         |number|0          |embedded icon y coordinate                      |        :o:|   :x:|:x:|
|angle      |number|0          |angle of rotation (90, 180, 270)                |        :o:|   :o:|:o:|
|progressive|char  |n          |make JPEG progressive (y, n)                    |        :o:|   :x:|:x:|
|cmyk2rgb   |char  |n          |convert colorspace from CMYK to sRGB (y, n)     |        :o:|   :x:|:x:|
|rmprof     |char  |n          |remove profile (y, n)                           |        :o:|   :x:|:x:|
|autoorient |char  |n          |enable adjust image orientation automatically (y, n)  |  :o:|   :x:|:x:|

The values of `da` are `l` and `s` and `n`. These present the meanings below.

 * `l`: long-edge based
 * `s`: short-edge based
 * `n`: nope

There are any limitations below.

 * `of=gif` and `of=webp` are not supported when `e=imlib2`.
 * `autoorient` is available ImageMagick-6.9.0 or later.
 * The value of `radius,sigma` for `sharpen` and `unsharp` and `blur` is limited by `small_light_radius_max` and `small_light_sigma_max`.

There are the types of each parameter below.

|Type  |Description                                      |
|------|-------------------------------------------------|
|coord |coordicante or pixel. percent when appending 'p' |
|char  |character                                        |
|number|integer number                                   |
|color |rrggbb or rrggbbaa                               |
|string|string                                           |

## Named Pattern

`ngx_small_light` supports to name comma-delimited parameters with the `small_light_define_patern`.

```nginx
small_light_pattern_define small dw=120,dh=120,q=80,e=imagemagick,jpeghint=y;
```

If the line above is added to some server context in nginx.conf, the two URLs below return same response.

 * `http://$host:$port/small_light(p=small)/img/image.jpg`
 * `http://$host:$port/small_light(dw=120,dh=120,q=80,e=imagemagick,jpeghint=y)/img/image.jpg`

## Using GET parameters

`ngx_small_light` supports to convert image not only by **small_light function** but by GET paramenters in `v0.5.0` or later.
You need to set both `small_light` and `small_light_getparam_mode` **on** to enable this feature.
At the expense of enabling this feature, **small_light function** (e.g. `/small_light(dw=300,dh=300)/img.jpg` is disabled.

```nginx
small_light on;
small_light_getparam_mode on;
```

In the configuration above, the url below does not return converted image.

```
http://localhost:8000/small_light(dw=200,dh=200)/img/image.jpg
```

Instead the url below returns converted image expected by right.

```
http://localhost:8000/img/image.jpg?dw=200&dh=200
```

## Enabling WebP Transformation

`ngx_small_light` supports WebP transformation with ImageMagick and GD.
Given `of=webp` to **small_light function**, `ngx_small_light` transforms image format into WebP.
But ImageMagick requires libwebp and GD requires libvpx.
You need to embed these libraries in building ImageMagick and GD for enabling WebP transformation.

If WebP transformation is not available, `nginx` outputs the line like below in error.log in processing image with `of=webp`.

```
WebP is not supported
```

If WebP transformation with ImageMagick is available, the output of `convert -list format` includes the line like below.

```
$ convert -list format | grep -i webp
     WEBP* WEBP      rw-   WebP Image Format (libwebp 0.5.0[0208])
```

If WebP transformation with GD is available, the output of `gdlib-config --libs` includes `-lvpx`.

In general, the packages of ImageMagick and GD provided from the linux distributions
such as Ubuntu and CentOS does not embed the library for WebP transformation by default.
In such cases, you need to build ImageMagick or GD yourself.

## Optimizing Tips

There are some optimizing tips for `ngx_small_light`.

### JPEG hinting

When the output format is JPEG and image-converting engine is ImageMagick or Imlib2,
you may give `jpeghint=y`. The speed of processing images is improved dramatically.

### Limit thread-number with OpenMP

When image-converting engine is ImageMagick and the version of `ngx_small_light` is lower than `v0.6.14`, 
giving 1 to `OMP_NUM_THREADS` or `MAGICK_THREAD_LIMIT` in `nginx.conf` is recommended strongly.
Because OpenMP is enabled in ImageMagick by default and ImageMagick enabled OpenMP is very slow on multi-process environment.

```nginx
env OMP_NUM_THREADS=1; # or env MAGICK_THREAD_LIMIT=1;
```

Or you can avoid this problem by building ImageMagick with `--disable-openmp`.

In `v0.6.14` or later, they are no longer required. Because `ngx_small_light` always sets the thread-number with OpenMP 1.

# Limitations

`ngx_small_light` has the limitations below.

## Not supported features with Imlib2

The transformation with Imlib2 does not support to write GIF-image.
Because Imlib2 has the function for loading GIF-image but does not have the function for saving.
Additionally, the transformation by Imlib2 does not support to write and read WebP-image.
So `of=gif` and `e=imlib2` are not enabled to specify at once.
If these are specified, `ngx_small_light` returns 415(Unsupported Media Type).

## Not supported features with GD

The transformation with GD supports to write WebP-image. But it is the experimental feature.

## Not supported animated GIF

`ngx_small_light` does not support the transformation kept animation for animated GIF.
Because it takes long time to transform(e.g. resize, crop) animated GIF kept animation.
So it is not realistic for `ngx_small_light` to support an animated GIF.

If the animated GIF is given, `ngx_small_light` transforms only the first frame.

# Running Tests

```sh
perl Build.PL
cpanm --installdeps .
NGINX_BIN=${nginx_prefix_dir}/sbin/nginx ./Build test
# or
NGINX_BIN=${nginx_prefix_dir}/sbin/nginx prove t/**/*.t
```

# License

Please read the [COPYING](https://github.com/cubicdaiya/ngx_small_light/blob/master/COPYING).
