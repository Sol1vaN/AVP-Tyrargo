/* Water simulation: fixed 120 Hz damped height/velocity field. The cosine
 * impulse and neighbour-wave approach follow OpenLara (XProger), BSD-2-Clause;
 * see OpenLara_LICENSE.txt. World units, damping and integration are AvP's.
 * No gameplay random numbers, wall clock, physics or palette are changed. */
#include "water_effect.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
int WaterEffectMode=1;
typedef struct {int used,x0,x1,z0,z1,y,nx,nz;double seen;float sx,sz;float h[WATER_GRID*WATER_GRID],v[WATER_GRID*WATER_GRID],old[WATER_GRID*WATER_GRID];} WaterBody;
static WaterBody bodies[WATER_BODIES];
typedef struct {int valid,x,z,amplitude;double time;} PendingDrop;
static PendingDrop pending[64];static unsigned pendingNext;
static void applyDrop(WaterBody*b,int x,int z,int amplitude);
static int gridNodes(int extent);
static double clockTime;
static unsigned waterGeneration;
unsigned WaterGeneration(void){return waterGeneration;}
static int64_t waveRemainder;
typedef struct {const void *key;int y;double seen,next;} Emitter;
static Emitter emitters[256];
double WaterTime(void){return clockTime;}
void WaterReset(void){++waterGeneration;memset(pending,0,sizeof pending);pendingNext=0;memset(bodies,0,sizeof bodies);memset(emitters,0,sizeof emitters);clockTime=0;waveRemainder=0;}
void WaterRegister(int x0,int x1,int z0,int z1,int y){
 int i,slot=-1;double oldest=clockTime;
 if(x1<=x0||z1<=z0)return;
 for(i=0;i<WATER_BODIES;i++){
  WaterBody*b=&bodies[i];
  if(b->used&&b->x0==x0&&b->x1==x1&&b->z0==z0&&b->z1==z1&&b->y==y){
   if(clockTime-b->seen>10){memset(b->h,0,sizeof b->h);memset(b->old,0,sizeof b->old);memset(b->v,0,sizeof b->v);}
   b->seen=clockTime;
   for(int n=0;n<64;n++){PendingDrop*d=&pending[n];if(d->valid&&clockTime-d->time<.25&&d->x>=x0&&d->x<=x1&&d->z>=z0&&d->z<=z1){applyDrop(b,d->x,d->z,d->amplitude);d->valid=0;}}
   return;
  }
  if(!b->used){if(oldest!=-1){slot=i;oldest=-1;}}else if(b->seen<oldest){oldest=b->seen;slot=i;}
 }
 /* Never evict a surface already submitted during this frame. */
 if(slot<0)return;
 WaterBody*b=&bodies[slot];memset(b,0,sizeof *b);b->used=1;b->x0=x0;b->x1=x1;b->z0=z0;b->z1=z1;b->y=y;b->nx=gridNodes(x1-x0);b->nz=gridNodes(z1-z0);b->sx=(b->nx-1)/(float)(x1-x0);b->sz=(b->nz-1)/(float)(z1-z0);b->seen=clockTime;
 for(int n=0;n<64;n++){PendingDrop*d=&pending[n];if(d->valid&&clockTime-d->time<.25&&d->x>=x0&&d->x<=x1&&d->z>=z0&&d->z<=z1){applyDrop(b,d->x,d->z,d->amplitude);d->valid=0;}}
}
/* World-space sampling keeps narrow impacts resolved on long/rectangular pools.
 * A bounded allocation avoids changing size or remeshing the field mid-wave. */
