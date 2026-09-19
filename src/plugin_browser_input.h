#pragma once
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include "plugin_browser_ui.h"
typedef struct { unsigned int previous_buttons; } PluginBrowserInput;
void plugin_browser_input_init(PluginBrowserInput *in);
int plugin_browser_input_update(PluginBrowserInput *in, PluginBrowserUI *ui);
