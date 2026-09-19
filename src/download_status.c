#include "download_status.h"
#include <string.h>
#include <stdio.h>
void download_status_reset(DownloadStatus*s){memset(s,0,sizeof(*s));s->stage=DS_IDLE;snprintf(s->message,sizeof(s->message),"Ready");}
void download_status_progress(DownloadStatus*s,uint64_t r,uint64_t t){
 s->stage=DS_DOWNLOADING;s->received=r;s->total=t;
 s->percent=t?(int)((r*100)/t):0;if(s->percent>100)s->percent=100;
 snprintf(s->message,sizeof(s->message),"DOWNLOADING... %d%%",s->percent);
}
const char*download_stage_name(DownloadStage s){
 switch(s){case DS_CONNECTING:return"CONNECTING";case DS_DOWNLOADING:return"DOWNLOADING";
 case DS_VERIFYING_SIZE:return"VERIFYING SIZE";case DS_VERIFYING_SHA256:return"VERIFYING SHA-256";
 case DS_VERIFYING_PACKAGE:return"VERIFYING PACKAGE";case DS_READY_TO_INSTALL:return"READY TO INSTALL";
 case DS_FAILED:return"VERIFICATION FAILED";default:return"READY";}
}
