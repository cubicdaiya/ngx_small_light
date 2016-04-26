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

#include "ngx_http_small_light_imlib2.h"
#include "ngx_http_small_light_size.h"
#include "ngx_http_small_light_parser.h"
#include "ngx_http_small_light_type.h"
#include "ngx_http_small_light_jpeg.h"

extern const char *ngx_http_small_light_image_types[];
extern const char *ngx_http_small_light_image_exts[];

ngx_int_t ngx_http_small_light_imlib2_init(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imlib2_ctx_t *ictx;

    ictx            = (ngx_http_small_light_imlib2_ctx_t *)ctx->ictx;
    ictx->image     = ctx->content;
    ictx->image_len = ctx->content_length;
    ictx->type      = ngx_http_small_light_type_detect(ictx->image, ictx->image_len);
    ictx->r         = r;
    if (ictx->type == NGX_HTTP_SMALL_LIGHT_IMAGE_NONE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to get image type %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }
    ictx->tf = ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
    if (ictx->tf == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to allocate memory from r->pool %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }
    ictx->tf->file.fd  = NGX_INVALID_FILE;
    ictx->tf->file.log = r->connection->log;
    ictx->tf->path     = ctx->imlib2_temp_dir;
    ictx->tf->pool     = r->pool;

    if (ngx_create_temp_file(&ictx->tf->file, ictx->tf->path, ictx->tf->pool, 1, 0, 0600) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to create temporary file %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    if (ngx_write_file(&ictx->tf->file, ictx->image, ictx->image_len, 0) == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to save temporary file %s:%d",
                      __FUNCTION__,
                      __LINE__);
        ngx_http_small_light_imlib2_term(ctx);
        return NGX_ERROR;
    }

    return NGX_OK;
}

void ngx_http_small_light_imlib2_term(void *data)
{
    ngx_http_small_light_ctx_t *ctx;
    ngx_http_small_light_imlib2_ctx_t *ictx;
    ngx_http_request_t *r;
    ctx  = (ngx_http_small_light_ctx_t *)data;
    ictx = (ngx_http_small_light_imlib2_ctx_t *)ctx->ictx;
    r    = ictx->r;

    if (ngx_delete_file(ictx->tf->file.name.data) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to delete temporary file %s:%d",
                      __FUNCTION__,
                      __LINE__);
    }
}

/** 
 * following original functions are brought from
 * mod_small_light(Dynamic image transformation module for Apache2) and customed
 */

