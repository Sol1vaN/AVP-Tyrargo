#include "shake_params.h"
#include "hud_motion.h"
#include "menu_preview.h"
#include "motion_params.h"
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "gamedef.h"
#include "stratdef.h"
#include "dynblock.h"
#include "bh_types.h"
#include "weapons.h"
#include "extents.h"
#include "usr_io.h"
#include "psnd.h"
#include "psndproj.h"
#include "camera_motion.h"
#include <math.h>
#include <string.h>
extern DISPLAYBLOCK *Player;
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern int NormalFrameTime,MultiplayerObservedPlayer;
extern float CameraZoomScale;
extern int CameraMotionTrace(VECTORCH *origin,VECTORCH *direction,int range,const char **texture);
static CAMERA_MOTION_STATE motion;
static CAMERA_MOTION_OUTPUT motionPresentation;
static HUD_MOTION_STATE marineHud;
static double hudProjection[9]={1,0,0,0,1,0,0,0,1};
static double hudBottomCorrection;
static int hudLayoutTop,hudLayoutHeight;
static double hudReloadBase[3],hudEquipBase[3],hudGesture[3];
static double hudRounds[2],hudFireBase[3];
static int hudFireValid,hudInputRunning;
static int hudWeapon=-1,hudReloadValid,hudEquipValid,hudEquipPhase;
static int hudGrounded,hudGroundValid,hudInputGrounded;
static double hudInputVelocity,hudFallSpeed,hudAirTime,hudDamage,hudShotPhase;
static unsigned hudImpactSerial;
static double hudDistance,hudDt;
static void resetHud(void){
 HudMotionReset(&marineHud);hudWeapon=-1;hudFireValid=0;hudInputRunning=0;hudReloadValid=hudEquipValid=hudEquipPhase=0;
 memset(hudGesture,0,sizeof hudGesture);hudDistance=hudDt=0;
 hudGroundValid=0;hudFallSpeed=hudAirTime=hudDamage=hudShotPhase=0;hudImpactSerial=0;
}

