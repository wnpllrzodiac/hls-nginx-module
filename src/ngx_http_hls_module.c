
/*
 * Copyright (C) cciijcq@leevid.com
 * Copyright (C) xunen@leevid.com
 * Copyright (C) Leevid Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#include "ngx_http_hls_module.h"


ngx_int_t ngx_http_hls_mp4_handler(ngx_http_request_t *r, ngx_http_hls_file_t *hls);
ngx_int_t ngx_http_hls_flv_handler(ngx_http_request_t *r, ngx_http_hls_file_t *hls);


static char *ngx_http_hls(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_hls_create_conf(ngx_conf_t *cf);
static char *ngx_http_hls_merge_conf(ngx_conf_t *cf, void *parent, void *child);


static ngx_command_t  ngx_http_hls_commands[] = {

    { ngx_string("hls"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_hls,
      0,
      0,
      NULL },

    { ngx_string("hls_buffers"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_conf_set_bufs_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_bufs),
      NULL },

    { ngx_string("hls_forward_args"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_forward_args),
      NULL },

    { ngx_string("hls_fragment"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_fragment),
      NULL },

    { ngx_string("hls_mp4_buffer_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_mp4_buffer_size),
      NULL },

    { ngx_string("hls_mp4_max_buffer_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_mp4_max_buffer_size),
      NULL },

    { ngx_string("hls_flv_buffer_size"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_size_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_flv_buffer_size),
      NULL },

    { ngx_string("hls_flag_usemem"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_flag_usemem),
      NULL },

    { ngx_string("hls_flag_removesei"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_flag_removesei),
      NULL },

    { ngx_string("hls_flag_removeps"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_hls_conf_t, hls_flag_removeps),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_hls_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,                          /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    ngx_http_hls_create_conf,      /* create location configuration */
    ngx_http_hls_merge_conf        /* merge location configuration */
};


