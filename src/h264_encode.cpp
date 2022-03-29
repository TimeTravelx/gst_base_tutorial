#include <cstdio>
#include <gst/gst.h>

/**
 * @brief encode yuv to h264
 * 
 * @pre use ffmepg to generate yuv raw data
 *      generate : ffmpeg -i input.mp4 -f rawvideo -vcodec rawvideo -pix_fmt yuv420p -s 320x240 -r 30 video.raw \
 *                                     -f rawaudio -vcodec rawaudio -f f32le -ac 1 -ar 44100 audio.raw
 *      test play : 
 *          ffplay -f rawvideo -pix_fmt yuv420p -video_size 320x240 rawvideo.yuv
 *          ffplay -f f32le -ac 1 -ar 44100 a.raw
 * 
 * @pre use gstreamer pipeline to encode yuv raw data -> h264 data
 *      gst-launch-1.0 filesrc location=video.yuv ! rawvideoparse width=320 height=240 framerate=30/1 ! x264enc ! filesink location=video.h264
 * 
 *      test play : ffplay video.h264
 *      
 * */

#define VIDEO_WIDTH     320
#define VIDEO_HEIGHT    240

static const char *src_filename = NULL;
static const char *dst_filename = NULL;

static GstElement* g_pipeline    = NULL;
static GstElement* g_filesrc     = NULL;
static GstElement* g_videoparse  = NULL;
static GstElement* g_x264enc     = NULL;
static GstElement* g_filesink    = NULL;
static GMainLoop * g_mainloop    = NULL;
static GstBus    * g_bus         = NULL;

static int init_h264_encode_pipeline();
static int need_video_data_callback();
static int enough_video_data_callback();
static gboolean gst_bus_callback(GstBus* bus, GstMessage* message, gpointer user_data);


int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s  rawvideo_file(320*240 30) h264_file\n"
            "API example program to show how to read frames from an input file.\n"
            "This program reads frames from a rawvideo_file, encode them, and writes to h264_file\n",
            argv[0]);
        exit(1);
    }

    src_filename = argv[1];
    dst_filename = argv[2];

    gst_init(NULL, NULL);

    init_h264_encode_pipeline();


    g_bus = gst_element_get_bus (g_pipeline);
    gst_bus_add_watch (g_bus, (GstBusFunc)gst_bus_callback, NULL);

    gst_element_set_state(g_pipeline, GST_STATE_PLAYING);
    printf("h264 encode....\n");
    
    
    g_mainloop = g_main_loop_new (NULL, FALSE);
    // printf("loop run.\n");
    
    g_main_loop_run (g_mainloop);

    gst_object_unref(GST_OBJECT (g_pipeline));
    gst_bus_remove_watch(g_bus);
    gst_object_unref(g_bus);
    g_main_loop_unref (g_mainloop);
    return 0;
}

int init_h264_encode_pipeline()
{
    
    g_pipeline   = gst_pipeline_new("h264_pipeline");
    g_filesrc    = gst_element_factory_make("filesrc"       , "h264_filesrc" );
    g_videoparse = gst_element_factory_make("rawvideoparse" , "h264_parse"   );
    g_x264enc    = gst_element_factory_make("x264enc"       , "h264_enc"     );
    g_filesink   = gst_element_factory_make("filesink"      , "h264_filesink");

    if( !g_pipeline || !g_filesrc || !g_videoparse || !g_x264enc || !g_filesink)
    {
        printf("[not all element created,(%s)(%s)(%s)(%s)(%s)]\n", 
            !g_pipeline     ?"ng":"ok",
            !g_filesrc      ?"ng":"ok",
            !g_videoparse   ?"ng":"ok",
            !g_x264enc      ?"ng":"ok",
            !g_filesink     ?"ng":"ok");
        if(!g_pipeline)
            gst_object_unref(GST_OBJECT (g_pipeline));
        if(!g_filesrc)
            gst_object_unref(GST_OBJECT (g_filesrc));
        if(!g_videoparse)
            gst_object_unref(GST_OBJECT (g_videoparse));
        if(!g_x264enc)
            gst_object_unref(GST_OBJECT (g_x264enc));
        if(!g_filesink)
            gst_object_unref(GST_OBJECT (g_filesink));           
        return -1;
    }

    gst_bin_add_many(GST_BIN(g_pipeline), 
                     g_filesrc, 
                     g_videoparse,
                     g_x264enc,
                     g_filesink,
                     NULL);
 
    g_object_set(G_OBJECT(g_filesrc), "location", src_filename, NULL);

    g_object_set(G_OBJECT(g_filesink), "location", dst_filename, NULL);

    g_object_set(G_OBJECT(g_videoparse), "width", 320, NULL);
    g_object_set(G_OBJECT(g_videoparse), "height", 240, NULL);
    g_object_set(G_OBJECT(g_videoparse), "framerate", 30, 1, NULL);

    if(!gst_element_link(g_filesrc      , g_videoparse))
    {
        printf("[%s => %s success]\n", GST_ELEMENT_NAME(g_filesrc), GST_ELEMENT_NAME(g_videoparse));
        gst_object_unref(GST_OBJECT (g_pipeline));
        return -1;
    }

    if(!gst_element_link(g_videoparse   , g_x264enc))
    {
        printf("[%s => %s success]\n", GST_ELEMENT_NAME(g_filesrc), GST_ELEMENT_NAME(g_videoparse));
        gst_object_unref(GST_OBJECT (g_pipeline));
        return -1;
    }

    if(!gst_element_link(g_x264enc      , g_filesink))
    {
        printf("[%s => %s success]\n", GST_ELEMENT_NAME(g_filesrc), GST_ELEMENT_NAME(g_videoparse));
        gst_object_unref(GST_OBJECT (g_pipeline));
        return -1;
    }    


    return 0;

}

gboolean gst_bus_callback(GstBus* bus, GstMessage* message, gpointer user_data)
{
    printf("gst_bus_callback bus:%s %s\n", GST_MESSAGE_SRC_NAME(message), GST_MESSAGE_TYPE_NAME(message));
    

    switch (GST_MESSAGE_TYPE(message))
    {
    case GST_MESSAGE_EOS: 
    {
       g_print("Element %s EOS.\n", GST_OBJECT_NAME (message->src));
       exit(0);
       break;
    }
    case GST_MESSAGE_ERROR: 
    {
       GError *err = NULL;
       gchar *dbg_info = NULL;

       gst_message_parse_error (message, &err, &dbg_info);
       g_printerr ("ERROR from element %s: %s\n",
           GST_OBJECT_NAME (message->src), err->message);
       g_printerr ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
       g_error_free (err);
       g_free (dbg_info);
       break;
    }
    case GST_MESSAGE_STREAM_STATUS:
    {
        GstElement *owner = NULL;
        GstStreamStatusType status;
        gst_message_parse_stream_status (message, &status, &owner);
        g_print("OWNER: %s Status: %d\n", GST_OBJECT_NAME (owner), status);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
       GstState old_state, new_state;
       gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
       g_print ("Element %s changed state from %s to %s.\n",
           GST_OBJECT_NAME (message->src),
           gst_element_state_get_name (old_state),
           gst_element_state_get_name (new_state));
    }  
    default:
        break;
    }
    return TRUE;
}