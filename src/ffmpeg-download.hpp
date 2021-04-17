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

struct Success {};
struct Again {};
struct Eoi {};

using DownloadResult = std::variant<Success, Again, std::string>;
using DecodeResult = std::variant<Success, Again, Eoi, std::string>;

struct Download : public Common {
    bool m_input_open = false;
    bool m_stream_detected = false;
    int m_video_stream_id;

    Download() {
        m_io_context = avio_alloc_context(
                reinterpret_cast<uint8_t*>(av_malloc(BUFFER_SIZE)),
                BUFFER_SIZE,
                0,
                this,
                [](void* handler, uint8_t* buf, int buf_size) {
                return reinterpret_cast<Download*>(handler)->read_packet(buf, buf_size);
                },
                nullptr,
                nullptr
                );
        if (!m_io_context) {
            throw std::runtime_error("Could not allocate io context");
        }

        m_format_context = avformat_alloc_context();
        if (!m_format_context) {
            throw std::runtime_error("Could not allocate format context");
        }

        m_format_context->pb = m_io_context;
        m_format_context->flags |= AVFMT_FLAG_CUSTOM_IO;

        av_init_packet(&m_packet);

        m_frame = av_frame_alloc();
        if (!m_frame) {
            throw std::runtime_error("Could not allocate frame");
        }
    }

    ~Download() {
        av_packet_unref(&m_packet);
        av_frame_free(&m_frame);

        if (m_input_open) {
            avformat_close_input(&m_format_context);
        }

        av_free(m_io_context->buffer);
        av_free(m_io_context);
    }

    AsaHandlerResponse download() {
        AsaHandlerResponse response;

        if (!ensure_open_input()) {
            asa_new_response_fatal("Could not open input", &response);
            return response;
        }

        {
            const auto detect = ensure_detect_stream();
            if (std::holds_alternative<Again>(detect)) {
                asa_new_response_not_ready(&response);
                return response;
            } else if (std::holds_alternative<std::string>(detect)) {
                asa_new_response_fatal(std::get<std::string>(detect).c_str(), &response);
                return response;
            }
        }

        {
            const auto decode = decode_next();
            if (std::holds_alternative<Again>(decode)) {
                asa_new_response_not_ready(&response);
                return response;
            } else if (std::holds_alternative<Eoi>(decode)) {
                asa_new_response_eoi(&response);
                return response;
            } else if (std::holds_alternative<std::string>(decode)) {
                asa_new_response_fatal(std::get<std::string>(decode).c_str(), &response);
                return response;
            }
        }

        convert_data(response);
        return response;
    }

    bool ensure_open_input() {
        if (!m_input_open) {
            if (avformat_open_input(&m_format_context, "", nullptr, nullptr) < 0) {
                return false;
            }

            m_input_open = true;
        }

        return true;
    }

    DownloadResult ensure_detect_stream() {
        if (!m_stream_detected) {
            if (avformat_find_stream_info(m_format_context, nullptr) < 0) {
                return Again{};
            }

            AVCodec* codec = nullptr;
            m_video_stream_id = av_find_best_stream(m_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
            if (m_video_stream_id < 0 || codec == nullptr) {
                return "Could not find video stream";
            }

            m_cc = avcodec_alloc_context3(codec);
            if (!m_cc) {
                return "Could not allocate codec context";
            }

            if (avcodec_parameters_to_context(m_cc, m_format_context->streams[m_video_stream_id]->codecpar) < 0) {
                return "Could not copy codec parameters";
            }

            if (auto code = avcodec_open2(m_cc, codec, nullptr); code < 0) {
                return "Could not open codec";
            }

            m_stream_detected = true;
        }

        return Success{};
    }

    DecodeResult decode_next() {
        if (avcodec_receive_frame(m_cc, m_frame) < 0) {
            do {
                if (auto code = av_read_frame(m_format_context, &m_packet); code < 0) {
                    if (code == AVERROR_EOF) {
                        return Eoi{};
                    } else {
                        return Again{};
                    }
                }
            } while (m_packet.stream_index != m_video_stream_id);

            if (auto code = avcodec_send_packet(m_cc, &m_packet); code < 0) {
                char buf[256] = {};
                av_strerror(code, buf, 256);
                return std::string{"Could not send packet: "} + buf;
            }

            if (auto code = avcodec_receive_frame(m_cc, m_frame); code < 0) {
                if (code == AVERROR(EAGAIN)) {
                    return Again{};
                } else if (code == AVERROR_EOF) {
                    return Eoi{};
                } else {
                    return "Decoding error";
                }
            }
        }

        return Success{};
    }

    void convert_data(AsaHandlerResponse& response) {
        const double time = m_frame->pts * get_time_base();

        auto* buffer = static_cast<uint8_t*>(asa_alloc(asa_video_frame_size(m_frame->width, m_frame->height)));

        auto sws_context = sws_getContext(
            m_frame->width, m_frame->height, static_cast<AVPixelFormat>(m_frame->format),
            m_frame->width, m_frame->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        int rgb_stride[3] = {3 * m_frame->width, 0, 0};
        uint8_t* rgb_data[3] = {buffer, nullptr, nullptr};

        sws_scale(
            sws_context, m_frame->data, m_frame->linesize, 0, m_frame->height, 
            rgb_data, rgb_stride);
        sws_freeContext(sws_context);

        AsaValueContainer* container = asa_alloc_container();
        asa_new_video_frame_take(buffer, m_frame->width, m_frame->height, container);
        container->timestamp = time;
        asa_new_response_normal(container, &response);
    }

    double get_time_base() {
        const auto time_base = m_format_context->streams[m_video_stream_id]->time_base;
        return static_cast<double>(time_base.num) / static_cast<double>(time_base.den);
    }

    int read_packet(uint8_t* buffer, size_t buf_size) {
        const size_t to_read = std::min(m_data.size(), buf_size);

        std::copy(m_data.begin(), m_data.begin() + to_read, buffer);
        m_data.erase(m_data.begin(), m_data.begin() + to_read);
        return to_read;
    }
};
