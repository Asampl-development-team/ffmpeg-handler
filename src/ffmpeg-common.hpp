#pragma once

#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

#include <asampl-ffi/ffi.h>
}

struct Common {
    std::vector<uint8_t> m_data;

    AVIOContext* m_io_context;
    AVFormatContext* m_format_context;

    AVCodecContext* m_cc;
    AVFrame* m_frame;
    AVPacket m_packet;

    void push(const AsaBytes* data) {
        m_data.insert(m_data.end(), data->data, data->data + data->size);
    }
};