static int gridNodes(int extent){return (int)fminf(WATER_GRID,fmaxf(3,ceilf(extent/WATER_SIM_SPACING)+1));}
int WaterMeshSegments(int extent){return (int)fminf(WATER_MESH_MAX_SEGMENTS,fmaxf(1,ceilf(fabsf((float)extent)/WATER_MESH_SPACING)));}
static void step(WaterBody*b){
 const float dt=1.0f/120.0f,dx=(b->x1-b->x0)/(float)(b->nx-1),dz=(b->z1-b->z0)/(float)(b->nz-1);
 /* CFL <= .45 on even the smallest authored water rectangles. */
 float c=fminf(1800.0f,.45f/(dt*sqrtf(1/(dx*dx)+1/(dz*dz))));
 float ax=c*c/(dx*dx),az=c*c/(dz*dz),damp=expf(-.9f*dt);
 memcpy(b->old,b->h,sizeof b->h);
 for(int x=0;x<b->nx;x++)for(int z=0;z<b->nz;z++){
  int i=x*WATER_GRID+z,xm=x?i-WATER_GRID:i,xp=x<b->nx-1?i+WATER_GRID:i,zm=z?i-1:i,zp=z<b->nz-1?i+1:i;
  float a=ax*(b->old[xm]+b->old[xp]-2*b->old[i])+az*(b->old[zm]+b->old[zp]-2*b->old[i]);
  b->v[i]=(b->v[i]+a*dt)*damp;b->h[i]=b->old[i]+b->v[i]*dt;
  /* Guard pathological crowds while keeping the wave differential smooth. */
  if(fabsf(b->h[i])>256){b->h[i]=copysignf(256,b->h[i]);b->v[i]*=.5f;}
 }
}
void WaterAdvance(int fixedDelta){
 if(fixedDelta<=0)return;
 clockTime+=fixedDelta/65536.0;
 waveRemainder+=(int64_t)fixedDelta*120;
 /* Engine dt, not render count. Long stalls settle instead of exploding. */
 if(waveRemainder>65536*120){for(int i=0;i<WATER_BODIES;i++){memset(bodies[i].h,0,sizeof bodies[i].h);memset(bodies[i].v,0,sizeof bodies[i].v);memset(bodies[i].old,0,sizeof bodies[i].old);}waveRemainder%=65536;}
 while(waveRemainder>=65536){for(int i=0;i<WATER_BODIES;i++)if(bodies[i].used&&clockTime-bodies[i].seen<10)step(&bodies[i]);waveRemainder-=65536;}
}
static void applyDrop(WaterBody*b,int x,int z,int amplitude){
  float dx=(b->x1-b->x0)/(float)(b->nx-1),dz=(b->z1-b->z0)/(float)(b->nz-1),r=fmaxf(620,4.0f*fmaxf(dx,dz));
  int loX=fmaxf(0,floorf((x-r-b->x0)/dx)),hiX=fminf(b->nx-1,ceilf((x+r-b->x0)/dx));
  int loZ=fmaxf(0,floorf((z-r-b->z0)/dz)),hiZ=fminf(b->nz-1,ceilf((z+r-b->z0)/dz));
  /* Zero-volume impulse: a crest surrounded by a compensating trough.
     Positive-only drops in a closed field would raise its mean indefinitely. */
  double crestSum=0,ringSum=0; /* Preserve zero volume on denser grids. */
  for(int ix=loX;ix<=hiX;ix++)for(int iz=loZ;iz<=hiZ;iz++){
   float px=b->x0+ix*dx-x,pz=b->z0+iz*dz-z,d=sqrtf(px*px+pz*pz)/r;
   if(d<1){float sn=sinf(d*3.141592654f);crestSum+=.5f*(1+cosf(d*3.141592654f));ringSum+=sn*sn;}
  }
  if(ringSum<.0001f)return;
  for(int ix=loX;ix<=hiX;ix++)for(int iz=loZ;iz<=hiZ;iz++){
   float px=b->x0+ix*dx-x,pz=b->z0+iz*dz-z,d=sqrtf(px*px+pz*pz)/r;
   if(d<1){int i=ix*WATER_GRID+iz;float sn=sinf(d*3.141592654f);
    float drop=(.5f*(1+cosf(d*3.141592654f))-crestSum/ringSum*sn*sn)*fminf(amplitude*.90f,180);
    b->h[i]=fmaxf(-256,fminf(256,b->h[i]+drop));b->old[i]=b->h[i];}
  }
}
void WaterDrop(int x,int z,int amplitude){
 int accepted=0;
 for(int n=0;n<WATER_BODIES;n++){
  WaterBody*b=&bodies[n];if(!b->used||x<b->x0||x>b->x1||z<b->z0||z>b->z1||clockTime-b->seen>10)continue;
  applyDrop(b,x,z,amplitude);accepted=1;
 }
 if(!accepted){PendingDrop*d=&pending[pendingNext++%64];*d=(PendingDrop){1,x,z,amplitude,clockTime};}
}
int WaterObjectRipple(const void*object,int x,int z,int y){
 int slot=-1;double oldest=clockTime;
 (void)x;(void)z;
 for(int i=0;i<256;i++){
  Emitter*e=&emitters[i];
  if(e->key==object&&e->y==y){
   double last=e->seen;e->seen=clockTime;
   if(clockTime-last>.30)e->next=clockTime;
   if(clockTime+1e-9<e->next)return 0;
   do{e->next+=.12;}while(e->next<=clockTime);
   return 1;
  }
  if(!e->key){slot=i;oldest=-1;}else if(e->seen<oldest){oldest=e->seen;slot=i;}
 }
 if(slot<0)return 0;
 emitters[slot]=(Emitter){object,y,clockTime,clockTime+.12};return 1;
}

