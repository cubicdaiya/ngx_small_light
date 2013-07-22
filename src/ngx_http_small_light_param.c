/**
   Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>

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


#include "ngx_http_small_light_param.h"
#include "ngx_http_small_light_parser.h"

static void ngx_http_small_light_init_params_default(ngx_http_small_light_ctx_t *ctx);

static void ngx_http_small_light_init_params_default(ngx_http_small_light_ctx_t *ctx)
{
    ngx_str_t p         = ngx_string("p");
    ngx_str_t sx        = ngx_string("sx");
    ngx_str_t sy        = ngx_string("sy");
    ngx_str_t sw        = ngx_string("sw");
    ngx_str_t sh        = ngx_string("sh");
    ngx_str_t dx        = ngx_string("dx");
    ngx_str_t dy        = ngx_string("dy");
    ngx_str_t dw        = ngx_string("dw");
    ngx_str_t dh        = ngx_string("dh");
    ngx_str_t da        = ngx_string("da");
    ngx_str_t ds        = ngx_string("ds");
    ngx_str_t cw        = ngx_string("cw");
    ngx_str_t ch        = ngx_string("ch");
    ngx_str_t cc        = ngx_string("cc");
    ngx_str_t bw        = ngx_string("bw");
    ngx_str_t bh        = ngx_string("bh");
    ngx_str_t bc        = ngx_string("bc");
    ngx_str_t pt        = ngx_string("pt");
    ngx_str_t q         = ngx_string("q");
    ngx_str_t of        = ngx_string("of");
    ngx_str_t info      = ngx_string("info");
    ngx_str_t inhexif   = ngx_string("inhexif");
    ngx_str_t jpeghint  = ngx_string("jpeghint");
    ngx_str_t rmprof    = ngx_string("rmprof");
    ngx_str_t embedicon = ngx_string("embedicon");
    ngx_str_t ix        = ngx_string("ix");
    ngx_str_t iy        = ngx_string("iy");
    ngx_str_t angle     = ngx_string("angle");
    ngx_str_t e         = ngx_string("e");

    ngx_hash_add_key(&ctx->params, &p,         "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &sx,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &sy,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &sw,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &sh,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &dx,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &dy,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &dw,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &dh,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &da,        "l",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &ds,        "n",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &cw,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &ch,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &cc,        "000000", NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &bw,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &bh,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &bc,        "000000", NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &pt,        "n",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &q,         "0",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &of,        "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &info,      "0",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &inhexif,   "n",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &jpeghint,  "n",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &rmprof,    "n",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &embedicon, "",       NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &ix,        "0",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &iy,        "0",      NGX_HASH_READONLY_KEY);
    ngx_hash_add_key(&ctx->params, &angle,     "0",      NGX_HASH_READONLY_KEY);

    ngx_hash_add_key(&ctx->params, &e, NGX_HTTP_SMALL_LIGHT_CONVERTER_IMAGEMAGICK, NGX_HASH_READONLY_KEY);
}

ngx_int_t ngx_http_small_light_init_params(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx, ngx_str_t *define_pattern, ngx_http_small_light_conf_t *srv_conf)
{
    char pv[BUFSIZ];
    pv[0] = '\0';
    if (ngx_http_small_light_parse_params(r, ctx, define_pattern, pv) != NGX_OK) {
        return NGX_ERROR;
    }

    if (*pv != '\0') {
        char *pval;
        pval = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&srv_conf->hash, pv);
        if (pval == NULL) {
            ngx_http_small_light_init_params_default(ctx);
            return NGX_OK;
        }
        ngx_str_t pval_str;
        pval_str.data = (u_char *)pval;
        pval_str.len  = ngx_strlen(pval);
        if (ngx_http_small_light_parse_params(r, ctx, &pval_str, pv) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    ngx_http_small_light_init_params_default(ctx);

    return NGX_OK;
}
