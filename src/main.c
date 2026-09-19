#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <vita2d.h>
#include <stdio.h>
#include <string.h>
#include "tai_config.h"
#include "ui_font.h"

#define CFG "ux0:data/VitaAutoPlugin/settings.cfg"
#define CLEAN_MARK "ux0:data/VitaAutoPlugin/clean_exit.marker"
#define TEST_OC "ux0:data/VitaAutoPlugin/test_overclock.cfg"
#define WORK_OC "ux0:data/VitaAutoPlugin/working_overclock.cfg"
#define RECOVERY_LOG "ux0:data/VitaAutoPlugin/recovery.log"
#define TAI_UR0 "ur0:tai/config.txt"
#define TAI_UX0 "ux0:tai/config.txt"
#define TAI_TEST "ux0:data/VitaAutoPlugin/config.test.txt"
#define TAI_WORK "ux0:data/VitaAutoPlugin/config.working.txt"
#define BACKUP_DIR "ux0:data/VitaAutoPlugin/backups"
#define TAI_BACKUP "ux0:data/VitaAutoPlugin/backups/config.backup.txt"
typedef struct {
 int hud,fps,cpu,gpu,ram,battery,temp,fahrenheit,auto_backup;
 int trophy_hunter,trophy_unlocker,auto_platinum,backup_trophy;
} Settings;
static Settings s={1,1,1,1,1,1,1,0,1, 0,0,0,1};

enum Screen { HOME, TROPHY, OVERCLOCK, MONITOR, PLUGINS, RECOVERY, UPDATE, NEWS, SETTINGS, ABOUT };
static enum Screen screen=HOME;
static int selected=0, running=1;
static int oc_enabled=0, oc_cpu=0, oc_gpu=0; /* 0=default, 1=verified high profile */
static int recovery_notice=0;
static TaiConfig plugin_cfg;
static int plugin_cfg_loaded=0;
static int plugin_cursor=0;
static int plugin_view=0;
static int test_oc_active=0;
static vita2d_pgf *ui_font=NULL;
static int quick_menu=0, trophy_quick_menu=0;
static char operation_status[128]="Ready";
static char last_backup_name[96]="None";
static const char *APP_VERSION="v0.22";

enum IconId { ICO_HOME,ICO_PLUGIN,ICO_TROPHY,ICO_MONITOR,ICO_OVERCLOCK,ICO_RECOVERY,ICO_UPDATE,ICO_NEWS,ICO_SETTINGS,ICO_ABOUT,ICO_GITHUB,ICO_CROWN,ICO_SAVE,ICO_BACKUP,ICO_RESTORE,ICO_WARNING,ICO_CHECK,ICO_STATUS,ICO_COUNT };
static vita2d_texture *icons[ICO_COUNT]={0};
static const char *icon_files[ICO_COUNT]={"home","plugin","trophy","monitor","overclock","recovery","update","news","settings","about","github","crown","save","backup","restore","warning","check","status"};
static void load_icons(void){ char pth[128]; for(int i=0;i<ICO_COUNT;i++){snprintf(pth,sizeof(pth),"app0:/assets/icons/%s.png",icon_files[i]);icons[i]=vita2d_load_PNG_file(pth);} }
static void free_icons(void){for(int i=0;i<ICO_COUNT;i++){if(icons[i]){vita2d_free_texture(icons[i]);icons[i]=NULL;}}}
static void draw_icon(int id,float x,float y,float scale){if(id>=0&&id<ICO_COUNT&&icons[id])vita2d_draw_texture_scale(icons[id],x,y,scale,scale);}


static const char *home_items[]={
 "Plugin Manager","Trophy Unlocker","System Monitor","Overclock",
 "Recovery & Backup","Update Center","News","Settings","About"
};
#define HOME_N 9

static void mkdirs(void){
 sceIoMkdir("ux0:data/VitaAutoPlugin",0777);
 sceIoMkdir(BACKUP_DIR,0777);
}
static void save(void){
 mkdirs(); FILE*f=fopen(CFG,"w"); if(!f)return;
 fprintf(f,"%d %d %d %d %d %d %d %d %d %d %d %d %d\n",s.hud,s.fps,s.cpu,s.gpu,s.ram,s.battery,s.temp,s.fahrenheit,s.auto_backup,s.trophy_hunter,s.trophy_unlocker,s.auto_platinum,s.backup_trophy);
 fclose(f);
}

static int exists(const char *path){ SceIoStat st; return sceIoGetstat(path,&st)>=0; }

static int copy_file(const char *src,const char *dst){
 FILE*a=fopen(src,"rb"); if(!a)return 0;
 FILE*b=fopen(dst,"wb"); if(!b){fclose(a);return 0;}
 char buf[4096]; size_t n; int ok=1;
 while((n=fread(buf,1,sizeof(buf),a))>0) if(fwrite(buf,1,n,b)!=n){ok=0;break;}
 fclose(a); fclose(b); return ok;
}
static const char *tai_config_path(void){
 if(exists(TAI_UR0)) return TAI_UR0;
 if(exists(TAI_UX0)) return TAI_UX0;
 return NULL;
}