/* Catmull-Rom reconstruction: continuous slope between simulation cells.
   Bilinear height differences exposed the grid in high-contrast caustics. */
static float cubic(float a,float b,float c,float d,float t){return b+.5f*t*(c-a+t*(2*a-5*b+4*c-d+t*(3*(b-c)+d-a)));}
static float cubicDerivative(float a,float b,float c,float d,float t){return .5f*(c-a+2*t*(2*a-5*b+4*c-d)+3*t*t*(3*(b-c)+d-a));}
static float sample(WaterBody*b,float x,float z,float*dx,float*dz){
 float sx=b->sx,sz=b->sz;
 float u=(x-b->x0)*sx,v=(z-b->z0)*sz;
 if(u<0)u=0;else if(u>b->nx-1)u=b->nx-1;
 if(v<0)v=0;else if(v>b->nz-1)v=b->nz-1;
 int ix=(int)u,iz=(int)v;float fx=u-ix,fz=v-iz,a=waveRemainder/65536.0f,rows[4],derivatives[4];
 for(int zc=0;zc<4;zc++){
  float values[4];int zi=iz+zc-1;if(zi<0)zi=0;if(zi>=b->nz)zi=b->nz-1;
  for(int xc=0;xc<4;xc++){int xi=ix+xc-1;if(xi<0)xi=0;if(xi>=b->nx)xi=b->nx-1;int i=xi*WATER_GRID+zi;values[xc]=b->old[i]*(1-a)+b->h[i]*a;}
  rows[zc]=cubic(values[0],values[1],values[2],values[3],fx);
  if(dx)derivatives[zc]=cubicDerivative(values[0],values[1],values[2],values[3],fx)*sx;
 }
 if(dx)*dx=cubic(derivatives[0],derivatives[1],derivatives[2],derivatives[3],fz);
 if(dz)*dz=cubicDerivative(rows[0],rows[1],rows[2],rows[3],fz)*sz;
 return cubic(rows[0],rows[1],rows[2],rows[3],fz);
}
float WaterHeight(float x,float z,int y){
 for(int i=0;i<WATER_BODIES;i++){WaterBody*b=&bodies[i];if(b->used&&b->y==y&&x>=b->x0&&x<=b->x1&&z>=b->z0&&z<=b->z1)return sample(b,x,z,0,0);}
 return 0;
}
void WaterGradient(float x,float z,int y,float*dx,float*dz){
 *dx=*dz=0;
 for(int i=0;i<WATER_BODIES;i++){WaterBody*b=&bodies[i];if(b->used&&b->y==y&&x>=b->x0&&x<=b->x1&&z>=b->z0&&z<=b->z1){sample(b,x,z,dx,dz);return;}}
}


