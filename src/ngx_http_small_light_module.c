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

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_small_light_module.h"
#include "ngx_http_small_light_param.h"
#include "ngx_http_small_light_parser.h"
#include "ngx_http_small_light_imagemagick.h"
#ifdef NGX_HTTP_SMALL_LIGHT_IMLIB2_ENABLED
#include "ngx_http_small_light_imlib2.h"
#endif
#ifdef NGX_HTTP_SMALL_LIGHT_GD_ENABLED
#include "ngx_http_small_light_gd.h"
#endif

#define NGX_HTTP_SMALL_LIGHT_IMAGE_BUFFERED 0x08

const char *ngx_http_small_light_image_types[] = {
    "image/jpeg",
    "image/gif",
    "image/png",
    "image/webp"
};

const char *ngx_http_small_light_image_exts[] = {
    "jpeg",
    "gif",
    "png",
    "webp"
};

static void *ngx_http_small_light_create_srv_conf(ngx_conf_t *cf);
static void *ngx_http_small_light_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_small_light_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_small_light_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_small_light_pattern_define(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_small_light_material_dir(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_small_light_image_read(ngx_http_request_t *r,
                                                 ngx_chain_t *in,
                                                 size_t buffer_size,
                                                 ngx_http_small_light_ctx_t *ctx);
static ngx_int_t ngx_http_small_light_finish(ngx_http_request_t *r, ngx_chain_t *out);
static ngx_int_t ngx_http_small_light_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_small_light_init_worker(ngx_cycle_t *cycle);
static void ngx_http_small_light_exit_worker(ngx_cycle_t *cycle);

static ngx_command_t  ngx_http_small_light_commands[] = {
    { 
        ngx_string("small_light"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_small_light_conf_t, enable),
        NULL
    },
    
    { 
        ngx_string("small_light_getparam_mode"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_small_light_conf_t, enable_getparam_mode),
        NULL
    },

    {
        ngx_string("small_light_pattern_define"),
        NGX_HTTP_SRV_CONF|NGX_CONF_TAKE2,
        ngx_http_small_light_pattern_define,
        NGX_HTTP_SRV_CONF_OFFSET,
        0,
        NULL
    },

    {
        ngx_string("small_light_radius_max"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_num_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_small_light_conf_t, radius_max),
        NULL
    },

    {
        ngx_string("small_light_sigma_max"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_num_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_small_light_conf_t, sigma_max),
        NULL
    },

    {
        ngx_string("small_light_material_dir"),
        NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
        ngx_http_small_light_material_dir,
        NGX_HTTP_SRV_CONF_OFFSET,
        0,
        NULL
    },

    { 
        ngx_string("small_light_imlib2_temp_dir"),
        NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1234,
        ngx_conf_set_path_slot,
        NGX_HTTP_SRV_CONF_OFFSET,
        offsetof(ngx_http_small_light_conf_t, imlib2_temp_dir),
        NULL
    },

    {
        ngx_string("small_light_buffer"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_small_light_conf_t, buffer_size),
        NULL
    },

    ngx_null_command
};

static ngx_http_module_t  ngx_http_small_light_module_ctx = {
    NULL,                                 /* preconfiguration */
    ngx_http_small_light_init,            /* postconfiguration */

    NULL,                                 /* create main configuration */
    NULL,                                 /* init main configuration */

    ngx_http_small_light_create_srv_conf, /* create server configuration */
    ngx_http_small_light_merge_srv_conf,  /* merge server configuration */

    ngx_http_small_light_create_loc_conf, /* create location configuration */
    ngx_http_small_light_merge_loc_conf   /* merge location configuration */
};

ngx_module_t  ngx_http_small_light_module = {
    NGX_MODULE_V1,
    &ngx_http_small_light_module_ctx, /* module context */
    ngx_http_small_light_commands,    /* module directives */
    NGX_HTTP_MODULE,                  /* module type */
    NULL,                             /* init master */
    NULL,                             /* init module */
    ngx_http_small_light_init_worker, /* init process */
    NULL,                             /* init thread */
    NULL,                             /* exit thread */
    ngx_http_small_light_exit_worker, /* exit process */
    NULL,                             /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_small_light_init_worker(ngx_cycle_t *cycle) {
    ngx_http_small_light_imagemagick_genesis();

    if (ngx_http_small_light_imagemagick_set_thread_limit(1) == 0) {
        ngx_log_error(NGX_LOG_WARN, cycle->log, 0,
                      "failed to set thread limit");
    }
    return NGX_OK;
}

static void ngx_http_small_light_exit_worker(ngx_cycle_t *cycle) {
    ngx_http_small_light_imagemagick_terminus();
}

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static ngx_int_t ngx_http_small_light_header_filter(ngx_http_request_t *r)
{
    ngx_http_small_light_conf_t *srv_conf;
    ngx_http_small_light_conf_t *loc_conf;
    ngx_http_small_light_ctx_t  *ctx;
    ngx_hash_init_t              hash_init;
    ngx_str_t                    define_pattern;
    char                        *converter;

    if (r->headers_out.status == NGX_HTTP_NOT_MODIFIED ||
        r->headers_out.content_length_n == 0)
    {
        return ngx_http_next_header_filter(r);
    }

    srv_conf = ngx_http_get_module_srv_conf(r, ngx_http_small_light_module);
    loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_small_light_module);

    if (!loc_conf->enable) {
        return ngx_http_next_header_filter(r);
    }

    if (!loc_conf->enable_getparam_mode) {
        if(ngx_http_small_light_parse_define_pattern(r, &r->unparsed_uri, &define_pattern) != NGX_OK) {
            return ngx_http_next_header_filter(r);
        }
    }
    
    ctx = ngx_http_get_module_ctx(r, ngx_http_small_light_module);
    if (ctx) {
        ngx_http_set_ctx(r, NULL, ngx_http_small_light_module);
        return ngx_http_next_header_filter(r);
    }

    if ((ctx = ngx_pcalloc(r->pool, sizeof(*ctx))) == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to allocate memory from r->pool %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    ctx->params.keys.pool = r->pool;
    ctx->params.temp_pool = r->pool;
    if (ngx_hash_keys_array_init(&ctx->params, NGX_HASH_SMALL) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to init hash keys for parameters %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    if (loc_conf->enable_getparam_mode) {
        if (ngx_http_small_light_init_getparams(r, ctx, srv_conf) != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to analyze parameters:%V %s:%d",
                          &define_pattern,
                          __FUNCTION__,
                          __LINE__);
            return NGX_ERROR;
        }
    } else {
        if (ngx_http_small_light_init_params(r, ctx, &define_pattern, srv_conf) != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "failed to analyze parameters:%V %s:%d",
                          &define_pattern,
                          __FUNCTION__,
                          __LINE__);
            return NGX_ERROR;
        }
    }

    hash_init.hash        = &ctx->hash;
    hash_init.key         = ngx_hash_key_lc;
    hash_init.max_size    = 128;
    hash_init.bucket_size = ngx_cacheline_size;
    hash_init.name        = "small_light_init_params";
    hash_init.pool        = ctx->params.keys.pool;
    hash_init.temp_pool   = NULL;

    if (ngx_hash_init(&hash_init, ctx->params.keys.elts, ctx->params.keys.nelts) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to init hash table for parameters %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    ctx->inf             = (char *)r->headers_out.content_type.data;
    ctx->material_dir    = &srv_conf->material_dir;
    ctx->imlib2_temp_dir = loc_conf->imlib2_temp_dir;
    ctx->radius_max      = loc_conf->radius_max;
    ctx->sigma_max       = loc_conf->sigma_max;

    if (r->headers_out.content_length_n < 0) {
        ctx->content_length = loc_conf->buffer_size;
    } else {
        ctx->content_length = r->headers_out.content_length_n;
    }

    converter = NGX_HTTP_SMALL_LIGHT_PARAM_GET_LIT(&ctx->hash, "e");
    if (ngx_strcmp(converter, NGX_HTTP_SMALL_LIGHT_CONVERTER_IMAGEMAGICK) == 0) {
        ctx->converter.init    = ngx_http_small_light_imagemagick_init;
        ctx->converter.term    = ngx_http_small_light_imagemagick_term;
        ctx->converter.process = ngx_http_small_light_imagemagick_process;
        ctx->ictx = ngx_pcalloc(r->pool, sizeof(ngx_http_small_light_imagemagick_ctx_t));
#ifdef NGX_HTTP_SMALL_LIGHT_IMLIB2_ENABLED
    } else if (ngx_strcmp(converter, NGX_HTTP_SMALL_LIGHT_CONVERTER_IMLIB2) == 0) {
        ctx->converter.init    = ngx_http_small_light_imlib2_init;
        ctx->converter.term    = ngx_http_small_light_imlib2_term;
        ctx->converter.process = ngx_http_small_light_imlib2_process;
        ctx->ictx = ngx_pcalloc(r->pool, sizeof(ngx_http_small_light_imlib2_ctx_t));
#endif
#ifdef NGX_HTTP_SMALL_LIGHT_GD_ENABLED
    } else if (ngx_strcmp(converter, NGX_HTTP_SMALL_LIGHT_CONVERTER_GD) == 0) {
        ctx->converter.init    = ngx_http_small_light_gd_init;
        ctx->converter.term    = ngx_http_small_light_gd_term;
        ctx->converter.process = ngx_http_small_light_gd_process;
        ctx->ictx = ngx_pcalloc(r->pool, sizeof(ngx_http_small_light_gd_ctx_t));
#endif
    } else {
        ctx->converter.init    = ngx_http_small_light_imagemagick_init;
        ctx->converter.term    = ngx_http_small_light_imagemagick_term;
        ctx->converter.process = ngx_http_small_light_imagemagick_process;
        ctx->ictx = ngx_pcalloc(r->pool, sizeof(ngx_http_small_light_imagemagick_ctx_t));
    }

    if (ctx->ictx == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to allocate memory from r->pool %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return NGX_ERROR;
    }

    ngx_http_set_ctx(r, ctx, ngx_http_small_light_module);

    if (r->headers_out.refresh) {
        r->headers_out.refresh->hash = 0;
    }

    r->main_filter_need_in_memory = 1;
    r->allow_ranges               = 0;

    return NGX_OK;
}

static ngx_int_t ngx_http_small_light_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_small_light_conf_t *loc_conf;
    ngx_http_small_light_ctx_t  *ctx;
    ngx_buf_t                   *b;
    ngx_pool_cleanup_t          *cln;
    ngx_chain_t                  out;
    ngx_int_t                    rc;
    size_t                       content_type_len;

    if (in == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_small_light_module);

    if (!loc_conf->enable) {
        return ngx_http_next_body_filter(r, in);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_small_light_module);

    if (ctx == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    if((rc = ngx_http_small_light_image_read(r, in, loc_conf->buffer_size, ctx)) != NGX_OK) {
        if (rc == NGX_AGAIN) {
            return NGX_OK;
        }
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to read image %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return ngx_http_filter_finalize_request(r,
                                                &ngx_http_small_light_module,
                                                NGX_HTTP_UNSUPPORTED_MEDIA_TYPE);
    }

    r->connection->buffered &= ~NGX_HTTP_SMALL_LIGHT_IMAGE_BUFFERED;

    rc = ctx->converter.init(r, ctx);
    if (rc != NGX_OK) {

        ngx_pfree(r->pool, ctx->content_orig);

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to process image %s:%d",
                      __FUNCTION__,
                      __LINE__);
        return ngx_http_filter_finalize_request(r,
                                                &ngx_http_small_light_module,
                                                NGX_HTTP_UNSUPPORTED_MEDIA_TYPE);
    }

    rc = ctx->converter.process(r, ctx);

    if (rc != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to process image %s:%d",
                      __FUNCTION__,
                      __LINE__);
        ctx->converter.term(ctx);
        return ngx_http_filter_finalize_request(r,
                                                &ngx_http_small_light_module,
                                                NGX_HTTP_UNSUPPORTED_MEDIA_TYPE);
    }

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to allocate memory from r->pool %s:%d",
                      __FUNCTION__,
                      __LINE__);
        ctx->converter.term(ctx);
        return NGX_ERROR;
    }
    b->pos      = ctx->content;
    b->last     = ctx->content + ctx->content_length;
    b->memory   = 1;
    b->last_buf = 1;

    out.buf  = b;
    out.next = NULL;

    r->headers_out.content_length_n = b->last - b->pos;
    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
    }
    content_type_len = ngx_strlen(ctx->of);
    r->headers_out.content_length       = NULL;
    r->headers_out.content_type_len     = content_type_len;
    r->headers_out.content_type.len     = content_type_len;
    r->headers_out.content_type.data    = (u_char *)ctx->of;
    r->headers_out.content_type_lowcase = NULL;

    cln = ngx_pool_cleanup_add(r->pool, 0);
    if (cln == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to allocate memory from r->pool %s:%d",
                      __FUNCTION__,
                      __LINE__);
        ctx->converter.term(ctx);
        return NGX_ERROR;
    }
    cln->handler = ctx->converter.term;
    cln->data    = ctx;

    return ngx_http_small_light_finish(r, &out);
}

