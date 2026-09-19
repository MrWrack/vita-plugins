#include "plugin_browser_ui.h"
#include <string.h>
void plugin_browser_ui_init(PluginBrowserUI*u,int count){
 memset(u,0,sizeof(*u));u->plugin_count=count;u->focus=PB_FOCUS_LIST;download_status_reset(&u->download);
}
void plugin_browser_ui_up(PluginBrowserUI*u){if(u->focus==PB_FOCUS_LIST&&u->selected_plugin>0)u->selected_plugin--;else u->focus=PB_FOCUS_LIST;}
void plugin_browser_ui_down(PluginBrowserUI*u){if(u->focus==PB_FOCUS_LIST&&u->selected_plugin<u->plugin_count-1)u->selected_plugin++;else u->focus=PB_FOCUS_DOWNLOAD;}
void plugin_browser_ui_left(PluginBrowserUI*u){if(u->focus>PB_FOCUS_DOWNLOAD)u->focus--;}
void plugin_browser_ui_right(PluginBrowserUI*u){
 if(u->focus==PB_FOCUS_LIST)u->focus=PB_FOCUS_DOWNLOAD;
 else if(u->focus==PB_FOCUS_DOWNLOAD && u->install_enabled)u->focus=PB_FOCUS_INSTALL;
 else if(u->focus==PB_FOCUS_INSTALL)u->focus=PB_FOCUS_BACK;
}
void plugin_browser_ui_sync_verify(PluginBrowserUI*u){
 u->verified=(u->download.stage==DS_READY_TO_INSTALL);
 u->install_enabled=u->verified;
 if(!u->install_enabled && u->focus==PB_FOCUS_INSTALL)u->focus=PB_FOCUS_DOWNLOAD;
}
/* return: 1 download, 2 install, 3 back, 0 none */
int plugin_browser_ui_activate(PluginBrowserUI*u){
 plugin_browser_ui_sync_verify(u);
 if(u->focus==PB_FOCUS_DOWNLOAD)return 1;
 if(u->focus==PB_FOCUS_INSTALL&&u->install_enabled)return 2;
 if(u->focus==PB_FOCUS_BACK)return 3;
 return 0;
}
/* 960x544 native Vita layout hit zones */
int plugin_browser_ui_touch(PluginBrowserUI*u,int x,int y){
 if(x>=610&&x<=760&&y>=420&&y<=475){u->focus=PB_FOCUS_DOWNLOAD;return 1;}
 if(x>=770&&x<=910&&y>=420&&y<=475){u->focus=PB_FOCUS_INSTALL;return u->install_enabled?2:0;}
 if(x>=20&&x<=150&&y>=480&&y<=535){u->focus=PB_FOCUS_BACK;return 3;}
 return 0;
}
