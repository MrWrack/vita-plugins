#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/power.h>
#include <psp2/io/fcntl.h>
#include <taihen.h>
#include <stdint.h>

/* Vita AutoPlugin Background v0.39 - SceShell hook fallback + diagnostics.
   Loaded into SceShell (*main) and optionally apps (*ALL).
   R + D-pad Up toggles visibility. Other input is never consumed. */

static SceUID g_hook = -1;
static tai_hook_ref_t g_display_ref;
static int g_visible = 1, g_latched = 0;
static uint64_t g_tick = 0;
static unsigned g_frames = 0, g_fps = 0;

static void log_line(const char *s){
  SceUID fd=sceIoOpen("ux0:data/VitaAutoPlugin/hud_plugin.log", SCE_O_WRONLY|SCE_O_CREAT|SCE_O_APPEND, 0666);
  if(fd>=0){const char *p=s; while(*p)p++; sceIoWrite(fd,s,(SceSize)(p-s)); sceIoWrite(fd,"\n",1); sceIoClose(fd);}
}
static void log_hook(const char *kind,SceUID id){
  char b[64],*p=b; while(*kind)*p++=*kind++; *p++=' '; *p++='0'; *p++='x';
  static const char h[]="0123456789ABCDEF"; unsigned v=(unsigned)id;
  for(int i=7;i>=0;i--)*p++=h[(v>>(i*4))&15]; *p=0; log_line(b);
}

static const uint8_t *glyph(char c) {
  static const uint8_t blank[7]={0,0,0,0,0,0,0};
  static const uint8_t A[7]={14,17,17,31,17,17,17}, B[7]={30,17,17,30,17,17,30};
  static const uint8_t C[7]={14,17,16,16,16,17,14}, D[7]={30,17,17,17,17,17,30};
  static const uint8_t E[7]={31,16,16,30,16,16,31}, F[7]={31,16,16,30,16,16,16};
  static const uint8_t G[7]={14,17,16,23,17,17,15}, H[7]={17,17,17,31,17,17,17};
  static const uint8_t I[7]={14,4,4,4,4,4,14}, M[7]={17,27,21,21,17,17,17};
  static const uint8_t N[7]={17,25,21,19,17,17,17}, P[7]={30,17,17,30,16,16,16};
  static const uint8_t R[7]={30,17,17,30,20,18,17}, S[7]={15,16,16,14,1,1,30};
  static const uint8_t T[7]={31,4,4,4,4,4,4}, U[7]={17,17,17,17,17,17,14};
  static const uint8_t zero[7]={14,17,19,21,25,17,14}, one[7]={4,12,4,4,4,4,14};
  static const uint8_t two[7]={14,17,1,2,4,8,31}, three[7]={30,1,1,14,1,1,30};
  static const uint8_t four[7]={2,6,10,18,31,2,2}, five[7]={31,16,16,30,1,1,30};
  static const uint8_t six[7]={14,16,16,30,17,17,14}, seven[7]={31,1,2,4,8,8,8};
  static const uint8_t eight[7]={14,17,17,14,17,17,14}, nine[7]={14,17,17,15,1,1,14};
  static const uint8_t dot[7]={0,0,0,0,0,6,6}, pct[7]={17,2,4,8,17,0,0};
  static const uint8_t colon[7]={0,6,6,0,6,6,0}, dash[7]={0,0,0,31,0,0,0};
  switch(c){case 'A':return A;case 'B':return B;case 'C':return C;case 'D':return D;case 'E':return E;case 'F':return F;case 'G':return G;case 'H':return H;case 'I':return I;case 'M':return M;case 'N':return N;case 'P':return P;case 'R':return R;case 'S':return S;case 'T':return T;case 'U':return U;
  case '0':return zero;case '1':return one;case '2':return two;case '3':return three;case '4':return four;case '5':return five;case '6':return six;case '7':return seven;case '8':return eight;case '9':return nine;case '.':return dot;case '%':return pct;case ':':return colon;case '-':return dash;default:return blank;}
}

