#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/io/fcntl.h>
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
typedef struct { int hud,fps,cpu,gpu,ram,battery,temp,fahrenheit,auto_backup; } Settings;
static Settings s={1,1,1,1,1,1,1,0,1};

enum Screen { HOME, TROPHY, OVERCLOCK, MONITOR, PLUGINS, RECOVERY, UPDATE, NEWS, SETTINGS, ABOUT };
static enum Screen screen=HOME;
static int selected=0, running=1;
static int trophy_hunter=0,trophy_unlocker=0,auto_platinum=0,backup_trophy=1;
static int oc_enabled=0, oc_cpu=0, oc_gpu=0; /* 0=default, 1=verified high profile */
static int recovery_notice=0;
static TaiConfig plugin_cfg;
static int plugin_cfg_loaded=0;
static int plugin_cursor=0;
static int plugin_view=0;
static int test_oc_active=0;
static vita2d_pgf *ui_font=NULL;

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
 fprintf(f,"%d %d %d %d %d %d %d %d %d\n",s.hud,s.fps,s.cpu,s.gpu,s.ram,s.battery,s.temp,s.fahrenheit,s.auto_backup);
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
 if(!plugin_cfg_loaded)return 0; int n=0;
 for(int i=0;i<plugin_cfg.count;i++)
  if(plugin_cfg.lines[i].type==TAI_PLUGIN ||
    (plugin_cfg.lines[i].type==TAI_COMMENT && strstr(plugin_cfg.lines[i].text,"# MRWRACK_DISABLED "))) n++;
 return n;
}
static TaiLine *plugin_at(int index){
 if(!plugin_cfg_loaded)return NULL; int n=0;
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
static int save_working_config(void){
 const char *src=exists(TAI_TEST)?TAI_TEST:tai_config_path();
 if(!src)return 0; mkdirs();
 if(!copy_file(src,TAI_WORK))return 0;
 /* Keep a separate snapshot too; this never happens automatically. */
 copy_file(src,TAI_BACKUP);
 return 1;
}
static int restore_working_config(void){
 const char *live=tai_config_path();
 if(!live || !exists(TAI_WORK))return 0;
 /* Backup current live config before manual restore. */
 copy_file(live,TAI_BACKUP);
 return copy_file(TAI_WORK,live);
}
static int create_config_backup(void){
 const char *src=tai_config_path(); if(!src)return 0;
 mkdirs(); return copy_file(src,TAI_BACKUP);
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
 fscanf(f,"%d %d %d %d %d %d %d %d %d",&s.hud,&s.fps,&s.cpu,&s.gpu,&s.ram,&s.battery,&s.temp,&s.fahrenheit,&s.auto_backup);
 fclose(f);
}
static void txt(float x,float y,float scale,const char*t){
 if(ui_font && t) vita2d_pgf_draw_text(ui_font,x,y,RGBA8(240,250,255,255),scale,t);
}
static void title(const char*t){
 vita2d_draw_rectangle(0,0,960,62,RGBA8(9,35,65,255));
 txt(25,41,1.22f,t); txt(725,40,.78f,"Created by MrWrack");
}
static void row(int i,float y,const char*t){
 if(i==selected) vita2d_draw_rectangle(22,y-18,430,25,RGBA8(15,87,130,255));
 txt(36,y,.82f,t);
}
static void footer(void){txt(24,525,.66f,"UP/DOWN Navigate   X Select/Toggle   O Back   R+UP Overlay");}

static void draw_home(void){
 title("Vita AutoPlugin");
 for(int i=0;i<HOME_N;i++) row(i,100+i*39,home_items[i]);
 txt(510,112,1.0f,"MrWrack Vita Control Center");
 txt(510,150,.75f,"Plugins, trophies, monitoring, overclock,");
 txt(510,175,.75f,"recovery and updates in one readable UI.");
}
static void draw_trophy(void){
 title("Trophy Unlocker");
 char t0[48],t1[48],t4[48],t5[64];
 snprintf(t0,sizeof(t0),"Trophy Hunter [%s]",trophy_hunter?"ON":"OFF");
 snprintf(t1,sizeof(t1),"Trophy Unlocker [%s]",trophy_unlocker?"ON":"OFF");
 snprintf(t4,sizeof(t4),"Auto Platinum [%s]",auto_platinum?"ON":"OFF");
 snprintf(t5,sizeof(t5),"Backup Before Unlock [%s]",backup_trophy?"ON":"OFF");
 const char* a[]={t0,t1,"Unlock Selected","Unlock All",t4,t5,"Save Trophy State","Restore Trophy Backup","History / Logs"};
 for(int i=0;i<9;i++)row(i,100+i*38,a[i]);
 txt(510,110,.76f,"L + SELECT Quick Menu requires Hunter + Unlocker ON.");
 txt(510,145,.76f,"Flow: Verify Game > Backup > Unlock > Verify Locally.");
 txt(510,180,.76f,"No PSN verification is claimed from local state.");
}
static void draw_overclock(void){
 title("Overclock");
 char a0[48],a1[48],a2[48];
 snprintf(a0,sizeof(a0),"Overclock [%s]",oc_enabled?"ON":"OFF");
 snprintf(a1,sizeof(a1),"CPU Clock [%s]",oc_cpu?"444 MHz":"Default");
 snprintf(a2,sizeof(a2),"GPU Clock [%s]",oc_gpu?"222 MHz":"Default");
 const char*a[]={a0,a1,a2,"Test Clock Profile","Save as Working Profile","Restore Last Working","Reset to Default","Apply on Boot [OFF]"};
 for(int i=0;i<8;i++)row(i,100+i*40,a[i]);
 txt(510,110,.76f,"Only verified Vita clock steps will be selectable.");
 txt(510,145,.76f,"New values remain a TEST profile until manually saved.");
 txt(510,180,.76f,"Apply on Boot defaults OFF.");
 if(recovery_notice) txt(510,205,.72f,"RECOVERY: prior TEST profile discarded; Working restored.");
 txt(510,230,.72f,"WARNING: higher clocks can increase heat, battery use,");
 txt(510,255,.72f,"instability and hardware stress.");
}
static void draw_monitor(void){
 title("System Monitor");
 char b[64]; const char*n[]={"HUD","FPS","CPU Clock","GPU Clock","RAM Usage","Battery %","BAT TEMP"};
 int*v[]={&s.hud,&s.fps,&s.cpu,&s.gpu,&s.ram,&s.battery,&s.temp};
 for(int i=0;i<7;i++){snprintf(b,sizeof(b),"%s [%s]",n[i],*v[i]?"ON":"OFF");row(i,100+i*39,b);}
 snprintf(b,sizeof(b),"Temperature Unit [%s]",s.fahrenheit?"F":"C"); row(7,373,b);
 txt(510,110,.76f,"HUD OFF hides all in-game monitor values.");
 txt(510,145,.76f,"Individual selections are preserved.");
 txt(510,180,.76f,"Temperature uses BAT TEMP; no fake RAM/CPU/GPU temp.");
}
static void draw_plugins(void){
 title(plugin_view?"Installed Plugins":"Plugin Manager");
 if(plugin_view){
  int n=plugin_count();
  if(!plugin_cfg_loaded){txt(40,115,.82f,"No taiHEN configuration could be loaded.");return;}
  if(!n){txt(40,115,.82f,"No .skprx/.suprx entries found.");return;}
  int first=plugin_cursor>8?plugin_cursor-8:0, shown=0;
  for(int k=first;k<n && shown<9;k++,shown++){
   TaiLine*l=plugin_at(k); if(!l)continue;
   const char*p=l->text; int en=l->type==TAI_PLUGIN;
   if(!en){const char*q=strstr(p,"# MRWRACK_DISABLED "); if(q)p=q+19;}
   char b[210]; snprintf(b,sizeof(b),"%s [%s]",p,en?"ENABLED":"DISABLED");
   if(k==plugin_cursor) vita2d_draw_rectangle(22,82+shown*42,900,31,RGBA8(15,87,130,255));
   txt(35,104+shown*42,.69f,b);
   char sec[96]; snprintf(sec,sizeof(sec),"Section: %s",l->section[0]?l->section:"(unknown)");
   txt(610,104+shown*42,.58f,sec);
  }
  txt(24,490,.64f,"X Toggle in TEST config   O Back");
 } else {
  const char*a[]={"Installed Plugins","Plugin Browser","Enable / Disable","Test Config","Save as Working Config","Plugin Order","Quarantine","Custom Repositories","Plugin Logs"};
  for(int i=0;i<9;i++)row(i,100+i*38,a[i]);
  txt(510,110,.76f,"NEW PLUGINS: DISABLED BY DEFAULT");
  txt(510,145,.76f,"Test Config never overwrites Last Working Config.");
  txt(510,180,.76f,tai_config_path()?"taiHEN config detected.":"taiHEN config not found.");
  txt(510,215,.76f,exists(TAI_TEST)?"Test Config: READY":"Test Config: not created");
  txt(510,250,.76f,exists(TAI_WORK)?"Last Working Config: SAVED":"Last Working Config: not saved");
 }
}
static void draw_recovery(void){
 title("Recovery & Backup");
 const char*a[]={"Save as Working Config","Create Backup","Restore Last Working","View Backups","Safe Mode","Error Logs","Auto Backup [ON]","Auto Save [OFF - LOCKED]"};
 for(int i=0;i<8;i++)row(i,100+i*40,a[i]);
 txt(510,110,.76f,"Auto Save is permanently locked OFF.");
 txt(510,145,.76f,"Crash recovery keeps the previous working config.");
 txt(510,180,.76f,exists(TAI_BACKUP)?"Config Backup: AVAILABLE":"Config Backup: none");
}
static void draw_update(void){
 title("Update Center");
 const char*a[]={"Check for Updates","App Update","UI Update","Plugin Updates","Plugin Catalog","News","Update History"};
 for(int i=0;i<7;i++)row(i,100+i*42,a[i]);
 txt(510,110,.76f,"Download > SHA-256 Verify > Backup > Apply > Verify");
 txt(510,145,.76f,"No forced app updates. Plugins never auto-enable.");
}
static void draw_news(void){
 title("News");
 const char*a[]={"NEW - App Updates","NEW - Plugin Releases","Plugin Updates","Fixes","Important Notices","Changelog","Mark All Read"};
 for(int i=0;i<7;i++)row(i,100+i*42,a[i]);
 txt(510,110,.76f,"Changelog: ADDED / CHANGED / FIXED / REMOVED");
}
static void draw_settings(void){
 title("Settings");
 const char*a[]={"Storage & Backup Paths","SAVE WORKING CONFIG","CREATE BACKUP NOW","HUD Settings","Plugin Settings","Update Settings","Recovery Settings","UI Settings","Controls","Temperature Unit"};
 for(int i=0;i<10;i++)row(i,92+i*34,a[i]);
 if(selected==0){
  txt(475,92,.76f,"STORAGE & BACKUP PATHS");
  txt(475,122,.60f,"Live: ur0:tai/config.txt");
  txt(475,146,.60f,"Fallback: ux0:tai/config.txt");
  txt(475,170,.60f,"Test: ux0:data/VitaAutoPlugin/config.test.txt");
  txt(475,194,.60f,"Working: ux0:data/VitaAutoPlugin/config.working.txt");
  txt(475,218,.60f,"Backups: ux0:data/VitaAutoPlugin/backups/");
  txt(475,242,.60f,"Recovery: ux0:data/VitaAutoPlugin/recovery.log");
  txt(475,266,.60f,"Settings: ux0:data/VitaAutoPlugin/settings.cfg");
  txt(475,306,.68f,"PC MANUAL EDITING");
  txt(475,334,.57f,"1. Copy the file/folder to PC using VitaShell USB/FTP.");
  txt(475,358,.57f,"2. Edit/copy it, then return it to the SAME path.");
  txt(475,382,.57f,"3. Create Backup > Test Config > test on Vita.");
  txt(475,406,.57f,"4. When stable, use SAVE WORKING CONFIG.");
  txt(475,446,.60f,"Auto Save: OFF (LOCKED)");
 } else {
  txt(475,110,.72f,"Backup and saving are intentionally easy to find.");
  txt(475,145,.72f,"Auto Save stays permanently OFF (locked).");
 }
}
static void draw_about(void){
 title("About");
 txt(70,135,1.45f,"Vita AutoPlugin");
 txt(70,180,.9f,"Created by MrWrack");
 txt(70,220,.76f,"PS Vita homebrew plugin management and system tools.");
 txt(70,260,.76f,"Version 0.16 source milestone");
 txt(70,315,.72f,"Independent homebrew project.");
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
    if(save_working_config()) log_recovery("Working config saved manually from Settings.");
  } else if(selected==2){
    if(create_config_backup()) log_recovery("Manual backup created from Settings.");
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
  if(selected==0){ if(save_working_config()) log_recovery("Last Working Config saved manually from Recovery."); }
  else if(selected==1){ if(create_config_backup()) log_recovery("Manual taiHEN config backup created."); }
  else if(selected==2){ if(restore_working_config()) log_recovery("Last Working Config restored manually."); }
  /* Auto Save remains locked OFF. */
 }
 if(screen==TROPHY){
  if(selected==0) trophy_hunter=!trophy_hunter;
  else if(selected==1) trophy_unlocker=!trophy_unlocker;
  else if(selected==4) auto_platinum=!auto_platinum;
  else if(selected==5) backup_trophy=!backup_trophy;
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
int main(void){
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG); load(); startup_recovery();
 vita2d_init(); vita2d_set_clear_color(RGBA8(6,18,36,255));
 ui_font=ui_font_load();
 if(!ui_font){ vita2d_fini(); sceKernelExitProcess(-1); return -1; }
 SceCtrlData p,o; memset(&o,0,sizeof(o));
 while(running){
  sceCtrlPeekBufferPositive(0,&p,1); unsigned q=p.buttons&~o.buttons;
  int n=count();
  if(q&SCE_CTRL_UP) selected=(selected+n-1)%n;
  if(q&SCE_CTRL_DOWN) selected=(selected+1)%n;
  if(screen==PLUGINS && plugin_view) plugin_cursor=selected;
  if(q&SCE_CTRL_CROSS) action();
  if(q&SCE_CTRL_CIRCLE){
   if(screen==PLUGINS && plugin_view){plugin_view=0;selected=0;}
   else if(screen==HOME)running=0;
   else{screen=HOME;selected=0;}
  }
  if(screen==MONITOR && selected==7 && (q&SCE_CTRL_LEFT)){s.fahrenheit=0;save();}
  if(screen==MONITOR && selected==7 && (q&SCE_CTRL_RIGHT)){s.fahrenheit=1;save();}
  vita2d_start_drawing(); vita2d_clear_screen();
  switch(screen){case HOME:draw_home();break;case TROPHY:draw_trophy();break;case OVERCLOCK:draw_overclock();break;case MONITOR:draw_monitor();break;case PLUGINS:draw_plugins();break;case RECOVERY:draw_recovery();break;case UPDATE:draw_update();break;case NEWS:draw_news();break;case SETTINGS:draw_settings();break;case ABOUT:draw_about();break;}
  footer(); vita2d_end_drawing(); vita2d_swap_buffers();
  o=p; sceKernelDelayThread(16000);
 }
 mark_clean_exit(); ui_font_free(ui_font); ui_font=NULL; vita2d_fini(); sceKernelExitProcess(0); return 0;
}