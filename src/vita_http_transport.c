#include "vita_http_transport.h"
#include <stdio.h>
#include <string.h>

/* Real VitaSDK HTTP implementation is compiled only when
   MRWRACK_ENABLE_VITA_HTTP is enabled after SDK/device validation.
   Default build remains fail-closed rather than pretending success. */
#ifdef MRWRACK_ENABLE_VITA_HTTP
#include <psp2/net/net.h>
#include <psp2/net/http.h>
#include <psp2/net/netctl.h>

int vita_https_download_to_file(const char *url,const char *path,DownloadStatus*s){
 if(!url||strncmp(url,"https://",8)!=0)return 0;
 s->stage=DS_CONNECTING;snprintf(s->message,sizeof(s->message),"CONNECTING...");
 /* Transport integration point. Vita HTTP init/request lifecycle must be
    validated against the target VitaSDK before enabling this build flag. */
 (void)path;
 s->stage=DS_FAILED;s->error=-100;
 snprintf(s->message,sizeof(s->message),"HTTP transport requires VitaSDK/device validation");
 return 0;
}
#else
int vita_https_download_to_file(const char *url,const char *path,DownloadStatus*s){
 (void)url;(void)path;
 if(s){s->stage=DS_FAILED;s->error=-100;snprintf(s->message,sizeof(s->message),"HTTP transport not enabled");}
 return 0;
}
#endif
