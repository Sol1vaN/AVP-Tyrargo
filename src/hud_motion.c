#include "shake_params.h"
#include "hud_motion.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
#define HUD_REFERENCE_HZ 60.0
#define HUD_PC_DEPTH H(PLANE_DEPTH)
static HUD_QUAT identity(void){HUD_QUAT q={1,0,0,0};return q;}
static HUD_QUAT conjugate(HUD_QUAT q){q.x=-q.x;q.y=-q.y;q.z=-q.z;return q;}
static HUD_QUAT normalize(HUD_QUAT q){
 double n=sqrt(q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z);
 if(!isfinite(n)||n<1e-12)return identity();
 q.w/=n;q.x/=n;q.y/=n;q.z/=n;
 if(q.w<0){q.w=-q.w;q.x=-q.x;q.y=-q.y;q.z=-q.z;}
 return q;
}
static HUD_QUAT multiply(HUD_QUAT a,HUD_QUAT b){
 HUD_QUAT q={a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
 a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
 a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
 a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};return normalize(q);
}
static HUD_QUAT blend(HUD_QUAT a,HUD_QUAT b,double t){
 double dot=a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;
 if(dot<0){b.w=-b.w;b.x=-b.x;b.y=-b.y;b.z=-b.z;dot=-dot;}
 double x=1-t,y=t;
 if(dot<.9995){double angle=acos(fmax(-1,fmin(1,dot))),den=sin(angle);x=sin((1-t)*angle)/den;y=sin(t*angle)/den;}
 HUD_QUAT q={a.w*x+b.w*y,a.x*x+b.x*y,a.y*x+b.y*y,a.z*x+b.z*y};return normalize(q);
}
HUD_QUAT HudMotionAngles(double p,double y,double r){
 HUD_QUAT pitch={cos(p*.5),sin(p*.5),0,0},yaw={cos(y*.5),0,sin(y*.5),0},roll={cos(r*.5),0,0,sin(r*.5)};
 return multiply(multiply(yaw,pitch),roll);
}
HUD_QUAT HudMotionOrientation(const double r[3][3]){
 double m[4]={1+r[0][0]+r[1][1]+r[2][2],1+r[0][0]-r[1][1]-r[2][2],1-r[0][0]+r[1][1]-r[2][2],1-r[0][0]-r[1][1]+r[2][2]};
 int k=0;for(int i=1;i<4;i++)if(m[i]>m[k])k=i;
 double a=sqrt(fmax(0,m[k]))*.5,b=a>1e-12?.25/a:0;
 HUD_QUAT q;
 if(k==0)q=(HUD_QUAT){a,(r[2][1]-r[1][2])*b,(r[0][2]-r[2][0])*b,(r[1][0]-r[0][1])*b};
 else if(k==1)q=(HUD_QUAT){(r[2][1]-r[1][2])*b,a,(r[0][1]+r[1][0])*b,(r[0][2]+r[2][0])*b};
 else if(k==2)q=(HUD_QUAT){(r[0][2]-r[2][0])*b,(r[0][1]+r[1][0])*b,a,(r[1][2]+r[2][1])*b};
 else q=(HUD_QUAT){(r[1][0]-r[0][1])*b,(r[0][2]+r[2][0])*b,(r[1][2]+r[2][1])*b,a};
 return normalize(q);
}
void HudMotionReset(HUD_MOTION_STATE *s){memset(s,0,sizeof(*s));s->lastView=s->history=s->relative=s->previousRelative=s->renderRelative=identity();}
void HudMotionEvent(HUD_MOTION_STATE *s,int kind,double x,double y,double roll){
 if(kind<0||kind>2)return;
 double impulse[3]={x,y,roll};
 for(int j=0;j<3;j++)if(isfinite(impulse[j]))
  s->eventV[kind][j]+=fmax(-1,fmin(1,impulse[j]));
}
void HudMotionStepScaled(HUD_MOTION_STATE *s,double gain){
 if(isfinite(gain))s->stepPending-=H(STEP_STRENGTH)*1.01175*fmax(0,fmin(2,gain));
}
void HudMotionStep(HUD_MOTION_STATE *s){HudMotionStepScaled(s,1);}
void HudMotionRunStep(HUD_MOTION_STATE *s,double gain){
 if(!isfinite(gain))return;
 double before=s->stepPending;HudMotionStepScaled(s,gain);
 s->runPending+=s->stepPending-before;
}
void HudMotionWalkStep(HUD_MOTION_STATE *s,double gain){
 if(!isfinite(gain))return;
 double before=s->stepPending;HudMotionStepScaled(s,gain);
 s->walkPending+=s->stepPending-before;
}
/* These authored profiles adapt CryEngine's per-shot recoil/hints and
 * recovery mechanism. The reference tree does NOT include the SCAR/Feline
 * XML or animation curves; these values are not claimed as original data. */
