#include "plugin_browser_screen.h"
#include "plugin_browser_ui.h"
#include "plugin_browser_input.h"
#include "plugin_browser_render.h"
#include <psp2/kernel/processmgr.h>
#include <string.h>
#include <stdio.h>

/* Returns to caller/main menu. Download action is intentionally not faked:
   transport must be enabled and wired with a real repository item first. */
int run_plugin_browser_screen(vita2d_pgf *font){
 PluginBrowserUI ui; PluginBrowserInput in;
 plugin_browser_ui_init(&ui,1); plugin_browser_input_init(&in);
 snprintf(ui.title,sizeof(ui.title),"Example Plugin");
 snprintf(ui.version,sizeof(ui.version),"1.0");
 snprintf(ui.developer,sizeof(ui.developer),"Developer");
 snprintf(ui.description,sizeof(ui.description),"Repository UI integration test entry.");
 int running=1;
 while(running){
  int action=plugin_browser_input_update(&in,&ui);
  if(action==3)running=0;
  /* action 1 = download, action 2 = verified install.
     Both remain integration hooks; no unsafe/fake install occurs here. */
  plugin_browser_ui_sync_verify(&ui);
  vita2d_start_drawing();
  vita2d_clear_screen();
  plugin_browser_render(&ui,font);
  vita2d_end_drawing();
  vita2d_swap_buffers();
  sceKernelDelayThread(16000);
 }
 return 0;
}
