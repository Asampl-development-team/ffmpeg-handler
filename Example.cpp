#include "interpreter.h"

#include <fstream>
#include <iostream>
#include <array>
#include <cassert>

std::vector<uint8_t> readData(const char* filename) {
    std::ifstream in{filename, std::ifstream::binary};
    return {std::istreambuf_iterator<char>{in}, {}};
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    Handler test_handler{"./ffmpeg-handler"};

    auto handler = test_handler.open_download();
    assert(handler);

    assert(test_handler.get_type(handler) == ASA_VIDEO);

    auto image_data = readData(argv[1]);
    AsaData data;
    data.size = image_data.size();
    data.data = image_data.data();
    test_handler.push(handler, &data);

    bool running = true;
    while (running) {
        AsaData* frame;

        frame = test_handler.download(handler);
        assert(frame);

        switch (frame->status) {
            case ASA_STATUS_FATAL:
                std::cerr << frame->error << std::endl;
                running = false;
                break;
            case ASA_STATUS_AGAIN:
                break;
            case ASA_STATUS_EOI:
                running = false;
                break;
            default:
                break;
        }

        if (frame->status == ASA_STATUS_NORMAL) {
            auto video_frame = reinterpret_cast<AsaVideoData*>(frame->data);

            cv::Mat image{video_frame->height, video_frame->width, CV_8UC3, video_frame->frame};

            cv::imshow("test", image);
            cv::waitKey(0);
        }

        test_handler.free(handler, frame);
    }
    cv::destroyAllWindows();

    test_handler.close(handler);
}