void HudMotionWeaponShot(HUD_MOTION_STATE *s,int weapon,int count){
 const double rifleHints[7]={H(RIFLE_HINT0),H(RIFLE_HINT1),H(RIFLE_HINT2),H(RIFLE_HINT3),H(RIFLE_HINT4),H(RIFLE_HINT5),H(RIFLE_HINT6)};
 const double smartHints[8]={H(SMART_HINT0),H(SMART_HINT1),H(SMART_HINT2),H(SMART_HINT3),H(SMART_HINT4),H(SMART_HINT5),H(SMART_HINT6),H(SMART_HINT7)};
 if(count<1)return;if(count>64)count=64;
 for(int n=0;n<count;n++){
  if(weapon==2){
   double side=rifleHints[s->weaponShotSerial[0]++%7];
   s->weaponV[0][0]+=H(RIFLE_X)*side;s->weaponV[0][1]-=H(RIFLE_Y);s->weaponV[0][2]+=H(RIFLE_ROLL)*side;
  }else if(weapon==3){
   double side=smartHints[s->weaponShotSerial[1]++%8];
   s->weaponV[1][0]+=H(SMART_X)*side;s->weaponV[1][1]-=H(SMART_Y);s->weaponV[1][2]+=H(SMART_ROLL)*side;
  }else if(weapon==8){
   /* Launch kick and slower shoulder/torsion recovery have separate poles.
    * Both start continuously; the native disc recoil phase supplies timing. */
   double side=(s->weaponShotSerial[2]++&1)?-H(DISC_ALTERNATE):1;
   s->weaponV[2][0]+=H(DISC_X)*side;s->weaponV[2][1]-=H(DISC_Y);s->weaponV[2][2]-=H(DISC_ROLL)*side;
   s->weaponV[3][0]-=H(DISC_TAIL_X)*side;s->weaponV[3][1]+=H(DISC_TAIL_Y);s->weaponV[3][2]+=H(DISC_TAIL_ROLL)*side;
  }
 }
}
void HudMotionFirePose(HUD_MOTION_STATE *s,const double pose[3]){
 for(int j=0;j<3;j++)s->fireTarget[j]=isfinite(pose[j])?H(FIRE_LIMIT)*tanh(pose[j]/H(FIRE_LIMIT)):0;
}
void HudMotionFlame(HUD_MOTION_STATE *s,int active){s->flameActive=active!=0;}
static void damped(double *x,double *v,double frequency,double damping,double dt){
 double a=frequency*damping,w=frequency*sqrt(1-damping*damping);
 double e=exp(-a*dt),c=cos(w*dt),sn=sin(w*dt),old=*x,velocity=*v;
 *x=e*(old*c+(velocity+a*old)*sn/w);
 *v=e*(velocity*c-(a*velocity+frequency*frequency*old)*sn/w);
}
static void critical(double *x,double *v,double target,double frequency,double dt){
 double d=*x-target,c=*v+frequency*d,e=exp(-frequency*dt);
 *x=target+(d+c*dt)*e;*v=(*v-frequency*c*dt)*e;
}
/* Deterministic 1D gradient noise with quintic interpolation (C2 continuous).
 * No gameplay RNG calls and no random value chosen once per rendered frame. */
