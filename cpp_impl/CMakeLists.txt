cmake_minimum_required(VERSION 3.10)
project(multistream_hailo)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})

add_executable(multistream_hailo multistream_hailo.cpp)

target_link_libraries(multistream_hailo ${GSTREAMER_LIBRARIES})
