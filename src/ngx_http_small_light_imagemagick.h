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

#ifndef NGX_HTTP_SMALL_LIGHT_IMAGEMAGICK_H
#define NGX_HTTP_SMALL_LIGHT_IMAGEMAGICK_H

#include <wand/MagickWand.h>

#include "ngx_http_small_light_module.h"

typedef struct {
    u_char *image;
    size_t image_len;
    MagickWand *wand;
    ngx_int_t type;
    ngx_flag_t complete;
} ngx_http_small_light_imagemagick_ctx_t;

ngx_int_t ngx_http_small_light_imagemagick_init(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx);
ngx_int_t ngx_http_small_light_imagemagick_process(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx);
void ngx_http_small_light_imagemagick_term(void *data);

void ngx_http_small_light_imagemagick_genesis(void);
void ngx_http_small_light_imagemagick_terminus(void);
int ngx_http_small_light_imagemagick_set_thread_limit(int limit);

#endif /* NGX_HTTP_SMALL_LIGHT_IMAGEMAGICK_H */
