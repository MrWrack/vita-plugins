#include "tai_config.h"
#include <stdio.h>
#include <string.h>

static void trim_eol(char *s){ size_t n=strlen(s); while(n && (s[n-1]=='\n'||s[n-1]=='\r')) s[--n]=0; }
static const char *skipws(const char *s){ while(*s==' '||*s=='\t')s++; return s; }

int tai_parse_file(const char *path, TaiConfig *cfg){
 FILE*f=fopen(path,"r"); if(!f||!cfg)return 0;
 memset(cfg,0,sizeof(*cfg)); char cur[64]="";
 char buf[TAI_LINE_LEN];
 while(cfg->count<TAI_MAX_LINES && fgets(buf,sizeof(buf),f)){
  trim_eol(buf); TaiLine*l=&cfg->lines[cfg->count++]; snprintf(l->text,sizeof(l->text),"%s",buf);
  const char*p=skipws(buf); snprintf(l->section,sizeof(l->section),"%s",cur);
  if(!*p) l->type=TAI_BLANK;
  else if(*p=='#') l->type=TAI_COMMENT;
  else if(*p=='*'){
   l->type=TAI_SECTION; snprintf(cur,sizeof(cur),"%.63s",p); snprintf(l->section,sizeof(l->section),"%s",cur);
  } else {
   /* Plugin paths normally contain .skprx/.suprx. Unknown lines are retained verbatim. */
   if(strstr(p,".skprx")||strstr(p,".suprx")){l->type=TAI_PLUGIN;l->enabled=1;}
   else l->type=TAI_OTHER;
  }
 }
 fclose(f); return 1;
}

int tai_write_file(const char *path,const TaiConfig*cfg){
 FILE*f=fopen(path,"w"); if(!f||!cfg)return 0;
 for(int i=0;i<cfg->count;i++) fprintf(f,"%s\n",cfg->lines[i].text);
 fclose(f); return 1;
}

int tai_set_plugin_enabled(TaiConfig*cfg,const char*plugin_path,int enabled){
 if(!cfg||!plugin_path)return 0;
 for(int i=0;i<cfg->count;i++){
  TaiLine*l=&cfg->lines[i]; const char*p=skipws(l->text);
  if(l->type==TAI_PLUGIN && strcmp(p,plugin_path)==0){
   /* Safe model: enabling/disabling is represented only in the TEST snapshot.
      Disabled entries use a comment marker and can be restored exactly. */
   if(!enabled){
    char tmp[TAI_LINE_LEN]; snprintf(tmp,sizeof(tmp),"# MRWRACK_DISABLED %s",p);
    snprintf(l->text,sizeof(l->text),"%s",tmp); l->type=TAI_COMMENT; l->enabled=0;
   }
   return 1;
  }
  if(l->type==TAI_COMMENT && strncmp(p,"# MRWRACK_DISABLED ",19)==0){
   const char*q=p+19;
   if(strcmp(q,plugin_path)==0 && enabled){
    snprintf(l->text,sizeof(l->text),"%s",q); l->type=TAI_PLUGIN;l->enabled=1; return 1;
   }
  }
 }
 return 0;
}
