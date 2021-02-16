cmake_minimum_required(VERSION 3.0.0)
include(ExternalProject)

project(ffmpeg-handler)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_subdirectory(deps/asampl-ffi)
endif()

if(${BUILD_OWN_FFMPEG})
    include(cmake/ffmpeg.cmake)
    set(FFMPEG_LIBRARIES FFmpeg::FFmpeg)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
endif()


set(CMAKE_CXX_STANDARD 17)
file(GLOB HANDLER_SRCS src/*.cpp)

add_compile_options(-g)

# handler
add_library(
    ffmpeg-handler SHARED
    ${HANDLER_SRCS})

target_link_libraries(
    ffmpeg-handler
    PRIVATE ${FFMPEG_LIBRARIES}
    PRIVATE asampl-ffi)

# install
install(TARGETS ffmpeg-handler LIBRARY DESTINATION handlers)