static void load_plugin_test_view(void){
 const char *src=exists(TAI_TEST)?TAI_TEST:tai_config_path();
 plugin_cfg_loaded = src ? tai_parse_file(src,&plugin_cfg) : 0;
 plugin_cursor=0;
}
static int plugin_count(void){
 if(!plugin_cfg_loaded) return 0;
 int n=0;
 for(int i=0;i<plugin_cfg.count;i++)
  if(plugin_cfg.lines[i].type==TAI_PLUGIN ||
    (plugin_cfg.lines[i].type==TAI_COMMENT && strstr(plugin_cfg.lines[i].text,"# MRWRACK_DISABLED "))) n++;
 return n;
}
static TaiLine *plugin_at(int index){
 if(!plugin_cfg_loaded) return NULL;
 int n=0;
 for(int i=0;i<plugin_cfg.count;i++){
  TaiLine*l=&plugin_cfg.lines[i];
  if(l->type==TAI_PLUGIN || (l->type==TAI_COMMENT && strstr(l->text,"# MRWRACK_DISABLED "))){
   if(n++==index)return l;
  }
 }
 return NULL;
}

static int create_test_config(void){
 const char *src=tai_config_path(); if(!src)return 0;
 mkdirs();
 if(!copy_file(src,TAI_TEST))return 0;
 return 1;
}
static void txt(float x,float y,float scale,const char*t);
static void txt_dim(float x,float y,float scale,const char*t);
static long file_size(const char *path){
 FILE*f=fopen(path,"rb"); if(!f)return -1; if(fseek(f,0,SEEK_END)!=0){fclose(f);return -1;} long n=ftell(f); fclose(f); return n;
}
static void progress_frame(const char *title_text,const char *stage,int percent){
 if(percent < 0) percent = 0;
 if(percent > 100) percent = 100;
 vita2d_start_drawing(); vita2d_clear_screen();
 vita2d_draw_rectangle(0,0,960,544,RGBA8(6,18,36,255));
 vita2d_draw_rectangle(120,150,720,244,RGBA8(7,25,45,250));
 vita2d_draw_rectangle(120,150,720,4,RGBA8(75,196,255,255));
 draw_icon(ICO_BACKUP,150,178,.95f);
 txt(205,210,1.08f,title_text); txt_dim(205,242,.70f,stage);
 vita2d_draw_rectangle(165,286,630,34,RGBA8(3,14,27,255));
 vita2d_draw_rectangle(169,290,(622.0f*percent)/100.0f,26,RGBA8(22,151,224,255));
 char b[32]; snprintf(b,sizeof(b),"%d%%",percent); txt(450,355,.95f,b);
 vita2d_end_drawing(); vita2d_swap_buffers();
}
static int copy_file_progress(const char *src,const char *dst,const char *label,int base,int span){
 FILE*a=fopen(src,"rb"); if(!a)return 0; FILE*b=fopen(dst,"wb"); if(!b){fclose(a);return 0;}
 if(fseek(a,0,SEEK_END)!=0){fclose(a);fclose(b);return 0;} long total=ftell(a); rewind(a);
 char buf[2048]; size_t n; long done=0; int ok=1; progress_frame(label,"Preparing...",base);
 while((n=fread(buf,1,sizeof(buf),a))>0){ if(fwrite(buf,1,n,b)!=n){ok=0;break;} done+=(long)n; int pc=base+(total>0?(int)((done*span)/total):span); progress_frame(label,"Writing and verifying data...",pc); }
 fclose(a); fclose(b); if(!ok)return 0;
 if(file_size(src) != file_size(dst)) return 0;
 progress_frame(label,"Verified",base+span);
 return 1;
}
static int save_working_config(void){
 const char *src=exists(TAI_TEST)?TAI_TEST:tai_config_path(); if(!src)return 0; mkdirs();
 if(!copy_file_progress(src,TAI_WORK,"Save Working Config",0,78))return 0;
 if(!copy_file_progress(src,TAI_BACKUP,"Save Working Config",78,22))return 0;
 progress_frame("Save Working Config","SAVE COMPLETE",100); sceKernelDelayThread(450000); return 1;
}
static int restore_working_config(void){
 const char *live=tai_config_path(); if(!live||!exists(TAI_WORK))return 0; mkdirs();
 if(!copy_file_progress(live,TAI_BACKUP,"Restore Last Working",0,35))return 0;
 if(!copy_file_progress(TAI_WORK,live,"Restore Last Working",35,65))return 0;
 progress_frame("Restore Last Working","RESTORE COMPLETE",100); sceKernelDelayThread(450000); return 1;
}
static int create_config_backup(void){
 const char *src=tai_config_path(); if(!src)return 0; mkdirs();
 if(!copy_file_progress(src,TAI_BACKUP,"Create Backup",0,50))return 0;
 char path[160]; for(int i=1;i<=9999;i++){ snprintf(path,sizeof(path),BACKUP_DIR "/config.backup.%04d.txt",i); if(!exists(path)){
   if(!copy_file_progress(src,path,"Create Backup",50,50))return 0;
   snprintf(last_backup_name,sizeof(last_backup_name),"config.backup.%04d.txt",i);
   progress_frame("Create Backup","BACKUP COMPLETE",100); sceKernelDelayThread(450000); return 1;
 }} return 0;
}