ngx_module_t  ngx_http_hls_module = {
    NGX_MODULE_V1,
    &ngx_http_hls_module_ctx,      /* module context */
    ngx_http_hls_commands,         /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_hls_handler(ngx_http_request_t *r)
{
    u_char                    *last, *hls_tag;
    size_t                     root;
    ngx_int_t                  rc;
    ngx_uint_t                 level;
    ngx_log_t                 *log;
    ngx_http_hls_file_t       *hls;
    ngx_open_file_info_t       of;
    ngx_http_core_loc_conf_t  *clcf;

    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    if (r->uri.data[r->uri.len - 1] == '/') {
        return NGX_DECLINED;
    }

    rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        return rc;
    }

    hls = ngx_pcalloc(r->pool, sizeof(ngx_http_hls_file_t));
    if (hls == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* m3u8/ts check */
    hls->ask_m3u8 = 0;
    hls->ask_ts = 0;
    hls_tag = r->uri.data + r->uri.len - 3;
    if (ngx_strncmp(hls_tag, ".ts", 3) == 0) {
        hls->ask_ts = 1;
        r->uri.len -= 3;
    } else {
        hls_tag -= 2;
    }
    if (!hls->ask_ts && ngx_strncmp(hls_tag, ".m3u8", 5) == 0) {
        hls->ask_m3u8 = 1;
        r->uri.len -= 5;
    }

    /* video type check */
    ngx_uint_t                   hls_type_hash;
    ngx_str_t                    hls_type_name;
    u_char                       lowcase[8];
    ngx_uint_t                   handler_default;
    ngx_http_variable_value_t   *vv;

    hls_type_name.len = 8;
    hls_type_name.data = lowcase;

    hls_type_hash = ngx_hash_strlow(lowcase, (u_char *)"hls_type", hls_type_name.len);

    vv = ngx_http_get_variable(r, &hls_type_name, hls_type_hash);

    if (vv == NULL || vv->not_found) {
        hls_tag = r->uri.data + r->uri.len - 4;
        if (ngx_strncmp(hls_tag, ".mp4", 4) == 0) {
            handler_default = 1;
        } else if (ngx_strncmp(hls_tag, ".flv", 4) == 0) {
            handler_default = 0;
        } else {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "file type does not know");
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    } else {
        if (vv->len == 3 && ngx_strncmp(vv->data, "flv", 3) == 0) {
            /* flv */
            handler_default = 0;
        } else if (vv->len == 3 && ngx_strncmp(vv->data, "mp4", 3) == 0) {
            /* mp4 */
            handler_default = 1;
        } else {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "hls_type must \'mp4\' or \'flv\'");
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
    }


    last = ngx_http_map_uri_to_path(r, &hls->path, &root, 0);
    if (last == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    log = r->connection->log;

    hls->path.len = last - hls->path.data;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                   "http hls filename: \"%V\"", &hls->path);

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    ngx_memzero(&of, sizeof(ngx_open_file_info_t));

    of.read_ahead = clcf->read_ahead;
    of.directio = NGX_MAX_OFF_T_VALUE;
    of.valid = clcf->open_file_cache_valid;
    of.min_uses = clcf->open_file_cache_min_uses;
    of.errors = clcf->open_file_cache_errors;
    of.events = clcf->open_file_cache_events;

    hls->of = &of;

    if (ngx_http_set_disable_symlinks(r, clcf, &hls->path, &of) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    if (ngx_open_cached_file(clcf->open_file_cache, &hls->path, &of, r->pool)
        != NGX_OK)
    {
        switch (of.err) {

        case 0:
            return NGX_HTTP_INTERNAL_SERVER_ERROR;

        case NGX_ENOENT:
        case NGX_ENOTDIR:
        case NGX_ENAMETOOLONG:

            level = NGX_LOG_ERR;
            rc = NGX_HTTP_NOT_FOUND;
            break;

        case NGX_EACCES:
#if (NGX_HAVE_OPENAT)
        case NGX_EMLINK:
        case NGX_ELOOP:
#endif

            level = NGX_LOG_ERR;
            rc = NGX_HTTP_FORBIDDEN;
            break;

        default:

            level = NGX_LOG_CRIT;
            rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
            break;
        }

        if (rc != NGX_HTTP_NOT_FOUND || clcf->log_not_found) {
            ngx_log_error(level, log, of.err,
                          "%s \"%s\" failed", of.failed, hls->path.data);
        }

        return rc;
    }

    if (!of.is_file) {

        if (ngx_close_file(of.fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed", hls->path.data);
        }

        return NGX_DECLINED;
    }

    r->root_tested = !r->error_page;
    r->allow_ranges = 1;
    r->headers_out.content_length_n = of.size;

    if (handler_default) {
        return ngx_http_hls_mp4_handler(r, hls);
    } else {
        return ngx_http_hls_flv_handler(r, hls);
    }
}


static char *
ngx_http_hls(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_hls_handler;

    return NGX_CONF_OK;
}


static void *
ngx_http_hls_create_conf(ngx_conf_t *cf)
{
    ngx_http_hls_conf_t  *conf;

    conf = ngx_palloc(cf->pool, sizeof(ngx_http_hls_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->hls_mp4_buffer_size = NGX_CONF_UNSET_SIZE;
    conf->hls_mp4_max_buffer_size = NGX_CONF_UNSET_SIZE;
    conf->hls_flv_buffer_size = NGX_CONF_UNSET_SIZE;
    conf->hls_forward_args = NGX_CONF_UNSET;
    conf->hls_flag_usemem = NGX_CONF_UNSET;
    conf->hls_flag_removesei = NGX_CONF_UNSET;
    conf->hls_flag_removeps = NGX_CONF_UNSET;
    conf->hls_bufs.num = 0;
    conf->hls_bufs.size = 0;
    conf->hls_fragment = NGX_CONF_UNSET_MSEC;

    return conf;
}


static char *
ngx_http_hls_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_hls_conf_t *prev = parent;
    ngx_http_hls_conf_t *conf = child;

    ngx_conf_merge_size_value(conf->hls_mp4_buffer_size, prev->hls_mp4_buffer_size,
                              512 * 1024);
    ngx_conf_merge_size_value(conf->hls_mp4_max_buffer_size, prev->hls_mp4_max_buffer_size,
                              10 * 1024 * 1024);

    ngx_conf_merge_size_value(conf->hls_flv_buffer_size, prev->hls_flv_buffer_size,
                              512 * 1024);

    ngx_conf_merge_bufs_value(conf->hls_bufs, prev->hls_bufs,
                              8, 2*1024*1024);
    ngx_conf_merge_value(conf->hls_forward_args,
                              prev->hls_forward_args, 0);
    ngx_conf_merge_value(conf->hls_flag_usemem,
                              prev->hls_flag_usemem, 1);
    ngx_conf_merge_value(conf->hls_flag_removesei,
                              prev->hls_flag_removesei, 1);
    ngx_conf_merge_value(conf->hls_flag_removeps,
                              prev->hls_flag_removeps, 0);

    ngx_conf_merge_msec_value(conf->hls_fragment,
                              prev->hls_fragment, 5000);

    return NGX_CONF_OK;
}

