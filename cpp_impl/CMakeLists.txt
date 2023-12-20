cmake_minimum_required(VERSION 3.10)
project(multistream_hailo)

set(CMAKE_CXX_STANDARD 11)


set(gst_req "1.0.0")

# Find GStreamer
find_package(PkgConfig)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=${gst_req})
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0>=${gst_req})


# Find Headers
set(tappas_path "/home/guyp/SDK/hailo_ai_sw_suite/artifacts/tappas/")

set(hailo_objects.hpp_path ${tappas_path}core/hailo/general/)
set(gst_hailo_meta.hpp_path /opt/hailo/tappas/include/gsthailometa/)
set(tensor_meta.hpp_path /usr/include/gstreamer-1.0/gst/hailo/)

if(NOT EXISTS ${hailo_objects.hpp_path} OR NOT EXISTS ${gst_hailo_meta.hpp_path} OR NOT EXISTS ${tensor_meta.hpp_path})
    message(FATAL_ERROR "tappas headers not found. please adjust headers paths in CMakeLists.txt")
endif()

include_directories(
    ${GSTREAMER_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${hailo_objects.hpp_path}
    ${gst_hailo_meta.hpp_path}
    ${tensor_meta.hpp_path}
)

# Find Hailo plugins Libraries
link_directories(
    /opt/hailo/tappas/lib/x86_64-linux-gnu/ #libgsthailometa.so
)

add_executable(multistream_hailo multistream_hailo.cpp)

target_link_libraries(multistream_hailo PUBLIC ${GSTREAMER_LIBRARIES} ${GST_APP_LIBRARIES} libgsthailometa.so)
