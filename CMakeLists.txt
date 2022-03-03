cmake_minimum_required(VERSION 3.0.0)
project(gst_record VERSION 0.1.0)

message("## PKGCONFIG ########################################################")
find_package(PkgConfig REQUIRED)
message("## version         : ${PKG_CONFIG_VERSION_STRING}")
message("## executable      : ${PKG_CONFIG_EXECUTABLE}"    )
message("## pkg config path : ${PKG_CONFIG_PATH}"    )
message("#####################################################################")

pkg_check_modules(GST     REQUIRED gstreamer-1.0    )
pkg_check_modules(GOBJECT REQUIRED gobject-2.0      )
pkg_check_modules(GLIB    REQUIRED glib-2.0         )
pkg_check_modules(GIO     REQUIRED gio-2.0          )

include_directories(${GST_INCLUDE_DIRS}             )
include_directories(${GOBJECT_INCLUDE_DIRS}         )
include_directories(${GLIB_INCLUDE_DIRS}            )
include_directories(${GIO_INCLUDE_DIRS}             )

link_directories(${GST_LIBRARY_DIRS}                )
link_directories(${GOBJECT_LIBRARY_DIRS}            )
link_directories(${GLIB_LIBRARY_DIRS}               )
link_directories(${GIO_LIBRARY_DIRS}                )







add_executable(playbinuse ${CMAKE_SOURCE_DIR}/src/playbinuse.c)
target_link_libraries(playbinuse gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)


add_executable(BT01 ${CMAKE_SOURCE_DIR}/src/BT01HelloGst.c)
target_link_libraries(BT01 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT02 ${CMAKE_SOURCE_DIR}/src/BT02Concepts.c)
target_link_libraries(BT02 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT03 ${CMAKE_SOURCE_DIR}/src/BT03DynamicPads.c)
target_link_libraries(BT03 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT04 ${CMAKE_SOURCE_DIR}/src/BT04Seeking.c)
target_link_libraries(BT04 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

# add_executable(BT05 ${CMAKE_SOURCE_DIR}/BT05GuiToolkit.c)
# target_link_libraries(BT05 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT06 ${CMAKE_SOURCE_DIR}/src/BT06Caps.c)
target_link_libraries(BT06 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(gst_record  ${CMAKE_SOURCE_DIR}/src/record.cpp)
target_link_libraries(gst_record gstreamer-1.0 glib-2.0 gobject-2.0)

add_executable(h264_encode ${CMAKE_SOURCE_DIR}/src/h264_encode.cpp)
target_link_libraries(h264_encode gstreamer-1.0 glib-2.0 gobject-2.0)