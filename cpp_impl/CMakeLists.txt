cmake_minimum_required(VERSION 3.10)
project(multistream_hailo)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
include_directories(
    ${GSTREAMER_INCLUDE_DIRS}
    ./hailo_core
    )
set(hailo_utils
    ./hailo_core/gst_hailo_meta.cpp
    ./hailo_core/tensor_meta.cpp
    )

add_executable(multistream_hailo multistream_hailo.cpp ${hailo_utils})

target_link_libraries(multistream_hailo PUBLIC ${GSTREAMER_LIBRARIES}) 
