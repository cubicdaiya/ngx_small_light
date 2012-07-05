/**
   Copyright (c) 2012 Tatsuhiko Kubo <cubicdaiya@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#ifndef NGX_HTTP_SMALL_LIGHT_MODULE_H
#define NGX_HTTP_SMALL_LIGHT_MODULE_H

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE (1.e+38)

#define NGX_HTTP_SMALL_LIGHT_PARAM_GET(hash, k) ngx_hash_find(hash, ngx_hash_key_lc(k, ngx_strlen(k)), k, ngx_strlen(k))

typedef enum {
    NGX_HTTP_SMALL_LIGHT_COORD_UNIT_NONE,
    NGX_HTTP_SMALL_LIGHT_COORD_UNIT_PIXEL,
    NGX_HTTP_SMALL_LIGHT_COORD_UNIT_PERCENT
} ngx_http_small_light_coord_unit_t;

typedef struct {
    double v;
    ngx_http_small_light_coord_unit_t u;
} ngx_http_small_light_coord_t;

typedef struct {
    short r;
    short g;
    short b;
    short a;
} ngx_http_small_light_color_t;

typedef struct {
    ngx_flag_t enable;
    ngx_hash_t hash;
    ngx_hash_keys_arrays_t patterns;
    ngx_str_t material_dir;
} ngx_http_small_light_conf_t;

typedef struct {
    double                       sx;
    double                       sy;
    double                       sw;
    double                       sh;
    double                       dx;
    double                       dy;
    double                       dw;
    double                       dh;
    double                       cw;
    double                       ch;
    ngx_http_small_light_color_t cc;
    double                       bw;
    double                       bh;
    ngx_http_small_light_color_t bc;
    double                       aspect;
    ngx_int_t                    pt_flg;
    ngx_int_t                    scale_flg;
    ngx_int_t                    inhexif_flg;
    ngx_int_t                    jpeghint_flg;    
} ngx_http_small_light_image_size_t;

typedef struct {
    ngx_hash_t hash;
    ngx_hash_keys_arrays_t params;
    size_t content_length;
    char *inf;
    char *of;
    u_char *content;
    u_char *last;
    void *ictx;
} ngx_http_small_light_ctx_t;

#endif // NGX_HTTP_SMALL_LIGHT_MODULE_H