static void putpx(const SceDisplayFrameBuf *fb,int x,int y,uint32_t c){
  if(!fb || !fb->base || x<0 || y<0 || x>=(int)fb->width || y>=(int)fb->height) return;
  ((uint32_t*)fb->base)[y*fb->pitch+x]=c;
}
static void chr(const SceDisplayFrameBuf *fb,int x,int y,char c){
  const uint8_t *g=glyph(c); for(int yy=0;yy<7;yy++) for(int xx=0;xx<5;xx++) if(g[yy]&(1<<(4-xx))) putpx(fb,x+xx,y+yy,0xFFFFFFFF);
}
static void chr2(const SceDisplayFrameBuf *fb,int x,int y,char c){
  const uint8_t *g=glyph(c);
  for(int yy=0;yy<7;yy++) for(int xx=0;xx<5;xx++) if(g[yy]&(1<<(4-xx))){
    putpx(fb,x+xx*2,y+yy*2,0xFFFFFFFF); putpx(fb,x+xx*2+1,y+yy*2,0xFFFFFFFF);
    putpx(fb,x+xx*2,y+yy*2+1,0xFFFFFFFF); putpx(fb,x+xx*2+1,y+yy*2+1,0xFFFFFFFF);
  }
}
static void str(const SceDisplayFrameBuf *fb,int x,int y,const char *s){for(;*s;s++,x+=12) chr2(fb,x,y,*s);}
static char *u32(char *p,unsigned v){char t[11];int n=0;if(!v){*p++='0';return p;}while(v){t[n++]=(char)('0'+v%10);v/=10;}while(n)*p++=t[--n];return p;}
static void metric(const SceDisplayFrameBuf *fb,int y,const char *name,unsigned val,const char *unit){char b[48],*p=b;while(*name)*p++=*name++;*p++=' ';p=u32(p,val);if(unit){*p++=' ';while(*unit)*p++=*unit++;}*p=0;str(fb,650,y,b);}

static void draw_hud(const SceDisplayFrameBuf *fb){
  int temp=scePowerGetBatteryTemp();
  str(fb,650,24,"HUD");
  metric(fb,44,"FPS",g_fps,0);
  metric(fb,62,"CPU",(unsigned)scePowerGetArmClockFrequency(),"MHZ");
  metric(fb,80,"GPU",(unsigned)scePowerGetGpuClockFrequency(),"MHZ");
  str(fb,650,98,"MEM N-A");
  metric(fb,116,"BAT",(unsigned)scePowerGetBatteryLifePercent(),"%");
  if(temp>=0 && temp<=10000){ char b[32],*p=b; const char *n="TEMP ";while(*n)*p++=*n++;p=u32(p,(unsigned)(temp/100));*p++='.';*p++=(char)('0'+((temp/10)%10));*p++='C';*p=0;str(fb,650,134,b);} else str(fb,650,134,"TEMP N-A");
}

static int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam,int sync){
  SceCtrlData pad;
  if(sceCtrlPeekBufferPositive(0,&pad,1)>0){int chord=(pad.buttons&SCE_CTRL_RTRIGGER)&&(pad.buttons&SCE_CTRL_UP);if(chord&&!g_latched){g_visible=!g_visible;g_latched=1;}if(!chord)g_latched=0;}
  uint64_t now=sceKernelGetProcessTimeWide(); if(!g_tick)g_tick=now; g_frames++; if(now-g_tick>=1000000){g_fps=g_frames;g_frames=0;g_tick=now;}
  if(g_visible && pParam && pParam->base && pParam->width >= 800 && pParam->height >= 500) draw_hud(pParam);
  return TAI_CONTINUE(int, g_display_ref, pParam, sync);
}

int module_start(SceSize argc,const void *args){
  (void)argc; (void)args;
  log_line("v0.39 module_start");
  /* First try the same import hook used by Framecounter/Screenie.
     On SceShell some firmwares/builds do not expose that import from the main
     module, so fall back to hooking the SceDisplay export in this process. */
  g_hook=taiHookFunctionImport(&g_display_ref,TAI_MAIN_MODULE,TAI_ANY_LIBRARY,0x7A410B64,sceDisplaySetFrameBuf_patched);
  log_hook("import",g_hook);
  if(g_hook<0){
    g_hook=taiHookFunctionExport(&g_display_ref,"SceDisplay",TAI_ANY_LIBRARY,0x7A410B64,sceDisplaySetFrameBuf_patched);
    log_hook("export",g_hook);
  }
  if(g_hook<0){log_line("HOOK FAILED"); return SCE_KERNEL_START_NO_RESIDENT;}
  log_line("HOOK OK");
  return SCE_KERNEL_START_SUCCESS;
}
int module_stop(SceSize argc,const void *args){(void)argc; (void)args; if(g_hook>=0)taiHookRelease(g_hook,g_display_ref);return SCE_KERNEL_STOP_SUCCESS;}
