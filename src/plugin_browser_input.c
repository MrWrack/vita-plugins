#include "plugin_browser_input.h"
#include <string.h>
void plugin_browser_input_init(PluginBrowserInput*i){memset(i,0,sizeof(*i));}
static int pressed(unsigned int now,unsigned int prev,unsigned int mask){return (now&mask)&&!(prev&mask);}
int plugin_browser_input_update(PluginBrowserInput*i,PluginBrowserUI*u){
 SceCtrlData pad; memset(&pad,0,sizeof(pad)); sceCtrlPeekBufferPositive(0,&pad,1);
 unsigned int n=pad.buttons,p=i->previous_buttons; int action=0;
 if(pressed(n,p,SCE_CTRL_UP))plugin_browser_ui_up(u);
 if(pressed(n,p,SCE_CTRL_DOWN))plugin_browser_ui_down(u);
 if(pressed(n,p,SCE_CTRL_LEFT))plugin_browser_ui_left(u);
 if(pressed(n,p,SCE_CTRL_RIGHT))plugin_browser_ui_right(u);
 if(pressed(n,p,SCE_CTRL_CROSS))action=plugin_browser_ui_activate(u);
 if(pressed(n,p,SCE_CTRL_CIRCLE))action=3;
 i->previous_buttons=n;

 SceTouchData t; memset(&t,0,sizeof(t));
 if(sceTouchPeek(SCE_TOUCH_PORT_FRONT,&t,1)>0 && t.reportNum>0){
  /* Vita front touch native coordinate range is mapped into 960x544 UI space. */
  int x=(int)((t.report[0].x*960)/1920);
  int y=(int)((t.report[0].y*544)/1088);
  int ta=plugin_browser_ui_touch(u,x,y); if(ta)action=ta;
 }
 return action;
}
