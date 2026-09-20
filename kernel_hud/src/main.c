#include <vitasdkkern.h>
#include <taihen.h>
#include <stdint.h>
#include <psp2kern/power.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>

/* Vita AutoPlugin HUD v0.46
   Kernel framebuffer hook: intended to stay visible on LiveArea and apps.
   R + D-pad Up toggles visibility. It never consumes controller input. */

static SceUID g_hook = -1;
static tai_hook_ref_t g_ref;
static SceUID g_thread = -1;
static volatile int g_run = 1;
static volatile int g_visible = 1;
static volatile int g_oc_open = 0;
static int g_oc_sel = 0;
static int g_save_flash = 0;

#define OC_CFG_PATH "ur0:data/VitaAutoPlugin/overclock.bin"
#define OC_CFG_MAGIC 0x56414F46u

typedef struct VapOcProfile {
  uint32_t magic;
  int enabled, cpu, gpu, bus, xbar, boost;
} VapOcProfile;

static VapOcProfile g_oc = {OC_CFG_MAGIC,0,333,111,166,111,0};

/* Stable Vita CPU choices used by this menu. */
static const int CPU_STEPS[]={333,444,500};
static const int GPU_STEPS[]={41,55,83,111,166,222};
static const int BUS_STEPS[]={55,83,111,166,222};
static const int XBAR_STEPS[]={83,111,166};
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
  static const uint8_t I[7]={14,4,4,4,4,4,14}, K[7]={17,18,20,24,20,18,17}, L[7]={16,16,16,16,16,16,31};
  static const uint8_t M[7]={17,27,21,21,17,17,17}, N[7]={17,25,21,19,17,17,17};
  static const uint8_t O[7]={14,17,17,17,17,17,14}, P[7]={30,17,17,30,16,16,16};
  static const uint8_t Q[7]={14,17,17,17,21,18,13}, R[7]={30,17,17,30,20,18,17}, S[7]={15,16,16,14,1,1,30};
  static const uint8_t T[7]={31,4,4,4,4,4,4}, U[7]={17,17,17,17,17,17,14}, V[7]={17,17,17,17,17,10,4}, W[7]={17,17,17,21,21,21,10}, X[7]={17,17,10,4,10,17,17}, Y[7]={17,17,10,4,4,4,4};
  static const uint8_t zero[7]={14,17,19,21,25,17,14}, one[7]={4,12,4,4,4,4,14};
  static const uint8_t two[7]={14,17,1,2,4,8,31}, three[7]={30,1,1,14,1,1,30};
  static const uint8_t four[7]={2,6,10,18,31,2,2}, five[7]={31,16,16,30,1,1,30};
  static const uint8_t six[7]={14,16,16,30,17,17,14}, seven[7]={31,1,2,4,8,8,8};
  static const uint8_t eight[7]={14,17,17,14,17,17,14}, nine[7]={14,17,17,15,1,1,14};
  static const uint8_t dot[7]={0,0,0,0,0,6,6}, pct[7]={17,2,4,8,17,0,0}, dash[7]={0,0,0,31,0,0,0}, gt[7]={8,4,2,1,2,4,8};
  switch(c){case 'A':return A;case 'B':return B;case 'C':return C;case 'D':return D;case 'E':return E;case 'F':return F;case 'G':return G;case 'H':return H;case 'I':return I;case 'K':return K;case 'L':return L;case 'M':return M;case 'N':return N;case 'O':return O;case 'P':return P;case 'Q':return Q;case 'R':return R;case 'S':return S;case 'T':return T;case 'U':return U;case 'V':return V;case 'W':return W;case 'X':return X;case 'Y':return Y;
  case '>':return gt;case '0':return zero;case '1':return one;case '2':return two;case '3':return three;case '4':return four;case '5':return five;case '6':return six;case '7':return seven;case '8':return eight;case '9':return nine;case '.':return dot;case '%':return pct;case '-':return dash;default:return z;}
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

static int nearest_step(const int *a,int n,int v){int best=0,d=0x7fffffff;for(int i=0;i<n;i++){int x=a[i]-v;if(x<0)x=-x;if(x<d){d=x;best=i;}}return best;}
static void step_value(int *v,const int *a,int n,int dir){int i=nearest_step(a,n,*v);i+=dir;if(i<0)i=0;if(i>=n)i=n-1;*v=a[i];}

