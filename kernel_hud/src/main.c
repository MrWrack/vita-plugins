#include <vitasdkkern.h>
#include <taihen.h>
#include <stdint.h>
#include <psp2kern/power.h>

/* Vita AutoPlugin HUD v0.44
   Kernel framebuffer hook: intended to stay visible on LiveArea and apps.
   R + D-pad Up toggles visibility. It never consumes controller input. */

static SceUID g_hook = -1;
static tai_hook_ref_t g_ref;
static SceUID g_thread = -1;
static volatile int g_run = 1;
static volatile int g_visible = 1;
static uint32_t g_old_buttons = 0;
static uint64_t g_tick = 0;
static unsigned g_frames = 0, g_fps = 0;

#define HUD_X 810

/* Resolved from ScePower exactly as PSVshell does; avoids a fake GPU value. */
static int (*g_gpu_get)(int *a1, int *a2) = 0;

/* PSVshell defines this private SceSysmem layout locally; it is not supplied by
   current VitaSDK public headers. Keep the exact four-field layout used there. */
typedef struct VapSysmemAddressSpaceInfo {
  uintptr_t base;
  uint32_t total;
  uint32_t free;
  uint32_t unkC;
} VapSysmemAddressSpaceInfo;

/* Optional live main-memory metric. If unavailable, the row is omitted (never N-A). */
static int (*g_addrspace_info)(uint32_t a1, VapSysmemAddressSpaceInfo *a2) = 0;
int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
SceUInt32 ksceKernelSysrootGetCurrentAddressSpaceCB(void);

static int gpu_mhz(void){
  int a=0,b=0;
  if(!g_gpu_get) return -1;
  if(g_gpu_get(&a,&b)<0) return -1;
  return (a>0 && a<1000) ? a : -1;
}

static int mem_used_mb(void){
  if(!g_addrspace_info) return -1;
  uint32_t cas=ksceKernelSysrootGetCurrentAddressSpaceCB();
  if(!cas) return -1;
  uint32_t asid=*(uint32_t *)(cas+328);
  if(!asid) return -1;
  VapSysmemAddressSpaceInfo info;
  if(g_addrspace_info(asid,&info)<0 || info.total<info.free) return -1;
  return (int)((info.total-info.free)/(1024u*1024u));
}

static const uint8_t *glyph(char c) {
  static const uint8_t z[7]={0,0,0,0,0,0,0};
  static const uint8_t A[7]={14,17,17,31,17,17,17}, B[7]={30,17,17,30,17,17,30};
  static const uint8_t C[7]={14,17,16,16,16,17,14}, D[7]={30,17,17,17,17,17,30};
  static const uint8_t E[7]={31,16,16,30,16,16,31}, F[7]={31,16,16,30,16,16,16};
  static const uint8_t G[7]={14,17,16,23,17,17,15}, H[7]={17,17,17,31,17,17,17};
  static const uint8_t I[7]={14,4,4,4,4,4,14}, L[7]={16,16,16,16,16,16,31};
  static const uint8_t M[7]={17,27,21,21,17,17,17}, N[7]={17,25,21,19,17,17,17};
  static const uint8_t O[7]={14,17,17,17,17,17,14}, P[7]={30,17,17,30,16,16,16};
  static const uint8_t R[7]={30,17,17,30,20,18,17}, S[7]={15,16,16,14,1,1,30};
  static const uint8_t T[7]={31,4,4,4,4,4,4}, U[7]={17,17,17,17,17,17,14};
  static const uint8_t zero[7]={14,17,19,21,25,17,14}, one[7]={4,12,4,4,4,4,14};
  static const uint8_t two[7]={14,17,1,2,4,8,31}, three[7]={30,1,1,14,1,1,30};
  static const uint8_t four[7]={2,6,10,18,31,2,2}, five[7]={31,16,16,30,1,1,30};
  static const uint8_t six[7]={14,16,16,30,17,17,14}, seven[7]={31,1,2,4,8,8,8};
  static const uint8_t eight[7]={14,17,17,14,17,17,14}, nine[7]={14,17,17,15,1,1,14};
  static const uint8_t dot[7]={0,0,0,0,0,6,6}, pct[7]={17,2,4,8,17,0,0}, dash[7]={0,0,0,31,0,0,0};
  switch(c){case 'A':return A;case 'B':return B;case 'C':return C;case 'D':return D;case 'E':return E;case 'F':return F;case 'G':return G;case 'H':return H;case 'I':return I;case 'L':return L;case 'M':return M;case 'N':return N;case 'O':return O;case 'P':return P;case 'R':return R;case 'S':return S;case 'T':return T;case 'U':return U;
  case '0':return zero;case '1':return one;case '2':return two;case '3':return three;case '4':return four;case '5':return five;case '6':return six;case '7':return seven;case '8':return eight;case '9':return nine;case '.':return dot;case '%':return pct;case '-':return dash;default:return z;}
}

