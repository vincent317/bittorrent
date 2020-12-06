
#include "upload_download_manager.h"

void create_download_manager(UploadDownloadManagerArgs* args) {
    UploadDownloadManagerArgs* args = (UploadDownloadManagerArgs*) vargp;
    printf("Created download manager!\n");
};

void create_upload_manager(UploadDownloadManagerArgs* args) {
    printf("Created upload manager!\n");
};

void* begin_upload_download(void* vargp) {
    UploadDownloadManagerArgs* args = (UploadDownloadManagerArgs*) vargp;

    if (args->is_upload) {
        create_upload_download_manger(args);
    } else {
        create_download_manager(args);
    }

    return NULL;
};