static double gradient(uint32_t lattice,uint32_t seed){
 uint32_t v=lattice^seed;v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;v^=v>>16;
 return (v&1u)?1:-1;
}
static double coherent(double t,uint32_t seed){
 double lattice=floor(t),f=t-lattice,u=f*f*f*(f*(f*6-15)+10);
 uint32_t i=(uint32_t)(uint64_t)lattice;
 double a=gradient(i,seed)*f,b=gradient(i+1u,seed)*(f-1);
 return 2*(a+(b-a)*u);
}
static void weaponAdvance(HUD_MOTION_STATE *s,double dt){
 const double frequency[4]={H(RIFLE_FREQUENCY),H(SMART_FREQUENCY),H(DISC_FREQUENCY),H(DISC_TAIL_FREQUENCY)},damping[4]={H(RIFLE_DAMPING),H(SMART_DAMPING),H(DISC_DAMPING),H(DISC_TAIL_DAMPING)};
 for(int k=0;k<4;k++)for(int j=0;j<3;j++)damped(&s->weaponX[k][j],&s->weaponV[k][j],frequency[k],damping[k],dt);
 for(int j=0;j<3;j++)critical(&s->fireX[j],&s->fireV[j],s->fireTarget[j],H(FIRE_RESPONSE),dt);
 /* Integrate only the continuous noise drive with bounded small steps.
  * Event recovery above remains analytic, independent of frame partition. */
 int steps=(int)ceil(dt*240);double step=dt/steps;
 for(int k=0;k<steps;k++){
  critical(&s->flameLevel,&s->flameVelocity,s->flameActive?1:0,s->flameActive?H(FLAME_ATTACK):H(FLAME_RELEASE),step);
  double time=s->flameTime+step*.5;
  for(int j=0;j<3;j++){
   uint32_t seed=0x5a17u+j*0x93a1u;
   double phase=time+.371+j*11.37;
   double noise=H(FLAME_WEIGHT0)*coherent(phase*H(FLAME_RATE0),seed)+H(FLAME_WEIGHT1)*coherent(phase*H(FLAME_RATE1),seed+1)+H(FLAME_WEIGHT2)*coherent(phase*H(FLAME_RATE2),seed+2);
   double target=noise*s->flameLevel*(j==0?H(FLAME_X):j==1?H(FLAME_Y):H(FLAME_ROLL));
   critical(&s->flameX[j],&s->flameV[j],target,H(FLAME_RESPONSE),step);
  }
  s->flameTime+=step;
 }
}
/* Analytic underdamped recovery: bounded, continuous onset and a small
 * settling oscillation. These are AvP presentation coefficients, not copied
 * Crysis weapon data. Recoil.cpp/HitRecoilGameEffect motivate separate events. */
