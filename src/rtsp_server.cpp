/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * ./rtsp_server2 "( appsrc name="myappsrc" ! rtph264pay name=pay0 pt=96 config-interval=1 )"
 * 
 * rtsp_client can use
 * (1) gst-launch-1.0 rtspsrc location="rtsp://127.0.0.1:8554/test" ! rtph264depay ! appsink
 * (2) ffplay rtsp://127.0.0.1:8554/test
 * */

extern "C"
{
#include <gst/gst.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <stdio.h>

#include <gst/rtsp-server/rtsp-server.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}


#include <list>
#include <memory>

#define SPS_PPS_LEN     (4+22+4+4)
uint32_t SPS_PPS_BUFFER[SPS_PPS_LEN] = {0};

int isH264Ifream(unsigned char *data)
{
    if (!data)
    {
        return 0;
    }

    unsigned char nal_type = data[4] & 0x1f;   //H264的分隔符可能是    00 00 00 01 或者 00 00 01

    if (nal_type == 5)// || nal_type == 7 || nal_type == 8 || nal_type == 2)
    {
        return 1;
    }
    else
    {
        return 0;
    }  
}


struct H264Frame
{
    H264Frame(const uint32_t& _size)
    {
        buf = (uint8_t*)malloc(_size);
        size = _size;
    }

    ~H264Frame()
    {
        if (buf)
        {
            free(buf);
            buf = nullptr;
        }
    }

    uint8_t* buf = nullptr;
    uint32_t size;
    bool     is_idr = false;
    uint64_t timestamp = 0ULL;
};

uint64_t g_timestamp = 0ULL;

using H264FramePtr = std::shared_ptr<H264Frame>;

std::list<H264FramePtr> g_list;

struct buffer_data {
    uint8_t* ptr;
    size_t size; ///< size left in the buffer
};

static int read_packet(void* opaque, uint8_t* buf, int buf_size)
{
    struct buffer_data* bd = (struct buffer_data*)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    if (!buf_size)
        return AVERROR_EOF;
    // printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
    bd->size -= buf_size;

    return buf_size;
}



#define DEFAULT_RTSP_PORT "8554"

static char* port = (char*)DEFAULT_RTSP_PORT;

static GOptionEntry entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_STRING, &port,
      "Port to listen on (default: " DEFAULT_RTSP_PORT ")", "PORT"},
  {NULL}
};

static FILE * appSrcFile = NULL;
static int  read_counter = 0;
static char read_buffer[4096];

static bool is_first_push = true;
 

void need_data_callback(GstElement* _appsrc, guint _length, gpointer _udata)
{
    printf("need_data_callback appsrc : %p \n", _appsrc);
    g_print("need_data_callback\n");


    GstBuffer* gst_buffer;
    

    if (is_first_push)
    {
        while (!g_list.empty())
        {
            if (g_list.front()->is_idr)
            {
                printf("find I frame \n");
                break;
            }
            else
            {
                printf("find P frame \n");

                g_list.pop_front();
            }
        }
        is_first_push = false;
    }
    
    H264FramePtr h264_frame_ptr = g_list.front();

    if (h264_frame_ptr->is_idr)
    {
        printf("push I frame.\n");
        gst_buffer = gst_buffer_new_allocate(NULL, h264_frame_ptr->size + SPS_PPS_LEN, NULL);
        gst_buffer_fill(gst_buffer, 0, (guchar*)SPS_PPS_BUFFER, SPS_PPS_LEN);
        gst_buffer_fill(gst_buffer, SPS_PPS_LEN, (guchar*)h264_frame_ptr->buf, h264_frame_ptr->size);
    }
    else
    {
        printf("push p frame.\n");
        gst_buffer = gst_buffer_new_allocate(NULL, h264_frame_ptr->size, NULL);
        gst_buffer_fill(gst_buffer, 0, (guchar*)h264_frame_ptr->buf, h264_frame_ptr->size);
    }


    GST_BUFFER_PTS(gst_buffer) = g_timestamp;
    GST_BUFFER_DTS(gst_buffer) = GST_BUFFER_PTS(gst_buffer);
    g_timestamp += (1000000000UL / 25UL);

    int ret = -1;
    g_signal_emit_by_name(_appsrc, "push-buffer", gst_buffer, &ret);
    gst_buffer_unref(gst_buffer);

    g_list.pop_front();

    printf("need end. ret:%d \n", ret);

}

void enough_data_callback(GstElement* _appsrc, guint _length, gpointer _udata)
{
    g_print("enough_data_callback\n");
}


