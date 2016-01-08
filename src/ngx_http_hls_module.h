
/*
 * Copyright (C) cciijcq@leevid.com
 * Copyright (C) xunen@leevid.com
 * Copyright (C) Leevid Inc.
 */


#ifndef _NGX_HTTP_HLS_MODULE_H_INCLUDED_
#define _NGX_HTTP_HLS_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct ngx_hls_frame_s  ngx_hls_frame_t;


struct ngx_hls_frame_s {
    ngx_hls_frame_t      *next;

    off_t                 file_pos;
    off_t                 file_last;
    u_char               *mem_start;
    u_char               *mem_end;
    size_t                size;

    uint64_t              pts;
    uint64_t              dts;

    unsigned              key:1;
    unsigned              in_mem:1;

    /** mp4 */
    ngx_uint_t            sample_id;

    /** flv */
    u_char                terminator;
    unsigned              is_script:1;
    unsigned              is_audio:1;
    unsigned              is_video:1;
};


typedef struct {
    ngx_open_file_info_t *of;

    ngx_str_t             path;

    u_char               *hls_buffer;
    u_char               *hls_buffer_start;
    u_char               *hls_buffer_end;
    size_t                hls_buffer_size;

    unsigned              count32:5;
    unsigned              ask_m3u8:1;
    unsigned              ask_ts:1;
    unsigned              strict_segment:1;

    off_t                 av_data_offset;
    size_t                av_data_size;
    u_char               *av_data_start;
    u_char               *av_data_end;

    ngx_uint_t            v_cycle_count;
    ngx_uint_t            a_cycle_count;
    ngx_uint_t            psi_cycle_count;
    ngx_uint_t            segment_len;

    u_char                sampling_index;

    ngx_str_t             sps_str;
    ngx_str_t             pps_str;

    ngx_hls_frame_t      *vframe_start;
    ngx_hls_frame_t      *vframe_end;
    ngx_hls_frame_t      *aframe_start;
    ngx_hls_frame_t      *aframe_end;

    uint32_t              ts_buf_array_size;
    uint32_t              use_count;
    ngx_buf_t            *ts_buf_array; //array size decided by ts_buf_array_size
    ngx_chain_t          *ts_chain_array; //array size is use_count

    u_char               *ts_header_buffer;
    u_char               *ts_header_buffer_pos;
    u_char               *ts_header_buffer_end;
    uint64_t              ts_header_buffer_size;
    uint32_t              ts_packets; //estimate the number of ts packets
    uint32_t              ts_packet_count;
} ngx_http_hls_file_t;


typedef struct {
    ngx_bufs_t            hls_bufs;
    ngx_flag_t            hls_forward_args;
    ngx_msec_t            hls_fragment;
    size_t                hls_mp4_buffer_size;
    size_t                hls_mp4_max_buffer_size;
    size_t                hls_flv_buffer_size;

    ngx_flag_t            hls_flag_usemem;
    ngx_flag_t            hls_flag_removesei;
    ngx_flag_t            hls_flag_removeps;
} ngx_http_hls_conf_t;


extern ngx_module_t  ngx_http_hls_module;


#endif /* _NGX_HTTP_HLS_MODULE_H_INCLUDED_ */
