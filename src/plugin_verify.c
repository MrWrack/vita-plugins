#include "plugin_verify.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

typedef struct { uint8_t data[64]; uint32_t datalen; uint64_t bitlen; uint32_t state[8]; } SHA256_CTX;
#define ROR(a,b) (((a)>>(b))|((a)<<(32-(b))))
#define CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define EP0(x) (ROR(x,2)^ROR(x,13)^ROR(x,22))
#define EP1(x) (ROR(x,6)^ROR(x,11)^ROR(x,25))
#define SIG0(x) (ROR(x,7)^ROR(x,18)^((x)>>3))
#define SIG1(x) (ROR(x,17)^ROR(x,19)^((x)>>10))
static const uint32_t k[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
static void tr(SHA256_CTX*c,const uint8_t d[]){
 uint32_t a,b,cc,dv,e,f,g,h,i,j,t1,t2,m[64];
 for(i=0,j=0;i<16;i++,j+=4)m[i]=((uint32_t)d[j]<<24)|((uint32_t)d[j+1]<<16)|((uint32_t)d[j+2]<<8)|d[j+3];
 for(;i<64;i++)m[i]=SIG1(m[i-2])+m[i-7]+SIG0(m[i-15])+m[i-16];
 a=c->state[0];b=c->state[1];cc=c->state[2];dv=c->state[3];e=c->state[4];f=c->state[5];g=c->state[6];h=c->state[7];
 for(i=0;i<64;i++){t1=h+EP1(e)+CH(e,f,g)+k[i]+m[i];t2=EP0(a)+MAJ(a,b,cc);h=g;g=f;f=e;e=dv+t1;dv=cc;cc=b;b=a;a=t1+t2;}
 c->state[0]+=a;c->state[1]+=b;c->state[2]+=cc;c->state[3]+=dv;c->state[4]+=e;c->state[5]+=f;c->state[6]+=g;c->state[7]+=h;
}
static void init(SHA256_CTX*c){c->datalen=0;c->bitlen=0;uint32_t s[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};memcpy(c->state,s,sizeof(s));}
static void upd(SHA256_CTX*c,const uint8_t*d,size_t n){for(size_t i=0;i<n;i++){c->data[c->datalen++]=d[i];if(c->datalen==64){tr(c,c->data);c->bitlen+=512;c->datalen=0;}}}
static void fin(SHA256_CTX*c,uint8_t hash[32]){
 uint32_t i=c->datalen;c->data[i++]=0x80;if(i>56){while(i<64)c->data[i++]=0;tr(c,c->data);i=0;}while(i<56)c->data[i++]=0;
 c->bitlen+=(uint64_t)c->datalen*8;for(int j=7;j>=0;j--)c->data[63-j]=(uint8_t)(c->bitlen>>(j*8));tr(c,c->data);
 for(i=0;i<4;i++)for(int j=0;j<8;j++)hash[i+4*j]=(uint8_t)((c->state[j]>>(24-i*8))&255);
}
int mrwrack_sha256_file(const char *path,char out[65]){
 FILE*f=fopen(path,"rb");if(!f)return 0;SHA256_CTX c;init(&c);uint8_t b[4096];size_t n;
 while((n=fread(b,1,sizeof(b),f))>0)upd(&c,b,n);if(ferror(f)){fclose(f);return 0;}fclose(f);
 uint8_t h[32];fin(&c,h);static const char*x="0123456789abcdef";for(int i=0;i<32;i++){out[i*2]=x[h[i]>>4];out[i*2+1]=x[h[i]&15];}out[64]=0;return 1;
}
int mrwrack_verify_sha256(const char *path,const char *expected){
 if(!expected||strlen(expected)!=64)return 0;char got[65];if(!mrwrack_sha256_file(path,got))return 0;
 for(int i=0;i<64;i++)if(tolower((unsigned char)got[i])!=tolower((unsigned char)expected[i]))return 0;return 1;
}
