Name
====

hls-nginx-module - support HTTP Live Streaming (HLS) for nginx

*This module is not distributed with the Nginx source.* See [the installation instructions](#installation).


Table of Contents
=================

* [Description](#description)
* [Example](#example)
* [Directives](#directives)
    * [hls](#hls)
    * [hls_buffers](#hls_buffers)
    * [hls_fragment](#hls_fragment)
    * [hls_mp4_buffer_size](#hls_mp4_buffer_size)
    * [hls_mp4_max_buffer_size](#hls_mp4_max_buffer_size)
* [Changes](#changes)
* [Test Suite](#test-suite)
* [Copyright and License](#copyright-and-license)
* [See Also](#see-also)

Description
===========

The *hls-nginx-module* module provides HTTP Live Streaming (HLS) server-side support for H.264/AAC files. Such files typically have the .mp4, .m4v, or .m4a filename extensions.

nginx supports two URIs for each MP4 file:

 * The playlist URI that ends with “.m3u8” and accepts the optional “len” argument that defines the fragment length in seconds;
 * The fragment URI that ends with “.ts” and accepts “start” and “end” arguments that define fragment boundaries in seconds.

[Back to TOC](#table-of-contents)

Example Configuration
=====================
```Example
location /video/ {
    hls;
    hls_fragment            5s;
    hls_buffers             10 10m;
    hls_mp4_buffer_size     1m;
    hls_mp4_max_buffer_size 5m;
    alias /var/video/;
}
```

With this configuration, the following URIs are supported for the “/var/video/test.mp4” file:

```url
http://hls.example.com/video/test.mp4.m3u8?len=8.000
http://hls.example.com/video/test.mp4.ts?start=1.000&end=2.200
```

[Back to TOC](#table-of-contents)

Directives
===========
hls
--------------------
**syntax:** *hls*

**default:** *-*

**context:** *http, server, location, location if*

Turns on HLS streaming in the surrounding location.


hls_buffers
--------------------
**syntax:** *hls_buffers number size;*

**default:** *hls_buffers 8 2m;*

**context:** *http, server, location*

Sets the maximum *number* and *size* of buffers that are used for reading and writing data frames.


hls_forward_args
--------------------
**syntax:** *hls_forward_args on | off;*

**Default:** *hls_forward_args off;*

**context:** *http, server, location*

Adds arguments from a playlist request to URIs of fragments. This may be useful for performing client authorization at the moment of requesting a fragment, or when protecting an HLS stream with the ngx_http_secure_link_module module.

For example, if a client requests a playlist http://example.com/hls/test.mp4.m3u8?a=1&b=2, the arguments a=1 and b=2 will be added to URIs of fragments after the arguments start and end:

```bash
#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:15
#EXT-X-PLAYLIST-TYPE:VOD

#EXTINF:9.333,
test.mp4.ts?start=0.000&end=9.333&a=1&b=2
#EXTINF:7.167,
test.mp4.ts?start=9.333&end=16.500&a=1&b=2
#EXTINF:5.416,
test.mp4.ts?start=16.500&end=21.916&a=1&b=2
#EXTINF:5.500,
test.mp4.ts?start=21.916&end=27.416&a=1&b=2
#EXTINF:15.167,
test.mp4.ts?start=27.416&end=42.583&a=1&b=2
#EXTINF:9.626,
test.mp4.ts?start=42.583&end=52.209&a=1&b=2

#EXT-X-ENDLIST
```
If an HLS stream is protected with the ngx_http_secure_link_module module, $uri should not be used in the secure_link_md5 expression because this will cause errors when requesting the fragments. Base URI should be used instead of $uri ($hls_uri in the example):

```Example
http {
    ...

    map $uri $hls_uri {
        ~^(?<base_uri>.*).m3u8$ $base_uri;
        ~^(?<base_uri>.*).ts$   $base_uri;
        default                 $uri;
    }

    server {
        ...

        location /hls {
            hls;
            hls_forward_args on;

            alias /var/videos;

            secure_link $arg_md5,$arg_expires;
            secure_link_md5 "$secure_link_expires$hls_uri$remote_addr secret";

            if ($secure_link = "") {
                return 403;
            }

            if ($secure_link = "0") {
                return 410;
            }
        }
    }
}
```

hls_fragment
--------------------
**syntax:** *hls_fragment time;*

**default:** *hls_fragment 5s;*

**context:** *http, server, location*

Defines the default fragment length for playlist URIs requested without the “len” argument.

hls_mp4_buffer_size
--------------------
**syntax:** *hls_mp4_buffer_size size;*

**default:** *hls_mp4_buffer_size 512k;*

**context:** *http, server, location*

Sets the initial size of the buffer used for processing MP4 and MOV files.

hls_mp4_max_buffer_size
-------------------------
**syntax:** *hls_mp4_max_buffer_size size;*

**default:** *hls_mp4_max_buffer_size 10m;*

**context:** *http, server, location*


During metadata processing, a larger buffer may become necessary. Its size cannot exceed the specified size, or else nginx will return the server error 500 (Internal Server Error), and log the following message:

```Example
"/some/movie/file.mp4" mp4 moov atom is too large:
12583268, you may want to increase hls_mp4_max_buffer_size
```

遗留问题
=========
（1）、Xiao.mp4 音频问题；
（2）、湖南卫视视频fhv 转 mp4 不能播放问题；
（3）、内存管理；


