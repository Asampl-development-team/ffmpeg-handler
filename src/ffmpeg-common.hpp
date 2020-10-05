#pragma once

#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include <handler_interface.h>

struct Common {
    std::vector<uint8_t> m_data;

    AVIOContext* m_io_context;
    AVFormatContext* m_format_context;

    AVCodecContext* m_cc;
    AVFrame* m_frame;
    AVPacket m_packet;

    void push(const AsaData* data) {
        m_data.insert(m_data.end(), data->data, data->data + data->size);
    }

    static AsaData* make_data_normal(float time, uint16_t width, uint16_t height) {
        const auto data_size = sizeof(uint16_t)*2 + width*height*3;

        auto data = new AsaData{
            ASA_STATUS_NORMAL,
            time,
            data_size,
            new uint8_t[data_size],
            nullptr
        };

        AsaVideoData* video_data = reinterpret_cast<AsaVideoData*>(data->data);
        video_data->width = width;
        video_data->height = height;

        return data;
    }

    static AsaData* make_data_fatal(const char* error) {
        char* error_str = new char[strlen(error)];
        strcpy(error_str, error);
        return new AsaData{ASA_STATUS_FATAL, 0, 0, nullptr, error_str};
    }

    static AsaData* make_data_again() {
        return new AsaData{ASA_STATUS_AGAIN};
    }

    static AsaData* make_data_eoi() {
        return new AsaData{ASA_STATUS_EOI};
    }

    static void free(AsaData* data) {
        delete [] data->data;
        delete [] data->error;
        delete data;
    }
};