static void pixel(const SceDisplayFrameBuf *fb,int x,int y,uint32_t color){
  if(!fb || !fb->base || x<0 || y<0 || x>=(int)fb->width || y>=(int)fb->height) return;
  uintptr_t dst=(uintptr_t)&((uint32_t*)fb->base)[y*fb->pitch+x];
  ksceKernelMemcpyKernelToUser((void *)dst,&color,sizeof(color));
}
static void ch2(const SceDisplayFrameBuf *fb,int x,int y,char c){
  const uint8_t *g=glyph(c);
  for(int yy=0;yy<7;yy++) for(int xx=0;xx<5;xx++) if(g[yy]&(1u<<(4-xx)))
    for(int dy=0;dy<2;dy++) for(int dx=0;dx<2;dx++) pixel(fb,x+xx*2+dx,y+yy*2+dy,0xFFFFFFFFu);
}
static void text(const SceDisplayFrameBuf *fb,int x,int y,const char *s){while(*s){ch2(fb,x,y,*s++);x+=12;}}
static char *u32s(char *p,unsigned v){char t[11];int n=0;if(!v){*p++='0';return p;}while(v){t[n++]=(char)('0'+v%10);v/=10;}while(n)*p++=t[--n];return p;}
static void metric(const SceDisplayFrameBuf *fb,int y,const char *name,int val,const char *unit){
  char b[48],*p=b; while(*name)*p++=*name++; *p++=' ';
  if(val<0) return; else p=u32s(p,(unsigned)val);
  if(unit){*p++=' ';while(*unit)*p++=*unit++;} *p=0; text(fb,HUD_X,y,b);
}
static void draw_hud(const SceDisplayFrameBuf *fb){
  int cpu=kscePowerGetArmClockFrequency();
  int bat=kscePowerGetBatteryLifePercent();
  int temp=kscePowerGetBatteryTemp();
  int gpu=gpu_mhz();
  int mem=mem_used_mb();
  int y=24;
  text(fb,HUD_X,y,"HUD"); y+=18;
  metric(fb,y,"FPS",(int)g_fps,0); y+=18;
  metric(fb,y,"CPU",cpu,"MHZ"); y+=18;
  if(gpu>=0){ metric(fb,y,"GPU",gpu,"MHZ"); y+=18; }
  if(mem>=0){ metric(fb,y,"MEM",mem,"MB"); y+=18; }
  metric(fb,y,"BAT",bat,"%"); y+=18;
  if(temp>=0 && temp<=10000){char b[32],*p=b;const char *s="TEMP ";while(*s)*p++=*s++;p=u32s(p,(unsigned)(temp/100));*p++='.';*p++=(char)('0'+((temp/10)%10));*p++='C';*p=0;text(fb,HUD_X,y,b);}
}

static int display_patched(int head,int index,const SceDisplayFrameBuf *fb,int sync){
  if(head && fb && fb->base && fb->width>=800 && fb->height>=500){
    uint64_t now=(uint64_t)ksceKernelGetSystemTimeWide();
    if(!g_tick) g_tick=now;
    g_frames++;
    if(now-g_tick>=1000000ULL){g_fps=g_frames;g_frames=0;g_tick=now;}
    if(g_visible) draw_hud(fb);
  }
  return TAI_CONTINUE(int,g_ref,head,index,fb,sync);
}

static int input_thread(SceSize args,void *argp){
  (void)args;(void)argp;
  while(g_run){
    SceCtrlData pad;
    int r=ksceCtrlPeekBufferPositive(0,&pad,1); if(r<0) r=ksceCtrlPeekBufferPositive(1,&pad,1);
    if(r>0){uint32_t now=pad.buttons; uint32_t chord=SCE_CTRL_RTRIGGER|SCE_CTRL_UP; if((now&chord)==chord && (g_old_buttons&chord)!=chord) g_visible=!g_visible; g_old_buttons=now;}
    ksceKernelDelayThread(50000);
  }
  return 0;
}

int module_start(SceSize argc,const void *args){
  (void)argc;(void)args;
  module_get_export_func(KERNEL_PID,"ScePower",0x1590166F,0x475BCC82,(uintptr_t *)&g_gpu_get);
  if(module_get_export_func(KERNEL_PID,"SceSysmem",0x63A519E5,0x3650963F,(uintptr_t *)&g_addrspace_info)<0)
    module_get_export_func(KERNEL_PID,"SceSysmem",0x02451F0F,0xB9B69700,(uintptr_t *)&g_addrspace_info);
  g_hook=taiHookFunctionExportForKernel(KERNEL_PID,&g_ref,"SceDisplay",0x9FED47AC,0x16466675,display_patched);
  if(g_hook<0) return SCE_KERNEL_START_NO_RESIDENT;
  g_thread=ksceKernelCreateThread("vap_hud_input",input_thread,0x3C,0x2000,0,0x10000,0);
  if(g_thread>=0) ksceKernelStartThread(g_thread,0,NULL);
  return SCE_KERNEL_START_SUCCESS;
}
int module_stop(SceSize argc,const void *args){
  (void)argc;(void)args; g_run=0;
  if(g_thread>=0){ksceKernelWaitThreadEnd(g_thread,NULL,NULL);ksceKernelDeleteThread(g_thread);}
  if(g_hook>=0) taiHookReleaseForKernel(g_hook,g_ref);
  return SCE_KERNEL_STOP_SUCCESS;
}
