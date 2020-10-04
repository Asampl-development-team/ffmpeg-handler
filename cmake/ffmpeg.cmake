ExternalProject_Add(
    ffmpeg
    GIT_REPOSITORY git://source.ffmpeg.org/ffmpeg.git
    GIT_TAG n4.3
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-shared --disable-stripping
    BUILD_COMMAND make -j4
    INSTALL_COMMAND make install
    UPDATE_COMMAND ""
    BUILD_ALWAYS FALSE)

ExternalProject_Get_Property(ffmpeg INSTALL_DIR)
set(FFMPEG_DIR ${INSTALL_DIR})

add_library(FFmpeg::avcodec SHARED IMPORTED)
set_target_properties(FFmpeg::avcodec PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_DIR}/lib/libavcodec.so)

add_library(FFmpeg::avformat SHARED IMPORTED)
set_target_properties(FFmpeg::avformat PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_DIR}/lib/libavformat.so)

add_library(FFmpeg::avutil SHARED IMPORTED)
set_target_properties(FFmpeg::avutil PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_DIR}/lib/libavutil.so)

add_library(FFmpeg::swresample SHARED IMPORTED)
set_target_properties(FFmpeg::swresample PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_DIR}/lib/libswresample.so)

add_library(FFmpeg::swscale SHARED IMPORTED)
set_target_properties(FFmpeg::swscale PROPERTIES
    IMPORTED_LOCATION ${FFMPEG_DIR}/lib/libswscale.so)

add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
add_dependencies(FFmpeg::FFmpeg ffmpeg)
#set_property(TARGET FFmpeg::FFmpeg
    #PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${FFMPEG_DIR}/include)
set_property(TARGET FFmpeg::FFmpeg
    PROPERTY INTERFACE_LINK_LIBRARIES FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::swresample FFmpeg::swscale)

include_directories(${FFMPEG_DIR}/include)
