#include <vector>
#include <utility>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

#include <handler_interface.h>

constexpr size_t BUFFER_SIZE = 8194;

struct Handler;

struct Handler {
    std::vector<uint8_t> m_data;

    AVIOContext* m_io_context;
    AVFormatContext* m_format_context;

    bool m_input_open = false;
    bool m_stream_detected = false;
    int m_video_stream_id;

    AVCodecContext* m_cc;
    AVFrame* m_frame;
    AVPacket m_packet;

    Handler() {
        m_io_context = avio_alloc_context(
            reinterpret_cast<uint8_t*>(av_malloc(BUFFER_SIZE)),
            BUFFER_SIZE,
            0,
            this,
            [](void* handler, uint8_t* buf, int buf_size) {
                return reinterpret_cast<Handler*>(handler)->read_packet(buf, buf_size);
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

    ~Handler() {
        av_packet_unref(&m_packet);
        av_frame_free(&m_frame);

        if (m_input_open) {
            avformat_close_input(&m_format_context);
        }

        av_free(m_io_context->buffer);
        av_free(m_io_context);
    }

    void push(const AsaData* data) {
        m_data.insert(m_data.end(), data->data, data->data + data->size);
    }

    AsaData* download() {
        if (!m_input_open) {
            if (avformat_open_input(&m_format_context, "", nullptr, nullptr) < 0) {
                return make_data_fatal("Could not open input");
            }

            m_input_open = true;
        }

        if (!m_stream_detected) {
            if (avformat_find_stream_info(m_format_context, nullptr) < 0) {
                return make_data_again();
            }

            AVCodec* codec = nullptr;
            m_video_stream_id = av_find_best_stream(m_format_context, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
            if (m_video_stream_id < 0 || codec == nullptr) {
                return make_data_fatal("Could not find video stream");
            }

            m_cc = avcodec_alloc_context3(codec);
            if (!m_cc) {
                return make_data_fatal("Could not allocate codec context");
            }

            if (avcodec_parameters_to_context(m_cc, m_format_context->streams[m_video_stream_id]->codecpar) < 0) {
                return make_data_fatal("Could not copy codec parameters");
            }

            if (auto code = avcodec_open2(m_cc, codec, nullptr); code < 0) {
                char buf[256] = {};
                av_strerror(code, buf, 256);
                auto err = std::string{"Could not open codec: "} + buf;
                return make_data_fatal(err.c_str());
            }

            m_stream_detected = true;
        }

        //do {
            //if (auto code = av_read_frame(m_format_context, &m_packet); code < 0) {
                //if (code == AVERROR_EOF) {
                    //return make_data_eoi();
                //} else {
                    //return make_data_again();
                //}
            //}
        //} while (m_packet.stream_index != m_video_stream_id);

        //int got_picture;
        //if (avcodec_decode_video2(m_cc, m_frame, &got_picture, &m_packet) < 0) {
            //return make_data_fatal("Could not decode frame");
        //}
        //if (got_picture == 0) {
            //return make_data_again();
        //}

        do {
            if (auto code = av_read_frame(m_format_context, &m_packet); code < 0) {
                if (code == AVERROR_EOF) {
                    return make_data_eoi();
                } else {
                    return make_data_again();
                }
            }
        } while (m_packet.stream_index != m_video_stream_id);

        if (avcodec_receive_frame(m_cc, m_frame) < 0) {
            if (auto code = avcodec_send_packet(m_cc, &m_packet); code < 0) {
                char buf[256] = {};
                av_strerror(code, buf, 256);
                auto err = std::string{"Could not send packet: "} + buf;
                return make_data_fatal(err.c_str());
            }

            if (auto code = avcodec_receive_frame(m_cc, m_frame); code < 0) {
                if (code == AVERROR(EAGAIN)) {
                    return make_data_again();
                } else if (code == AVERROR_EOF) {
                    return make_data_eoi();
                } else {
                    return make_data_fatal("Decoding error");
                }
            }
        }

        AsaData* data = make_data_normal(0.f, m_frame->width, m_frame->height);
        AsaVideoData* video_data = reinterpret_cast<AsaVideoData*>(data->data);

        auto sws_context = sws_getContext(
            m_frame->width, m_frame->height, static_cast<AVPixelFormat>(m_frame->format),
            m_frame->width, m_frame->height, AV_PIX_FMT_BGR24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        int rgb_stride[3] = {3 * m_frame->width, 0, 0};
        uint8_t* rgb_data[3] = {video_data->frame, nullptr, nullptr};
        sws_scale(
            sws_context, m_frame->data, m_frame->linesize, 0, m_frame->height, 
            rgb_data, rgb_stride);
        sws_freeContext(sws_context);

        return data;
    }

    AsaData* upload() {
        return make_data_fatal("Uploading not supported");
    }

    int read_packet(uint8_t* buffer, size_t buf_size) {
        const size_t to_read = std::min(m_data.size(), buf_size);

        std::copy(m_data.begin(), m_data.begin() + to_read, buffer);
        m_data.erase(m_data.begin(), m_data.begin() + to_read);
        return to_read;
    }

    static void free(AsaData* data) {
        delete [] data->data;
        delete [] data->error;
        delete data;
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
};

extern "C" {

struct Handler* asa_handler_open() {
    try {
        return new Handler;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return nullptr;
    }
}

void asa_handler_close(Handler* self) {
    delete self;
}

int asa_handler_push(Handler* self, const AsaData* data) {
    self->push(data);
    return 0;
}

AsaValueType asa_handler_get_type(Handler*) {
    return ASA_VIDEO;
}

AsaData* asa_handler_download(Handler* self) {
    return self->download();
}

AsaData* asa_handler_upload(Handler* self) {
    return self->upload();
}

void asa_handler_free(Handler*, AsaData* data) {
    Handler::free(data);
}

}
