#pragma once
#include "download_status.h"
typedef enum { PB_FOCUS_LIST=0, PB_FOCUS_DOWNLOAD, PB_FOCUS_INSTALL, PB_FOCUS_BACK } PluginBrowserFocus;
typedef struct {
 int selected_plugin;
 int plugin_count;
 PluginBrowserFocus focus;
 DownloadStatus download;
 int verified;
 int install_enabled;
 char title[80], version[24], developer[64], description[180];
} PluginBrowserUI;
void plugin_browser_ui_init(PluginBrowserUI *ui,int count);
void plugin_browser_ui_up(PluginBrowserUI *ui);
void plugin_browser_ui_down(PluginBrowserUI *ui);
void plugin_browser_ui_left(PluginBrowserUI *ui);
void plugin_browser_ui_right(PluginBrowserUI *ui);
void plugin_browser_ui_sync_verify(PluginBrowserUI *ui);
int plugin_browser_ui_activate(PluginBrowserUI *ui);
int plugin_browser_ui_touch(PluginBrowserUI *ui,int x,int y);
