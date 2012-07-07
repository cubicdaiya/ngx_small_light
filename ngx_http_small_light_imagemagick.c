/**
   Copyright (c) 2012 Tatsuhiko Kubo <cubicdaiya@gmail.com>
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

#include "ngx_http_small_light_imagemagick.h"
#include "ngx_http_small_light_parser.h"

void ngx_http_small_light_imagemagick_init(ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imagemagick_ctx_t *ictx;
    MagickWandGenesis();
    ictx            = (ngx_http_small_light_imagemagick_ctx_t *)ctx->ictx;
    ictx->wand      = NewMagickWand();
    ictx->image     = ctx->content;
    ictx->image_len = ctx->content_length;
}

void ngx_http_small_light_imagemagick_term(ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imagemagick_ctx_t *ictx;
    ictx = (ngx_http_small_light_imagemagick_ctx_t *)ctx->ictx;
    DestroyMagickWand(ictx->wand);
    MagickWandTerminus();
}

// 
// following original functions are brought from mod_small_light(Dynamic image transformation module for Apache2) and customed
// 

ngx_int_t ngx_http_small_light_imagemagick_process(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imagemagick_ctx_t *ictx;
    ngx_http_small_light_image_size_t sz;
    MagickBooleanType status;

    status = MagickFalse;

    ictx = (ngx_http_small_light_imagemagick_ctx_t *)ctx->ictx;

    // adjust image size
    ngx_http_small_light_calc_image_size(r, ctx, &sz, 10000.0, 10000.0);

    // init
    ictx->wand = NewMagickWand();

    // prepare
    if (sz.jpeghint_flg != 0) {
        char *jpeg_size_opt;
        jpeg_size_opt = ngx_pcalloc(r->pool, 32 + 1);
        if (jpeg_size_opt == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }
        ngx_snprintf(jpeg_size_opt, 32 + 1, "%dx%d", (ngx_int_t)sz.dw, (ngx_int_t)sz.dh);
        MagickSetOption(ictx->wand, "jpeg:size", jpeg_size_opt);
    }

    // load image.
    status = MagickReadImageBlob(ictx->wand, (void *)ictx->image, ictx->image_len);
    if (status == MagickFalse) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "couldn't read image %s:%d", __FUNCTION__, __LINE__);
        r->err_status = NGX_HTTP_INTERNAL_SERVER_ERROR;
        return NGX_ERROR;
    }

    // remove all profiles
    int rmprof_flg = ngx_http_small_light_parse_flag(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "rmprof"));
    if (rmprof_flg != 0) {
        status = MagickProfileImage(ictx->wand, "*", NULL, 0);
        if (status == MagickFalse) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "couldn't profiling image %s:%d", __FUNCTION__, __LINE__);
        }
    }

    // calc size.
    double iw = (double)MagickGetImageWidth(ictx->wand);
    double ih = (double)MagickGetImageHeight(ictx->wand);
    ngx_http_small_light_calc_image_size(r, ctx, &sz, iw, ih);

    // pass through.
    if (sz.pt_flg != 0) {
        return NGX_OK;
    }

    // crop, scale.
    status = MagickTrue;
    if (sz.scale_flg != 0) {
        char *crop_geo;
        char *size_geo;
        crop_geo = ngx_pcalloc(r->pool, 128 + 1);
        if (crop_geo == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }
        size_geo = ngx_pcalloc(r->pool, 128 + 1);
        if (size_geo == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }
        ngx_snprintf(crop_geo, 128 + 1, "%f!x%f!+%f+%f", sz.sw, sz.sh, sz.sx, sz.sy);
        ngx_snprintf(size_geo, 128 + 1, "%f!x%f!",       sz.dw, sz.dh);
        MagickWand *trans_wand;
        trans_wand = MagickTransformImage(ictx->wand, crop_geo, size_geo);
        if (trans_wand == NULL || trans_wand == ictx->wand) {
            r->err_status = NGX_HTTP_INTERNAL_SERVER_ERROR;
            return NGX_ERROR;
        }
        DestroyMagickWand(ictx->wand);
        ictx->wand = trans_wand;
    }

    // create canvas then draw image to the canvas.
    if (sz.cw > 0.0 && sz.ch > 0.0) {
        MagickWand *canvas_wand  = NewMagickWand();
        PixelWand  *canvas_color = NewPixelWand();
        PixelSetRed(canvas_color,   sz.cc.r / 255.0);
        PixelSetGreen(canvas_color, sz.cc.g / 255.0);
        PixelSetBlue(canvas_color,  sz.cc.b / 255.0);
        PixelSetAlpha(canvas_color, sz.cc.a / 255.0);
        status = MagickNewImage(canvas_wand, sz.cw, sz.ch, canvas_color);
        DestroyPixelWand(canvas_color);
        if (status == MagickFalse) {
            r->err_status = NGX_HTTP_INTERNAL_SERVER_ERROR;
            return NGX_ERROR;
        }
        status = MagickCompositeImage(canvas_wand, ictx->wand, AtopCompositeOp, sz.dx, sz.dy);
        if (status == MagickFalse) {
            r->err_status = NGX_HTTP_INTERNAL_SERVER_ERROR;
            return NGX_ERROR;
        }
        DestroyMagickWand(ictx->wand);
        ictx->wand = canvas_wand;
    }

    // effects.
    char *unsharp = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "unsharp");
    if (unsharp != NULL) {
        GeometryInfo geo;
        ParseGeometry(unsharp, &geo);
        status = MagickUnsharpMaskImage(ictx->wand, geo.rho, geo.sigma, geo.xi, geo.psi);
        if (status == MagickFalse) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "unsharp failed %s:%d", __FUNCTION__, __LINE__);
        }
    }

    char *sharpen = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "sharpen");
    if (sharpen != NULL) {
        GeometryInfo geo;
        ParseGeometry(sharpen, &geo);
        status = MagickSharpenImage(ictx->wand, geo.rho, geo.sigma);
        if (status == MagickFalse) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "sharpen failed %s:%d", __FUNCTION__, __LINE__);
        }
    }

    char *blur = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "blur");
    if (blur) {
        GeometryInfo geo;
        ParseGeometry(blur, &geo);
        status = MagickBlurImage(ictx->wand, geo.rho, geo.sigma);
        if (status == MagickFalse) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "blur failed %s:%d", __FUNCTION__, __LINE__);
        }
    }

    // border.
    if (sz.bw > 0.0 || sz.bh > 0.0) {
        DrawingWand *border_wand = NewDrawingWand();
        PixelWand *border_color;
        border_color = NewPixelWand();
        PixelSetRed(border_color,   sz.bc.r / 255.0);
        PixelSetGreen(border_color, sz.bc.g / 255.0);
        PixelSetBlue(border_color,  sz.bc.b / 255.0);
        PixelSetAlpha(border_color, sz.bc.a / 255.0);
        DrawSetFillColor(border_wand, border_color);
        DrawSetStrokeColor(border_wand, border_color);
        DrawSetStrokeWidth(border_wand, 1);
        DrawRectangle(border_wand, 0, 0, sz.cw - 1, sz.bh - 1);
        DrawRectangle(border_wand, 0, 0, sz.bw - 1, sz.ch - 1);
        DrawRectangle(border_wand, 0, sz.ch - sz.bh, sz.cw - 1, sz.ch - 1);
        DrawRectangle(border_wand, sz.cw - sz.bw, 0, sz.cw - 1, sz.ch - 1);
        MagickDrawImage(ictx->wand, border_wand);
        DestroyPixelWand(border_color);
        DestroyDrawingWand(border_wand);
    }

    // embed icon
    char *embedicon = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "embedicon");
    if (ngx_strlen(ctx->material_dir) > 0 && ngx_strlen(embedicon) > 0) {
        MagickWand *base_wand = NewMagickWand();
        MagickWand *icon_wand = NewMagickWand();
        char   *p;
        char   *embedicon_path;
        size_t  embedicon_path_len;
        size_t  embedicon_len;

        embedicon_len      = ngx_strlen(embedicon);
        embedicon_path_len = ctx->material_dir->len + ngx_strlen("/") + embedicon_len;
        embedicon_path     = ngx_palloc(r->pool, embedicon_path_len + 1);

        p = embedicon_path;
        p = ngx_cpystrn(p, ctx->material_dir->data, ctx->material_dir->len + 1);
        p = ngx_cpystrn(p, "/", 1 + 1);
        p = ngx_cpystrn(p, embedicon, embedicon_len + 1);
        
        if (ngx_open_file(embedicon_path, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0) == NGX_INVALID_FILE) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }

        if (MagickReadImage(icon_wand, embedicon_path) == MagickFalse) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to read embed icon image. %s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }

        MagickCompositeImageChannel(ictx->wand, AllChannels, icon_wand, OverCompositeOp, sz.ix, sz.iy);
        ClearMagickWand(icon_wand);
    }

    // set params.
    double q = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "q"));
    if (q > 0.0) {
        MagickSetImageCompressionQuality(ictx->wand, q);
    }
    char *of = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "of");
    if (ngx_strlen(of) > 0) {
        MagickSetFormat(ictx->wand, of);
        char *s = ngx_pcalloc(r->pool, 10 + 1);
        if (s == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }
        ngx_snprintf(s, 10 + 1, "image/%s", of);
        ctx->of = s;
    } else {
        MagickSetFormat(ictx->wand, ctx->inf);
        ctx->of = ctx->inf;
    }

    // get small_lighted image as binary.
    u_char *canvas_buf;
    u_char *sled_image;
    size_t sled_image_size;
    canvas_buf = MagickGetImageBlob(ictx->wand, &sled_image_size);
    sled_image = ngx_pcalloc(r->pool, sled_image_size);
    if (sled_image == NULL) {
        MagickRelinquishMemory(canvas_buf);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }
    ngx_cpymem(sled_image, canvas_buf, sled_image_size);
    MagickRelinquishMemory(canvas_buf);

    r->headers_out.content_type.data    = ctx->of;
    r->headers_out.content_type.len     = ngx_strlen(ctx->of);
    r->headers_out.content_type_len     = ngx_strlen(ctx->of);
    r->headers_out.content_type_lowcase = NULL;

    ctx->content        = sled_image;
    ctx->content_length = sled_image_size;

    return NGX_OK;
}
