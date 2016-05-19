/**
   Copyright (c) 2012-2016 Tatsuhiko Kubo <cubicdaiya@gmail.com>
   Copyright (c) 1996-2011 livedoor Co.,Ltd.

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

#include "ngx_http_small_light_parser.h"

ngx_int_t ngx_http_small_light_parse_define_pattern(ngx_http_request_t *r, ngx_str_t *unparsed_uri, ngx_str_t *define_pattern)
{
    u_char errstr[NGX_MAX_CONF_ERRSTR];
    ngx_regex_compile_t rgc;
    ngx_str_t pattern = ngx_string("small_light\\(([^\\)]*)\\)");
    int capture_start, capture_end, capture_len, captures[(1 + 2) * 3];
    ngx_int_t rc;

    ngx_memzero(&rgc, sizeof(ngx_regex_compile_t));

    rgc.pattern  = pattern;
    rgc.pool     = r->pool;
    rgc.err.len  = NGX_MAX_CONF_ERRSTR;
    rgc.err.data = errstr;

    if (ngx_regex_compile(&rgc) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%V", &rgc.err);
        return NGX_ERROR;
    }

    rc = ngx_regex_exec(rgc.regex, unparsed_uri, captures, (1 + 2) * 3);

    if (rc >= 0) {
        capture_start        = captures[2];
        capture_end          = captures[3];
        capture_len          = capture_end - capture_start;
        define_pattern->data = unparsed_uri->data + capture_start;
        define_pattern->len  = capture_len;
    } else {
        return NGX_ERROR;
    }

    return NGX_OK;
}

ngx_int_t ngx_http_small_light_parse_params(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx, ngx_str_t *define_pattern, char *pv)
{
    char *tk, *tv, *sp1, *sp2;
    char *k, *kk, *v, *vv;
    size_t kl, vl;
    ngx_str_t ks;
    char p[BUFSIZ];
    if (define_pattern->len > BUFSIZ - 1) {
        return NGX_ERROR;
    }
    ngx_cpystrn((u_char *)p, define_pattern->data, define_pattern->len + 1);
    tk = strtok_r(p, ",", &sp1);
    while (tk != NULL) {
        tv = strtok_r(tk, "=", &sp2);
        k  = tv;
        v  = strtok_r(NULL, "=", &sp2);
        if (k == NULL || v == NULL) {
            return NGX_OK;
        }
        vl = ngx_strlen(v);
        if (ngx_strcmp(k, "p") == 0) {
            ngx_cpystrn((u_char *)pv, (u_char *)v, vl + 1);
        } else {
            kl = ngx_strlen(k);
            kk = ngx_palloc(r->pool, kl + 1);
            if (kk == NULL) {
                return NGX_ERROR;
            }
            ngx_cpystrn((u_char *)kk, (u_char *)k, kl + 1);
            ks.data = (u_char *)kk;
            ks.len  = kl;
            vv = ngx_palloc(r->pool, vl + 1);
            if (vv == NULL) {
                return NGX_ERROR;
            }
            ngx_cpystrn((u_char *)vv, (u_char *)v, vl + 1);
            ngx_hash_add_key(&ctx->params, &ks, vv, NGX_HASH_READONLY_KEY);
        }
        tk = strtok_r(NULL, ",", &sp1);
    }

    return NGX_OK;
}

/** 
 * following original functions are brought from
 * mod_small_light(Dynamic image transformation module for Apache2) and customed
 */

ngx_int_t ngx_http_small_light_parse_flag(const char *s)
{
    if (s != NULL && s[0] == 'y') {
        return 1;
    }
    return 0;
}

ngx_int_t ngx_http_small_light_parse_int(const char *s)
{
    return atoi(s);
}

double ngx_http_small_light_parse_double(const char *s)
{
    return atof(s);
}

ngx_int_t ngx_http_small_light_parse_coord(ngx_http_small_light_coord_t *crd, const char *s)
{
    if (s[0] == '\0') {
        crd->v = 0;
        crd->u = NGX_HTTP_SMALL_LIGHT_COORD_UNIT_NONE;
        return NGX_OK;
    }
    crd->v = atof(s);
    while (((*s >= '0' && *s <= '9') || *s == '.') && *s != '\0') s++;
    if (*s == 'p') {
        crd->u = NGX_HTTP_SMALL_LIGHT_COORD_UNIT_PERCENT;
        return NGX_OK;
    } else if (*s == '\0') {
        crd->u = NGX_HTTP_SMALL_LIGHT_COORD_UNIT_PIXEL;
        return NGX_OK;
    }
    return NGX_ERROR;
}

double ngx_http_small_light_calc_coord(ngx_http_small_light_coord_t *crd, double v)
{
    if (crd->u == NGX_HTTP_SMALL_LIGHT_COORD_UNIT_PIXEL) {
        return crd->v;
    } else if (crd->u == NGX_HTTP_SMALL_LIGHT_COORD_UNIT_PERCENT) {
        return (v * crd->v * 0.01);
    }
    return NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE;
}

ngx_int_t ngx_http_small_light_parse_color(ngx_http_small_light_color_t *color, const char *s)
{
    int res;
    size_t len = strlen(s);
    if (len == 3) {
        res = sscanf(s, "%1hx%1hx%1hx", &color->r, &color->g, &color->b);
        if (res != EOF) {
            color->a = 255;
            return NGX_OK;
        }
    } else if (len == 4) {
        res = sscanf(s, "%1hx%1hx%1hx%1hx", &color->r, &color->g, &color->b, &color->a);
        if (res != EOF) {
            return NGX_OK;
        }
    } else if (len == 6) {
        res = sscanf(s, "%02hx%02hx%02hx", &color->r, &color->g, &color->b);
        if (res != EOF) {
            color->a = 255;
            return NGX_OK;
        }
    } else if (len == 8) {
        res = sscanf(s, "%02hx%02hx%02hx%02hx", &color->r, &color->g, &color->b, &color->a);
        if (res != EOF) {
            return NGX_OK;
        }
    }
    return NGX_ERROR;
}