static void *ngx_http_small_light_create_srv_conf(ngx_conf_t *cf)
{
    ngx_http_small_light_conf_t *srv_conf;
    ngx_pool_t *pool, *temp_pool;
    srv_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_small_light_conf_t));
    if (srv_conf == NULL) {
        return NGX_CONF_ERROR;
    }
    pool = ngx_create_pool(16384, cf->log);
    if (pool == NULL) {
        return NGX_CONF_ERROR;
    }
    temp_pool = ngx_create_pool(16384, cf->log);
    if (temp_pool == NULL) {
        return NGX_CONF_ERROR;
    }
    srv_conf->patterns.keys.pool = pool;
    srv_conf->patterns.temp_pool = temp_pool;
    if (ngx_hash_keys_array_init(&srv_conf->patterns, NGX_HASH_SMALL) != NGX_OK) {
        return NGX_CONF_ERROR;
    }
    return srv_conf;
}

static void *ngx_http_small_light_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_small_light_conf_t *loc_conf;
    loc_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_small_light_conf_t));
    if (loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }
    loc_conf->enable               = NGX_CONF_UNSET;
    loc_conf->enable_getparam_mode = NGX_CONF_UNSET;
    loc_conf->buffer_size          = NGX_CONF_UNSET_SIZE;
    loc_conf->radius_max           = NGX_CONF_UNSET_UINT;
    loc_conf->sigma_max            = NGX_CONF_UNSET_UINT;
    return loc_conf;
}

