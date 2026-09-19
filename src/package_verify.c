#include "package_verify.h"
#include <stdio.h>
/* Lightweight pre-install container sanity check.
   VPK is ZIP-based. Full archive-entry validation is a later milestone. */
PackageCheckResult mrwrack_check_vpk_container(const char *path,uint64_t *size_out){
 FILE*f=fopen(path,"rb"); if(!f)return PKG_CHECK_NOT_FOUND;
 if(fseek(f,0,SEEK_END)!=0){fclose(f);return PKG_CHECK_EMPTY;}
 long n=ftell(f); if(n<=0){fclose(f);return PKG_CHECK_EMPTY;}
 if(size_out)*size_out=(uint64_t)n;
 if(n<22){fclose(f);return PKG_CHECK_TOO_SMALL;}
 rewind(f); unsigned char h[4]={0};
 if(fread(h,1,4,f)!=4){fclose(f);return PKG_CHECK_TOO_SMALL;}
 fclose(f);
 /* ZIP local header, empty archive EOCD, or spanning signature */
 if(h[0]!='P'||h[1]!='K'||!((h[2]==3&&h[3]==4)||(h[2]==5&&h[3]==6)||(h[2]==7&&h[3]==8)))
   return PKG_CHECK_BAD_ZIP;
 return PKG_CHECK_OK;
}
