#ifndef TYRARGO_NPC_PRECISION_H
#define TYRARGO_NPC_PRECISION_H
#include <math.h>
#include <string.h>
typedef struct {
 double distance, speed, remainder[3], travelRemainder[2];
 int sampleTime, valid, stillTime, moving, lastVelocity[3];
 int routeTime, routeX, routeZ, routeValid, recoveryTime, recovering;
 int strafeTime, strafeSide;
} NPC_MOTION_RUNTIME;
static void NPCMotionReset(NPC_MOTION_RUNTIME *r)
{memset(r,0,sizeof(*r));r->moving=1;}
/* Observe resolved travel, not requested velocity. A tenth-second sample
 * prevents integer millimetres from alternating stand/run at high FPS. */
static void NPCMotionObserve(NPC_MOTION_RUNTIME *r,double dx,double dz,int dt)
{
 if(dt<=0)return;
 r->distance+=sqrt(dx*dx+dz*dz);r->sampleTime+=dt;
 if(r->sampleTime>=65536/10){
  r->speed=r->distance*65536.0/r->sampleTime;r->valid=1;
  if(r->speed>180){r->moving=1;r->stillTime=0;}
  else if(r->speed<80){r->stillTime+=r->sampleTime;if(r->stillTime>=65536/5)r->moving=0;}
  else r->stillTime=0;
  r->distance=0;r->sampleTime=0;
 }
}
static void NPCAccelerate(NPC_MOTION_RUNTIME *r,int velocity[3],const int target[3],double acceleration,int dt)
{
 double v[3],delta[3],mag=0,step=acceleration*dt/65536.0;int i;
 if(dt<=0)return;
 for(i=0;i<3;i++){
  if(velocity[i]!=r->lastVelocity[i])r->remainder[i]=0;
  v[i]=velocity[i]+r->remainder[i];delta[i]=target[i]-v[i];mag+=delta[i]*delta[i];
 }
 mag=sqrt(mag);if(mag>step && mag>0)step/=mag;else step=1;
 for(i=0;i<3;i++){
  v[i]+=delta[i]*step;velocity[i]=(int)lround(v[i]);
  r->remainder[i]=v[i]-velocity[i];r->lastVelocity[i]=velocity[i];
 }
}
/* Integrate horizontal travel with an unbiased fractional carry. The remainder
 * stays below half a millimetre; collision resolution still owns final position. */
static int NPCTravel(NPC_MOTION_RUNTIME *r,int axis,int velocity,int dt)
{
 double travel;int step;
 if(dt<=0)return 0;
 if(!velocity){r->travelRemainder[axis]=0;return 0;}
 travel=velocity*(dt/65536.0)+r->travelRemainder[axis];step=(int)lround(travel);
 r->travelRemainder[axis]=travel-step;return step;
}
/* Positive root of |relativePosition + targetVelocity*t| = projectileSpeed*t.
 * No solution or a very long flight returns zero (direct aim). */
static double NPCInterceptTime(const double p[3],const double v[3],double speed,double limit)
{
 double a=-speed*speed,b=0,c=0,d,t=0,t2;int i;
 for(i=0;i<3;i++){a+=v[i]*v[i];b+=2*p[i]*v[i];c+=p[i]*p[i];}
 if(c<1 || speed<=0)return 0;
 if(fabs(a)<1e-9){if(b<0)t=-c/b;}
 else {d=b*b-4*a*c;if(d>=0){d=sqrt(d);t=(-b-d)/(2*a);t2=(-b+d)/(2*a);if(t<=0 || (t2>0 && t2<t))t=t2;}}
 return t>0 && t<=limit?t:0;
}
static int NPCPickupWithinReach(double dx,double dy,double dz,double top,double bottom)
{return dx*dx+dz*dz<=950.0*950.0 && dy>=top-200.0 && dy<=bottom+150.0;}
#endif
