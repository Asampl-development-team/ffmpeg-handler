cmake_minimum_required(VERSION 3.0.0)
include(ExternalProject)

project(ffmpeg-handler)

include(cmake/asampl.cmake)
include(cmake/ffmpeg.cmake)

set(CMAKE_CXX_STANDARD 17)
file(GLOB HANDLER_SRCS src/*.cpp)

add_compile_options(-g)

add_library(
    ffmpeg-handler SHARED
    ${HANDLER_SRCS})

target_link_libraries(
    ffmpeg-handler
    PRIVATE FFmpeg::FFmpeg
    PRIVATE Asampl)
