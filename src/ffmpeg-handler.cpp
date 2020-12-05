#include <utility>
#include <iostream>

#include <handler_interface.h>

#include "ffmpeg-download.hpp"

#if HANDLER_INTERFACE_VERSION_MAJOR != 1
#error "Handler requires interface version 1"
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

int asa_handler_push(Download* self, const AsaData* data) {
    self->push(data);
    return 0;
}

AsaValueType asa_handler_get_type(Download*) {
    return ASA_VIDEO;
}

AsaData* asa_handler_download(Download* self) {
    return self->download();
}

AsaData* asa_handler_upload(void* self) {
    return Common::make_data_fatal("Uploading is not supported");
}

void asa_handler_free(void*, AsaData* data) {
    Common::free(data);
}

}