static void log_recovery(const char *msg){
 mkdirs(); FILE*f=fopen(RECOVERY_LOG,"a"); if(!f)return; fprintf(f,"%s\n",msg); fclose(f);
}
static void mark_running(void){
 mkdirs();
 /* Absence of CLEAN_MARK while running means the previous session did not
    finish through our normal shutdown path. */
 sceIoRemove(CLEAN_MARK);
}
static void mark_clean_exit(void){
 mkdirs(); FILE*f=fopen(CLEAN_MARK,"w"); if(f){fputs("clean\n",f);fclose(f);}
}
static void save_test_oc(void){
 mkdirs(); FILE*f=fopen(TEST_OC,"w"); if(!f)return;
 fprintf(f,"%d %d %d\n",oc_enabled,oc_cpu,oc_gpu); fclose(f); test_oc_active=1;
}
static void save_working_oc(void){
 mkdirs(); FILE*f=fopen(WORK_OC,"w"); if(!f)return;
 fprintf(f,"%d %d %d\n",oc_enabled,oc_cpu,oc_gpu); fclose(f);
 sceIoRemove(TEST_OC); test_oc_active=0;
}
static void restore_working_oc(void){
 FILE*f=fopen(WORK_OC,"r");
 if(f){fscanf(f,"%d %d %d",&oc_enabled,&oc_cpu,&oc_gpu);fclose(f);}
 else {oc_enabled=0;oc_cpu=0;oc_gpu=0;}
 sceIoRemove(TEST_OC); test_oc_active=0;
}
static void startup_recovery(void){
 /* clean marker exists only after a normal exit. First-ever launch is not
    treated as a crash unless a staged test profile also exists. */
 int clean=exists(CLEAN_MARK);
 int staged=exists(TEST_OC);
 if(!clean && staged){
   restore_working_oc();
   recovery_notice=1;
   log_recovery("UNEXPECTED EXIT: staged overclock discarded; Last Working profile restored.");
 }
 mark_running();
}

static void load(void){
 FILE*f=fopen(CFG,"r"); if(!f)return;
 /* Backward compatible: v0.19 files contain 9 values. New trophy values
    keep their defaults when the older file ends after auto_backup. */
 fscanf(f,"%d %d %d %d %d %d %d %d %d %d %d %d %d",
  &s.hud,&s.fps,&s.cpu,&s.gpu,&s.ram,&s.battery,&s.temp,&s.fahrenheit,&s.auto_backup,
  &s.trophy_hunter,&s.trophy_unlocker,&s.auto_platinum,&s.backup_trophy);
 fclose(f);
}
static void txt(float x,float y,float scale,const char*t){
 if(ui_font && t) vita2d_pgf_draw_text(ui_font,x,y,RGBA8(245,250,255,255),scale,t);
}
static void txt_dim(float x,float y,float scale,const char*t){
 if(ui_font && t) vita2d_pgf_draw_text(ui_font,x,y,RGBA8(185,205,225,255),scale,t);
}
static void panel(float x,float y,float w,float h){
 vita2d_draw_rectangle(x,y,w,h,RGBA8(7,25,45,244));
 vita2d_draw_rectangle(x,y,w,2,RGBA8(20,112,176,255));
 vita2d_draw_rectangle(x,y,2,h,RGBA8(13,72,119,255));
}
static void github_mark(float x,float y,float sc){ draw_icon(ICO_GITHUB,x,y,sc*.80f); }
static void crown_mark(float x,float y,float sc){ draw_icon(ICO_CROWN,x,y,sc*.92f); }
static void brand(float x,float y,float scale){
 github_mark(x,y-25*scale,scale);
 txt_dim(x+39*scale,y-11*scale,.52f*scale,"Made by");
 txt(x+39*scale,y+10*scale,.82f*scale,"MrWrack");
 crown_mark(x+132*scale,y-22*scale,.72f*scale);
}