static void apply_oc(void){
  if(!g_oc.enabled){
    kscePowerSetArmClockFrequency(333);
    kscePowerSetGpuClockFrequency(111);
    kscePowerSetBusClockFrequency(166);
    kscePowerSetGpuXbarClockFrequency(111);
    return;
  }
  int cpu=g_oc.boost?500:g_oc.cpu;
  int gpu=g_oc.boost?222:g_oc.gpu;
  int bus=g_oc.boost?222:g_oc.bus;
  int xbar=g_oc.boost?166:g_oc.xbar;
  kscePowerSetArmClockFrequency(cpu);
  kscePowerSetGpuClockFrequency(gpu);
  kscePowerSetBusClockFrequency(bus);
  kscePowerSetGpuXbarClockFrequency(xbar);
}
static int save_oc(void){
  ksceIoMkdir("ur0:data/VitaAutoPlugin",0777);
  int fd=ksceIoOpen(OC_CFG_PATH,SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0777);
  if(fd<0)return fd;
  int r=ksceIoWrite(fd,&g_oc,sizeof(g_oc));
  ksceIoClose(fd);
  return r==(int)sizeof(g_oc)?0:-1;
}
static void load_oc(void){
  VapOcProfile t;
  int fd=ksceIoOpen(OC_CFG_PATH,SCE_O_RDONLY,0);
  if(fd<0)return;
  int r=ksceIoRead(fd,&t,sizeof(t));ksceIoClose(fd);
  if(r==(int)sizeof(t)&&t.magic==OC_CFG_MAGIC){g_oc=t;apply_oc();}
}
static void draw_oc(const SceDisplayFrameBuf *fb){
  int x=590,y=24; char b[48],*p; const char *q;
  text(fb,x,y,"OVERCLOCK");y+=20;

  p=b;*p++=(g_oc_sel==0)?'>':' ';q="OVERCLOCK ";while(*q)*p++=*q++;q=g_oc.enabled?"ON":"OFF";while(*q)*p++=*q++;*p=0;text(fb,x,y,b);y+=18;

  const char *names[4]={"CPU","GPU","BUS","XBAR"}; int vals[4]={g_oc.cpu,g_oc.gpu,g_oc.bus,g_oc.xbar};
  for(int i=0;i<4;i++){p=b;*p++=(g_oc_sel==1+i)?'>':' ';q=names[i];while(*q)*p++=*q++;*p++=' ';p=u32s(p,(unsigned)vals[i]);*p++=' ';*p++='M';*p++='H';*p++='Z';*p=0;text(fb,x,y,b);y+=18;}
  p=b;*p++=(g_oc_sel==5)?'>':' ';q="FPS BOOST ";while(*q)*p++=*q++;q=g_oc.boost?"ON":"OFF";while(*q)*p++=*q++;*p=0;text(fb,x,y,b);y+=18;
  const char *acts[3]={"APPLY","SAVE","RESET"};for(int i=0;i<3;i++){p=b;*p++=(g_oc_sel==6+i)?'>':' ';q=acts[i];while(*q)*p++=*q++;*p=0;text(fb,x,y,b);y+=18;}
  if(g_save_flash>0){text(fb,x,y,"SAVED");g_save_flash--;}
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
    if(g_oc_open) draw_oc(fb);
  }
  return TAI_CONTINUE(int,g_ref,head,index,fb,sync);
}

static int input_thread(SceSize args,void *argp){
  (void)args;(void)argp;
  while(g_run){
    SceCtrlData pad;
    int r=ksceCtrlPeekBufferPositive(0,&pad,1); if(r<0) r=ksceCtrlPeekBufferPositive(1,&pad,1);
    if(r>0){
      uint32_t now=pad.buttons;
      uint32_t hud_chord=SCE_CTRL_RTRIGGER|SCE_CTRL_UP;
      uint32_t oc_chord=SCE_CTRL_TRIANGLE|SCE_CTRL_UP;
      uint32_t pressed=now & ~g_old_buttons;
      if((now&hud_chord)==hud_chord && (g_old_buttons&hud_chord)!=hud_chord) g_visible=!g_visible;
      if((now&oc_chord)==oc_chord && (g_old_buttons&oc_chord)!=oc_chord) g_oc_open=!g_oc_open;
      else if(g_oc_open){
        if(pressed&SCE_CTRL_DOWN){g_oc_sel++;if(g_oc_sel>8)g_oc_sel=0;}
        if((pressed&SCE_CTRL_UP) && !(now&SCE_CTRL_TRIANGLE)){g_oc_sel--;if(g_oc_sel<0)g_oc_sel=8;}
        if(pressed&SCE_CTRL_LEFT){if(g_oc_sel==0)g_oc.enabled=0;else if(g_oc_sel==1)step_value(&g_oc.cpu,CPU_STEPS,3,-1);else if(g_oc_sel==2)step_value(&g_oc.gpu,GPU_STEPS,6,-1);else if(g_oc_sel==3)step_value(&g_oc.bus,BUS_STEPS,5,-1);else if(g_oc_sel==4)step_value(&g_oc.xbar,XBAR_STEPS,3,-1);else if(g_oc_sel==5)g_oc.boost=0;}
        if(pressed&SCE_CTRL_RIGHT){if(g_oc_sel==0)g_oc.enabled=1;else if(g_oc_sel==1)step_value(&g_oc.cpu,CPU_STEPS,3,1);else if(g_oc_sel==2)step_value(&g_oc.gpu,GPU_STEPS,6,1);else if(g_oc_sel==3)step_value(&g_oc.bus,BUS_STEPS,5,1);else if(g_oc_sel==4)step_value(&g_oc.xbar,XBAR_STEPS,3,1);else if(g_oc_sel==5)g_oc.boost=1;}
        if(pressed&SCE_CTRL_CROSS){
          if(g_oc_sel==0)g_oc.enabled=!g_oc.enabled;
          else if(g_oc_sel==5)g_oc.boost=!g_oc.boost;
          else if(g_oc_sel==6)apply_oc();
          else if(g_oc_sel==7){apply_oc();if(save_oc()==0)g_save_flash=40;}
          else if(g_oc_sel==8){g_oc.enabled=0;g_oc.cpu=333;g_oc.gpu=111;g_oc.bus=166;g_oc.xbar=111;g_oc.boost=0;apply_oc();}
        }
        if(pressed&SCE_CTRL_CIRCLE)g_oc_open=0;
      }
      g_old_buttons=now;
    }
    ksceKernelDelayThread(50000);
  }
  return 0;
}

int module_start(SceSize argc,const void *args){
  (void)argc;(void)args;
  module_get_export_func(KERNEL_PID,"ScePower",0x1590166F,0x475BCC82,(uintptr_t *)&g_gpu_get);
  if(module_get_export_func(KERNEL_PID,"SceSysmem",0x63A519E5,0x3650963F,(uintptr_t *)&g_addrspace_info)<0)
    module_get_export_func(KERNEL_PID,"SceSysmem",0x02451F0F,0xB9B69700,(uintptr_t *)&g_addrspace_info);
  load_oc(); /* applies saved clocks, but menu itself always starts closed */
  g_oc_open=0;
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
