#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <string.h>

/*
 Vita AutoPlugin Background v0.31
 --------------------------------
 This module only establishes persistent hotkey state in a taiHEN-loaded
 process. Rendering hooks and verified telemetry are deliberately NOT claimed
 complete yet. They must be validated on real hardware before release.
*/
static volatile int g_running=1;
static volatile int g_menu_open=0;
static int g_quick_latched=0;

static int worker(SceSize argc, void *argp){
 SceCtrlData now, old;
 memset(&old,0,sizeof(old));
 while(g_running){
  memset(&now,0,sizeof(now));
  if(sceCtrlPeekBufferPositive(0,&now,1)>0){
   int chord=(now.buttons&SCE_CTRL_RTRIGGER) && (now.buttons&SCE_CTRL_UP);

   /* Toggle once, then require a release before another toggle. */
   if(chord && !g_quick_latched){
    g_quick_latched=1;
    g_menu_open=!g_menu_open;
   }
   if(!chord) g_quick_latched=0;

   /* Normal Quick Menu is toggled only by R + D-pad Up. */
   old=now;
  }
  sceKernelDelayThread(16000);
 }
 return 0;
}

int module_start(SceSize argc, const void *args){
 SceUID th=sceKernelCreateThread("MrWrackQuickMenu",
   worker,0x10000100,0x4000,0,0,NULL);
 if(th>=0) sceKernelStartThread(th,0,NULL);
 return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args){
 g_running=0;
 return SCE_KERNEL_STOP_SUCCESS;
}