void media_configure_callback(GstRTSPMediaFactory* _factory, GstRTSPMedia* _media, gpointer _udata)
{
    g_print("media_configure_callback \n");

    GstElement* element;
    GstElement* appsrc;

    element = gst_rtsp_media_get_element(_media);
    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "myappsrc");
    if (!G_IS_OBJECT(appsrc))
    {
        g_print("not find aapsrc  \n");
    }
    printf(" appsrc : %p \n", appsrc);
    g_print("find success.\n");
    
      /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");


    g_object_set(G_OBJECT(appsrc), "caps",
        gst_caps_new_simple("video/x-h264",
            "stream-format", G_TYPE_STRING, "byte-stream",
            "width", G_TYPE_INT, 384,
            "height", G_TYPE_INT, 288,
            "alignment", G_TYPE_STRING, "au",
            "framerate", GST_TYPE_FRACTION, 25, 1, NULL), NULL);

    g_signal_connect(appsrc, "need-data", (GCallback)(need_data_callback), NULL);
    g_signal_connect(appsrc, "enough-data", (GCallback)(enough_data_callback), NULL);

    gst_object_unref(appsrc);
    gst_object_unref(element);
}



int
main(int argc, char* argv[])
{
    //////////////////////////////////////////////////////////////////////////////


    AVFormatContext* fmt_ctx = NULL;
    AVIOContext* avio_ctx = NULL;
    uint8_t* buffer = NULL, * avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
    char* input_filename = NULL;
    int ret = 0;
    struct buffer_data bd = { 0 };


    input_filename = "test.264";

    /* slurp file content into buffer */
    ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
    // if (ret < 0)
    //     goto end;

    /* fill opaque structure used by the AVIOContext read callback */
    bd.ptr = buffer;
    bd.size = buffer_size;

    if (!(fmt_ctx = avformat_alloc_context())) {
        // ret = AVERROR(ENOMEM);
        // goto end;
    }

    avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        // goto end;
    }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
        0, &bd, &read_packet, NULL, NULL);
    if (!avio_ctx) {
        // ret = AVERROR(ENOMEM);
        // goto end;
    }
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        // fprintf(stderr, "Could not open input\n");
        // goto end;
    }

    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    // av_dump_format(fmt_ctx, 0, input_filename, 0);
    AVPacket packet;
    bool first_read = true;
    while (ret >= 0)
    {
        if (av_read_frame(fmt_ctx, &packet) < 0)
        {
            break;
        }
        
        static int k = 0;
        if (packet.stream_index == 0)
        {
            
            if(first_read)
            {
                memcpy(SPS_PPS_BUFFER, packet.data, SPS_PPS_LEN);
                H264FramePtr ptr = H264FramePtr(new H264Frame(packet.size - SPS_PPS_LEN));
                memcpy(ptr->buf, packet.data + SPS_PPS_LEN, packet.size - SPS_PPS_LEN);
                first_read = false;
                g_list.emplace_back(ptr);
            }
            else
            {
                // printf("frame size:%d\n", packet.size);
                H264FramePtr ptr = H264FramePtr(new H264Frame(packet.size));
                memcpy(ptr->buf, packet.data, packet.size);
                g_list.emplace_back(ptr);
            }
            k++;

        }

        // printf("k = %d\n", k);

        av_packet_unref(&packet);
    }
    int i = 0;
    printf("-----------------------------------------\n");
    for(auto var : g_list)
    {
        if((var->buf[4] & 0x1f) == 5UL)
        {
            var->is_idr = true;
            // printf("idr index:%d\n",i );
        }
        i++;
    }


    GMainLoop* loop;
    GstRTSPServer* server;
    GstRTSPMountPoints* mounts;
    GstRTSPMediaFactory* factory;
    GOptionContext* optctx;
    GError* error = NULL;

    optctx = g_option_context_new("<launch line> - Test RTSP Server, Launch\n\n"
        "Example: \"( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )\"");
    g_option_context_add_main_entries(optctx, entries, NULL);
    g_option_context_add_group(optctx, gst_init_get_option_group());
    if (!g_option_context_parse(optctx, &argc, &argv, &error)) {
        g_printerr("Error parsing options: %s\n", error->message);
        g_option_context_free(optctx);
        g_clear_error(&error);
        return -1;
    }
    g_option_context_free(optctx);

    loop = g_main_loop_new(NULL, FALSE);

    /* create a server instance */
    server = gst_rtsp_server_new();
    g_object_set(server, "service", port, NULL);

    /* get the mount points for this server, every server has a default object
     * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points(server);

    /* make a media factory for a test stream. The default media factory can use
     * gst-launch syntax to create pipelines.
     * any launch line works as long as it contains elements named pay%d. Each
     * element with pay%d names will be a stream */
    factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, argv[1]);


    g_signal_connect(factory,
        "media-configure",
        (GCallback)(media_configure_callback),
        NULL);

    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

    /* don't need the ref to the mapper anymore */
    g_object_unref(mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach(server, NULL);

    /* start serving */
    g_print("stream ready at rtsp://127.0.0.1:%s/test\n", port);




    g_main_loop_run(loop);

    return 0;
}
