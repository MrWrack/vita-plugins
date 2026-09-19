#include "plugin_browser_render.h"
#include <stdio.h>
#define RGBA(r,g,b,a) ((unsigned int)((r)|((g)<<8)|((b)<<16)|((a)<<24)))
static void txt(vita2d_pgf*f,float x,float y,float s,const char*t){
 if(f&&t) vita2d_pgf_draw_text(f,x,y,RGBA(238,242,248,255),s,t);
}
static void box(float x,float y,float w,float h,unsigned int c){vita2d_draw_rectangle(x,y,w,h,c);}
static void button(vita2d_pgf*f,float x,float y,float w,float h,const char*label,int focused,int enabled){
 unsigned int c=!enabled?RGBA(45,48,55,255):(focused?RGBA(75,105,150,255):RGBA(55,62,75,255));
 box(x,y,w,h,c); txt(f,x+16,y+34,1.0f,label);
}
void plugin_browser_render(const PluginBrowserUI*u,vita2d_pgf*f){
 if(!u||!f)return;
 box(0,0,960,544,RGBA(18,21,27,255));
 txt(f,28,42,1.25f,"Vita AutoPlugin - MrWrack");
 txt(f,28,75,0.9f,"PLUGIN BROWSER");
 box(20,96,390,350,RGBA(27,31,39,255));
 box(430,96,510,350,RGBA(27,31,39,255));

 char b[256];
 for(int i=0;i<u->plugin_count && i<10;i++){
  float y=126+i*30;
  if(i==u->selected_plugin)box(30,y-21,370,27,RGBA(55,72,98,255));
  snprintf(b,sizeof(b),"%s Plugin %d",i==u->selected_plugin?">":" ",i+1);
  txt(f,42,y,0.82f,b);
 }
 txt(f,452,132,1.0f,u->title[0]?u->title:"Selected Plugin");
 snprintf(b,sizeof(b),"Version: %s",u->version[0]?u->version:"-");txt(f,452,166,0.82f,b);
 snprintf(b,sizeof(b),"Developer: %s",u->developer[0]?u->developer:"-");txt(f,452,194,0.82f,b);
 txt(f,452,236,0.78f,u->description[0]?u->description:"Select a plugin to view details.");

 snprintf(b,sizeof(b),"Status: %s",download_stage_name(u->download.stage));txt(f,452,338,0.84f,b);
 if(u->download.stage==DS_DOWNLOADING){
  box(452,358,430,12,RGBA(45,48,55,255));
  box(452,358,(430.0f*u->download.percent)/100.0f,12,RGBA(105,135,175,255));
  snprintf(b,sizeof(b),"%d%%",u->download.percent);txt(f,890,371,0.72f,b);
 }
 button(f,610,420,150,55,"DOWNLOAD",u->focus==PB_FOCUS_DOWNLOAD,1);
 button(f,770,420,140,55,u->install_enabled?"INSTALL":"INSTALL LOCKED",u->focus==PB_FOCUS_INSTALL,u->install_enabled);
 button(f,20,480,130,45,"BACK",u->focus==PB_FOCUS_BACK,1);
 txt(f,210,520,0.72f,"D-Pad Navigate   X Select   O Back   Touch Supported");
}
