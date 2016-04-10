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

#ifndef NGX_HTTP_SMALL_LIGHT_PARSER_H
#define NGX_HTTP_SMALL_LIGHT_PARSER_H

#include "ngx_http_small_light_module.h"

ngx_int_t ngx_http_small_light_parse_define_pattern(ngx_http_request_t *r, ngx_str_t *unparsed_uri, ngx_str_t *define_pattern);
ngx_int_t ngx_http_small_light_parse_params(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx, ngx_str_t *define_pattern, char *pv);

ngx_int_t ngx_http_small_light_parse_flag(const char *s);
ngx_int_t ngx_http_small_light_parse_int(const char *s);
double ngx_http_small_light_parse_double(const char *s);
double ngx_http_small_light_calc_coord(ngx_http_small_light_coord_t *crd, double v);
ngx_int_t ngx_http_small_light_parse_coord(ngx_http_small_light_coord_t *crd, const char *s);
ngx_int_t ngx_http_small_light_parse_color(ngx_http_small_light_color_t *color, const char *s);

#endif /* NGX_HTTP_SMALL_LIGHT_PARSER_H */
