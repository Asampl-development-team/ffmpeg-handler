#pragma once

#include <vector>
#include <utility>
#include <iostream>
#include <variant>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include "ffmpeg-common.hpp"

constexpr size_t BUFFER_SIZE = 8194;

struct Upload : public Common {
    Upload() {
        m_io_context = avio_alloc_context(
                reinterpret_cast<uint8_t*>(av_malloc(BUFFER_SIZE)),
                BUFFER_SIZE,
                0,
                this,
                nullptr,
                [](void* handler, uint8_t* buf, int buf_size) {
                return reinterpret_cast<Upload*>(handler)->write_packet(buf, buf_size);
                },
                nullptr
                );

        if (avformat_alloc_output_context2(&m_format_context, nullptr, "mpeg", nullptr) < 0) {
            throw std::runtime_error("Could not allocate context");
        }

        m_format_context->pb = m_io_context;
        m_format_context->flags |= AVFMT_FLAG_CUSTOM_IO;
    }

    int write_packet(uint8_t* buffer, size_t buf_size) {
        auto target = m_data.end();
        m_data.resize(m_data.size() + buf_size);

        std::copy(buffer, buffer + buf_size, target);

        return buf_size;
    }
};
