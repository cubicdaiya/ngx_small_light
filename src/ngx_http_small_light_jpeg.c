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

#include <nginx.h>

#include <Imlib2.h>
#include <setjmp.h>

#include "ngx_http_small_light_jpeg.h"

#include <jpeglib.h>

/*
** defines.
*/
#define MAX_MARKERS 16

#define M_SOF       0xc0
#define M_SOF_MIN   (M_SOF + 0)
#define M_SOF_MAX   (M_SOF + 15)
#define M_SOI       0xd8
#define M_EOI       0xd9
#define M_SOS       0xdA
#define M_APP       0xe0
#define M_APP0      (M_APP + 0)
#define M_APP1      (M_APP + 1)
#define M_COM       0xfe /* comment */

/*
** functions.
*/

/* this IMAGE_DIMENSION_OK was ported from Imlib2 loader. */
# define IMAGE_DIMENSIONS_OK(w, h) \
    ( ((w) > 0) && ((h) > 0) && \
      ((unsigned long long)(w) * (unsigned long long)(h) <= (1ULL << 29) - 1) )

/* Some ImLib2_JPEG structures or JPEGHandler functions were ported from Imlib2 loader. */
struct ImLib_JPEG_error_mgr {
    struct jpeg_error_mgr pub;
    sigjmp_buf          setjmp_buffer;
};
typedef struct ImLib_JPEG_error_mgr *emptr;

static void
_JPEGFatalErrorHandler(j_common_ptr cinfo)
{
    emptr errmgr;

    errmgr = (emptr) cinfo->err;
    /*   cinfo->err->output_message(cinfo);*/
    siglongjmp(errmgr->setjmp_buffer, 1);
    return;
}

static void
_JPEGErrorHandler(j_common_ptr cinfo)
{
    /* emptr               errmgr;*/

    /* errmgr = (emptr) cinfo->err;*/
    /*   cinfo->err->output_message(cinfo);*/
    /*   siglongjmp(errmgr->setjmp_buffer, 1);*/
    return;
}

static void
_JPEGErrorHandler2(j_common_ptr cinfo, int msg_level)
{
    /*emptr               errmgr;*/

    /*errmgr = (emptr) cinfo->err;*/
    /*   cinfo->err->output_message(cinfo);*/
    /*   siglongjmp(errmgr->setjmp_buffer, 1);*/
    return;
}

/* The load_jpeg function is based on Imlib2 loader. */
ngx_int_t ngx_http_small_light_load_jpeg(
          void **dest_data, int *width, int *height, const ngx_http_request_t *r,
          const char *filename, int hint_w, int hint_h)
{
    int                            w, h;
    struct jpeg_decompress_struct  cinfo;
    struct ImLib_JPEG_error_mgr    jerr;
    FILE                          *f;
    ngx_int_t                      fd;
    int                            denom;
    DATA8                         *ptr, *line[16], *data;
    DATA32                        *ptr2, *dest;
    int                            x, y, l, i, scans;

    *dest_data = NULL;
    *width = *height = 0;

    fd = ngx_open_file(filename, NGX_FILE_RDONLY, NGX_FILE_OPEN, NGX_FILE_DEFAULT_ACCESS);
    if (fd == NGX_INVALID_FILE) {
        return NGX_ERROR;
    }
    f = fdopen(fd, "rb");

    cinfo.err = jpeg_std_error(&(jerr.pub));
    jerr.pub.error_exit = _JPEGFatalErrorHandler;
    jerr.pub.emit_message = _JPEGErrorHandler2;
    jerr.pub.output_message = _JPEGErrorHandler;
    if (sigsetjmp(jerr.setjmp_buffer, 1)) {
        jpeg_destroy_decompress(&cinfo);
        ngx_close_file(fd);
        fclose(f);
        return NGX_ERROR;
    }
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);
    cinfo.do_fancy_upsampling = FALSE;
    cinfo.do_block_smoothing = FALSE;

    jpeg_start_decompress(&cinfo);
    w = cinfo.output_width;
    h = cinfo.output_height;
    denom = w / hint_w;
    if (denom > h / hint_h) {
        denom = h / hint_h;
    }
    denom = denom >= 1 ? denom : 1;
    denom = denom <= 8 ? denom : 8;
    jpeg_destroy_decompress(&cinfo);
    fseek(f, 0, SEEK_SET);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);
    cinfo.do_fancy_upsampling = FALSE;
    cinfo.do_block_smoothing = FALSE;
    cinfo.scale_denom = denom;

    jpeg_start_decompress(&cinfo);

    w = cinfo.output_width;
    h = cinfo.output_height;

    if ((cinfo.rec_outbuf_height > 16) || (cinfo.output_components <= 0) ||
        !IMAGE_DIMENSIONS_OK(w, h))
    {
        jpeg_destroy_decompress(&cinfo);
        ngx_close_file(fd);
        fclose(f);
        return NGX_ERROR;
    }
    data = ngx_palloc(r->pool, w * 16 * cinfo.output_components);
    if (!data) {
        jpeg_destroy_decompress(&cinfo);
        ngx_close_file(fd);
        fclose(f);
        return NGX_ERROR;
    }
    /* must set the im->data member before callign progress function */
    ptr2 = dest = ngx_palloc(r->pool, w * h * sizeof(DATA32));
    if (!dest) {
        jpeg_destroy_decompress(&cinfo);
        ngx_close_file(fd);
        fclose(f);
        return NGX_ERROR;
    }
    if (cinfo.output_components > 1) {
        for (i = 0; i < cinfo.rec_outbuf_height; i++) {
            line[i] = data + (i * w * cinfo.output_components);
        }
        for (l = 0; l < h; l += cinfo.rec_outbuf_height) {
            jpeg_read_scanlines(&cinfo, line, cinfo.rec_outbuf_height);
            scans = cinfo.rec_outbuf_height;
            if ((h - l) < scans) {
                scans = h - l;
            }
            ptr = data;
            for (y = 0; y < scans; y++) {
                for (x = 0; x < w; x++) {
                    *ptr2 =
                        (0xff000000)     |
                        ((ptr[0]) << 16) |
                        ((ptr[1]) <<  8) |
                        (ptr[2]);
                    ptr += cinfo.output_components;
                    ptr2++;
                }
            }
        }
    } else if (cinfo.output_components == 1) {
        for (i = 0; i < cinfo.rec_outbuf_height; i++) {
            line[i] = data + (i * w);
        }
        for (l = 0; l < h; l += cinfo.rec_outbuf_height) {
            jpeg_read_scanlines(&cinfo, line, cinfo.rec_outbuf_height);
            scans = cinfo.rec_outbuf_height;
            if ((h - l) < scans) {
                scans = h - l;
            }
            ptr = data;
            for (y = 0; y < scans; y++) {
                for (x = 0; x < w; x++) {
                    *ptr2 =
                        (0xff000000)     |
                        ((ptr[0]) << 16) |
                        ((ptr[0]) <<  8) |
                        (ptr[0]);
                    ptr++;
                    ptr2++;
                }
            }
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    ngx_close_file(fd);
    fclose(f);

    *dest_data = dest;
    *width = w;
    *height = h;

    return NGX_OK;
}