static void badge(float x,float y,const char *label){
 vita2d_draw_rectangle(x,y,34,34,RGBA8(10,99,166,255));
 vita2d_draw_rectangle(x+2,y+2,30,30,RGBA8(8,39,70,255));
 txt(x+10,y+24,.72f,label);
}
static void title(const char*t){
 vita2d_draw_rectangle(0,0,960,66,RGBA8(4,18,34,255));
 vita2d_draw_rectangle(0,62,960,4,RGBA8(13,142,221,255));
 txt(22,34,1.16f,"Vita AutoPlugin"); txt(214,34,.82f,APP_VERSION);
 txt_dim(22,55,.61f,"Plugins  |  System  |  Tools  |  Recovery");
 brand(720,38,.92f);
 if(t && strcmp(t,"Vita AutoPlugin")) txt(360,41,.82f,t);
}
static void row(int i,float y,const char*t){
 if(i==selected){
  vita2d_draw_rectangle(18,y-26,430,36,RGBA8(10,105,174,255));
  vita2d_draw_rectangle(18,y-26,5,36,RGBA8(94,220,255,255));
 }
 txt(36,y,.88f,t);
}
static void footer(void){
 vita2d_draw_rectangle(0,506,960,38,RGBA8(3,15,29,255));
 vita2d_draw_rectangle(0,506,960,2,RGBA8(12,74,120,255));
 txt(20,531,.66f,"X Select / Toggle     O Back     D-PAD Navigate");
 txt_dim(635,531,.61f,"R + UP Quick Menu   L + SELECT Trophy");
}
static void info_heading(const char*t){txt(505,112,.94f,t);}
static void info_line(float y,const char*t){txt_dim(505,y,.70f,t);}
static void nav_item(int i,float y,const char *tag,const char *label){
 static const int ids[9]={ICO_PLUGIN,ICO_TROPHY,ICO_MONITOR,ICO_OVERCLOCK,ICO_RECOVERY,ICO_UPDATE,ICO_NEWS,ICO_SETTINGS,ICO_ABOUT};
 if(i==selected){vita2d_draw_rectangle(12,y-26,278,38,RGBA8(10,105,174,255));vita2d_draw_rectangle(12,y-26,5,38,RGBA8(100,225,255,255));}
 if(i>=0&&i<9&&icons[ids[i]]) draw_icon(ids[i],25,y-27,.72f); else badge(24,y-29,tag); txt(70,y,.79f,label);
}
static void small_status(float x,float y,const char *name,const char *value){
 txt_dim(x,y,.64f,name); txt(x+190,y,.64f,value);
}
static void action_button(float x,float y,const char *tag,const char *label){
 vita2d_draw_rectangle(x,y,278,34,RGBA8(9,35,61,255));
 vita2d_draw_rectangle(x,y,3,34,RGBA8(15,117,184,255));
 int id=-1; if(!strcmp(tag,"P"))id=ICO_PLUGIN; else if(!strcmp(tag,"T"))id=ICO_TROPHY; else if(!strcmp(tag,"R"))id=ICO_RECOVERY; else if(!strcmp(tag,"S"))id=ICO_SAVE; else if(!strcmp(tag,"B"))id=ICO_BACKUP;
 if(id>=0&&icons[id])draw_icon(id,x+9,y+3,.68f); else txt(x+13,y+23,.65f,tag); txt(x+45,y+23,.66f,label);
}

