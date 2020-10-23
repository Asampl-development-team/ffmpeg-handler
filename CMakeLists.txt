cmake_minimum_required(VERSION 3.0.0)
include(ExternalProject)

project(ffmpeg-handler)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_subdirectory(deps/Asampl)
endif()

include(cmake/ffmpeg.cmake)

set(CMAKE_CXX_STANDARD 17)
file(GLOB HANDLER_SRCS src/*.cpp)

add_compile_options(-g)

# handler
add_library(
    ffmpeg-handler SHARED
    ${HANDLER_SRCS})

target_link_libraries(
    ffmpeg-handler
    PRIVATE FFmpeg::FFmpeg
    PRIVATE asampl-interface)

# example
add_executable(example EXCLUDE_FROM_ALL Example.cpp)

target_link_libraries(example asampl-lib)
