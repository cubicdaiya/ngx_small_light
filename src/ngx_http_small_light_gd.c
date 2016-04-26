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

#include "ngx_http_small_light_gd.h"
#include "ngx_http_small_light_size.h"
#include "ngx_http_small_light_parser.h"
#include "ngx_http_small_light_type.h"

extern const char *ngx_http_small_light_image_types[];

static gdImagePtr ngx_http_small_light_gd_new(int w, int h, int colors)
{
    gdImagePtr img;
    if (colors == 0) {
        img = gdImageCreateTrueColor(w, h);
        if (img == NULL) {
            return NULL;
        }
    } else {
        img = gdImageCreate(w, h);
        if (img == NULL) {
            return NULL;
        }
    }

    return img;
}

static gdImagePtr ngx_http_small_light_gd_src(ngx_http_small_light_gd_ctx_t *ictx)
{
    gdImagePtr src = NULL;
    switch (ictx->type) {
    case NGX_HTTP_SMALL_LIGHT_IMAGE_JPEG:
        src = gdImageCreateFromJpegPtr(ictx->image_len, ictx->image);
        break;
    case NGX_HTTP_SMALL_LIGHT_IMAGE_GIF:
        src = gdImageCreateFromGifPtr(ictx->image_len,  ictx->image);
        break;
    case NGX_HTTP_SMALL_LIGHT_IMAGE_PNG:
        src = gdImageCreateFromPngPtr(ictx->image_len,  ictx->image);
        break;
    default:
        break;
    }

    return src;
}

static u_char *ngx_http_small_light_gd_out(gdImagePtr img, ngx_int_t type, int *size, double q)
{
    u_char *out = NULL;
    switch (type) {
    case NGX_HTTP_SMALL_LIGHT_IMAGE_JPEG:
        out = gdImageJpegPtr(img, size, (int)q);
        break;
    case NGX_HTTP_SMALL_LIGHT_IMAGE_GIF:
        out = gdImageGifPtr(img, size);
        break;
    case NGX_HTTP_SMALL_LIGHT_IMAGE_PNG:
        out = gdImagePngPtr(img, size);
        break;
    case NGX_HTTP_SMALL_LIGHT_IMAGE_WEBP:
#ifdef NGX_HTTP_SMALL_LIGHT_GD_WEBP_ENABLED
        out = gdImageWebpPtrEx(img, size, (int)q);
#endif
        break;
    default:
        break;
    }

    return out;
}