static void draw_home(void){
 title("Vita AutoPlugin");
 panel(10,78,286,414); panel(310,78,638,414);
 const char *tags[]={"H","P","T","M","OC","R","U","N","S"};
 for(int i=0;i<HOME_N;i++) nav_item(i,112+i*40,tags[i],home_items[i]);

 /* Dashboard hero */
 vita2d_draw_rectangle(326,92,606,94,RGBA8(5,31,57,255));
 vita2d_draw_rectangle(326,92,606,3,RGBA8(22,151,224,255));
 txt(348,127,1.18f,"Vita AutoPlugin"); txt(536,127,.83f,APP_VERSION);
 txt_dim(348,153,.66f,"PS Vita plugin management, recovery and system tools");
 brand(760,142,.82f);

 panel(326,200,286,132); panel(626,200,306,132);
 txt(344,226,.78f,"System Status");
 small_status(344,252,"Vita AutoPlugin","Running");
 small_status(344,276,"TrophyHax 2.0","Enabled");
 small_status(344,300,"Config","Loaded");
 small_status(344,324,"Backup",exists(TAI_BACKUP)?"Available":"None");

 txt(644,226,.78f,"Quick Actions");
 action_button(644,238,"P","Plugin Manager");
 action_button(644,276,"T","Trophy Unlocker");
 action_button(644,314,"R","Recovery & Backup");

 panel(326,346,286,132); panel(626,346,306,132);
 txt(344,372,.78f,"Trophy Unlocker");
 small_status(344,400,"Trophy Hunter",s.trophy_hunter?"ON":"OFF");
 small_status(344,424,"Trophy Unlocker",s.trophy_unlocker?"ON":"OFF");
 txt_dim(344,452,.58f,(s.trophy_hunter&&s.trophy_unlocker)?"L + SELECT quick menu ready":"Enable both for Trophy Quick Menu");

 txt(644,372,.78f,"Recovery & Backup");
 action_button(644,384,"S","Save Working Config");
 action_button(644,422,"B","Create Backup Now");
 txt_dim(644,458,.58f,operation_status); char lb[128]; snprintf(lb,sizeof(lb),"Last Backup: %s",last_backup_name); txt_dim(644,478,.54f,lb);
}
static void draw_trophy(void){
 title("Trophy Unlocker"); panel(18,78,442,414); panel(482,78,460,414);
 char t0[48],t1[48],t4[48],t5[64];
 snprintf(t0,sizeof(t0),"Trophy Hunter [%s]",s.trophy_hunter?"ON":"OFF");
 snprintf(t1,sizeof(t1),"Trophy Unlocker [%s]",s.trophy_unlocker?"ON":"OFF");
 snprintf(t4,sizeof(t4),"Auto Platinum [%s]",s.auto_platinum?"ON":"OFF");
 snprintf(t5,sizeof(t5),"Backup Before Unlock [%s]",s.backup_trophy?"ON":"OFF");
 const char* a[]={t0,t1,"Unlock Selected","Unlock All",t4,t5,"Save Trophy State","Restore Trophy Backup","History / Logs"};
 for(int i=0;i<9;i++)row(i,112+i*40,a[i]);
 info_heading("Trophy Safety");
 info_line(151,"L + SELECT requires Hunter + Unlocker ON.");
 info_line(184,"Verify Game > Backup > Unlock > Verify Locally");
 info_line(217,"Already unlocked trophies remain unchanged.");
 info_line(250,"Platinum is processed last.");
 info_line(303,"Unlock writes remain blocked until validation");
 info_line(330,"and backup handling are fully implemented.");
 info_line(375,(s.trophy_hunter && s.trophy_unlocker)?"L + SELECT Quick Menu: READY":"L + SELECT Quick Menu: DISABLED");
}
static void draw_overclock(void){
 title("Overclock"); panel(18,78,442,414); panel(482,78,460,414);
 char a0[48],a1[48],a2[48];
 snprintf(a0,sizeof(a0),"Overclock [%s]",oc_enabled?"ON":"OFF");
 snprintf(a1,sizeof(a1),"CPU Clock [%s]",oc_cpu?"444 MHz":"Default");
 snprintf(a2,sizeof(a2),"GPU Clock [%s]",oc_gpu?"222 MHz":"Default");
 const char*a[]={a0,a1,a2,"Test Clock Profile","Save Working Profile","Restore Last Working","Reset to Default","Apply on Boot [OFF]"};
 for(int i=0;i<8;i++)row(i,112+i*43,a[i]);
 info_heading("Test First");
 info_line(151,"New values remain a TEST profile.");
 info_line(181,"Apply on Boot stays OFF by default.");
 info_line(226,"WARNING");
 info_line(256,"Higher clocks can increase heat, battery use,");
 info_line(283,"instability, crashes and hardware stress.");
 if(recovery_notice) info_line(335,"RECOVERY: prior TEST profile was discarded.");
 info_line(390,"Hardware clock writes are not enabled yet.");
}
static void draw_monitor(void){
 title("System Monitor"); panel(18,78,442,414); panel(482,78,460,414);
 char b[64]; const char*n[]={"HUD","FPS","CPU Clock","GPU Clock","RAM Usage","Battery %","BAT TEMP"};
 int*v[]={&s.hud,&s.fps,&s.cpu,&s.gpu,&s.ram,&s.battery,&s.temp};
 for(int i=0;i<7;i++){snprintf(b,sizeof(b),"%s [%s]",n[i],*v[i]?"ON":"OFF");row(i,112+i*43,b);}
 snprintf(b,sizeof(b),"Temperature Unit [%s]",s.fahrenheit?"F":"C"); row(7,413,b);
 info_heading("HUD Settings");
 info_line(151,"HUD OFF hides all monitor values immediately.");
 info_line(181,"Individual choices are preserved while HUD is OFF.");
 info_line(226,"Available values:");
 info_line(256,"FPS / CPU Clock / GPU Clock / RAM Usage");
 info_line(283,"Battery % / BAT TEMP");
 info_line(328,"No fake CPU, GPU or RAM temperatures.");
}
static void draw_plugins(void){
 title(plugin_view?"Installed Plugins":"Plugin Manager"); panel(18,78,924,414);
 if(plugin_view){
  int n=plugin_count();
  if(!plugin_cfg_loaded){txt(40,125,.90f,"No taiHEN configuration could be loaded.");return;}
  if(!n){txt(40,125,.90f,"No .skprx/.suprx entries found.");return;}
  int first=plugin_cursor>7?plugin_cursor-7:0, shown=0;
  for(int k=first;k<n && shown<8;k++,shown++){
   TaiLine*l=plugin_at(k); if(!l)continue;
   const char*p=l->text; int en=l->type==TAI_PLUGIN;
   if(!en){const char*q=strstr(p,"# MRWRACK_DISABLED "); if(q)p=q+19;}
   char b[320]; snprintf(b,sizeof(b),"%.250s [%s]",p,en?"ENABLED":"DISABLED");
   if(k==plugin_cursor){vita2d_draw_rectangle(24,88+shown*48,910,38,RGBA8(12,116,183,255));vita2d_draw_rectangle(24,88+shown*48,5,38,RGBA8(110,220,255,255));}
   txt(38,114+shown*48,.76f,b);
   char sec[96]; snprintf(sec,sizeof(sec),"Section: %.70s",l->section[0]?l->section:"(unknown)");
   txt_dim(665,114+shown*48,.62f,sec);
  }
 } else {
  const char*a[]={"Installed Plugins","Plugin Browser","Enable / Disable","Test Config","Save Working Config","Plugin Order","Quarantine","Custom Repositories","Plugin Logs"};
  for(int i=0;i<9;i++)row(i,112+i*40,a[i]);
  info_heading("Plugin Safety");
  info_line(151,"NEW PLUGINS: DISABLED BY DEFAULT");
  info_line(184,"Changes are made in TEST config first.");
  info_line(217,"Last Working Config is never auto-overwritten.");
  info_line(262,tai_config_path()?"taiHEN config: DETECTED":"taiHEN config: NOT FOUND");
  info_line(292,exists(TAI_TEST)?"Test Config: READY":"Test Config: NOT CREATED");
  info_line(322,exists(TAI_WORK)?"Last Working: SAVED":"Last Working: NOT SAVED");
 }
}
static void draw_recovery(void){
 title("Recovery & Backup"); panel(18,78,442,414); panel(482,78,460,414);
 const char*a[]={"SAVE WORKING CONFIG","CREATE BACKUP NOW","Restore Last Working","View Backups","Safe Mode","Error Logs","Auto Backup [ON]","Auto Save [OFF - LOCKED]"};
 for(int i=0;i<8;i++)row(i,112+i*43,a[i]);
 info_heading("Recovery Protection");
 info_line(151,"Auto Save is permanently locked OFF.");
 info_line(181,"Create a backup before changing taiHEN config.");
 info_line(226,"A failed TEST should not replace Last Working.");
 info_line(271,exists(TAI_BACKUP)?"Config Backup: AVAILABLE":"Config Backup: NONE");
 info_line(315,operation_status); char lb[128]; snprintf(lb,sizeof(lb),"Last Backup: %s",last_backup_name); info_line(345,lb);
}
static void draw_update(void){
 title("Update Center"); panel(18,78,442,414); panel(482,78,460,414);
 const char*a[]={"Check for Updates","App Update","UI Update","Plugin Updates","Plugin Catalog","News","Update History"};
 for(int i=0;i<7;i++)row(i,112+i*46,a[i]);
 info_heading("Verified Update Flow");
 info_line(151,"Download > Size > SHA-256 > Package Verify");
 info_line(181,"Backup > Apply / Install > Verify");
 info_line(226,"No forced app updates.");
 info_line(256,"Plugins never auto-enable.");
 info_line(286,"Verification failure stops installation.");
}
static void draw_news(void){
 title("News"); panel(18,78,442,414); panel(482,78,460,414);
 const char*a[]={"NEW - App Updates","NEW - Plugin Releases","Plugin Updates","Fixes","Important Notices","Changelog","Mark All Read"};
 for(int i=0;i<7;i++)row(i,112+i*46,a[i]);
 info_heading("News & Changelog");
 info_line(151,"ADDED / CHANGED / FIXED / REMOVED");
 info_line(196,"Catalog and news may refresh automatically.");
 info_line(226,"Installation and activation remain manual.");
}
static void draw_settings(void){
 title("Settings"); panel(18,78,442,414); panel(482,78,460,414);
 const char*a[]={"Storage & Backup Paths","SAVE WORKING CONFIG","CREATE BACKUP NOW","HUD Settings","Plugin Settings","Update Settings","Recovery Settings","UI Settings","Controls","Temperature Unit"};
 for(int i=0;i<10;i++)row(i,105+i*37,a[i]);
 if(selected==0){
  info_heading("Storage & Backup Paths");
  info_line(145,"Live: ur0:tai/config.txt");
  info_line(171,"Fallback: ux0:tai/config.txt");
  info_line(197,"Test: ux0:data/VitaAutoPlugin/config.test.txt");
  info_line(223,"Working: ux0:data/VitaAutoPlugin/config.working.txt");
  info_line(249,"Backups: ux0:data/VitaAutoPlugin/backups/");
  info_line(275,"Recovery: ux0:data/VitaAutoPlugin/recovery.log");
  info_line(301,"Settings: ux0:data/VitaAutoPlugin/settings.cfg");
  txt(505,350,.78f,"PC Manual Editing");
  info_line(379,"Backup > Test Config > test on Vita > Save Working");
  info_line(424,"Auto Save: OFF (LOCKED)");
 } else {
  info_heading("Safe Settings");
  info_line(151,"Backup and working-config actions stay visible.");
  info_line(181,"Auto Save remains permanently OFF.");
 }
}
static void draw_about(void){
 title("About"); panel(18,78,924,414);
 txt(55,145,1.55f,"Vita AutoPlugin");
 brand(55,205,1.20f);
 txt(55,242,.82f,"PS Vita homebrew plugin management and system tools.");
 txt(55,286,.82f,"Version 0.22 - Progress, Icons & Quick Menu Prep");
 txt_dim(55,340,.74f,"Independent homebrew project.");
 txt_dim(55,374,.74f,"New plugins are disabled by default. Auto Save is locked OFF.");
}
static int count(void){
 switch(screen){case HOME:return 9;case TROPHY:return 9;case OVERCLOCK:return 8;case MONITOR:return 8;case PLUGINS:return plugin_view?(plugin_count()?plugin_count():1):9;case RECOVERY:return 8;case UPDATE:return 7;case NEWS:return 7;case SETTINGS:return 10;default:return 1;}
}
static void enter_home(void){
 enum Screen map[]={PLUGINS,TROPHY,MONITOR,OVERCLOCK,RECOVERY,UPDATE,NEWS,SETTINGS,ABOUT};
 screen=map[selected]; selected=0;
}
static void action(void){
 if(screen==HOME){enter_home();return;}
 if(screen==MONITOR){
  int*v[]={&s.hud,&s.fps,&s.cpu,&s.gpu,&s.ram,&s.battery,&s.temp};
  if(selected<7){*v[selected]=!*v[selected];save();}
  else {s.fahrenheit=!s.fahrenheit;save();}
 }
 if(screen==SETTINGS){
  if(selected==1){
    if(save_working_config()){strcpy(operation_status,"WORKING CONFIG SAVED");log_recovery("Working config saved manually from Settings.");}else strcpy(operation_status,"SAVE FAILED - taiHEN config not found");
  } else if(selected==2){
    if(create_config_backup()){strcpy(operation_status,"BACKUP CREATED");log_recovery("Manual backup created from Settings.");}else strcpy(operation_status,"BACKUP FAILED - taiHEN config not found");
  }
 }
 if(screen==PLUGINS){
  if(plugin_view){
   TaiLine*l=plugin_at(plugin_cursor);
   if(l){
    const char *raw=l->text; int currently=l->type==TAI_PLUGIN;
    const char *path=raw;
    if(!currently){const char*q=strstr(raw,"# MRWRACK_DISABLED "); if(q)path=q+19;}
    if(!exists(TAI_TEST)) create_test_config();
    /* Reload TEST snapshot before editing, then write only TEST snapshot. */
    if(tai_parse_file(TAI_TEST,&plugin_cfg)){
      if(tai_set_plugin_enabled(&plugin_cfg,path,!currently)){
        tai_write_file(TAI_TEST,&plugin_cfg);
        load_plugin_test_view();
        log_recovery("Plugin state changed in TEST config only.");
      }
    }
   }
  } else {
   if(selected==0){load_plugin_test_view();plugin_view=1;selected=0;}
   else if(selected==3){if(create_test_config()){load_plugin_test_view();log_recovery("Test Config created from current taiHEN config.");}}
   else if(selected==4){if(save_working_config())log_recovery("Last Working Config saved manually.");}
  }
 }
 if(screen==RECOVERY){
  if(selected==0){ if(save_working_config()){strcpy(operation_status,"WORKING CONFIG SAVED");log_recovery("Last Working Config saved manually from Recovery.");}else strcpy(operation_status,"SAVE FAILED - taiHEN config not found"); }
  else if(selected==1){ if(create_config_backup()){strcpy(operation_status,"BACKUP CREATED");log_recovery("Manual taiHEN config backup created.");}else strcpy(operation_status,"BACKUP FAILED - taiHEN config not found"); }
  else if(selected==2){ if(restore_working_config()){strcpy(operation_status,"LAST WORKING RESTORED");log_recovery("Last Working Config restored manually.");}else strcpy(operation_status,"RESTORE FAILED - no working config"); }
  /* Auto Save remains locked OFF. */
 }
 if(screen==TROPHY){
  if(selected==0){s.trophy_hunter=!s.trophy_hunter;save();}
  else if(selected==1){s.trophy_unlocker=!s.trophy_unlocker;save();}
  else if(selected==4){s.auto_platinum=!s.auto_platinum;save();}
  else if(selected==5){s.backup_trophy=!s.backup_trophy;save();}
  /* Unlock actions stay blocked until title/trophy DB validation + backup
     are implemented. This avoids unsafe placeholder writes. */
 }
 if(screen==OVERCLOCK){
  if(selected==0){oc_enabled=!oc_enabled; save_test_oc();}
  else if(selected==1){oc_cpu=!oc_cpu; save_test_oc();}
  else if(selected==2){oc_gpu=!oc_gpu; save_test_oc();}
  else if(selected==3){save_test_oc();}
  else if(selected==4){save_working_oc();}
  else if(selected==5){restore_working_oc();}
  else if(selected==6){oc_enabled=0;oc_cpu=0;oc_gpu=0;sceIoRemove(TEST_OC);test_oc_active=0;}
  /* This milestone persists TEST/WORKING profiles and recovery state but still
     does not write hardware clocks. Actual clock writes come only after the
     complete validation + rollback path is tested on-device. */
 }
}
static void draw_quick_overlay(void){
 vita2d_draw_rectangle(500,82,430,360,RGBA8(4,20,38,245));
 vita2d_draw_rectangle(500,82,430,4,RGBA8(110,220,255,255));
 txt(525,122,1.05f,trophy_quick_menu?"Trophy Quick Menu":"Vita AutoPlugin Quick Menu");
 if(trophy_quick_menu){
  txt(525,165,.82f,s.trophy_hunter?"Trophy Hunter [ON]":"Trophy Hunter [OFF]");
  txt(525,200,.82f,s.trophy_unlocker?"Trophy Unlocker [ON]":"Trophy Unlocker [OFF]");
  txt_dim(525,244,.72f,"Unlock actions require game/DB verification.");
  txt_dim(525,275,.72f,"Backup is required before trophy writes.");
  txt(525,330,.78f,"O  Close Quick Menu");
 }else{
  txt(525,165,.82f,"System Monitor / HUD");
  txt(525,200,.82f,"Overclock Status");
  txt(525,235,.82f,"Plugin Status");
  txt(525,270,.82f,"Recovery & Backup");
  txt(525,330,.78f,"O  Close Quick Menu");
 }
}

