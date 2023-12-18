cmake_minimum_required(VERSION 3.10)
project(multistream_hailo)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

include_directories(
    ${GSTREAMER_INCLUDE_DIRS}
    /home/guyp/SDK/hailo_ai_sw_suite/artifacts/tappas/core/hailo/general/
    /opt/hailo/tappas/include/gsthailometa/
    /usr/include/gstreamer-1.0/gst/hailo/   
    )

link_directories(
    /opt/hailo/tappas/lib/x86_64-linux-gnu/
)
    
add_executable(multistream_hailo multistream_hailo.cpp)

target_link_libraries(multistream_hailo PUBLIC ${GSTREAMER_LIBRARIES} libgsthailometa.so)
