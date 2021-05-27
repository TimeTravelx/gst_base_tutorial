cmake_minimum_required(VERSION 3.0.0)
project(GST_BASIC_TUTORIAL VERSION 0.1.0)

include_directories(/usr/include/gstreamer-1.0)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include)


add_executable(playbinuse ${CMAKE_SOURCE_DIR}/playbinuse.c)
target_link_libraries(playbinuse gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)


add_executable(BT01 ${CMAKE_SOURCE_DIR}/BT01HelloGst.c)
target_link_libraries(BT01 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT02 ${CMAKE_SOURCE_DIR}/BT02Concepts.c)
target_link_libraries(BT02 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT03 ${CMAKE_SOURCE_DIR}/BT03DynamicPads.c)
target_link_libraries(BT03 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT04 ${CMAKE_SOURCE_DIR}/BT04Seeking.c)
target_link_libraries(BT04 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

# add_executable(BT05 ${CMAKE_SOURCE_DIR}/BT05GuiToolkit.c)
# target_link_libraries(BT05 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)

add_executable(BT06 ${CMAKE_SOURCE_DIR}/BT06Caps.c)
target_link_libraries(BT06 gio-2.0 gobject-2.0 glib-2.0 gstreamer-1.0)