int main(void){
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG); load(); startup_recovery();
 vita2d_init(); vita2d_set_clear_color(RGBA8(6,18,36,255));
 ui_font=ui_font_load(); load_icons();
 if(!ui_font){ vita2d_fini(); sceKernelExitProcess(-1); return -1; }
 SceCtrlData p,o; memset(&o,0,sizeof(o));
 while(running){
  sceCtrlPeekBufferPositive(0,&p,1); unsigned q=p.buttons&~o.buttons;
  int n=count();
  if((p.buttons&SCE_CTRL_RTRIGGER) && (q&SCE_CTRL_UP)){quick_menu=!quick_menu;trophy_quick_menu=0;}
  if((p.buttons&SCE_CTRL_LTRIGGER) && (q&SCE_CTRL_SELECT) && s.trophy_hunter && s.trophy_unlocker){trophy_quick_menu=!trophy_quick_menu;quick_menu=0;}
  if((quick_menu||trophy_quick_menu) && (q&SCE_CTRL_CIRCLE)){quick_menu=0;trophy_quick_menu=0;q&=~SCE_CTRL_CIRCLE;}
  if(!(quick_menu||trophy_quick_menu) && (q&SCE_CTRL_UP)) selected=(selected+n-1)%n;
  if(!(quick_menu||trophy_quick_menu) && (q&SCE_CTRL_DOWN)) selected=(selected+1)%n;
  if(screen==PLUGINS && plugin_view) plugin_cursor=selected;
  if(!(quick_menu||trophy_quick_menu) && (q&SCE_CTRL_CROSS)) action();
  if(q&SCE_CTRL_CIRCLE){
   if(screen==PLUGINS && plugin_view){plugin_view=0;selected=0;}
   else if(screen==HOME)running=0;
   else{screen=HOME;selected=0;}
  }
  if(screen==MONITOR && selected==7 && (q&SCE_CTRL_LEFT)){s.fahrenheit=0;save();}
  if(screen==MONITOR && selected==7 && (q&SCE_CTRL_RIGHT)){s.fahrenheit=1;save();}
  vita2d_start_drawing(); vita2d_clear_screen();
  switch(screen){case HOME:draw_home();break;case TROPHY:draw_trophy();break;case OVERCLOCK:draw_overclock();break;case MONITOR:draw_monitor();break;case PLUGINS:draw_plugins();break;case RECOVERY:draw_recovery();break;case UPDATE:draw_update();break;case NEWS:draw_news();break;case SETTINGS:draw_settings();break;case ABOUT:draw_about();break;}
  footer(); if(quick_menu||trophy_quick_menu)draw_quick_overlay(); vita2d_end_drawing(); vita2d_swap_buffers();
  o=p; sceKernelDelayThread(16000);
 }
 mark_clean_exit(); free_icons(); ui_font_free(ui_font); ui_font=NULL; vita2d_fini(); sceKernelExitProcess(0); return 0;
}