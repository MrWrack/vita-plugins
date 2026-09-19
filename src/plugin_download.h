#pragma once
#include <stdint.h>
#include "download_status.h"
typedef enum {
 DL_OK=0, DL_BAD_URL=-1, DL_IO=-2, DL_NETWORK_UNAVAILABLE=-3,
 DL_VERIFY_FAILED=-4, DL_SIZE_MISMATCH=-5, DL_PACKAGE_INVALID=-6
} DownloadResult;
DownloadResult plugin_download_verified(const char *url,const char *tmp_path,
 const char *sha256,uint64_t expected_size,int require_vpk,DownloadStatus *status);