ngx_int_t ngx_http_small_light_imlib2_process(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imlib2_ctx_t *ictx;
    ngx_http_small_light_image_size_t sz;
    Imlib_Image image_org, image_dst, image_tmp;
    Imlib_Load_Error err;
    ngx_file_info_t fi;
    ngx_fd_t fd;
    char *filename, *sharpen, *blur, *of, *buf;
    void *data;
    int w, h, radius, orientation;
    double iw, ih, q;
    ngx_int_t type;
    const char *ext;
    ssize_t size;

    ictx = (ngx_http_small_light_imlib2_ctx_t *)ctx->ictx;

    filename = (char *)ictx->tf->file.name.data;

    /* adjust image size */
    ngx_http_small_light_calc_image_size(r, ctx, &sz, 10000.0, 10000.0);

    if (sz.jpeghint_flg != 0) {
        if (ngx_http_small_light_load_jpeg((void**)&data, &w, &h, r, filename, sz.dw, sz.dh) != NGX_OK) {
            image_org = imlib_load_image_immediately_without_cache(filename);
            if (image_org == NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "failed to load image %s:%d",
                              __FUNCTION__,
                              __LINE__);
                return NGX_ERROR;
            }
        } else {
            image_org = imlib_create_image_using_data(w, h, data);
        }
    } else {
        image_org = imlib_load_image_immediately_without_cache(filename);
        if (image_org == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to load image %s:%d",
                          __FUNCTION__,
                          __LINE__);
            return NGX_ERROR;
        }
    }

    /* rotate. */
    if (sz.angle) {
        orientation = 0;
        switch (sz.angle) {
        case 90:
            orientation = 1;
            break;
        case 180:
            orientation = 2;
            break;
        case 270:
            orientation = 3;
            break;
        default:
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "image not rotated. 'angle'(%ui) must be 90 or 180 or 270. %s:%d",
                          sz.angle,
                          __FUNCTION__,
                          __LINE__);
            break;
        }

        imlib_context_set_image(image_org);
        imlib_image_orientate(orientation);
    }

    /* calc size. */
    imlib_context_set_image(image_org);
    iw = (double)imlib_image_get_width();
    ih = (double)imlib_image_get_height();
    ngx_http_small_light_calc_image_size(r, ctx, &sz, iw, ih);

    /* pass through. */
    if (sz.pt_flg != 0) {
        ctx->of = ctx->inf;
        return NGX_OK;
    }

    /* crop, scale. */
    if (sz.scale_flg != 0) {
        image_dst = imlib_create_cropped_scaled_image((int)sz.sx, (int)sz.sy, (int)sz.sw, (int)sz.sh, (int)sz.dw, (int)sz.dh);
        imlib_context_set_image(image_org);
        imlib_free_image();
    } else {
        image_dst = image_org;
    }

    if (image_dst == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "imlib_create_cropped_scaled_image failed. %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    /* create canvas then draw image to the canvas. */
    if (sz.cw > 0.0 && sz.ch > 0.0) {
        image_tmp = imlib_create_image(sz.cw, sz.ch);
        if (image_tmp == NULL) {
            imlib_context_set_image(image_dst);
            imlib_free_image();
            return NGX_ERROR;
        }
        imlib_context_set_image(image_tmp);
        imlib_context_set_color(sz.cc.r, sz.cc.g, sz.cc.b, sz.cc.a);
        imlib_image_fill_rectangle(0, 0, sz.cw, sz.ch);
        imlib_blend_image_onto_image(image_dst, 255, 0, 0,
                                     (int)sz.dw, (int)sz.dh, (int)sz.dx, (int)sz.dy, (int)sz.dw, (int)sz.dh);
        imlib_context_set_image(image_dst);
        imlib_free_image();
        image_dst = image_tmp;
    }

    /* effects. */
    sharpen = NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(&ctx->hash, "sharpen");
    if (sharpen) {
        radius = ngx_http_small_light_parse_int(sharpen);
        if (radius > 0 && radius <= (int)ctx->radius_max) {
            imlib_context_set_image(image_dst);
            imlib_image_sharpen(radius);
        } else {
            ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                          "As sharpen geometry is out of range, ignored. %s:%d",
                          __FUNCTION__,
                          __LINE__);
        }
    }

    blur = NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(&ctx->hash, "blur");
    if (blur) {
        radius = ngx_http_small_light_parse_int(blur);
        if (radius > 0 && radius <= (int)ctx->radius_max) {
            imlib_context_set_image(image_dst);
            imlib_image_blur(radius);
        } else {
            ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                          "As blur geometry is out of range, ignored. %s:%d",
                          __FUNCTION__,
                          __LINE__);
        }
    }

    /* border. */
    if (sz.bw > 0.0 || sz.bh > 0.0) {
        imlib_context_set_color(sz.bc.r, sz.bc.g, sz.bc.b, sz.bc.a);
        imlib_context_set_image(image_dst);
        if (sz.cw > 0.0 && sz.ch > 0.0) {
            imlib_image_fill_rectangle(0, 0, sz.cw, sz.bh);
            imlib_image_fill_rectangle(0, 0, sz.bw, sz.ch);
            imlib_image_fill_rectangle(0, sz.ch - sz.bh, sz.cw, sz.bh);
            imlib_image_fill_rectangle(sz.cw - sz.bw, 0, sz.bw, sz.ch);
        } else {
            imlib_image_fill_rectangle(0, 0, sz.dw, sz.bh);
            imlib_image_fill_rectangle(0, 0, sz.bw, sz.ch);
            imlib_image_fill_rectangle(0, sz.dh - sz.bh, sz.dw, sz.bh);
            imlib_image_fill_rectangle(sz.dw - sz.bw, 0, sz.bw, sz.dh);
        }
    }

    /* set params. */
    imlib_context_set_image(image_dst);
    q = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(&ctx->hash, "q"));
    if (q > 0.0) {
        imlib_image_attach_data_value("quality", NULL, q, NULL);
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
            of = (char *)ngx_http_small_light_image_exts[ictx->type - 1];
        } else if (type == NGX_HTTP_SMALL_LIGHT_IMAGE_WEBP) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "WebP is not supported %s:%d",
                          __FUNCTION__,
                          __LINE__);
            of = (char *)ngx_http_small_light_image_exts[ictx->type - 1];
        } else {
            ictx->type = type;
        }
        imlib_image_set_format(of);
        ctx->of = ngx_http_small_light_image_types[ictx->type - 1];
    } else {
        ext = ngx_http_small_light_image_exts[ictx->type - 1];
        imlib_image_set_format(ext);
        ctx->of = ctx->inf;
    }

    /* save image. */
    imlib_save_image_with_error_return(filename, &err);
    imlib_free_image();

    /* check error. */
    if (err != IMLIB_LOAD_ERROR_NONE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to imlib_save_error %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    if (ngx_file_info(filename, &fi) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to ngx_file_info %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    fd = ngx_open_file(filename, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to open fd %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to ngx_fd_info %s:%d",
                      __FUNCTION__,
                      __LINE__);
        ngx_close_file(fd);
        return NGX_ERROR;
    } 

    buf = ngx_palloc(r->pool, ngx_file_size(&fi));
    if (buf == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to allocate memory from r->pool %s:%d",
                      __FUNCTION__,
                      __LINE__);
        ngx_close_file(fd);
        return NGX_ERROR;
    }
    size = ngx_read_fd(fd, buf, ngx_file_size(&fi));
    if (size == -1) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to ngx_read_fd %s:%d",
                      __FUNCTION__,
                      __LINE__);
        ngx_close_file(fd);
        return NGX_ERROR;
    }

    if ((size_t)size > ctx->content_length) {
        ctx->content = ngx_palloc(r->pool, size);
        if (ctx->content == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to allocate memory from r->pool %s:%d",
                          __FUNCTION__,
                          __LINE__);
            ngx_close_file(fd);
            return NGX_ERROR;
        }
    }

    ngx_memcpy(ctx->content, buf, size);

    ngx_close_file(fd);

    ctx->content_length = size;

    return NGX_OK;
}
