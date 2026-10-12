#ifndef TYRARGO_ALIEN_MOTION_H
#define TYRARGO_ALIEN_MOTION_H
#include <math.h>
#include <string.h>
typedef struct {
 double distance;
 int sampleTime,stillTime,moving;
 int routeTime,routeX,routeY,routeZ,routeSample,recoveryTime;
 int aftermath,pace,paceTimer,corpsePhase,corpseRetry,feedTime,feedStall,feedBestDistance,bitten;
 int launchLocked,launchAirborne,launchTime;
 int feedTimer,feedSound;
 char corpseName[8]; /* SB_NAME_LENGTH; binary name, not a C string */
} ALIEN_MOTION_RUNTIME;
static void AlienMotionReset(ALIEN_MOTION_RUNTIME *r)
{ memset(r,0,sizeof(*r));r->moving=1;r->pace=65536; }
static void AlienMotionObserve(ALIEN_MOTION_RUNTIME *r,double distance,int dt)
{
 if(dt<=0)return;
 r->distance+=distance;r->sampleTime+=dt;
 if(r->sampleTime>=65536/10){
  double speed=r->distance*65536.0/r->sampleTime;
  if(speed>180.0){r->moving=1;r->stillTime=0;}
  else if(speed<80.0){r->stillTime+=r->sampleTime;if(r->stillTime>=65536/5)r->moving=0;}
  else r->stillTime=0;
  r->sampleTime=0;r->distance=0;
 }
}
/* Runtime only: no change to ALIEN_SAVE_BLOCK. A stationary navigation intent
 * may otherwise persist forever when waypoint output or support projection is zero. */
static int AlienRouteStalled(ALIEN_MOTION_RUNTIME *r,int x,int y,int z,int dt,int navigating)
{
 double dx,dy,dz;
 if(dt<=0)return 0;
 if(r->recoveryTime>0){r->recoveryTime-=dt;if(r->recoveryTime<0)r->recoveryTime=0;r->routeSample=0;return 0;}
 if(!navigating){r->routeSample=0;r->routeTime=0;return 0;}
 if(!r->routeSample){r->routeX=x;r->routeY=y;r->routeZ=z;r->routeTime=0;r->routeSample=1;}
 dx=(double)x-r->routeX;dy=(double)y-r->routeY;dz=(double)z-r->routeZ;
 if(dx*dx+dy*dy+dz*dz>200.0*200.0){r->routeX=x;r->routeY=y;r->routeZ=z;r->routeTime=0;return 0;}
 r->routeTime+=dt;
 if(r->routeTime<3*65536/2)return 0;
 r->routeSample=0;r->routeTime=0;r->recoveryTime=65536;return 1;
}
/* An impulse is a launch event, never a per-frame acceleration.
 * Re-arm after actual flight/landing, or a blocked launch at the support. */
static void AlienLaunchObserve(ALIEN_MOTION_RUNTIME *r,int contact,int departing,int dt)
{
 if(!r->launchLocked || dt<=0)return;
 if(r->launchTime<65536)r->launchTime+=dt;
 if(!contact)r->launchAirborne=1;
 else if(!departing && (r->launchAirborne || r->launchTime>=65536/4)){
  r->launchLocked=0;r->launchAirborne=0;r->launchTime=0;
 }
}
static int AlienLaunchBegin(ALIEN_MOTION_RUNTIME *r,int contact)
{
 if(r->launchLocked || !contact)return 0;
 r->launchLocked=1;r->launchAirborne=0;r->launchTime=0;return 1;
}
/* Sequence flags are transient; consume once per feeding cycle. Some models
 * lack a bite marker, so the mid-cycle fallback cannot emit every frame. */
static int AlienFeedSoundEvent(ALIEN_MOTION_RUNTIME *r,int timer,unsigned int flags,int tweening)
{
 if(tweening)return 0;
 if(timer<r->feedTimer)r->feedSound=0;
 r->feedTimer=timer;
 if(!r->feedSound && (flags || timer>=32768)){
  r->feedSound=1;return 1;
 }
 return 0;
}
/* Orthonormalise a continuous yaw step in the current support plane.
 * The legacy matrix is rounded only once, after all floating-point work. */
static int AlienTurnBasis(double b[9],double vx,double vy,double vz,double maxAngle)
{
 double y[3]={b[3],b[4],b[5]},z[3],x[3],m,localX,localZ,angle,c,s;
 int i,aligned;
 m=sqrt(y[0]*y[0]+y[1]*y[1]+y[2]*y[2]);if(m<1e-9)return 1;
 for(i=0;i<3;i++)y[i]/=m;
 localX=vx*b[0]+vy*b[1]+vz*b[2];localZ=vx*b[6]+vy*b[7]+vz*b[8];
 if(fabs(localX)+fabs(localZ)<1e-9)return 1;
 angle=atan2(localX,localZ);aligned=fabs(angle)<=maxAngle;
 if(angle>maxAngle)angle=maxAngle;
 if(angle< -maxAngle)angle= -maxAngle;
 c=cos(angle);s=sin(angle);
 for(i=0;i<3;i++)z[i]=s*b[i]+c*b[6+i];
 m=z[0]*y[0]+z[1]*y[1]+z[2]*y[2];for(i=0;i<3;i++)z[i]-=m*y[i];
 m=sqrt(z[0]*z[0]+z[1]*z[1]+z[2]*z[2]);if(m<1e-9)return 1;
 for(i=0;i<3;i++)z[i]/=m;
 x[0]=y[1]*z[2]-y[2]*z[1];x[1]=y[2]*z[0]-y[0]*z[2];x[2]=y[0]*z[1]-y[1]*z[0];
 for(i=0;i<3;i++){b[i]=x[i];b[3+i]=y[i];b[6+i]=z[i];}return aligned;
}
#endif