static int water,preciseValid;
static double world[3],rotation[3][3];
static unsigned int stepIndex;
extern HMODELCONTROLLER PlayersWeaponHModelController;
void MarineMotionReset(void){CameraMotionReset(&motion);resetHud();water=preciseValid=0;}
void MarineMotionWater(int value){water=value;}
int MarineMotionOwnsWaterSteps(void){return AvP.PlayerType==I_Marine&&(CameraMotionFlags(MarineCameraMode)&2)&&M(STEPS_ENABLE)>=.5&&!MultiplayerObservedPlayer;}
static void matrix(double out[3][3],const MATRIXCH*m){
 out[0][0]=m->mat11/65536.0;out[0][1]=m->mat21/65536.0;out[0][2]=m->mat31/65536.0;
 out[1][0]=m->mat12/65536.0;out[1][1]=m->mat22/65536.0;out[1][2]=m->mat32/65536.0;
 out[2][0]=m->mat13/65536.0;out[2][1]=m->mat23/65536.0;out[2][2]=m->mat33/65536.0;
}
static void footstep(int crouched){
 VECTORCH origin=Player->ObWorld,direction={0,65536,0};const char*texture=0;
 int surface,volume,pitch;
 CameraMotionTrace(&origin,&direction,4000,&texture);surface=CameraMotionSurface(texture,water);
 /* Native landing/boot sample: material-specific pitch/volume, not imported
    combat or machinery effects. Water uses the original splash bank. */
 /* AVP volume is a nonlinear attenuation index, not a percentage.
    43 was only 0.078 gain; these indices produce an audible native step. */
 volume=(int)lround(surface==1?M(VOL_METAL):surface==2?M(VOL_SOFT):surface==3?M(VOL_WOOD):surface==4?M(VOL_WATER):M(VOL_GENERIC));
 if(crouched)volume-=(int)lround(M(VOL_CROUCH));if(volume<0)volume=0;
 pitch=(int)lround(surface==1?M(PITCH_METAL):surface==2?M(PITCH_SOFT):surface==3?M(PITCH_WOOD):0);
 stepIndex++;{unsigned variation=(unsigned)lround(M(PITCH_VARIATION));pitch+=(int)(stepIndex*37u%(2u*variation+1u))-(int)variation;}
 if(surface==4)Sound_Play((SOUNDINDEX)(SID_SPLASH1+(stepIndex&3u)),"vp",volume,pitch);
 else Sound_Play(SID_MARINE_SMALLLANDING,"vp",volume,pitch);
}
static void reloadCentroid(const SECTION_DATA*node,const double pm[3][3],const double pt[3],double sum[3],int*count,int depth){
 for(;node&&*count<96;node=node->Next_Sibling){
  double lm[3][3],m[3][3],t[3],v[3]={node->Offset.vx,node->Offset.vy,node->Offset.vz};int i,j,k;
  if(depth>24)return;
  matrix(lm,&node->RelSecMat);
  for(i=0;i<3;i++){
   t[i]=pt[i];for(k=0;k<3;k++)t[i]+=pm[i][k]*v[k];
   for(j=0;j<3;j++){m[i][j]=0;for(k=0;k<3;k++)m[i][j]+=pm[i][k]*lm[k][j];}
  }
  if(node->Shape){for(i=0;i<3;i++)sum[i]+=t[i];(*count)++;}
  if(node->First_Child)reloadCentroid(node->First_Child,m,t,sum,count,depth+1);
 }
}
static double reloadWeaponGain(int weapon){
 switch(weapon){
 case WEAPON_MARINE_PISTOL:return M(RELOAD_GUN_0);case WEAPON_TWO_PISTOLS:return M(RELOAD_GUN_1);
 case WEAPON_PULSERIFLE:return M(RELOAD_GUN_2);case WEAPON_SMARTGUN:return M(RELOAD_GUN_3);
 case WEAPON_MINIGUN:return M(RELOAD_GUN_4);case WEAPON_FLAMETHROWER:return M(RELOAD_GUN_5);
 case WEAPON_GRENADELAUNCHER:return M(RELOAD_GUN_6);case WEAPON_SADAR:return M(RELOAD_GUN_7);
 case WEAPON_FRISBEE_LAUNCHER:return M(RELOAD_GUN_8);default:return 1;
 }
}
static double equipRadius(const SECTION_DATA *node,int depth,int *visited){
 double radius=0;
 if(depth>24)return 0;
 for(;node&&*visited<96;node=node->Next_Sibling){
  ++*visited;
  if(node->Shape)radius=fmax(radius,node->Shape->shaperadius);
  if(node->First_Child)radius=fmax(radius,equipRadius(node->First_Child,depth+1,visited));
 }
 return radius;
}
/* Visual inertia coefficients, not gameplay mass. Each trajectory is combined
   with the actual animated hierarchy and native equip phase timing. */
