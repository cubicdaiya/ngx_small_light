/**
   Copyright (c) 2012-2016 Tatsuhiko Kubo <cubicdaiya@gmail.com>

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

#define NGX_HTTP_SMALL_LIGHT_CONVERTER_IMAGEMAGICK "imagemagick"
#define NGX_HTTP_SMALL_LIGHT_CONVERTER_IMLIB2      "imlib2"
#define NGX_HTTP_SMALL_LIGHT_CONVERTER_GD          "gd"

#define NGX_HTTP_SMALL_LIGHT_IMAGE_NONE 0
#define NGX_HTTP_SMALL_LIGHT_IMAGE_JPEG 1
#define NGX_HTTP_SMALL_LIGHT_IMAGE_GIF  2
#define NGX_HTTP_SMALL_LIGHT_IMAGE_PNG  3
#define NGX_HTTP_SMALL_LIGHT_IMAGE_WEBP 4

#define NGX_HTTP_SMALL_LIGHT_PARAM_GET(hash, k) \
    ngx_hash_find(hash, ngx_hash_key_lc((u_char *)k, ngx_strlen(k)), (u_char *)k, ngx_strlen(k))
#define NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(hash, lit) \
    ngx_hash_find(hash, ngx_hash_key_lc((u_char *)lit, sizeof(lit) - 1), (u_char *)lit, sizeof(lit) - 1)

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
    ngx_flag_t enable_getparam_mode;
    ngx_hash_t hash;
    ngx_hash_keys_arrays_t patterns;
    ngx_str_t material_dir;
    ngx_path_t *imlib2_temp_dir;
    size_t buffer_size;
    ngx_uint_t radius_max;
    ngx_uint_t sigma_max;
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
    ngx_int_t                    ix;
    ngx_int_t                    iy;
    double                       aspect;
    ngx_int_t                    pt_flg;
    ngx_int_t                    scale_flg;
    ngx_int_t                    jpeghint_flg;
    ngx_uint_t                   angle;
} ngx_http_small_light_image_size_t;

typedef struct ngx_http_small_light_ctx_t {
    ngx_hash_t hash;
    ngx_hash_keys_arrays_t params;
    size_t content_length;
    const char *inf;
    const char *of;
    u_char *content_orig;
    u_char *content;
    u_char *last;
    void *ictx;
    ngx_uint_t radius_max;
    ngx_uint_t sigma_max;
    ngx_str_t *material_dir;
    ngx_path_t *imlib2_temp_dir;
    struct ngx_http_small_light_converter_t {
        ngx_int_t (*init)(ngx_http_request_t *r, struct ngx_http_small_light_ctx_t *ctx);
        ngx_int_t (*process)(ngx_http_request_t *r, struct ngx_http_small_light_ctx_t *ctx);
        ngx_pool_cleanup_pt term;
    } converter;
} ngx_http_small_light_ctx_t;

typedef void      (*init)(struct ngx_http_small_light_ctx_t *ctx);
typedef void      (*term)(struct ngx_http_small_light_ctx_t *ctx);
typedef ngx_int_t (*process)(ngx_http_request_t *r, struct ngx_http_small_light_ctx_t *ctx);

#endif /* NGX_HTTP_SMALL_LIGHT_MODULE_H */
