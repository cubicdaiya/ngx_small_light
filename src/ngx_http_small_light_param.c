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


#include "ngx_http_small_light_param.h"
#include "ngx_http_small_light_parser.h"

static void ngx_http_small_light_init_params_default(ngx_http_small_light_ctx_t *ctx);

typedef struct ngx_http_small_light_param_t {
    ngx_str_t param;
    char *default_param;
} ngx_http_small_light_param_t;

static const ngx_http_small_light_param_t ngx_http_small_light_params[] = {
    { ngx_string("p"),         ""},
    { ngx_string("e"),         NGX_HTTP_SMALL_LIGHT_CONVERTER_IMAGEMAGICK },
    { ngx_string("q"),         "0"},
    { ngx_string("of"),        ""},
    { ngx_string("jpeghint"),  "n"},
    { ngx_string("dw"),        ""},
    { ngx_string("dh"),        ""},
    { ngx_string("dx"),        ""},
    { ngx_string("dy"),        ""},
    { ngx_string("da"),        "l"},
    { ngx_string("ds"),        "n"},
    { ngx_string("cw"),        ""},
    { ngx_string("ch"),        ""},
    { ngx_string("cc"),        "000000"},
    { ngx_string("bw"),        ""},
    { ngx_string("bh"),        ""},
    { ngx_string("bc"),        "000000"},
    { ngx_string("sw"),        ""},
    { ngx_string("sh"),        ""},
    { ngx_string("sx"),        ""},
    { ngx_string("sy"),        ""},
    { ngx_string("pt"),        "n"},
    { ngx_string("sharpen"),   ""},
    { ngx_string("unsharp"),   ""},
    { ngx_string("blur"),      ""},
    { ngx_string("embedicon"), ""},
    { ngx_string("ix"),        "0"},
    { ngx_string("iy"),        "0"},
    { ngx_string("angle"),     "0"},
    { ngx_string("progressive"), "n"},
    { ngx_string("cmyk2rgb"),  "n"},
    { ngx_string("rmprof"),    "n"},
    { ngx_string("autoorient"),"n"}
};

static const ngx_str_t ngx_http_small_light_getparams[] = {
    ngx_string("arg_p"),
    ngx_string("arg_e"),
    ngx_string("arg_q"),
    ngx_string("arg_of"),
    ngx_string("arg_jpeghint"),
    ngx_string("arg_dw"),
    ngx_string("arg_dh"),
    ngx_string("arg_dx"),
    ngx_string("arg_dy"),
    ngx_string("arg_da"),
    ngx_string("arg_ds"),
    ngx_string("arg_cw"),
    ngx_string("arg_ch"),
    ngx_string("arg_cc"),
    ngx_string("arg_bw"),
    ngx_string("arg_bh"),
    ngx_string("arg_bc"),
    ngx_string("arg_sw"),
    ngx_string("arg_sh"),
    ngx_string("arg_sx"),
    ngx_string("arg_sy"),
    ngx_string("arg_pt"),
    ngx_string("arg_sharpen"),
    ngx_string("arg_unsharp"),
    ngx_string("arg_blur"),
    ngx_string("arg_embedicon"),
    ngx_string("arg_ix"),
    ngx_string("arg_iy"),
    ngx_string("arg_angle"),
    ngx_string("arg_progressive"),
    ngx_string("arg_cmyk2rgb"),
    ngx_string("arg_rmprof"),
    ngx_string("arg_autoorient")
};

static void ngx_http_small_light_init_params_default(ngx_http_small_light_ctx_t *ctx)
{
    ngx_uint_t i, c;
    c = sizeof(ngx_http_small_light_params) / sizeof(ngx_http_small_light_param_t);
    for (i=0;i<c;i++) {
        ngx_hash_add_key(&ctx->params,
                         (ngx_str_t *)&ngx_http_small_light_params[i].param,
                         ngx_http_small_light_params[i].default_param,
                         NGX_HASH_READONLY_KEY);
    }
}

ngx_int_t ngx_http_small_light_init_params(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx, ngx_str_t *define_pattern, ngx_http_small_light_conf_t *srv_conf)
{
    char      pv[BUFSIZ], *pval;
    ngx_str_t pval_str;

    pv[0] = '\0';
    if (ngx_http_small_light_parse_params(r, ctx, define_pattern, pv) != NGX_OK) {
        return NGX_ERROR;
    }

    if (*pv != '\0') {
        pval = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&srv_conf->hash, pv);
        if (pval == NULL) {
            ngx_http_small_light_init_params_default(ctx);
            return NGX_OK;
        }
        pval_str.data = (u_char *)pval;
        pval_str.len  = ngx_strlen(pval);
        if (ngx_http_small_light_parse_params(r, ctx, &pval_str, pv) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    ngx_http_small_light_init_params_default(ctx);

    return NGX_OK;
}

ngx_int_t ngx_http_small_light_init_getparams(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx, ngx_http_small_light_conf_t *srv_conf)
{
    ngx_uint_t                 i;
    size_t                     args_size, arg_prefix_len;
    u_char                    *low, *v;
    ngx_http_variable_value_t *var;
    ngx_uint_t                 key;
    ngx_str_t                  ks, pval_str, *args;
    char                       pv[BUFSIZ], *pval;

    pv[0] = '\0';
    args = (ngx_str_t *)&ngx_http_small_light_getparams;
    args_size = sizeof(ngx_http_small_light_getparams) / sizeof(ngx_str_t);
    arg_prefix_len = sizeof("arg_") - 1;
    for (i=0;i<args_size;i++) {
        low = ngx_pnalloc(r->pool, args[i].len);
        if (low == NULL) {
            return NGX_ERROR;
        }
        key = ngx_hash_strlow(low, args[i].data, args[i].len);
        var = ngx_http_get_variable(r, &args[i], key);

        if (!var->not_found) {
            ks.data = ngx_palloc(r->pool, args[i].len + 1);
            if (ks.data == NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "failed to allocate memory from r->pool %s:%d",
                              __FUNCTION__,
                              __LINE__);
                return NGX_ERROR;
            }
            ngx_cpystrn(ks.data, args[i].data + arg_prefix_len, args[i].len - arg_prefix_len + 1);
            ks.len = args[i].len - arg_prefix_len;

            if (i == 0) { /* arg_p is found */
                ngx_cpystrn((u_char *)pv, var->data, var->len + 1);
            } else {
                v = ngx_palloc(r->pool, var->len + 1);
                if (v == NULL) {
                    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                                  "failed to allocate memory from r->pool %s:%d",
                                  __FUNCTION__,
                                  __LINE__);
                    return NGX_ERROR;
                }
                ngx_cpystrn(v, var->data, var->len + 1);
                ngx_hash_add_key(&ctx->params, &ks, v, NGX_HASH_READONLY_KEY);
            }
        }
    }

    if (*pv != '\0') {
        pval = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&srv_conf->hash, pv);
        if (pval == NULL) {
            ngx_http_small_light_init_params_default(ctx);
            return NGX_OK;
        }
        pval_str.data = (u_char *)pval;
        pval_str.len  = ngx_strlen(pval);
        if (ngx_http_small_light_parse_params(r, ctx, &pval_str, pv) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    ngx_http_small_light_init_params_default(ctx);

    return NGX_OK;
}