ngx_int_t ngx_http_small_light_gd_init(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_gd_ctx_t *ictx;

    ictx            = (ngx_http_small_light_gd_ctx_t *)ctx->ictx;
    ictx->image     = ctx->content;
    ictx->image_len = ctx->content_length;
    ictx->complete  = 0;

    ictx->type = ngx_http_small_light_type_detect(ictx->image, ictx->image_len);
    if (ictx->type == NGX_HTTP_SMALL_LIGHT_IMAGE_NONE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to get image type %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    return NGX_OK;
}

void ngx_http_small_light_gd_term(void *data)
{
    ngx_http_small_light_ctx_t *ctx;
    ngx_http_small_light_gd_ctx_t *ictx;
    ctx = (ngx_http_small_light_ctx_t *)data;
    ictx = (ngx_http_small_light_gd_ctx_t *)ctx->ictx;

    if (ictx->complete) {
        gdFree(ctx->content);
    }
}

ngx_int_t ngx_http_small_light_gd_process(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_gd_ctx_t     *ictx;
    ngx_http_small_light_image_size_t  sz;
    gdImagePtr                         src, dst, canvas;
    ngx_int_t                          colors, transparent, palette, red, green, blue;
    ngx_int_t                          ax, ay, ox, oy, type;
    int                                iw, ih, radius, ccolor, bcolor;
    char                              *sharpen, *of;
    u_char                            *out;
    int                                size;
    double                             q;

    ictx = (ngx_http_small_light_gd_ctx_t *)ctx->ictx;
    src  = ngx_http_small_light_gd_src(ictx);
    if (src == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to get image source %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    /* adjust image size */
    iw = gdImageSX(src);
    ih = gdImageSY(src);
    ngx_http_small_light_calc_image_size(r, ctx, &sz, iw, ih);

    colors      = gdImageColorsTotal(src);
    palette     = 0;
    transparent = -1;
    red         = 0;
    green       = 0;
    blue        = 0;

    if (colors) {
        transparent = gdImageGetTransparent(src);

        if (transparent != -1) {
            palette = colors;
            red     = gdImageRed(src,   transparent);
            green   = gdImageGreen(src, transparent);
            blue    = gdImageBlue(src,  transparent);
        }
    }

    gdImageColorTransparent(src, -1);

    /* rotate. */
    if (sz.angle) {
        dst = src;
        ax  = (iw % 2 == 0) ? 1 : 0;
        ay  = (ih % 2 == 0) ? 1 : 0;
        switch (sz.angle) {
        case 90:
        case 270:
            dst = ngx_http_small_light_gd_new(ih, iw, palette);
            if (dst == NULL) {
                gdImageDestroy(src);
                return NGX_ERROR;
            }

            if (sz.angle == 90) {
                ox = ih / 2 - ay;
                oy = iw / 2 + ax;
            } else {
                ox = ih / 2 + ay;
                oy = iw / 2 - ax;
            }

            gdImageCopyRotated(dst, src, ox, oy, 0, 0,
                               iw, ih, -sz.angle);
            gdImageDestroy(src);

            break;
        case 180:
            dst = ngx_http_small_light_gd_new(iw, ih, palette);
            if (dst == NULL) {
                gdImageDestroy(src);
                return NGX_ERROR;
            }
            gdImageCopyRotated(dst, src, iw / 2 - ax, ih / 2 - ay, 0, 0,
                               iw + ax, ih + ay, sz.angle);
            gdImageDestroy(src);
            break;
        default:
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "image not rotated. 'angle'(%ui) must be 90 or 180 or 270. %s:%d",
                          sz.angle,
                          __FUNCTION__,
                          __LINE__);
            break;
        }
        src = dst;
    }

    /* calc size. */
    iw = gdImageSX(src);
    ih = gdImageSY(src);
    ngx_http_small_light_calc_image_size(r, ctx, &sz, iw, ih);

    /* pass through. */
    if (sz.pt_flg != 0) {
        gdImageDestroy(src);
        ctx->of = ctx->inf;
        return NGX_OK;
    }

    /* crop, scale. */
    if (sz.scale_flg != 0) {
        dst = ngx_http_small_light_gd_new(sz.dw, sz.dh, palette);
        if (dst == NULL) {
            gdImageDestroy(src);
            return NGX_ERROR;
        }

        if (colors == 0) {
            gdImageSaveAlpha(dst, 1);
            gdImageAlphaBlending(dst, 0);
        }

        gdImageCopyResampled(dst, src, 0, 0, sz.sx, sz.sy, sz.dw, sz.dh, sz.sw, sz.sh);

        if (colors) {
            gdImageTrueColorToPalette(dst, 1, 256);
        }

        gdImageDestroy(src);
    } else {
        dst = src;
    }

    if (transparent != -1 && colors) {
        gdImageColorTransparent(dst, gdImageColorExact(dst, red, green, blue));
    }

    /* effects. */
    sharpen = NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(&ctx->hash, "sharpen");
    if (sharpen != NULL) {
        radius = ngx_http_small_light_parse_int(sharpen);
        if (radius > 0 && radius <= (int)ctx->radius_max) {
            gdImageSharpen(dst, radius);
        } else {
            ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                          "As sharp geometry is out of range, ignored. %s:%d",
                          __FUNCTION__,
                          __LINE__);
        }
    }

    /* interlace */
    gdImageInterlace(dst, 1);

    /* create canvas then draw image to the canvas. */
    if (sz.cw > 0.0 && sz.ch > 0.0) {
        canvas = gdImageCreateTrueColor(sz.cw, sz.ch);
        if (canvas == NULL) {
            gdImageDestroy(dst);
            return NGX_ERROR;
        }
        ccolor = gdImageColorAllocateAlpha(canvas, sz.cc.r, sz.cc.g, sz.cc.b, sz.cc.a);
        gdImageFilledRectangle(canvas, 0, 0, sz.cw, sz.ch, ccolor);
        gdImageCopy(canvas, dst, sz.dx, sz.dy, 0, 0, sz.dw, sz.dh);
        gdImageDestroy(dst);
        dst = canvas;
    }

    /* border. */
    if (sz.bw > 0.0 || sz.bh > 0.0) {
        bcolor = gdImageColorAllocateAlpha(dst, sz.bc.r, sz.bc.g, sz.bc.b, sz.bc.a);
        if (sz.cw > 0.0 && sz.ch > 0.0) {
            gdImageFilledRectangle(dst, 0, 0, sz.cw, sz.bh, bcolor);
            gdImageFilledRectangle(dst, 0, 0, sz.bw, sz.ch, bcolor);
            gdImageFilledRectangle(dst, 0, sz.ch - sz.bh, sz.cw - 1, sz.ch - 1, bcolor);
            gdImageFilledRectangle(dst, sz.cw - sz.bw, 0, sz.cw - 1, sz.ch - 1, bcolor);
        } else {
            gdImageFilledRectangle(dst, 0, 0, sz.dw, sz.bh, bcolor);
            gdImageFilledRectangle(dst, 0, 0, sz.bw, sz.dh, bcolor);
            gdImageFilledRectangle(dst, 0, sz.dh - sz.bh, sz.dw - 1, sz.dh - 1, bcolor);
            gdImageFilledRectangle(dst, sz.dw - sz.bw, 0, sz.dw - 1, sz.dh - 1, bcolor);
        }
    }

    of = NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(&ctx->hash, "of");
    if (ngx_strlen(of) > 0) {
        type = ngx_http_small_light_type(of);
        if (type == NGX_HTTP_SMALL_LIGHT_IMAGE_NONE) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "of is invalid(%s) %s:%d",
                          of,
                          __FUNCTION__,
                          __LINE__);
        } else if (type == NGX_HTTP_SMALL_LIGHT_IMAGE_WEBP) {
#ifdef NGX_HTTP_SMALL_LIGHT_GD_WEBP_ENABLED
            ictx->type = type;
#else
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "WebP is not supported %s:%d",
                          __FUNCTION__,
                          __LINE__);
#endif
        } else {
            ictx->type = type;
        }
    }

    ctx->of = ngx_http_small_light_image_types[ictx->type - 1];

    q = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(&ctx->hash, "q"));
    if (q == 0) {
        q = 100;
    }
    out = ngx_http_small_light_gd_out(dst, ictx->type, (int *)&size, q);
    gdImageDestroy(dst);

    if (out == NULL) {
        return NGX_ERROR;
    }

    /* get small_lighted image as binary. */
    ctx->content        = out;
    ctx->content_length = size;

    ngx_pfree(r->pool, ctx->content_orig);

    ictx->complete = 1;

    return NGX_OK;
}
