#include "plugin_download.h"
#include "plugin_verify.h"
#include "package_verify.h"
#include "vita_http_transport.h"
#include <stdio.h>
#include <string.h>
static int get_size(const char*p,uint64_t*out){
 FILE*f=fopen(p,"rb");if(!f)return 0;if(fseek(f,0,SEEK_END)!=0){fclose(f);return 0;}
 long n=ftell(f);fclose(f);if(n<0)return 0;*out=(uint64_t)n;return 1;
}
static DownloadResult fail(DownloadStatus*s,const char*tmp,int code,const char*msg){
 remove(tmp);if(s){s->stage=DS_FAILED;s->error=code;snprintf(s->message,sizeof(s->message),"%s",msg);}return (DownloadResult)code;
}
DownloadResult plugin_download_verified(const char *url,const char *tmp,const char *sha,uint64_t expected,int require_vpk,DownloadStatus*s){
 if(s)download_status_reset(s);
 if(!url||strncmp(url,"https://",8)!=0)return fail(s,tmp,DL_BAD_URL,"HTTPS REQUIRED");
 remove(tmp);
 if(!vita_https_download_to_file(url,tmp,s))return fail(s,tmp,DL_NETWORK_UNAVAILABLE,"DOWNLOAD FAILED");
 uint64_t actual=0;if(s){s->stage=DS_VERIFYING_SIZE;snprintf(s->message,sizeof(s->message),"VERIFYING SIZE...");}
 if(!get_size(tmp,&actual))return fail(s,tmp,DL_IO,"FILE READ FAILED");
 if(expected&&actual!=expected)return fail(s,tmp,DL_SIZE_MISMATCH,"SIZE MISMATCH");
 if(s){s->stage=DS_VERIFYING_SHA256;snprintf(s->message,sizeof(s->message),"VERIFYING SHA-256...");}
 if(!mrwrack_verify_sha256(tmp,sha))return fail(s,tmp,DL_VERIFY_FAILED,"SHA-256 MISMATCH");
 if(require_vpk){
  if(s){s->stage=DS_VERIFYING_PACKAGE;snprintf(s->message,sizeof(s->message),"VERIFYING PACKAGE...");}
  if(mrwrack_check_vpk_container(tmp,0)!=PKG_CHECK_OK)return fail(s,tmp,DL_PACKAGE_INVALID,"PACKAGE INVALID");
 }
 if(s){s->stage=DS_READY_TO_INSTALL;s->percent=100;snprintf(s->message,sizeof(s->message),"VERIFIED - READY TO INSTALL");}
 return DL_OK;
}
