#include <gst/gst.h>

//1. 使用 gst_init() 初始化 GStreamer
//2. 使用 gst_parse_launch 快速建立一个 pipeline，可以解析 字符串 以播放数据
//3. 使用 gst_element_set_state() 改变状态，驱动视频播放
//4. 使用 gst_element_get_bus() & gst_bus_timed_pop_filtered() 监听 bus 上的消息

int main (int argc, char *argv[]) 
{
    GstElement *pipeline = NULL;
    GstBus *bus = NULL;
    GstMessage *msg = NULL;

    gst_init(&argc, &argv);

    pipeline = gst_parse_launch("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",NULL);
    /*gst_parse_launch 用法类似 gst-launch-1.0, DEBUG-TOOL 的 C 实现版本*/


    // pipeline = gst_element_factory_make ("playbin", "player");
    // g_object_set (pipeline, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
    /* playbin gst_element_factory_make 用法*/
    
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    /* 通过设置 PLAYING 驱动了整个 pipeline的创建，如何做到的，后面需要着重看一下*/

    bus = gst_element_get_bus(pipeline); 
    /* Wait until error or EOS, it will block*/
    msg = gst_bus_timed_pop_filtered (
        bus, 
        GST_CLOCK_TIME_NONE/*超时时间*/, 
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS /*捕获事件类型*/
        );

    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}