static char *ngx_http_small_light_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    return NGX_CONF_OK;
}

static ngx_path_init_t  ngx_http_small_light_imlib2_temp_dir = {
    ngx_string("/tmp"), { 1, 2, 0 }
};

static char *ngx_http_small_light_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_small_light_conf_t *prev = parent;
    ngx_http_small_light_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable,               prev->enable,               0);
    ngx_conf_merge_value(conf->enable_getparam_mode, prev->enable_getparam_mode, 0);

    if (ngx_conf_merge_path_value(cf, &conf->imlib2_temp_dir,
                                  prev->imlib2_temp_dir,
                                  &ngx_http_small_light_imlib2_temp_dir)
        != NGX_OK)
    {
        return NGX_CONF_ERROR;
    }

    ngx_conf_merge_size_value(conf->buffer_size, prev->buffer_size, 1 * 1024 * 1024);

    ngx_conf_merge_uint_value(conf->radius_max, prev->radius_max, 10);
    ngx_conf_merge_uint_value(conf->sigma_max, prev->sigma_max, 10);

    return NGX_CONF_OK;
}

static char *ngx_http_small_light_pattern_define(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_small_light_conf_t *srv_conf;
    ngx_uint_t                   rc;
    ngx_str_t                   *ptn_name;
    ngx_str_t                   *ptn_str;
    ngx_str_t                   *value;
    ngx_hash_init_t              hash;
    
    srv_conf = conf;
    value    = cf->args->elts;
    ptn_name = &value[1];
    ptn_str  = &value[2];
    
    rc  = ngx_hash_add_key(&srv_conf->patterns, ptn_name, ptn_str->data, NGX_HASH_READONLY_KEY);

    if (rc != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    hash.hash        = &srv_conf->hash;
    hash.key         = ngx_hash_key_lc;
    hash.max_size    = 128;
    hash.bucket_size = ngx_cacheline_size;
    hash.name        = "small_light_pattern_define";
    hash.pool        = srv_conf->patterns.keys.pool;
    hash.temp_pool   = NULL;

    if (ngx_hash_init(&hash, srv_conf->patterns.keys.elts, srv_conf->patterns.keys.nelts) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

static char *ngx_http_small_light_material_dir(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_small_light_conf_t *srv_conf;
    ngx_str_t                   *value;
    ngx_dir_t                   dir;

    srv_conf = conf;
    value    = cf->args->elts;

    if (ngx_open_dir(&value[1], &dir) == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    ngx_memcpy(&srv_conf->material_dir, &value[1], sizeof(ngx_str_t));

    return NGX_CONF_OK;
}    


static ngx_int_t
ngx_http_small_light_image_read(ngx_http_request_t *r,
                                ngx_chain_t *in,
                                size_t buffer_size,
                                ngx_http_small_light_ctx_t *ctx)
{
    u_char      *p;
    size_t       size, rest;
    ngx_buf_t   *b;
    ngx_chain_t *cl;

    if (ctx->content == NULL) {
        ctx->content = ngx_palloc(r->pool, ctx->content_length);
        if (ctx->content == NULL) {
            return NGX_ERROR;
        }

        ctx->last = ctx->content;
        ctx->content_orig = ctx->content;
    }

    p = ctx->last;

    for (cl=in;cl!=NULL;cl=cl->next) {
        b    = cl->buf;
        size = b->last - b->pos;
        rest = ctx->content + ctx->content_length - p;
        if (size > rest) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log,
                          0,
                          "small_light_buffer(%z byte) is small %s:%d",
                          buffer_size,
                          __FUNCTION__,
                          __LINE__);
            return NGX_ERROR;
        }
        p       = ngx_cpymem(p, b->pos, size);
        b->pos += size;
        if (b->last_buf) {
            ctx->last = p;
            if (ctx->content_length == buffer_size) {
                ctx->content_length = ctx->last - ctx->content;
            }
            return NGX_OK;
        }
    }

    ctx->last = p;
    r->connection->buffered |= NGX_HTTP_SMALL_LIGHT_IMAGE_BUFFERED;
    
    return NGX_AGAIN;

}

static ngx_int_t ngx_http_small_light_finish(ngx_http_request_t *r, ngx_chain_t *out)
{
    ngx_int_t rc;

    rc = ngx_http_next_header_filter(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return NGX_ERROR;
    }

    return ngx_http_next_body_filter(r, out);
}

static ngx_int_t ngx_http_small_light_init(ngx_conf_t *cf)
{

    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter  = ngx_http_small_light_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter  = ngx_http_small_light_body_filter;

    return NGX_OK;
}
