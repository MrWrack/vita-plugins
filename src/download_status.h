#pragma once
#include <stdint.h>
typedef enum {
 DS_IDLE=0, DS_CONNECTING, DS_DOWNLOADING, DS_VERIFYING_SIZE,
 DS_VERIFYING_SHA256, DS_VERIFYING_PACKAGE, DS_READY_TO_INSTALL, DS_FAILED
} DownloadStage;
typedef struct {
 DownloadStage stage;
 uint64_t received, total;
 int percent;
 int error;
 char message[96];
} DownloadStatus;
void download_status_reset(DownloadStatus *s);
void download_status_progress(DownloadStatus *s,uint64_t received,uint64_t total);
const char *download_stage_name(DownloadStage s);
