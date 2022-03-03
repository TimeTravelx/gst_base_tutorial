#include <cstdio>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>


#define TAG "gst_record"

#define GST_OBJECT_UNREF(element) \
    if(element) {\
        gst_object_unref(GST_OBJECT (element)); \
    }


#define RECORD_AUDIO_RATE                   48000
#define RECORD_AUDIO_CHANNEL                1
#define RECORD_VIDEO_WIDTH                  1920
#define RECORD_VIDEO_HEIGHT                 1080
#define RECORD_VIDEO_FPS                    30
#define RECORD_MAX_NSEC                     300*1000*1000*1000ULL
#define RECORD_BYTES_PER_SEC                500
#define RECORD_MOOV_UPDATE_PERIOD           1*1000*1000*1000

static GstElement* g_pipeline     = nullptr;
static GstElement* g_video_src    = nullptr;
static GstElement* g_audio_src    = nullptr;
static GstElement* g_h264_parse   = nullptr;
static GstElement* g_faac         = nullptr;
static GstElement* g_aac_parse    = nullptr;
static GstElement* g_qtmux        = nullptr;
static GstElement* g_splitmuxsink = nullptr;

static int init_record_pipeline();

static int need_audio_data_callback();
static int enough_audio_data_callback();

static int need_video_data_callback();
static int enough_video_data_callback();

static gchar* update_record_dest_callback(GstElement* object, 
                                          guint       arg0, 
                                          gpointer    user_data);

static void splitmuxsink_muxer_added_callback(GstElement* object,
                                              GstElement* arg0,
                                              gpointer user_data);

static void splitmuxsink_sink_added_callback(GstElement * object,
                                             GstElement * arg0,
                                             gpointer user_data);





int main()
{
    printf("gst record \n");
    gst_init(NULL, NULL);
    init_record_pipeline();

    while(1);
    return 0;
}