static void strideAdvance(double *position,double *impulse,double *pending,double dt){
 double a=-60*log(H(STEP_DECAY)),k=1/H(STEP_EASE),d=exp(-a*dt),e=exp(-k*dt),b=k-a;
 double source=*pending;
 *position=d*(*position+(60*H(STEP_DECAY))**impulse*dt)
  +(60*H(STEP_DECAY))*k*source*(dt*d/b-(d-e)/(b*b));
 *impulse=d**impulse+k*source*(d-e)/b;
 *pending*=e;
}
static void eventAdvance(HUD_MOTION_STATE *s,double dt){
 const double frequency[3]={H(SHOT_FREQUENCY),H(LAND_FREQUENCY),H(DAMAGE_FREQUENCY)},damping[3]={H(SHOT_DAMPING),H(LAND_DAMPING),H(DAMAGE_DAMPING)};
 for(int k=0;k<3;k++)for(int j=0;j<3;j++){
  double a=frequency[k]*damping[k],w=frequency[k]*sqrt(1-damping[k]*damping[k]);
  double e=exp(-a*dt),c=cos(w*dt),sn=sin(w*dt),x=s->eventX[k][j],v=s->eventV[k][j];
  s->eventX[k][j]=e*(x*c+(v+a*x)*sn/w);
  s->eventV[k][j]=e*(v*c-(a*v+frequency[k]*frequency[k]*x)*sn/w);
 }
 /* Exact convolution: 22 ms input easing ahead of the original .9/.9
  * footstep. Peak normalization compensates the small easing attenuation. */
 strideAdvance(&s->stepPosition,&s->stepImpulse,&s->stepPending,dt);
 strideAdvance(&s->runPosition,&s->runImpulse,&s->runPending,dt);
 strideAdvance(&s->walkPosition,&s->walkImpulse,&s->walkPending,dt);
}
static void pitchRecovery(HUD_MOTION_STATE *s,double target,double dt){
 double frequency=H(MOUSE_PITCH_RESPONSE),speed=(target-s->pitchInput)/dt;
 double x=s->pitchPosition-s->pitchInput+2*speed/frequency;
 double v=s->pitchVelocity-speed,c=v+frequency*x,e=exp(-frequency*dt);
 s->pitchPosition=target-2*speed/frequency+(x+c*dt)*e;
 s->pitchVelocity=speed+(v-frequency*c*dt)*e;
 s->pitchInput=target;
}
void HudMotionUpdate(HUD_MOTION_STATE *s,double dt,HUD_QUAT view,const double kick[3],int mode){
 int j;
 if(mode<1||mode>3||!isfinite(dt)||dt<0||dt>.25){HudMotionReset(s);return;}
 view=normalize(view);
 if(!s->initialized||s->mode!=mode){HudMotionReset(s);s->initialized=1;s->mode=mode;s->history=s->lastView=view;}
 if(dt==0)return;
 eventAdvance(s,dt);
 weaponAdvance(s,dt);
 /* The reference orientation recurrence has a negative feedback pole. Run it
  * at a fixed presentation cadence, interpolating sampled camera orientation
  * and the RELATIVE result, never filtering the gameplay camera or aiming. */
 {
  double tick=1/HUD_REFERENCE_HZ,elapsed=tick-s->remainder;
  while(elapsed<=dt+1e-12){
   HUD_QUAT current=blend(s->lastView,view,fmin(1,elapsed/dt));
   /* Cry_Quat operator /(q,p) is inverse(p)*q (local difference). */
   HUD_QUAT difference=multiply(conjugate(current),s->history);
   difference.w*=H(MOUSE_WEIGHT);difference=normalize(difference);
   s->history=multiply(current,conjugate(difference));
   s->previousRelative=s->relative;
   s->relative=multiply(conjugate(current),s->history);
   elapsed+=tick;
  }
  s->remainder=fmod(s->remainder+dt,tick);
  if(s->remainder<1e-10||tick-s->remainder<1e-10)s->remainder=0;
  s->renderRelative=blend(s->previousRelative,s->relative,s->remainder/tick);
  s->lastView=view;
  /* Only HUD pitch is eased. Keep yaw, roll and the input camera unchanged. */
  double target=2*atan2(s->renderRelative.x,s->renderRelative.w);
  pitchRecovery(s,target,dt);
  s->renderRelative.x=s->renderRelative.w*tan(s->pitchPosition*.5);
  s->renderRelative=normalize(s->renderRelative);
 }
 /* Exact fractional power of the reference's two-stage damping matrix:
  * i'=d*i, p'=d*(p+i'). At 60 Hz this is identical to its .9/.9 update.
  * Kicks are events/displacements, not an amplitude added once per frame. */
 {
  double n=dt*HUD_REFERENCE_HZ,d=pow(H(GESTURE_DECAY),n);
  for(j=0;j<3;j++){
   double add=isfinite(kick[j])?fmax(-.05,fmin(.05,kick[j])):0;
   s->impulse[j]+=add;
   s->position[j]=d*(s->position[j]+n*H(GESTURE_DECAY)*s->impulse[j]);
   s->impulse[j]*=d;
  }
 }
}
static void point(const double h[9],double x,double y,double *px,double *py){
 double z=h[6]*x+h[7]*y+h[8];*px=(h[0]*x+h[1]*y+h[2])/z;*py=(h[3]*x+h[4]*y+h[5])/z;
}
void HudMotionMatrix(const HUD_MOTION_STATE *s,int width,int height,int ox,int oy,double h[9]){
 HudMotionMatrixIntensity(s,width,height,ox,oy,h,1);
}
void HudMotionMatrixIntensity(const HUD_MOTION_STATE *s,int width,int height,int ox,int oy,double h[9],double intensity){
 double cx=ox+width*.5,cy=oy+height*.5,focal=height*H(PLANE_FOCAL),d=HUD_PC_DEPTH;
 HUD_QUAT q=s->renderRelative;
 double angle=2*acos(fmax(-1,fmin(1,q.w)));
 int i,j;
 memset(h,0,9*sizeof(*h));h[0]=h[4]=h[8]=1;
 if(!s->initialized||width<1||height<1)return;
 /* Preserve the reference in ordinary motion. Extreme flicks are smoothly
  * bounded so an entire edge cannot vanish off screen. */
 if(angle>1e-9)q=blend(identity(),q,H(MOUSE_LIMIT)*tanh(angle/H(MOUSE_LIMIT))/angle);
 /* Additional 35% yaw reduction relative to probe 187; pitch is unchanged. */
 q.y*=H(MOUSE_YAW);q.x*=H(MOUSE_PITCH);q.z*=H(MOUSE_ROLL);q=normalize(q);
 if(intensity!=1)q=blend(identity(),q,intensity);
 double x=q.x,y=q.y,z=q.z,w=q.w;
 double r[3][3]={{1-2*(y*y+z*z),2*(x*y-w*z),2*(x*z+w*y)},
 {2*(x*y+w*z),1-2*(x*x+z*z),2*(y*z-w*x)},
 {2*(x*z-w*y),2*(y*z+w*x),1-2*(x*x+y*y)}};
 double translation[3];for(i=0;i<3;i++)translation[i]=intensity*H(PLANE_LIMIT)*tanh((s->position[i]+(i? s->stepPosition:0))/H(PLANE_LIMIT));
 /* Map the already anchored native HUD onto a camera-local 3D plane.
  * Output homogeneous coordinates retain perspective-correct texture sampling. */
 for(j=0;j<3;j++){
  double a=r[0][j],b=r[1][j],c=r[2][j];
  if(j<2){h[j]=(focal*a+cx*c)/focal;h[3+j]=(focal*b+cy*c)/focal;h[6+j]=c/focal;}
 }
 h[2]=focal*(r[0][2]+translation[0]/d)+cx*(r[2][2]+translation[2]/d)-h[0]*cx-h[1]*cy;
 h[5]=focal*(r[1][2]+translation[1]/d)+cy*(r[2][2]+translation[2]/d)-h[3]*cx-h[4]*cy;
 h[8]=r[2][2]+translation[2]/d-h[6]*cx-h[7]*cy;
 /* Native AvP panels reach the screen edges; CryEngine's meshes have a safe
  * area. Fit this dynamic plane about its existing centre, preserving the
  * chosen anchors and all internal spacing, with exact identity at rest. */
 double extentX=width*.5,extentY=height*.5;
 for(i=0;i<4;i++){double px,py;point(h,ox+(i&1?width:0),oy+(i&2?height:0),&px,&py);extentX=fmax(extentX,fabs(px-cx));extentY=fmax(extentY,fabs(py-cy));}
 double fit=fmin(width*.5/extentX,height*.5/extentY);
 for(j=0;j<3;j++){h[j]=fit*h[j]+(1-fit)*cx*h[6+j];h[3+j]=fit*h[3+j]+(1-fit)*cy*h[6+j];}
 /* Shot/impact shake is a rigid screen-plane gesture after safe fitting.
  * It cannot become the zoom-out artifact caused by fitting each impulse.
  * No perspective-depth kick is generated by firing. */
 double event[3]={0};for(i=0;i<3;i++)for(j=0;j<3;j++)event[j]+=s->eventX[i][j];
 for(i=0;i<4;i++)for(j=0;j<3;j++)event[j]+=s->weaponX[i][j];
 for(j=0;j<3;j++)event[j]+=s->fireX[j]+s->flameX[j];
 double tx=intensity*height*H(LIMIT_X)*tanh(event[0]/H(LIMIT_X)),ty=intensity*height*H(LIMIT_Y)*tanh(event[1]/H(LIMIT_Y));
 double roll=intensity*H(LIMIT_ROLL)*tanh(event[2]/H(LIMIT_ROLL)),c=cos(roll),sn=sin(roll);
 for(j=0;j<3;j++){
  double x=h[j]-cx*h[6+j],y=h[3+j]-cy*h[6+j];
  h[j]=c*x-sn*y+(cx+tx)*h[6+j];
  h[3+j]=sn*x+c*y+(cy+ty)*h[6+j];
 }

}