/* Separable reconstruction of the identical sample() polynomial. A horizontal
 * row is evaluated once and shared by adjacent output rows. No downsampling,
 * changed timestep, approximated normals or vendor-specific instructions. */
void WaterGradientField(float x0,float x1,float z0,float z1,int y,int size,float *field){
 WaterBody *body=0;
 for(int n=0;n<WATER_BODIES;n++){
  WaterBody*b=&bodies[n];if(!b->used||b->y!=y||x1<b->x0||x0>b->x1||z1<b->z0||z0>b->z1)continue;
  if(x0>=b->x0&&x1<=b->x1&&z0>=b->z0&&z1<=b->z1)body=b;
  break; /* Preserve first-body priority even for overlapping surfaces. */
 }
 if(!body||size>256){
  for(int z=0;z<size;z++)for(int x=0;x<size;x++){
   int i=(z*size+x)*4;WaterGradient(x0+(x+.5f)*(x1-x0)/size,z0+(z+.5f)*(z1-z0)/size,y,&field[i],&field[i+1]);
   field[i+2]=0;field[i+3]=1;
  }return;
 }
 WaterBody*b=body;float alpha=waveRemainder/65536.0f;
 static float horizontal[WATER_GRID][256],derivative[WATER_GRID][256];
 unsigned char ready[WATER_GRID]={0};int ix[256];float fx[256];
 for(int x=0;x<size;x++){
  float u=(x0+(x+.5f)*(x1-x0)/size-b->x0)*b->sx;
  if(u<0)u=0;else if(u>b->nx-1)u=b->nx-1;ix[x]=(int)u;fx[x]=u-ix[x];
 }
 for(int z=0;z<size;z++){
  float v=(z0+(z+.5f)*(z1-z0)/size-b->z0)*b->sz;
  if(v<0)v=0;else if(v>b->nz-1)v=b->nz-1;int iz=(int)v;float fz=v-iz;int zi[4];
  for(int k=0;k<4;k++){
   int row=iz+k-1;if(row<0)row=0;if(row>=b->nz)row=b->nz-1;zi[k]=row;
   if(ready[row])continue;ready[row]=1;
   for(int x=0;x<size;x++){
    float a[4];for(int j=0;j<4;j++){
     int col=ix[x]+j-1;if(col<0)col=0;if(col>=b->nx)col=b->nx-1;
     int i=col*WATER_GRID+row;a[j]=b->old[i]*(1-alpha)+b->h[i]*alpha;
    }
    horizontal[row][x]=cubic(a[0],a[1],a[2],a[3],fx[x]);
    derivative[row][x]=cubicDerivative(a[0],a[1],a[2],a[3],fx[x])*b->sx;
   }
  }
  for(int x=0;x<size;x++){
   int i=(z*size+x)*4;
   field[i]=cubic(derivative[zi[0]][x],derivative[zi[1]][x],derivative[zi[2]][x],derivative[zi[3]][x],fz);
   field[i+1]=cubicDerivative(horizontal[zi[0]][x],horizontal[zi[1]][x],horizontal[zi[2]][x],horizontal[zi[3]][x],fz)*b->sz;
   field[i+2]=0;field[i+3]=1;
  }
 }
}

/* The same continuous ambient phase as the approved optical normal field.
 * Contact/rain events enter WaterDrop through the existing AddRipple bridge. */
float WaterSurfaceHeight(float x,float z,int y){
 float phase=(float)WaterTime()*2048.0f,scale=6.283185307f/4096.0f;
 float offset=32*sinf((x+z+phase)*scale)+16*sinf((x-z*2+phase*.5f)*scale)+WaterHeight(x,z,y);
 return fmaxf(-256,fminf(256,offset));
}