int init_record_pipeline()
{
     ///////////////////////////////////////////////////////////////////////////
    // Create elements
    ////////////////////////////////////////////////////////////////////////////

    auto record_elements_unref_fn = []() {
        GST_OBJECT_UNREF(g_pipeline    );
        GST_OBJECT_UNREF(g_video_src   );
        GST_OBJECT_UNREF(g_audio_src   );
        GST_OBJECT_UNREF(g_h264_parse  );
        GST_OBJECT_UNREF(g_faac        );
        GST_OBJECT_UNREF(g_aac_parse   );
        GST_OBJECT_UNREF(g_qtmux       );
        GST_OBJECT_UNREF(g_splitmuxsink);
    };

    g_pipeline      = gst_pipeline_new("dvr_pipeline");
    g_video_src     = gst_element_factory_make("appsrc"      , "record_video_src" );
    g_audio_src     = gst_element_factory_make("appsrc"      , "record_audio_src" );
    g_h264_parse    = gst_element_factory_make("h264parse"   , "record_h264_parse");
    g_faac          = gst_element_factory_make("faac"        , "record_faac"      );
    g_aac_parse     = gst_element_factory_make("aacparse"    , "record_aac_parse" );
    g_qtmux         = gst_element_factory_make("qtmux"       , "record_mux"       );
    g_splitmuxsink  = gst_element_factory_make("splitmuxsink", "record_sink"      );


    if( !g_pipeline || !g_video_src || !g_audio_src   || !g_h264_parse ||
        !g_faac     || !g_aac_parse || !g_qtmux       || !g_splitmuxsink)
    {
        printf("[%s][not all element created,(%s)(%s)(%s)(%s)(%s)(%s)(%s)(%s)]\n", 
            TAG,
            !g_pipeline     ?"ng":"ok",
            !g_video_src    ?"ng":"ok",
            !g_audio_src    ?"ng":"ok",
            !g_h264_parse   ?"ng":"ok",
            !g_faac         ?"ng":"ok",
            !g_aac_parse    ?"ng":"ok",
            !g_qtmux        ?"ng":"ok",
            !g_splitmuxsink ?"ng":"ok");

        record_elements_unref_fn();
        return -1;
    }

    ////////////////////////////////////////////////////////////////////////////
    // set elements properties
    ////////////////////////////////////////////////////////////////////////////

    // record_audio_src  properties --------------------------------------------
    
    // how to set properties
    g_object_set(G_OBJECT(g_audio_src)  , 
                 "stream-type"          , GST_APP_STREAM_TYPE_STREAM, 
                 "format"               , GST_FORMAT_TIME, 
                 NULL);

    g_object_set(G_OBJECT(g_audio_src), "min-percent", 0, NULL);

    GstCaps *caps_audio_src 
        = gst_caps_new_simple("audio/x-raw",
                              "format"  , G_TYPE_STRING, "S16LE"              ,
                              "layout"  , G_TYPE_STRING, "interleaved"        ,
                              "rate"    , G_TYPE_INT   , RECORD_AUDIO_RATE    ,
                              "channels", G_TYPE_INT   , RECORD_AUDIO_CHANNEL , 
                              NULL);

    g_object_set(G_OBJECT(g_audio_src), "caps", caps_audio_src, NULL);
    gst_caps_unref(caps_audio_src);

    // how to set callback
    g_signal_connect(g_audio_src, 
                     "need-data"  , G_CALLBACK(need_audio_data_callback  ), 
                     NULL/*user data*/);
    g_signal_connect(g_audio_src, 
                     "enough-data", G_CALLBACK(enough_audio_data_callback), 
                     NULL/*user data*/);



    // record_video_src  properties --------------------------------------------
    g_object_set(G_OBJECT(g_video_src)  , 
                 "stream-type"          , GST_APP_STREAM_TYPE_STREAM, 
                 "format"               , GST_FORMAT_TIME, 
                 NULL);

    g_object_set(G_OBJECT(g_video_src), "min-percent", 0, NULL);

    

    // I don't think it is necessary
    GstCaps *caps_video_src 
        = gst_caps_new_simple("video/x-h264",
                              "format"   , G_TYPE_STRING     , "byte-stream"       ,
                              "alignment", G_TYPE_STRING     , "au"                ,
                              "width"    , G_TYPE_INT        , RECORD_VIDEO_WIDTH  ,
                              "height"   , G_TYPE_INT        , RECORD_VIDEO_HEIGHT , 
                              "framerate", GST_TYPE_FRACTION , RECORD_VIDEO_FPS    , 1,
                              NULL);

    g_object_set(G_OBJECT(g_video_src), "caps", caps_video_src, NULL);
    gst_caps_unref(caps_video_src);

    g_signal_connect(g_video_src, 
                     "need-data"  , G_CALLBACK(need_video_data_callback  ), 
                     NULL/*user data*/);
    g_signal_connect(g_video_src, 
                     "enough-data", G_CALLBACK(enough_video_data_callback), 
                     NULL/*user data*/);

    // record_h264_parse  properties -------------------------------------------

    // 如果h264流数据中IDR中没有SPS\PPS串，这里需要设置成-1，在feed时填充
    // Send SPS and PPS Insertion Interval in seconds (sprop parameter sets will 
    //be multiplexed in the data stream when detected.) 
    //(0 = disabled, -1 = send with every IDR frame)
    g_object_set(G_OBJECT(g_h264_parse), "config-interval", -1, NULL);

    // qt_mux  properties ------------------------------------------------------
    g_object_set(G_OBJECT(g_qtmux), 
                 "reserved-max-duration"        , (guint64)(RECORD_MAX_NSEC), 
                 NULL);
    g_object_set(G_OBJECT(g_qtmux), 
                 "reserved-bytes-per-sec"       , (guint32)(RECORD_BYTES_PER_SEC), 
                 NULL);
    g_object_set(G_OBJECT(g_qtmux), 
                 "reserved-moov-update-period"  , (guint64)(RECORD_MOOV_UPDATE_PERIOD), 
                 NULL);

    // record sink  properties -------------------------------------------------
    g_object_set(G_OBJECT(g_splitmuxsink), "muxer", g_qtmux, NULL);


    g_object_set(G_OBJECT(g_splitmuxsink), 
                 "max-size-time", (guint64)(RECORD_MAX_NSEC), 
                 NULL);
    g_signal_connect(g_splitmuxsink, 
                     "format-location", G_CALLBACK(update_record_dest_callback), 
                     NULL);
    g_signal_connect(g_splitmuxsink, 
                     "sink-added"     , G_CALLBACK(splitmuxsink_sink_added_callback), 
                     NULL);
    g_signal_connect(g_splitmuxsink, 
                     "muxer-added"    , G_CALLBACK(splitmuxsink_muxer_added_callback), 
                     NULL);                     
    
    ////////////////////////////////////////////////////////////////////////////
    // link elements
    //
    // appsrc -> h264parse ---------->|
    //                                |  -> splitmuxsink
    // appsrc -> facc -> aacparse --->|
    ////////////////////////////////////////////////////////////////////////////
    gst_bin_add_many(GST_BIN(g_pipeline), 
                    g_video_src, 
                    g_h264_parse, 
                    g_audio_src, 
                    g_faac, 
                    g_aac_parse,
                    g_splitmuxsink,
                    NULL);
    // link appsrc -> h264parse -> splitmuxsink
    if(!gst_element_link_many(g_video_src, g_h264_parse, NULL))
    {
        printf("[%s][link video src => h264 parse failed]\n", TAG);
        record_elements_unref_fn();
        return -1;
    }

    if(!gst_element_link_pads(g_h264_parse, "src", g_splitmuxsink, "video"))
    {
        printf("[%s][link h264 parse => mux sink failed]\n", TAG);
        GST_OBJECT_UNREF(g_pipeline    );
        return -1;  
    }
    
    // link appsrc -> faac -> aacparse -> splitmuxsink
    GstCaps* caps_src2faac
        = gst_caps_new_simple("audio/x-raw",
                              "format"  , G_TYPE_STRING, "S16LE"              ,
                              "layout"  , G_TYPE_STRING, "interleaved"        ,
                              "rate"    , G_TYPE_INT   , RECORD_AUDIO_RATE    ,
                              "channels", G_TYPE_INT   , RECORD_AUDIO_CHANNEL , 
                              NULL);
    if(!gst_element_link_filtered(g_audio_src, g_faac, caps_src2faac))
    {
        printf("[%s][link appsrc => faac failed]\n", TAG);
        gst_caps_unref(caps_src2faac);
        GST_OBJECT_UNREF(g_pipeline   );
        return -1;  
    }
    gst_caps_unref(caps_src2faac);



    GstCaps* caps_faac2accparse
        = gst_caps_new_simple("audio/mpeg",
                              "mpegversion"     , G_TYPE_INT    , 4,
                              "channels"        , G_TYPE_INT    , RECORD_AUDIO_CHANNEL,
                              "rate"            , G_TYPE_INT    , RECORD_AUDIO_RATE,
                              "stream-format"   , G_TYPE_STRING , "raw",
                              "base-profile"    , G_TYPE_STRING , "lc",
                              "framed"          , G_TYPE_BOOLEAN, TRUE, 
                              NULL);
    if(!gst_element_link_filtered(g_faac, g_aac_parse, caps_faac2accparse))
    {
        printf("[%s][link faac => aac parse failed]\n", TAG);
        gst_caps_unref(caps_faac2accparse);
        GST_OBJECT_UNREF(g_pipeline        );
        return -1;  
    }
    gst_caps_unref(caps_faac2accparse);

    if(!gst_element_link_pads(g_aac_parse, "src", g_splitmuxsink, "audio_%u"))
    {
        GST_OBJECT_UNREF(g_pipeline        );
        return -1;
    }

    
    gst_element_set_state(g_pipeline, GST_STATE_PLAYING);

    GST_OBJECT_UNREF(g_pipeline    );
    printf("end\n");


    return 0;
}

int need_audio_data_callback()
{
    printf("need_audio_data_callback \n");
    return 0;
}

int enough_audio_data_callback()
{
    printf("enough_audio_data_callback \n");
    return 0;
}

int need_video_data_callback()
{
    printf("need_video_data_callback \n");
    return 0;
}

int enough_video_data_callback()
{
    printf("enough_video_data_callback \n");
    return 0;
}

gchar* update_record_dest_callback(GstElement* object, guint arg0, gpointer user_data)
{
    printf("update_record_dest_callback \n");
    return NULL;  
}

void splitmuxsink_muxer_added_callback(GstElement* object,
                                       GstElement* arg0,
                                       gpointer user_data)
{
    printf("splitmuxsink_muxer_added_callback object:%s ele:%s \n",
        GST_ELEMENT_NAME(object), GST_ELEMENT_NAME(arg0));
}

void splitmuxsink_sink_added_callback(GstElement * object,
                                      GstElement * arg0,
                                      gpointer user_data)
{
    printf("splitmuxsink_sink_added_callback object:%s ele:%s \n",
        GST_ELEMENT_NAME(object), GST_ELEMENT_NAME(arg0));
}