static void equipProfile(int weapon,CAMERA_MOTION_INPUT *in){
 static const double profiles[9][4]={
  { .75, 12, 32,-18}, {1.05,-14,42,-22}, {1.30,18,50,-30},
  {1.85,-22,55,-42}, {2.50,28,65,-50}, {1.65,-16,48,-38},
  {1.45,22,46,-34}, {2.10,-12,62,-48}, {1.20,16,38,-26}};
 int index=-1;
 switch(weapon){
 case WEAPON_MARINE_PISTOL:index=0;break;case WEAPON_TWO_PISTOLS:index=1;break;
 case WEAPON_PULSERIFLE:index=2;break;case WEAPON_SMARTGUN:index=3;break;
 case WEAPON_MINIGUN:index=4;break;case WEAPON_FLAMETHROWER:index=5;break;
 case WEAPON_GRENADELAUNCHER:index=6;break;case WEAPON_SADAR:index=7;break;
 case WEAPON_FRISBEE_LAUNCHER:index=8;break;
 }
 in->equipMass=1;in->equipSize=1;
 if(index<0){in->equipPhase=0;return;}
 in->equipMass=profiles[index][0];in->equipGain=MotionValues[MP_EQUIP_GUN_0+index];
 for(int i=0;i<3;i++)in->equipProfile[i]=profiles[index][i+1];
 if(in->equipPhase<0)in->equipProfile[0]*=-1;
 if(in->equipPhase&&PlayersWeaponHModelController.section_data){
  int visited=0;double radius=equipRadius(PlayersWeaponHModelController.section_data,0,&visited);
  if(radius>0)in->equipSize=fmax(.5,fmin(2,radius/600.0));
 }
}
static double CameraSupportHeightDelta(const DYNAMICSBLOCK *dyn){
 if(!dyn->IsInContactWithFloor)return 0;
 for(const COLLISIONREPORT *r=dyn->CollisionReportPtr;r;r=r->NextCollisionReportPtr){
  const STRATEGYBLOCK *support=r->ObstacleSBPtr;
  if(r->ObstacleNormal.vy < -60000 && support &&
     support->I_SBtype==I_BehaviourPlatform && support->DynPtr){
   const DYNAMICSBLOCK *floor=support->DynPtr;
   return (double)floor->Position.vy-floor->PrevPosition.vy;
  }
 }
 return 0;
}
void MarineMotionUpdateCamera(void){
 CAMERA_MOTION_INPUT in={0};PLAYER_STATUS *ps;DYNAMICSBLOCK *dyn;PLAYER_WEAPON_DATA *weapon;
 double dx,dz,base[3][3],body[3][3],c,s;int i,j;
 preciseValid=0;
 if(!Player||!Player->ObStrategyBlock||!Player->ObStrategyBlock->DynPtr){MarineMotionReset();return;}
 ps=(PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;dyn=Player->ObStrategyBlock->DynPtr;
 if(!ps){MarineMotionReset();return;}
 in.active=AvP.PlayerType==I_Marine&&ps->IsAlive&&!MultiplayerObservedPlayer&&!ps->MyFaceHugger;
 if(!in.active||MarineCameraMode<1||MarineCameraMode>7){MarineMotionReset();return;}
 weapon=&ps->WeaponSlot[ps->SelectedWeaponSlot];in.weapon=weapon->WeaponIDNumber;
 in.dt=NormalFrameTime/65536.0;dx=(double)dyn->Position.vx-dyn->PrevPosition.vx;dz=(double)dyn->Position.vz-dyn->PrevPosition.vz;
 in.distance=sqrt(dx*dx+dz*dz);in.grounded=dyn->IsInContactWithFloor;
 in.crouched=ps->ShapeState!=PMph_Standing;
 hudInputRunning=ps->Mvt_InputRequests.Flags.Rqst_Faster&&ps->Encumberance.CanRun&&!in.crouched;
 in.eyeHeight=in.crouched?CollisionExtents[CE_MARINE].CrouchingTop:CollisionExtents[CE_MARINE].StandingTop;
 in.verticalVelocity=(double)dyn->LinVelocity.vy+dyn->LinImpulse.vy;
 in.bodyHeight=dyn->Position.vy;
 in.supportHeightDelta=CameraSupportHeightDelta(dyn);
 matrix(body,&Player->ObMat);
 if(in.dt>0){
  in.localSide=(dx*body[0][0]+dz*body[2][0])/in.dt;
  in.localForward=(dx*body[0][2]+dz*body[2][2])/in.dt;
 }
 in.yawDelta=MouseRawYawDelta*(6.283185307179586/(4096.0*65536.0));
 in.pitchDelta=MouseRawPitchDelta*(6.283185307179586/(4096.0*65536.0));
 in.pitch=((double)ps->ViewPanX+MouseRawPitchFraction/65536.0)*(6.283185307179586/4096.0);
 in.reloading=weapon->CurrentState==WEAPONSTATE_RELOAD_PRIMARY||weapon->CurrentState==WEAPONSTATE_RELOAD_SECONDARY;
 in.equipPhase=(weapon->CurrentState==WEAPONSTATE_SWAPPING_IN||weapon->CurrentState==WEAPONSTATE_READYING)?1:
  (weapon->CurrentState==WEAPONSTATE_SWAPPING_OUT||weapon->CurrentState==WEAPONSTATE_UNREADYING)?-1:0;
 in.equipProgress=1-weapon->StateTimeOutCounter/65536.0;
 equipProfile(in.weapon,&in);
 in.zoom=CameraZoomScale;
 in.reloadGain=reloadWeaponGain(in.weapon);
 if((in.reloading||in.equipPhase)&&PlayersWeaponHModelController.section_data){
  double identity[3][3]={{1,0,0},{0,1,0},{0,0,1}},zero[3]={0};int count=0;
  reloadCentroid(PlayersWeaponHModelController.section_data,identity,zero,in.reloadSample,&count,0);
  if(count){for(i=0;i<3;i++)in.reloadSample[i]/=count;in.reloadSampleValid=1;memcpy(in.equipSample,in.reloadSample,sizeof in.equipSample);in.equipSampleValid=1;}
 }
 CameraMotionUpdate(&motion,&in,CameraMotionFlags(MarineCameraMode));
 CameraMotionCompose(&motion,&motionPresentation,HUDSwayIntensity,HUDBobbingIntensity);
 hudDistance=in.distance;hudDt=in.dt;hudInputGrounded=in.grounded;hudInputVelocity=in.verticalVelocity;
 if(!motion.initialized)return;
 if((CameraMotionFlags(MarineCameraMode)&2)&&motion.output.step&&in.dt>0&&M(STEPS_ENABLE)>=.5)footstep(in.crouched);
 matrix(base,&Global_VDB_Ptr->VDB_Mat);matrix(body,&Player->ObMat);
 world[0]=Global_VDB_Ptr->VDB_World.vx;world[1]=Global_VDB_Ptr->VDB_World.vy;world[2]=Global_VDB_Ptr->VDB_World.vz;
 for(i=0;i<3;i++)for(j=0;j<3;j++)world[i]+=body[i][j]*motionPresentation.camera[j];
 /* A short camera sweep prevents smooth crouch/bob from crossing a ceiling. */
 {
  VECTORCH origin=Global_VDB_Ptr->VDB_World,direction;double delta[3]={world[0]-origin.vx,world[1]-origin.vy,world[2]-origin.vz};
  double length=sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
  if(length>1){int hit;direction.vx=(int)lround(delta[0]*65536/length);direction.vy=(int)lround(delta[1]*65536/length);direction.vz=(int)lround(delta[2]*65536/length);
   hit=CameraMotionTrace(&origin,&direction,(int)ceil(length)+12,0);
   if(hit<length+12){double fraction=fmax(0,(hit-12)/length);world[0]=origin.vx+delta[0]*fraction;world[1]=origin.vy+delta[1]*fraction;world[2]=origin.vz+delta[2]*fraction;}
  }
 }
 c=cos(motionPresentation.cameraRoll);s=sin(motionPresentation.cameraRoll);
 for(i=0;i<3;i++){
  double cp=cos(motionPresentation.cameraPitch),sp=sin(motionPresentation.cameraPitch);
  double by=base[i][1]*cp+base[i][2]*sp,bz=-base[i][1]*sp+base[i][2]*cp;
  rotation[i][0]=base[i][0]*c+by*s;rotation[i][1]=-base[i][0]*s+by*c;rotation[i][2]=bz;
 }
 Global_VDB_Ptr->VDB_World.vx=(int)lround(world[0]);Global_VDB_Ptr->VDB_World.vy=(int)lround(world[1]);Global_VDB_Ptr->VDB_World.vz=(int)lround(world[2]);
 #define SET(r,c,field) Global_VDB_Ptr->VDB_Mat.field=(int)lround(rotation[r][c]*65536)
 SET(0,0,mat11);SET(0,1,mat21);SET(0,2,mat31);SET(1,0,mat12);SET(1,1,mat22);SET(1,2,mat32);SET(2,0,mat13);SET(2,1,mat23);SET(2,2,mat33);
 #undef SET
 preciseValid=1;
}
void MarineMotionGeometryView(double*v,double*eye,double xs,double ys,double zs){
 double scale[3]={xs,ys,zs};int i,j;if(!preciseValid)return;
 for(i=0;i<3;i++){eye[i]=world[i];for(j=0;j<3;j++)v[i*4+j]=rotation[j][i]*scale[i];}
}
void MarineMotionViewMatrix(float*v,double xScale,double yScale,double zScale){
 double scale[3]={xScale,yScale,zScale};int i,j;if(!preciseValid)return;
 /* Culling uses the rounded VDB. Rendering retains the fractional camera
    pose so procedural movement does not reintroduce integer stepping. */
 for(i=0;i<3;i++){double t=0;for(j=0;j<3;j++){v[i*4+j]=(float)(rotation[j][i]*scale[i]);t-=rotation[j][i]*world[j];}v[i*4+3]=(float)(t*scale[i]);}
}
void MarineMotionWeaponPose(double m[3][3],double t[3]){
 double r[3][3],out[3][3],x,y,z,cx,sx,cy,sy,cz,sz;int i,j,k;
 if(!preciseValid||AvP.PlayerType!=I_Marine||MarineCameraMode==0)return;
 x=motionPresentation.weaponAngle[0];y=motionPresentation.weaponAngle[1];z=motionPresentation.weaponAngle[2];cx=cos(x);sx=sin(x);cy=cos(y);sy=sin(y);cz=cos(z);sz=sin(z);
 r[0][0]=cz*cy;r[0][1]=cz*sy*sx-sz*cx;r[0][2]=cz*sy*cx+sz*sx;
 r[1][0]=sz*cy;r[1][1]=sz*sy*sx+cz*cx;r[1][2]=sz*sy*cx-cz*sx;
 r[2][0]=-sy;r[2][1]=cy*sx;r[2][2]=cy*cx;
 for(i=0;i<3;i++){t[i]+=motionPresentation.weapon[i];for(j=0;j<3;j++){out[i][j]=0;for(k=0;k<3;k++)out[i][j]+=m[i][k]*r[k][j];}}
 memcpy(m,out,sizeof out);
}


/* Camera/weapon-to-HUD adapter: native assets stay native, including their
 * reload trajectory and firing cadence. No animation state is advanced here. */
static int hudWeaponProfile(int weapon){
 switch(weapon){
 case WEAPON_MARINE_PISTOL:return 0;case WEAPON_TWO_PISTOLS:return 1;
 case WEAPON_PULSERIFLE:return 2;case WEAPON_SMARTGUN:return 3;
 case WEAPON_MINIGUN:return 4;case WEAPON_FLAMETHROWER:return 5;
 case WEAPON_GRENADELAUNCHER:return 6;case WEAPON_SADAR:return 7;
 case WEAPON_FRISBEE_LAUNCHER:return 8;default:return -1;
 }
}
void MarineHUDMotionDamage(double damage){
 if(AvP.PlayerType==I_Marine&&(CameraMotionFlags(MarineCameraMode)&4)&&!RenderPausedPreview&&damage>0&&isfinite(damage))
  hudDamage=fmin(100,hudDamage+damage);
}
void MarineHUDMotionUpdate(void){
 if(RenderPausedPreview)return;
 if(!preciseValid||AvP.PlayerType!=I_Marine||!(CameraMotionFlags(MarineCameraMode)&4)){resetHud();return;}
 PLAYER_STATUS *ps=(PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;
 if(!ps||!ps->IsAlive||ps->MyFaceHugger||MultiplayerObservedPlayer){resetHud();return;}
 if(hudDt<=0)return;
 if(hudDt>.25||hudDistance>3000){resetHud();return;}
 PLAYER_WEAPON_DATA *weapon=&ps->WeaponSlot[ps->SelectedWeaponSlot];
 int index=hudWeaponProfile(weapon->WeaponIDNumber);
 if(index<0){resetHud();return;}
 HUD_QUAT view=HudMotionOrientation(rotation);
 if(!marineHud.initialized)HudMotionUpdate(&marineHud,0,view,(double[3]){0},3);
 /* Reload transmission is unchanged from probe 186. */
 const double profile[9][3]={{H(RELOAD_0_0),H(RELOAD_0_1),H(RELOAD_0_2)},{H(RELOAD_1_0),H(RELOAD_1_1),H(RELOAD_1_2)},{H(RELOAD_2_0),H(RELOAD_2_1),H(RELOAD_2_2)},{H(RELOAD_3_0),H(RELOAD_3_1),H(RELOAD_3_2)},{H(RELOAD_4_0),H(RELOAD_4_1),H(RELOAD_4_2)},{H(RELOAD_5_0),H(RELOAD_5_1),H(RELOAD_5_2)},{H(RELOAD_6_0),H(RELOAD_6_1),H(RELOAD_6_2)},{H(RELOAD_7_0),H(RELOAD_7_1),H(RELOAD_7_2)},{H(RELOAD_8_0),H(RELOAD_8_1),H(RELOAD_8_2)}};
 const double *gain=profile[index];
 int reloading=weapon->CurrentState==WEAPONSTATE_RELOAD_PRIMARY||weapon->CurrentState==WEAPONSTATE_RELOAD_SECONDARY;
 int equip=(weapon->CurrentState==WEAPONSTATE_SWAPPING_IN||weapon->CurrentState==WEAPONSTATE_READYING)?1:
  (weapon->CurrentState==WEAPONSTATE_SWAPPING_OUT||weapon->CurrentState==WEAPONSTATE_UNREADYING)?-1:0;
 int changed=hudWeapon!=weapon->WeaponIDNumber;
 double target[3]={0},kick[3]={0},sample[3]={0},rounds[2]={weapon->PrimaryRoundsRemaining/65536.0,weapon->SecondaryRoundsRemaining/65536.0};
 if(changed){hudFireValid=0;hudWeapon=weapon->WeaponIDNumber;hudReloadValid=hudEquipValid=0;hudEquipPhase=0;}
 if(equip!=hudEquipPhase){hudEquipValid=0;hudEquipPhase=equip;}
 int firing=weapon->CurrentState==WEAPONSTATE_FIRING_PRIMARY||weapon->CurrentState==WEAPONSTATE_FIRING_SECONDARY||
  weapon->CurrentState==WEAPONSTATE_RECOIL_PRIMARY||weapon->CurrentState==WEAPONSTATE_RECOIL_SECONDARY;
 int nativeFeedback=(index==2||index==3||index==5||index==8)&&firing;
 int count=0;
 if((reloading||equip||nativeFeedback)&&PlayersWeaponHModelController.section_data){
  double id[3][3]={{1,0,0},{0,1,0},{0,0,1}},zero[3]={0};
  reloadCentroid(PlayersWeaponHModelController.section_data,id,zero,sample,&count,0);
  if(count)for(int j=0;j<3;j++)sample[j]/=count;
 }
 if(reloading){
  if(count){
   if(!hudReloadValid){memcpy(hudReloadBase,sample,sizeof sample);hudReloadValid=1;}
   for(int j=0;j<3;j++)target[j]+=(sample[j]-hudReloadBase[j])*H(RELOAD_NATIVE)*gain[j];
  }else{
   double phase=fmax(0,fmin(1,1-weapon->StateTimeOutCounter/65536.0));
   target[0]=sin(phase*6.283185307)*H(RELOAD_FALLBACK_X)*gain[0];
   target[1]=sin(phase*3.141592654)*H(RELOAD_FALLBACK_Y)*gain[1];
   target[2]=sin(phase*3.141592654)*H(RELOAD_FALLBACK_Z)*gain[2];
  }
 }else hudReloadValid=0;
 if(equip){
  CAMERA_MOTION_INPUT shape={0};
  const double ep[9][4]={{H(EQUIP_0_0),H(EQUIP_0_1),H(EQUIP_0_2),H(EQUIP_0_3)},{H(EQUIP_1_0),H(EQUIP_1_1),H(EQUIP_1_2),H(EQUIP_1_3)},{H(EQUIP_2_0),H(EQUIP_2_1),H(EQUIP_2_2),H(EQUIP_2_3)},{H(EQUIP_3_0),H(EQUIP_3_1),H(EQUIP_3_2),H(EQUIP_3_3)},{H(EQUIP_4_0),H(EQUIP_4_1),H(EQUIP_4_2),H(EQUIP_4_3)},{H(EQUIP_5_0),H(EQUIP_5_1),H(EQUIP_5_2),H(EQUIP_5_3)},{H(EQUIP_6_0),H(EQUIP_6_1),H(EQUIP_6_2),H(EQUIP_6_3)},{H(EQUIP_7_0),H(EQUIP_7_1),H(EQUIP_7_2),H(EQUIP_7_3)},{H(EQUIP_8_0),H(EQUIP_8_1),H(EQUIP_8_2),H(EQUIP_8_3)}};
  const double en[9][3]={{H(EQUIP_NATIVE_0_0),H(EQUIP_NATIVE_0_1),H(EQUIP_NATIVE_0_2)},{H(EQUIP_NATIVE_1_0),H(EQUIP_NATIVE_1_1),H(EQUIP_NATIVE_1_2)},{H(EQUIP_NATIVE_2_0),H(EQUIP_NATIVE_2_1),H(EQUIP_NATIVE_2_2)},{H(EQUIP_NATIVE_3_0),H(EQUIP_NATIVE_3_1),H(EQUIP_NATIVE_3_2)},{H(EQUIP_NATIVE_4_0),H(EQUIP_NATIVE_4_1),H(EQUIP_NATIVE_4_2)},{H(EQUIP_NATIVE_5_0),H(EQUIP_NATIVE_5_1),H(EQUIP_NATIVE_5_2)},{H(EQUIP_NATIVE_6_0),H(EQUIP_NATIVE_6_1),H(EQUIP_NATIVE_6_2)},{H(EQUIP_NATIVE_7_0),H(EQUIP_NATIVE_7_1),H(EQUIP_NATIVE_7_2)},{H(EQUIP_NATIVE_8_0),H(EQUIP_NATIVE_8_1),H(EQUIP_NATIVE_8_2)}};
  shape.equipMass=ep[index][0];shape.equipSize=1;
  for(int j=0;j<3;j++)shape.equipProfile[j]=ep[index][j+1];
  if(equip<0)shape.equipProfile[0]*=-1;
  if(PlayersWeaponHModelController.section_data){int visited=0;double radius=equipRadius(PlayersWeaponHModelController.section_data,0,&visited);if(radius>0)shape.equipSize=fmax(.5,fmin(2,radius/600.0));}
  if(count&&!hudEquipValid){memcpy(hudEquipBase,sample,sizeof sample);hudEquipValid=1;}
  double phase=fmax(0,fmin(1,1-weapon->StateTimeOutCounter/65536.0));
  double envelope=16*phase*phase*(1-phase)*(1-phase);
  for(int j=0;j<3;j++){
   double measured=count&&hudEquipValid?sample[j]-hudEquipBase[j]:0;
   target[j]+=measured*H(EQUIP_NATIVE)*en[index][j]+envelope*shape.equipProfile[j]*shape.equipMass*shape.equipSize*H(EQUIP_ENVELOPE);
  }
 }else hudEquipValid=0;
 for(int j=0;j<3;j++){
  target[j]=H(GESTURE_LIMIT)*tanh(target[j]/H(GESTURE_LIMIT));
  kick[j]+=(target[j]-hudGesture[j])*H(GESTURE_GAIN);hudGesture[j]=target[j];
 }
 /* Only running steps are reduced; walking/crouching keep their strength. */
 if(motion.output.step){
  if(hudInputRunning)HudMotionRunStep(&marineHud,H(RUN_GAIN));
  else HudMotionWalkStep(&marineHud,H(WALK_GAIN));
 }
 /* Sample the animated hierarchy in its local frame. Never turn recoil Z
  * into HUD depth (that caused zooming). Pose contribution eases back to rest. */
 double firePose[3]={0};
 if(nativeFeedback&&!reloading&&!equip){
  if(count&&!hudFireValid){memcpy(hudFireBase,sample,sizeof sample);hudFireValid=1;}
  double side=count&&hudFireValid?sample[0]-hudFireBase[0]:0;
  double vertical=count&&hudFireValid?sample[1]-hudFireBase[1]:0;
  int pitch=((weapon->DirectionOffset.EulerX+2048)&4095)-2048;
  int yaw=((weapon->DirectionOffset.EulerY+2048)&4095)-2048;
  int roll=((weapon->DirectionOffset.EulerZ+2048)&4095)-2048;
  firePose[0]=(side+weapon->PositionOffset.vx)*H(POSE_POSITION);
  firePose[1]=(vertical+weapon->PositionOffset.vy)*H(POSE_POSITION)-pitch*H(POSE_ANGLE);
  firePose[2]=-side*H(POSE_SIDE)+yaw*H(POSE_SIDE)+roll*H(POSE_ANGLE);
 }else hudFireValid=0;
 double poseGain=index==2?H(POSE_GAIN2):index==3?H(POSE_GAIN3):index==5?H(POSE_GAIN5):index==8?H(POSE_GAIN8):1;
 for(int j=0;j<3;j++)firePose[j]*=poseGain;
 HudMotionFirePose(&marineHud,firePose);
 HudMotionFlame(&marineHud,0);
 if(!changed&&!reloading&&!equip){
  double primary=fmax(0,hudRounds[0]-rounds[0]),secondary=fmax(0,hudRounds[1]-rounds[1]);
  if(index==2||index==3||index==8){
   /* Match FireAutomaticWeapon: ceil(old ammo)-ceil(new ammo). Fractional
    * bookkeeping alone must not smear every bullet into a continuous lift. */
   int bullets=(int)fmax(0,ceil(hudRounds[0])-ceil(rounds[0]));
   HudMotionWeaponShot(&marineHud,index,bullets);
   if(index==2&&secondary>0)HudMotionEvent(&marineHud,0,0,-H(RIFLE_GRENADE_Y)*secondary,H(RIFLE_GRENADE_ROLL)*secondary);
  }else if(index==5){
   /* Real fuel flow gates the organic pressure/vibration envelope. */
   HudMotionFlame(&marineHud,primary>0);
  }else{
   double shots=fmin(12,primary+secondary);
   const double shot[9][3]={{H(SHOT_0_0),H(SHOT_0_1),H(SHOT_0_2)},{H(SHOT_1_0),H(SHOT_1_1),H(SHOT_1_2)},{0,0,0},{0,0,0},{H(SHOT_4_0),H(SHOT_4_1),H(SHOT_4_2)},{0,0,0},{H(SHOT_6_0),H(SHOT_6_1),H(SHOT_6_2)},{H(SHOT_7_0),H(SHOT_7_1),H(SHOT_7_2)},{0,0,0}};
   if(shots>0){
    double lateral=(cos(hudShotPhase)-cos(hudShotPhase+shots*H(SHOT_PHASE)))/H(SHOT_PHASE);
    hudShotPhase=fmod(hudShotPhase+shots*H(SHOT_PHASE),6.283185307);
    HudMotionEvent(&marineHud,0,shot[index][0]*lateral,-shot[index][1]*shots,shot[index][2]*lateral);
   }
  }
 }
 memcpy(hudRounds,rounds,sizeof rounds);
 if(hudGroundValid){
  if(hudGrounded&&!hudInputGrounded&&hudInputVelocity< -100){
   HudMotionEvent(&marineHud,1,H(JUMP_X),H(JUMP_Y),H(JUMP_ROLL));hudFallSpeed=0;hudAirTime=0;
  }
  if(!hudGrounded&&hudInputGrounded&&hudAirTime>.05&&hudFallSpeed>250){
   /* CPlayer::Landed uses clamp(.03*fallSpeed,0,.5). AvP is mm/s. */
   double intensity=fmin(H(LAND_MAX),H(LAND_GAIN)*hudFallSpeed/1000.0);
   double direction=(++hudImpactSerial&1)?1:-1;
   HudMotionEvent(&marineHud,1,H(LAND_X)*intensity*direction,H(LAND_Y)*intensity,H(LAND_ROLL)*intensity*direction);
  }
 }
 if(!hudInputGrounded){hudFallSpeed=fmax(hudFallSpeed,hudInputVelocity);hudAirTime+=hudDt;}
 else{hudFallSpeed=0;hudAirTime=0;}
 hudGrounded=hudInputGrounded;hudGroundValid=1;
 if(hudDamage>0){
  /* Actual hit events, including armour-only hits. Healing/max-health
   * decay is never mistaken for a hit; sustained damage remains proportional. */
  double intensity=fmin(H(DAMAGE_MAX),hudDamage*H(DAMAGE_GAIN));
  double direction=(hudImpactSerial&1)?1:-1;
  HudMotionEvent(&marineHud,2,H(DAMAGE_X)*intensity*direction,H(DAMAGE_Y)*intensity,H(DAMAGE_ROLL)*intensity*direction);
  hudDamage=0;
 }
 HudMotionUpdate(&marineHud,hudDt,view,kick,3);
}
void MarineHUDMotionPrepare(int width,int height,int ox,int oy){
 hudBottomCorrection=0;hudLayoutTop=oy;hudLayoutHeight=height;
 if(AvP.PlayerType!=I_Marine||!(CameraMotionFlags(MarineCameraMode)&4)||!preciseValid){
  memset(hudProjection,0,sizeof hudProjection);hudProjection[0]=hudProjection[4]=hudProjection[8]=1;return;
 }
 HudMotionMatrixIntensity(&marineHud,width,height,ox,oy,hudProjection,CameraMotionIntensityGain(HUDShakeIntensity));
 hudBottomCorrection=HudMotionBottomCompensationIntensity(&marineHud,width,height,ox,oy,hudProjection,CameraMotionIntensityGain(HUDShakeIntensity));
}
float MarineHUDMotionTransform(float *x,float *y){
 if(AvP.PlayerType!=I_Marine||!(CameraMotionFlags(MarineCameraMode)&4))return 1;
 return HudMotionTransformPoint(hudProjection,hudBottomCorrection,hudLayoutTop,hudLayoutHeight,x,y);
}
