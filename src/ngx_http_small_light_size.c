/**
   Copyright (c) 2013 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

#include "ngx_http_small_light_size.h"
#include "ngx_http_small_light_parser.h"

// 
// following original functions are brought from mod_small_light(Dynamic image transformation module for Apache2) and customed
// 

void ngx_http_small_light_calc_image_size(ngx_http_request_t *r,
                                          ngx_http_small_light_ctx_t *ctx,
                                          ngx_http_small_light_image_size_t *sz,
                                          double iw, double ih)
{
    
    ngx_http_small_light_coord_t sx_coord, sy_coord, sw_coord, sh_coord;
    ngx_http_small_light_coord_t dx_coord, dy_coord, dw_coord, dh_coord;

    ngx_http_small_light_parse_coord(&sx_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "sx"));
    ngx_http_small_light_parse_coord(&sy_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "sy"));
    ngx_http_small_light_parse_coord(&sw_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "sw"));
    ngx_http_small_light_parse_coord(&sh_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "sh"));
    ngx_http_small_light_parse_coord(&dx_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "dx"));
    ngx_http_small_light_parse_coord(&dy_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "dy"));
    ngx_http_small_light_parse_coord(&dw_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "dw"));
    ngx_http_small_light_parse_coord(&dh_coord, NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "dh"));

    sz->sx = ngx_http_small_light_calc_coord(&sx_coord, iw);
    if (sz->sx == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
        sz->sx = 0;
    }
    sz->sy = ngx_http_small_light_calc_coord(&sy_coord, ih);
    if (sz->sy == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
        sz->sy = 0;
    }
    sz->sw = ngx_http_small_light_calc_coord(&sw_coord, iw);
    if (sz->sw == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
        sz->sw = iw;
    }
    sz->sh = ngx_http_small_light_calc_coord(&sh_coord, ih);
    if (sz->sh == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
        sz->sh = ih;
    }
    sz->dx = ngx_http_small_light_calc_coord(&dx_coord, iw);
    sz->dy = ngx_http_small_light_calc_coord(&dy_coord, ih);
    sz->dw = ngx_http_small_light_calc_coord(&dw_coord, iw);
    sz->dh = ngx_http_small_light_calc_coord(&dh_coord, ih);
    sz->aspect = sz->sw / sz->sh;

    char *da_str = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "da");
    char  da     = da_str[0] ? da_str[0] : 'l';
    if (sz->dw != NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE && sz->dh != NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
        if (da == 'l') {
            if (sz->sw / sz->dw < sz->sh / sz->dh) {
                sz->dw = sz->dh * sz->aspect;
            } else {
                sz->dh = sz->dw / sz->aspect;
            }
        } else if (da == 's') {
            if (sz->sw / sz->dw < sz->sh / sz->dh) {
                sz->dh = sz->dw / sz->aspect;
            } else {
                sz->dw = sz->dh * sz->aspect;
            }
        }
    } else {
        if (sz->dw == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE && sz->dh == sz->dw) {
            double dwo = sz->dw;
            sz->dw = sz->dh / sz->aspect;
            sz->dh = dwo / sz->aspect;
        } else if (sz->dw == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
            sz->dw = sz->dh * sz->aspect;
        } else if (sz->dh == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
            sz->dh = sz->dw / sz->aspect;
        }
    }
    sz->cw = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "cw"));
    sz->ch = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "ch"));
    sz->bw = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "bw"));
    sz->bh = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "bh"));
    sz->ix = ngx_http_small_light_parse_int(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "ix"));
    sz->iy = ngx_http_small_light_parse_int(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "iy"));
    ngx_http_small_light_parse_color(&sz->cc,  NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "cc"));
    ngx_http_small_light_parse_color(&sz->bc,  NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "bc"));
#if 0
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
                  "size info:sx=%f,sy=%f,sw=%f,sh=%f,dw=%f,dh=%f,cw=%f,ch=%f,bw=%f,bh=%f,ix=%d,iy=%d",
                  sz->sx, sz->sy, sz->sw, sz->sh,
                  sz->dw, sz->dh, sz->cw, sz->ch, sz->bw, sz->bh, 
                  sz->ix, sz->iy);
#endif

    // get pass through option.
    ngx_int_t  pt_flg = 0;
    char      *pt     = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "pt");
    if (pt[0] == '\0' || ngx_strcmp(pt, "ptss") == 0) {
        if (sz->sw < sz->cw && sz->sh < sz->ch) {
            pt_flg = 1;
        }
    } else if (ngx_strcmp(pt, "ptls") == 0) {
        if (sz->sw > sz->cw || sz->sh > sz->ch) {
            pt_flg = 1;
        }
    }
    sz->pt_flg = pt_flg;

    // get scaling option.
    char *prm_ds_str = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "ds");
    char  prm_ds     = prm_ds_str[0] ? prm_ds_str[0] : 'l';
    if (prm_ds == 's' || (sz->dw < sz->sw - sz->sx) || (sz->dh < sz->sh - sz->sy)) {
        sz->scale_flg = 1;
    } else {
        sz->scale_flg = 0;
        sz->dw = iw;
        sz->dh = ih;
    }
    if (sz->dx == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
        sz->dx = (sz->cw - sz->dw) * 0.5;
    }
    if (sz->dy == NGX_HTTP_SMALL_LIGHT_COORD_INVALID_VALUE) {
        sz->dy = (sz->ch - sz->dh) * 0.5;
    }

    // get exif option.
    ngx_int_t  inhexif_flg = 0;
    char      *inhexif     = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "inhexif");
    if (inhexif[0] == 'y') {
        inhexif_flg = 1;
    } else {
        inhexif_flg = 0;
    }
    sz->inhexif_flg = inhexif_flg;

    sz->jpeghint_flg = ngx_http_small_light_parse_flag(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "jpeghint"));
    sz->angle        = ngx_http_small_light_parse_int(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "angle"));

}
