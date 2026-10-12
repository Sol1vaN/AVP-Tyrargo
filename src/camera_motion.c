#include "camera_motion.h"
#include "motion_params.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
int MarineCameraMode=7;
int HUDShakeIntensity=50;
int HUDSwayIntensity=50,HUDBobbingIntensity=50;
/* UI 1/50/100 maps to authored intensity 15/50/75%; gain is relative
 * to the user's calibrated source baseline, never to spring frequency. */
double CameraMotionIntensityGain(int value){
 if(value<1)value=1;if(value>100)value=100;
 return value<=50 ? .3+.7*(value-1)/49.0 : 1.0+.5*(value-50)/50.0;
}
double CameraBobbingIntensityGain(int value){
 if(value<1)value=1;if(value>100)value=100;
 return value<=50 ? .3+.7*(value-1)/49.0 : 1.0+.3*(value-50)/50.0;
}
/* Starting from the original combined result preserves exact calibration at
 * 50, including floating point rounding. Scaling never feeds back to springs. */
void CameraMotionCompose(const CAMERA_MOTION_STATE *s,CAMERA_MOTION_OUTPUT *out,int sway,int bobbing){
 double sg=CameraMotionIntensityGain(sway)-1,bg=CameraBobbingIntensityGain(bobbing)-1;
 *out=s->output;
 if(sg==0 && bg==0)return;
 for(int j=0;j<3;j++){
  out->camera[j]+=sg*s->swayOutput.camera[j]+bg*s->bobbingOutput.camera[j];
  out->weapon[j]+=sg*s->swayOutput.weapon[j]+bg*s->bobbingOutput.weapon[j];
  out->weaponAngle[j]+=sg*s->swayOutput.weaponAngle[j]+bg*s->bobbingOutput.weaponAngle[j];
 }
 out->cameraPitch+=sg*s->swayOutput.cameraPitch+bg*s->bobbingOutput.cameraPitch;
 out->cameraRoll+=sg*s->swayOutput.cameraRoll+bg*s->bobbingOutput.cameraRoll;
}
int CameraMotionFlags(int selection){
 static const int flags[8]={0,1,2,4,3,5,6,7};
 return selection>=0&&selection<8?flags[selection]:0;
}
#define PI 3.14159265358979323846
#define RAD (PI/180.0)
static double clamp(double x,double a,double b){return x<a?a:x>b?b:x;}
/* Bound the target BEFORE integration. A flick approaches the limit smoothly;
   clipping the integrated output would create a flat stop at the extreme. */
