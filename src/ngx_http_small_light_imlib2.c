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

#include "ngx_http_small_light_imlib2.h"
#include "ngx_http_small_light_size.h"
#include "ngx_http_small_light_parser.h"
#include "ngx_http_small_light_jpeg.h"

ngx_int_t ngx_http_small_light_imlib2_init(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imlib2_ctx_t *ictx;

    ictx            = (ngx_http_small_light_imlib2_ctx_t *)ctx->ictx;
    ictx->image     = ctx->content;
    ictx->image_len = ctx->content_length;
    ictx->tf = ngx_pcalloc(r->pool, sizeof(ngx_temp_file_t));
    if (ictx->tf == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to allocate memory from r->pool %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }
    ictx->tf->file.fd  = NGX_INVALID_FILE;
    ictx->tf->file.log = r->connection->log;
    ictx->tf->path     = ctx->imlib2_temp_dir;
    ictx->tf->pool     = r->pool;

    if (ngx_create_temp_file(&ictx->tf->file, ictx->tf->path, ictx->tf->pool, 1, 0, 0600) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to create temporary file %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    if (ngx_write_file(&ictx->tf->file, ictx->image, ictx->image_len, 0) == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to save temporary file %s:%d", __FUNCTION__, __LINE__);
        ngx_http_small_light_imlib2_term(r, ctx);
        return NGX_ERROR;
    }

    return NGX_OK;
}

ngx_int_t ngx_http_small_light_imlib2_term(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imlib2_ctx_t *ictx;
    ictx = (ngx_http_small_light_imlib2_ctx_t *)ctx->ictx;

    if (ngx_delete_file(ictx->tf->file.name.data) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to delete temporary file %s:%d", __FUNCTION__, __LINE__);
    }

    return NGX_OK;
}

// 
// following original functions are brought from mod_small_light(Dynamic image transformation module for Apache2) and customed
// 

ngx_int_t ngx_http_small_light_imlib2_process(ngx_http_request_t *r, ngx_http_small_light_ctx_t *ctx)
{
    ngx_http_small_light_imlib2_ctx_t *ictx;
    ngx_http_small_light_image_size_t sz;
    char *filename;

    ictx = (ngx_http_small_light_imlib2_ctx_t *)ctx->ictx;

    filename = (char *)ictx->tf->file.name.data;

    // adjust image size
    ngx_http_small_light_calc_image_size(r, ctx, &sz, 10000.0, 10000.0);

    Imlib_Image image_org;

    if (sz.jpeghint_flg != 0) {
        void *data;
        int w, h;
        if (load_jpeg((void**)&data, &w, &h, r, filename, sz.dw, sz.dh) != NGX_OK) {
            image_org = imlib_load_image_immediately_without_cache(filename);
            if (image_org == NULL) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to load image %s:%d", __FUNCTION__, __LINE__);
                return NGX_ERROR;
            }
        } else {
            image_org = imlib_create_image_using_data(w, h, data);
        }
    } else {
        image_org = imlib_load_image_immediately_without_cache(filename);
        if (image_org == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to load image %s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }
    }

    // calc size.
    imlib_context_set_image(image_org);
    double iw = (double)imlib_image_get_width();
    double ih = (double)imlib_image_get_height();
    ngx_http_small_light_calc_image_size(r, ctx, &sz, iw, ih);

    // pass through.
    if (sz.pt_flg != 0) {
        return NGX_OK;
    }

    char *inf;
    ngx_str_t inf_imlib;
    inf_imlib.data = (u_char *)imlib_image_format();
    inf_imlib.len  = ngx_strlen(inf_imlib.data);
    inf = (char *)ngx_pstrdup(r->pool, &inf_imlib);
    inf[inf_imlib.len] = '\0';

    Imlib_Image image_dst;

    // crop, scale.
    if (sz.scale_flg != 0) {
        image_dst = imlib_create_cropped_scaled_image((int)sz.sx, (int)sz.sy, (int)sz.sw, (int)sz.sh, (int)sz.dw, (int)sz.dh);
        imlib_context_set_image(image_org);
        imlib_free_image();
    } else {
        image_dst = image_org;
    }

    if (image_dst == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "imlib_create_cropped_scaled_image failed. %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    if (sz.angle == 90 || sz.angle == 180 || sz.angle == 270) {
        ngx_int_t t;
        imlib_context_set_image(image_dst);
        switch(sz.angle) {
        case 90:
            imlib_image_orientate(1);
            break;
        case 180:
            imlib_image_orientate(2);
            break;
        case 270:
            imlib_image_orientate(3);
            break;
        }

        t     = sz.dw;
        sz.dw = sz.dh;
        sz.dh = t;

    }

    // create canvas then draw image to the canvas.
    if (sz.cw > 0.0 && sz.ch > 0.0) {
        Imlib_Image image_tmp = imlib_create_image(sz.cw, sz.ch);
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

    // effects.
    char *sharpen = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "sharpen");
    if (sharpen) {
        int radius = ngx_http_small_light_parse_int(sharpen);
        if (radius > 0) {
            imlib_context_set_image(image_dst);
            imlib_image_sharpen(radius);
        }
    }

    char *blur = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "blur");
    if (blur) {
        int radius = ngx_http_small_light_parse_int(blur);
        if (radius > 0) {
            imlib_context_set_image(image_dst);
            imlib_image_blur(radius);
        }
    }

    // border.
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

    // set params.
    imlib_context_set_image(image_dst);
    double q = ngx_http_small_light_parse_double(NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "q"));
    if (q > 0.0) {
        imlib_image_attach_data_value("quality", NULL, q, NULL);
    }

    char *of = NGX_HTTP_SMALL_LIGHT_PARAM_GET(&ctx->hash, "of");
    if (ngx_strlen(of) > 0) {
        imlib_image_set_format(of);
        u_char *s = ngx_pcalloc(r->pool, 10 + 1);
        if (s == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to allocate memory from r->pool %s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }
        ngx_snprintf(s, 10 + 1, "image/%s", of);
        ctx->of = (char *)s;
    } else {
        imlib_image_set_format(inf);
        ctx->of = ctx->inf;
    }

    // save image.
    Imlib_Load_Error err;
    imlib_save_image_with_error_return(filename, &err);
    imlib_free_image();

    // check error.
    if (err != IMLIB_LOAD_ERROR_NONE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to imlib_save_error %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    ngx_file_info_t fi;
    if (ngx_file_info(filename, &fi) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to ngx_file_info %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    ngx_fd_t fd = ngx_open_file(filename, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
    if (fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to open fd %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    if (ngx_fd_info(fd, &fi) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to ngx_fd_info %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    } 

    char *buf;
    buf = ngx_palloc(r->pool, ngx_file_size(&fi));
    ssize_t size = ngx_read_fd(fd, buf, ngx_file_size(&fi));
    if (size == -1) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to ngx_read_fd %s:%d", __FUNCTION__, __LINE__);
        return NGX_ERROR;
    }

    if ((size_t)size > ctx->content_length) {
        ctx->content = ngx_palloc(r->pool, size);
        if (ctx->content == NULL) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "failed to allocate memory from r->pool %s:%d", __FUNCTION__, __LINE__);
            return NGX_ERROR;
        }
    }

    ngx_memcpy(ctx->content, buf, size);

    ngx_close_file(fd);

    ctx->content_length = size;

    return NGX_OK;
}
