#include <utility>
#include <iostream>

#include "ffmpeg-download.hpp"

#if ASAMPL_FFI_VERSION_MAJOR != 0
#error "Handler requires interface version 0"
#endif

extern "C" {

struct Download* asa_handler_open_download() {
    try {
        return new Download;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return nullptr;
    }
}

void* asa_handler_open_upload() {
    return nullptr;
}

void asa_handler_close(Download* self) {
    delete self;
}

int asa_handler_push(Download* self, const AsaBytes* data) {
    self->push(data);
    return 0;
}

AsaHandlerResponse asa_handler_download(Download* self) {
    return self->download();
}

AsaHandlerResponse asa_handler_upload(void*) {
    AsaHandlerResponse response;
    asa_new_response_fatal("Uploading is not supported", &response);
    return response;
}

}
