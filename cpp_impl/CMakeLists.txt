cmake_minimum_required(VERSION 3.10)
project(multistream_hailo)

set(CMAKE_CXX_STANDARD 11)


set(gst_req "1.0.0")
# Find GStreamer
find_package(PkgConfig)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=${gst_req})

# Find GStreamer Base
# pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0>=${gst_req})

# Find GStreamer App
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=${gst_req})

# Find GStreamer Video
# pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0>=${gst_req})

include_directories(
    ${GSTREAMER_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    /home/guyp/SDK/hailo_ai_sw_suite/artifacts/tappas/core/hailo/general/ #hailo_objects.hpp 
    /opt/hailo/tappas/include/gsthailometa/ #gst_hailo_meta.hpp
    /usr/include/gstreamer-1.0/gst/hailo/ #tensor_meta.hpp
)

link_directories(
    /opt/hailo/tappas/lib/x86_64-linux-gnu/ #libgsthailometa.so
)

add_executable(multistream_hailo multistream_hailo.cpp)

target_link_libraries(multistream_hailo PUBLIC ${GSTREAMER_LIBRARIES} ${GST_APP_LIBRARIES} libgsthailometa.so)