static double soft(double x,double limit){return limit>0?limit*tanh(x/limit):0;}
static void spring(MOTION_SPRING*s,double target,double frequency,double dt){
 double d=s->x-target,c=s->v+frequency*d,e=exp(-frequency*dt);
 s->x=target+(d+c*dt)*e;s->v=(s->v-frequency*c*dt)*e;
}
void CameraMotionReset(CAMERA_MOTION_STATE*s){memset(s,0,sizeof(*s));}
unsigned int CameraMotionPack(unsigned int hud,int mode){return(hud&0xffffffu)|((0xc0u+(unsigned)(mode>=0&&mode<=7?mode:0))<<24);}
int CameraMotionUnpack(unsigned int packed){
 unsigned m=packed>>24;
 if(m>=0xc0u&&m<=0xc7u)return (int)(m-0xc0u);
 if(m>=0xb0u&&m<=0xb5u)return m==0xb5u?7:(int)(m-0xb0u);
 /* Legacy Sway+Bobbing keeps its meaning; Shake is explicitly opt-in. */
 if(m>=0xa4u&&m<=0xa7u){static const int old[4]={0,1,2,4};return old[m-0xa4u];}
 return 0;
}
void CameraMotionUpdate(CAMERA_MOTION_STATE*s,const CAMERA_MOTION_INPUT*in,int mode){
 int n,k,j,bob=mode&2,sway=mode&1;double dt=in->dt,step,speed,attenuation;
 if(!in->active||mode<1||mode>7){CameraMotionReset(s);return;}
 if(!isfinite(dt)||dt<0||dt>.25||!isfinite(in->distance)||in->distance>3000){CameraMotionReset(s);return;}
 s->output.step=0;
 if(!s->initialized||s->mode!=mode){
  CameraMotionReset(s);s->initialized=1;s->mode=mode;s->grounded=in->grounded;
  s->eye.x=in->eyeHeight;s->bodyHeight=in->bodyHeight-fmax(0,in->supportHeightDelta);s->weapon=in->weapon;
 }
 if(dt==0)return;
 if(s->weapon!=in->weapon){
  /* Keep presentation springs continuous across weapon changes. */
  s->equipBaseValid=0;
  memset(s->reloadMotion,0,sizeof s->reloadMotion);s->reloadBaseValid=0;s->weapon=in->weapon;
 }
 /* Ground height changes are visual smoothing only. Do not filter ballistic
    motion, and snap on large drops/teleports instead of floating in space. */
 if(bob&&in->grounded&&s->grounded&&in->verticalVelocity>=-100){
  /* Downward support travel must not leave the camera floating above
     the lift. Upward travel deliberately retains the original stair spring:
     its lag gives the knee-flex/inertia response while the lift rises. */
  double delta=in->bodyHeight-s->bodyHeight-fmax(0,in->supportHeightDelta);
  if(fabs(delta)<=M(STAIR_HEIGHT))s->stair.x=clamp(s->stair.x-delta,-M(STAIR_HEIGHT),M(STAIR_HEIGHT));
  else s->stair=(MOTION_SPRING){0};
 }
 s->bodyHeight=in->bodyHeight;
 if(bob&&s->grounded&&!in->grounded&&in->verticalVelocity< -100){s->impactTime[0]=0;s->impactAmplitude[0]=-M(JUMP_IMPULSE)*.06;s->weaponImpactTime[0]=0;s->weaponImpactAmplitude[0]=-M(W_JUMP_IMPULSE)*.06;}
 if(bob&&!s->grounded&&in->grounded){s->impactTime[1]=0;s->impactAmplitude[1]=M(LAND_IMPULSE)*.06;s->weaponImpactTime[1]=0;s->weaponImpactAmplitude[1]=M(W_LAND_IMPULSE)*.06;}
 s->grounded=in->grounded;
 if(!in->reloading){s->reloadBaseValid=0;}
 else if(in->reloadSampleValid&&!s->reloadBaseValid){memcpy(s->reloadBase,in->reloadSample,sizeof s->reloadBase);s->reloadBaseValid=1;}
 s->reloading=in->reloading;
 if(in->equipPhase!=s->equipPhase){s->equipBaseValid=0;s->equipPhase=in->equipPhase;}
 if(!in->equipPhase)s->equipBaseValid=0;
 else if(in->equipSampleValid&&!s->equipBaseValid){memcpy(s->equipBase,in->equipSample,sizeof s->equipBase);s->equipBaseValid=1;}
 speed=in->grounded?clamp(in->distance/dt,0,15000):0;
 attenuation=clamp(in->zoom,M(ZOOM_MIN),1);
 n=(int)ceil(dt*240);step=dt/n;
 for(k=0;k<n;k++){
  double level,oldPhase,stride,increment=0,a;
  spring(&s->speed,speed,M(SPEED_RESPONSE),step);level=clamp(s->speed.x/M(SPEED),0,1);
  a=level*(in->crouched?M(BOB_CROUCH):1);
  spring(&s->eye,in->eyeHeight,M(EYE_RESPONSE),step);
  spring(&s->stance,soft(s->eye.v*M(STANCE_GAIN),M(STANCE_LIMIT)),M(STANCE_RESPONSE),step);
  if(bob||(mode&4)){
   /* A zero-slope pulse separates gesture duration from spring response.
      Jump and landing can overlap without resetting the live camera/weapon springs. */
   double target=0;
   for(j=0;j<2;j++){
    double duration=j?M(LAND_DURATION):M(JUMP_DURATION);
    double phase=(s->impactTime[j]+step*.5)/duration;
    if(phase<1){double wave=sin(PI*phase);target+=s->impactAmplitude[j]*wave*wave;}
    s->impactTime[j]+=step;
    if(s->impactTime[j]>=duration)s->impactAmplitude[j]=0;
   }
   spring(&s->impact,soft(target,M(IMPACT_LIMIT)),M(IMPACT_RESPONSE),step);
   /* Descending treads settle sooner to avoid hovering above the step.
    * Positive lag (upstairs and rising lifts) retains the authored response. */
   spring(&s->stair,0,M(STAIR_RESPONSE)*(s->stair.x<0?1.5:1.0),step);
   oldPhase=s->phase;stride=in->crouched?M(CROUCH_LENGTH):M(STEP_LENGTH)+M(RUN_LENGTH)*level;
   if(in->grounded&&in->distance>0&&speed>50){
    increment=PI*(in->distance/n)/stride;
    s->phase+=increment;
    if((int)(s->phase/PI)!=(int)(oldPhase/PI))s->output.step=1;
    s->phase=fmod(s->phase,2*PI);
   }
   s->headPhase=fmod(s->headPhase+increment*M(HEAD_RATE),2*PI);
   s->weaponPhase=fmod(s->weaponPhase+fmin(increment*M(WB_RATE),2*PI*M(WB_MAX_HZ)*step),2*PI);
   spring(&s->bob[0],sin(s->weaponPhase+.35)*M(WB_X)*a,M(WB_RESPONSE),step);
   spring(&s->bob[1],cos(2*s->weaponPhase)*M(WB_Y)*a,M(WB_RESPONSE),step);
   spring(&s->bob[2],cos(s->weaponPhase)*M(WB_Z)*a,M(WB_RESPONSE),step);
  }
  /* Weapon pulses have their own clocks and amplitudes. They never sample
     the camera spring, its response, its limit, or its gesture durations. */
  {
   double target=0;
   if(bob)for(j=0;j<2;j++){
    double duration=j?M(W_LAND_DURATION):M(W_JUMP_DURATION);
    double phase=(s->weaponImpactTime[j]+step*.5)/duration;
    if(phase<1){double wave=sin(PI*phase);target+=s->weaponImpactAmplitude[j]*wave*wave;}
    s->weaponImpactTime[j]+=step;
    if(s->weaponImpactTime[j]>=duration)s->weaponImpactAmplitude[j]=0;
   }
   spring(&s->weaponImpact,soft(target*M(IMPACT_WEAPON),M(IMPACT_WLIMIT)),M(IMPACT_WRESPONSE),step);
  }
  spring(&s->yaw,sway?soft(-clamp(in->yawDelta/dt,-16,16)*M(SWAY_GAIN),M(SWAY_LIMIT)):0,M(SWAY_RESPONSE),step);
  spring(&s->pitch,sway?soft(clamp(in->pitchDelta/dt,-16,16)*M(SWAY_GAIN),M(SWAY_LIMIT)):0,M(SWAY_RESPONSE),step);
  /* Absolute look pointing is distinct from turn-rate sway. */
  spring(&s->look,sway?-sin(in->pitch)*M(LOOK_GAIN)*RAD:0,M(LOOK_RESPONSE),step);
  spring(&s->side,sway?soft(in->localSide/M(SPEED),1):0,M(MOVE_RESPONSE),step);
  spring(&s->forward,sway?soft(in->localForward/M(SPEED),1):0,M(MOVE_RESPONSE),step);
  {
   double target[5]={0},weight=clamp(in->equipMass,.5,3),size=clamp(in->equipSize,.5,2);
   if(in->equipPhase){
    double progress=clamp(in->equipProgress,0,1);
    /* Smooth zero endpoints follow the native phase duration, not a new clock. */
    double envelope=16*progress*progress*(1-progress)*(1-progress);
    double gain=M(EQUIP_GAIN)*in->equipGain;
    for(j=0;j<3;j++){
     double measured=(in->equipSampleValid&&s->equipBaseValid)?in->equipSample[j]-s->equipBase[j]:0;
     target[j]=soft((measured*.12+in->equipProfile[j]*envelope*size)*gain,M(EQUIP_LIMIT));
    }
    target[3]=soft((-target[1]*.045+target[2]*.025),M(EQUIP_ANGLE))*RAD;
    target[4]=soft(target[0]*.06,M(EQUIP_ANGLE))*RAD;
   }
   for(j=0;j<5;j++)spring(&s->equipMotion[j],target[j],M(EQUIP_RESPONSE)/weight,step);
   for(j=0;j<3;j++)spring(&s->equipCamera[j],soft(s->equipMotion[j].x*M(EQUIP_CAMERA),M(EQUIP_CLIMIT)),M(EQUIP_CRESPONSE),step);
   spring(&s->equipCamera[3],s->equipMotion[3].x*M(EQUIP_CPITCH),M(EQUIP_CRESPONSE),step);
   spring(&s->equipCamera[4],s->equipMotion[4].x*M(EQUIP_CROLL),M(EQUIP_CRESPONSE),step);
  }
  for(j=0;j<3;j++){
   double delta=(bob&&in->reloading&&in->reloadSampleValid&&s->reloadBaseValid)?(in->reloadSample[j]-s->reloadBase[j])*in->reloadGain:0;
   spring(&s->reloadMotion[j],soft(delta,400),M(RELOAD_RESPONSE),step);
  }
 }
 {int foot=s->output.step;memset(&s->output,0,sizeof s->output);s->output.step=foot;}
 if(bob){
  double a=clamp(s->speed.x/M(SPEED),0,1)*(in->crouched?M(BOB_CROUCH):1),phase=s->headPhase;
  double impact=s->impact.x;
  double rx=s->reloadMotion[0].x,ry=s->reloadMotion[1].x,rz=s->reloadMotion[2].x;
  s->output.camera[0]=(sin(phase)*M(HEAD_X)*a+soft(rx*M(RELOAD_X),M(RELOAD_LIMIT)))*attenuation;
  s->output.camera[1]=s->eye.x-in->eyeHeight+s->stair.x+(cos(2*phase)*M(HEAD_Y)*a+impact+soft(ry*M(RELOAD_Y),M(RELOAD_LIMIT)))*attenuation;
  s->output.camera[2]=soft(rz*M(RELOAD_Z),M(RELOAD_LIMIT))*attenuation;
  s->output.cameraPitch=soft(ry*M(RELOAD_PITCH),M(RELOAD_ANGLE))*RAD*attenuation;
  s->output.cameraRoll=(sin(phase)*M(HEAD_ROLL)*a+impact*M(IMPACT_ROLL)+soft(rx*M(RELOAD_ROLL),M(RELOAD_ANGLE)))*RAD*attenuation;
  for(j=0;j<3;j++)s->output.weapon[j]=(s->bob[j].x+soft(s->reloadMotion[j].x*M(RELOAD_WEAPON),M(RELOAD_LIMIT)))*attenuation;
  s->output.weapon[1]+=(s->weaponImpact.x+s->stair.x*M(STAIR_WEAPON))*attenuation;
  s->output.weaponAngle[2]=(sin(s->weaponPhase)*M(WB_ROLL)*a+s->weaponImpact.x*M(W_IMPACT_ROLL))*RAD*attenuation;
 }
 s->bobbingOutput=s->output;
 if(sway){
  double yaw=s->yaw.x,pitch=s->pitch.x;
  s->output.weapon[0]+=(yaw*M(SWAY_X)-s->side.x*M(MOVE_SIDE))*attenuation;
  s->output.weapon[1]+=pitch*M(SWAY_Y)*attenuation;
  /* +Z goes into the screen: forward inward, backward outward. */
  s->output.weapon[2]+=s->forward.x*M(MOVE_DEPTH)*attenuation;
  s->output.weaponAngle[0]+=-(pitch*M(SWAY_PITCH)+s->look.x)*attenuation;
  s->output.weaponAngle[1]+=yaw*M(SWAY_YAW)*attenuation;
  s->output.weaponAngle[2]+=(yaw*M(SWAY_ROLL)-s->side.x*M(MOVE_ROLL)*RAD)*attenuation;
  s->output.cameraRoll+=yaw*M(SWAY_CAMERA)*attenuation;
 }
 memset(&s->swayOutput,0,sizeof s->swayOutput);
 for(j=0;j<3;j++){
  s->swayOutput.weapon[j]=s->output.weapon[j]-s->bobbingOutput.weapon[j];
  s->swayOutput.weaponAngle[j]=s->output.weaponAngle[j]-s->bobbingOutput.weaponAngle[j];
 }
 s->swayOutput.cameraRoll=s->output.cameraRoll-s->bobbingOutput.cameraRoll;
 /* Legacy shared crouch/equip gestures keep their existing mode coverage.
  * Crouch belongs to sway when enabled; equip belongs to bobbing when enabled. */
 {
  CAMERA_MOTION_OUTPUT *stance=sway?&s->swayOutput:&s->bobbingOutput;
  CAMERA_MOTION_OUTPUT *equip=bob?&s->bobbingOutput:&s->swayOutput;
  stance->weapon[1]+=s->stance.x*attenuation;
  for(j=0;j<3;j++){
   equip->weapon[j]+=s->equipMotion[j].x*attenuation;
   equip->camera[j]+=s->equipCamera[j].x*attenuation;
  }
  equip->weaponAngle[0]+=s->equipMotion[3].x*attenuation;
  equip->weaponAngle[2]+=s->equipMotion[4].x*attenuation;
  equip->cameraPitch+=s->equipCamera[3].x*attenuation;
  equip->cameraRoll+=s->equipCamera[4].x*attenuation;
 }
 s->output.weapon[1]+=s->stance.x*attenuation;
 for(j=0;j<3;j++)s->output.weapon[j]+=s->equipMotion[j].x*attenuation;
 s->output.weaponAngle[0]+=s->equipMotion[3].x*attenuation;
 s->output.weaponAngle[2]+=s->equipMotion[4].x*attenuation;
 for(j=0;j<3;j++)s->output.camera[j]+=s->equipCamera[j].x*attenuation;
 s->output.cameraPitch+=s->equipCamera[3].x*attenuation;
 s->output.cameraRoll+=s->equipCamera[4].x*attenuation;
 /* Shake alone observes stride without enabling weapon/camera animation. */
 if(!(mode&3)){memset(&s->bobbingOutput,0,sizeof s->bobbingOutput);memset(&s->swayOutput,0,sizeof s->swayOutput);int foot=s->output.step;memset(&s->output,0,sizeof s->output);s->output.step=foot;}
}
/* RIFs have texture names, not a universal physical-material table. Match
   the actual hit texture's basename conservatively; unknown stays generic. */
int CameraMotionSurface(const char*texture,int water)
{
 char name[160];const char*base=texture;unsigned i=0;if(water)return 4;if(!texture)return 0;
 for(const char*p=texture;*p;p++)if(*p=='/'||*p=='\\')base=p+1;
 while(base[i]&&i<sizeof(name)-1){name[i]=(char)tolower((unsigned char)base[i]);i++;}name[i]=0;
 if(strstr(name,"metal")||strstr(name,"grate")||strstr(name,"grill")||strstr(name,"steel"))return 1;
 if(strstr(name,"dirt")||strstr(name,"sand")||strstr(name,"mud")||strstr(name,"grass"))return 2;
 if(strstr(name,"wood")||strstr(name,"plank"))return 3;
 return 0;
}