/* Each reference removes only its own stride history. Running and walking
 * keep independent decay tails across transitions; other events stay intact.
 * The lower-panel mask is applied later, preserving the upper HUD. */
static double strideCompensation(const HUD_MOTION_STATE *s,double stride,double gain,
 int width,int height,int ox,int oy,const double h[9],double intensity){
 if(stride==0||gain==0)return 0;
 HUD_MOTION_STATE reference=*s;double ref[9],x,y,rx,ry;
 reference.stepPosition-=stride;
 HudMotionMatrixIntensity(&reference,width,height,ox,oy,ref,intensity);
 point(h,ox+width*.5,oy+height,&x,&y);point(ref,ox+width*.5,oy+height,&rx,&ry);
 return gain*fmax(0,ry-y);
}
double HudMotionBottomCompensation(const HUD_MOTION_STATE *s,int width,int height,int ox,int oy,const double h[9]){
 return HudMotionBottomCompensationIntensity(s,width,height,ox,oy,h,1);
}
double HudMotionBottomCompensationIntensity(const HUD_MOTION_STATE *s,int width,int height,int ox,int oy,const double h[9],double intensity){
 if(!s->initialized||width<1||height<1)return 0;
 return strideCompensation(s,s->runPosition,H(RUN_BOTTOM_COMPENSATION),width,height,ox,oy,h,intensity)
       +strideCompensation(s,s->walkPosition,H(WALK_BOTTOM_COMPENSATION),width,height,ox,oy,h,intensity);
}
float HudMotionTransformPoint(const double h[9],double correction,int top,int height,float *x,float *y){
 double px=*x,py=*y,w=h[6]*px+h[7]*py+h[8];
 if(!isfinite(w)||w<.1)return 1;
 *x=(float)((h[0]*px+h[1]*py+h[2])/w);
 double weight=height>0?fmax(0,fmin(1,((py-top)/height-.5)*4)):0;
 weight=weight*weight*(3-2*weight);
 *y=(float)((h[3]*px+h[4]*py+h[5])/w+correction*weight);
 return (float)w;
}
