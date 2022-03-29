#include <gst/gst.h>
// video/x-h264:
//   stream-format: { (string)avc, (string)avc3 }
//       alignment: au
//           width: [ 16, 2147483647 ]
//          height: [ 16, 2147483647 ]

/**
 * gst-launch-1.0 filesrc location=../media/video.h264 ! video/x-h264,width=320,height=240 framerate=30/1 ! qtmux ! filesink location=video.mov
 * gst-launch-1.0 filesrc location=../media/video.h264 ! h264parse config-interval=-1 ! qtmux ! filesink location=video.mov
 * gst-launch-1.0 filesrc location=../media/video.yuv ! rawvideoparse width=320 height=240 framerate=30/1 ! x264enc ! qtmux ! filesink location=xxx.mov
 * gst-launch-1.0 filesrc location=../media/video.raw ! rawvideoparse width=320 height=240 framerate=30/1 ! x264enc ! h264parse ! qtmux ! filesink location=xxx.mov
 * */

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s  h264raw(320*240 30) record_file\n"
            "API example program to show how to read frames from an input file.\n"
            "This program reads frames from a rawvideo_file, encode them, and writes to h264_file\n",
            argv[0]);
        exit(1);
    }


}