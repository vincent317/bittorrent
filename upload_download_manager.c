
#include "upload_download_manager.h"

void create_download_manager(UploadDownloadManagerArgs* args) {
    printf("Created download manager!\n");
    write(args->write_fd, "f", sizeof(char));
};

void create_upload_manager(UploadDownloadManagerArgs* args) {
    printf("Created upload manager!\n");
    write(args->write_fd, "f", sizeof(char));
};

void* begin_upload_download(void* vargp) {
    UploadDownloadManagerArgs* args = (UploadDownloadManagerArgs*) vargp;

    if (args->is_upload) {
        create_upload_manager(args);
    } else {
        create_download_manager(args);
    }

    free(vargp);
    return NULL;
};
