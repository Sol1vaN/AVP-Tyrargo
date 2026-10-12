#include "camera_motion.h"
#include "motion_params.h"
#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "stratdef.h"
#include "gamedef.h"
#include "kshape.h"
#include "weapons.h"
#include "viewmodel.h"
#include "dev_viewmodel.h"
#include "projection.h"
#include "player_fov.h"
#include "dynblock.h"
#include "bh_types.h"
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
#include "avp_userprofile.h"
#include <math.h>
#include <string.h>

/* Row-major affine matrices, evaluated without a world-space round trip. */
typedef struct { double m[3][3], t[3]; } VM_POSE;
static VM_POSE baseWeaponPose, weaponPose, flashPose, drawPose, sectionPose, muzzlePose;
int ViewmodelMuzzleActive;
static int poseValid;
static struct section_data *drawSection;
int ViewmodelDrawActive;
int ViewmodelShapeActive;
double ViewmodelPoints[maxrotpts][3];
double ViewmodelLocalEye[3];
extern VECTORCH RotatedPts[];
extern VECTORCH GunMuzzleDirectionInVS, CentreOfMuzzleOffset;
extern SECTION_DATA *PWMFSDP;
extern struct Target PlayersTarget;
extern DISPLAYBLOCK PlayersWeapon, PlayersWeaponMuzzleFlash;
extern int PredatorVisionChangeCounter, CloakingPhase;
extern float CameraZoomScale;

static void VMMatrix(double out[3][3], const MATRIXCH *m)
{
 out[0][0]=m->mat11/65536.0; out[0][1]=m->mat21/65536.0; out[0][2]=m->mat31/65536.0;
 out[1][0]=m->mat12/65536.0; out[1][1]=m->mat22/65536.0; out[1][2]=m->mat32/65536.0;
 out[2][0]=m->mat13/65536.0; out[2][1]=m->mat23/65536.0; out[2][2]=m->mat33/65536.0;
}
static void VMCompose(VM_POSE *out, const VM_POSE *parent, const VM_POSE *local)
{
 VM_POSE result; int i,j,k;
 for(i=0;i<3;i++) {
  result.t[i]=parent->t[i];
  for(k=0;k<3;k++) result.t[i]+=parent->m[i][k]*local->t[k];
  for(j=0;j<3;j++) {
   result.m[i][j]=0;
   for(k=0;k<3;k++) result.m[i][j]+=parent->m[i][k]*local->m[k][j];
  }
 }
 *out=result;
}
static void VMSectionPose(VM_POSE *out, const SECTION_DATA *section)
{
 VM_POSE parent,local={0};
 if(section->My_Parent) VMSectionPose(&parent,section->My_Parent);
 else parent=weaponPose;
 if(section->RenderPoseValid){memcpy(local.m,section->RenderLocalMatrix,sizeof local.m);memcpy(local.t,section->RenderOffset,sizeof local.t);}
 else{VMMatrix(local.m,&section->RelSecMat);local.t[0]=section->Offset.vx;local.t[1]=section->Offset.vy;local.t[2]=section->Offset.vz;}
 VMCompose(out,&parent,&local);
}
void ViewmodelUpdatePose(const VECTORCH *offset, const EULER *direction, int aim)
{
 int i; VM_POSE local={0};
 memset(&weaponPose,0,sizeof weaponPose);
 weaponPose.m[0][0]=weaponPose.m[1][1]=weaponPose.m[2][2]=1;
 weaponPose.t[0]=offset->vx; weaponPose.t[1]=offset->vy; weaponPose.t[2]=offset->vz;
 if(aim) {
  double z[3]={0,0,8388608.0},n,xn;
  if(PlayersTarget.DispPtr && PlayersTarget.Distance>0) {
   z[0]=GunMuzzleDirectionInVS.vx; z[1]=GunMuzzleDirectionInVS.vy; z[2]=GunMuzzleDirectionInVS.vz;
   n=sqrt(z[0]*z[0]+z[1]*z[1]+z[2]*z[2]);
   if(n>0) for(i=0;i<3;i++) z[i]*=PlayersTarget.Distance/n;
  }
  /* Preserve the legacy quarter-offset convergence, in camera coordinates. */
  for(i=0;i<3;i++) z[i]+=weaponPose.t[i]*0.25;
  n=sqrt(z[0]*z[0]+z[1]*z[1]+z[2]*z[2]);
  if(n>0) for(i=0;i<3;i++) z[i]/=n;
  xn=sqrt(z[0]*z[0]+z[2]*z[2]);
  if(xn>1e-12) {
   weaponPose.m[0][0]=z[2]/xn; weaponPose.m[2][0]=-z[0]/xn;
   for(i=0;i<3;i++) weaponPose.m[i][2]=z[i];
  }
  if(direction->EulerX || direction->EulerY || direction->EulerZ) {
   MATRIXCH m; EULER e=*direction;
   e.EulerX&=4095; e.EulerY&=4095; e.EulerZ&=4095;
   CreateEulerMatrix(&e,&m); TransposeMatrixCH(&m); VMMatrix(local.m,&m);
   VMCompose(&weaponPose,&weaponPose,&local);
  }
 }
 baseWeaponPose=weaponPose;
 MarineMotionWeaponPose(weaponPose.m,weaponPose.t);
 poseValid=1;
}
void ViewmodelUpdateFlash(int size, int length, const MATRIXCH *roll)
{
 VM_POSE local={0}; int i;
 VMMatrix(local.m,roll);
 for(i=0;i<3;i++) { local.m[0][i]*=size/65536.0; local.m[1][i]*=size/65536.0; local.m[2][i]*=length/65536.0; }
 local.t[0]=CentreOfMuzzleOffset.vx; local.t[1]=CentreOfMuzzleOffset.vy; local.t[2]=CentreOfMuzzleOffset.vz+200;
 VMCompose(&flashPose,&weaponPose,&local);
}
void ViewmodelBegin(DISPLAYBLOCK *dptr)
{
 ViewmodelDrawActive=poseValid && (dptr==&PlayersWeapon || dptr==&PlayersWeaponMuzzleFlash);
 ViewmodelShapeActive=0; drawSection=NULL;
 drawPose=weaponPose;
 if(dptr==&PlayersWeaponMuzzleFlash) {
  if(PWMFSDP) VMSectionPose(&drawPose,PWMFSDP);
  else drawPose=flashPose;
 }
}
void ViewmodelEnd(void) { ViewmodelMuzzleActive=ViewmodelDrawActive=ViewmodelShapeActive=0; drawSection=NULL; }
void ViewmodelSection(struct section_data *section) { drawSection=section; }

/* Solve M*eye=-t for exact local-space backface tests, including scaled flashes. */
static void VMLocalEye(const VM_POSE *p)
{
 double a=p->m[0][0],b=p->m[0][1],c=p->m[0][2],d=p->m[1][0],e=p->m[1][1],f=p->m[1][2],g=p->m[2][0],h=p->m[2][1],i=p->m[2][2];
 double det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
 if(fabs(det)<1e-12) { memset(ViewmodelLocalEye,0,sizeof ViewmodelLocalEye); return; }
 ViewmodelLocalEye[0]=-((e*i-f*h)*p->t[0]+(c*h-b*i)*p->t[1]+(b*f-c*e)*p->t[2])/det;
 ViewmodelLocalEye[1]=-((f*g-d*i)*p->t[0]+(a*i-c*g)*p->t[1]+(c*d-a*f)*p->t[2])/det;
 ViewmodelLocalEye[2]=-((d*h-e*g)*p->t[0]+(b*g-a*h)*p->t[1]+(a*e-b*d)*p->t[2])/det;
}
int ViewmodelTransform(SHAPEINSTR *instruction)
{
 const VECTORCH *v=(const VECTORCH*)*(int**)instruction->sh_instr_data;
 double scale[3]={1, (1+PredatorVisionChangeCounter/65536.0)*4.0/3.0, CameraZoomScale};
 int n,j;
 if(!ViewmodelDrawActive) return 0;
 sectionPose=drawPose;
 if(drawSection) VMSectionPose(&sectionPose,drawSection);
 VMLocalEye(&sectionPose);
 if(NAUSEA_CHEATMODE) {
  double sn=GetSin((CloakingPhase/3)&4095)/65536.0,co=GetCos((CloakingPhase/5)&4095)/65536.0;
  scale[0]=1+co*co; scale[1]=(1+sn*sn)*4.0/3.0;
 }
 if(MIRROR_CHEATMODE) scale[0]=-scale[0];
 for(n=0;n<instruction->sh_numitems;n++) {
  for(j=0;j<3;j++) ViewmodelPoints[n][j]=(sectionPose.t[j]+sectionPose.m[j][0]*v[n].vx+sectionPose.m[j][1]*v[n].vy+sectionPose.m[j][2]*v[n].vz)*scale[j];
  RotatedPts[n].vx=(int)ViewmodelPoints[n][0]; RotatedPts[n].vy=(int)ViewmodelPoints[n][1]; RotatedPts[n].vz=(int)ViewmodelPoints[n][2];
 }
 ViewmodelShapeActive=1;
 return 1;
}

/* Shared by float CPU clipping and GPU depth projection. World clipping is untouched. */
double ViewmodelNearClip(void)
{
 return AvP.PlayerType==I_Marine ? fmax(.1,M(WEAPON_NEAR)*CameraZoomScale) : 4;
}
void ViewmodelBeginMuzzle(void)
{
 ViewmodelBegin(&PlayersWeaponMuzzleFlash);
 ViewmodelMuzzleActive=ViewmodelDrawActive;
 ViewmodelShapeActive=ViewmodelMuzzleActive;
 if(PWMFSDP) VMSectionPose(&muzzlePose,PWMFSDP);
 else {
  VM_POSE local={{{1,0,0},{0,1,0},{0,0,1}},{0,0,0}};
  local.t[0]=CentreOfMuzzleOffset.vx;local.t[1]=CentreOfMuzzleOffset.vy;local.t[2]=CentreOfMuzzleOffset.vz;
  VMCompose(&muzzlePose,&weaponPose,&local);
 }
}
void ViewmodelMuzzlePoint(const VECTORCH *local,double out[3])
{
 double scale[3]={1,(1+PredatorVisionChangeCounter/65536.0)*4.0/3.0,CameraZoomScale};
 double point[3]={local->vx,local->vy,local->vz};
 if(NAUSEA_CHEATMODE){double sn=GetSin((CloakingPhase/3)&4095)/65536.0,co=GetCos((CloakingPhase/5)&4095)/65536.0;scale[0]=1+co*co;scale[1]=(1+sn*sn)*4.0/3.0;}
 if(MIRROR_CHEATMODE)scale[0]=-scale[0];
 for(int i=0;i<3;i++){out[i]=muzzlePose.t[i];for(int j=0;j<3;j++)out[i]+=muzzlePose.m[i][j]*point[j];out[i]*=scale[i];}
}

/* Return the actual rendered nozzle in the WORLD lens's view coordinates.
 * Conversion of X/Y by the lens ratio lets the regular world particle pass
 * meet the independent weapon viewport, without changing gameplay geometry. */
int ViewmodelFlameAnchor(double out[3])
{
 VIEWDESCRIPTORBLOCK *vdb=Global_VDB_Ptr;
 PLAYER_STATUS *ps;
 VM_POSE nozzle;
 extern int Observer;
 if(!poseValid || !vdb || !Player || !Player->ObStrategyBlock || AvP.PlayerType!=I_Marine || Observer)return 0;
 ps=(PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr;
 if(!ps || !ps->IsAlive || ps->WeaponSlot[ps->SelectedWeaponSlot].WeaponIDNumber!=WEAPON_FLAMETHROWER)return 0;
 weaponPose=baseWeaponPose;MarineMotionWeaponPose(weaponPose.m,weaponPose.t);
 if(PWMFSDP)VMSectionPose(&nozzle,PWMFSDP);
 else{
  VM_POSE local={{{1,0,0},{0,1,0},{0,0,1}},{0,0,0}};
  local.t[0]=CentreOfMuzzleOffset.vx;local.t[1]=CentreOfMuzzleOffset.vy;local.t[2]=CentreOfMuzzleOffset.vz;
  VMCompose(&nozzle,&weaponPose,&local);
 }
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)nozzle.t[i]+=weaponPose.m[i][j]*DevFlameValue(j);
 double scale[3]={1,(1+PredatorVisionChangeCounter/65536.0)*4.0/3.0,CameraZoomScale};
 if(NAUSEA_CHEATMODE){double sn=GetSin((CloakingPhase/3)&4095)/65536.0,co=GetCos((CloakingPhase/5)&4095)/65536.0;scale[0]=1+co*co;scale[1]=(1+sn*sn)*4.0/3.0;}
 if(MIRROR_CHEATMODE)scale[0]=-scale[0];
 double lens=WeaponFOVProjectionForViewSize(vdb->VDB_ClipRight-vdb->VDB_ClipLeft,vdb->VDB_ClipDown-vdb->VDB_ClipUp);
 double px=AvpProjectionX(vdb),py=AvpProjectionY(vdb);
 if(px<=0 || py<=0)return 0;
 out[0]=nozzle.t[0]*scale[0]*lens/px;
 out[1]=nozzle.t[1]*scale[1]*(lens*.75)/py;
 out[2]=nozzle.t[2]*scale[2];
 return out[2]>ViewmodelNearClip();
}

void ViewmodelFlameDirection(double viewDelta[3]){
 double local[3]={0,0,0},v[3]={viewDelta[0],viewDelta[1]*.75,viewDelta[2]};
 if(!poseValid || (!DevFlameValue(3)&&!DevFlameValue(4)&&!DevFlameValue(5)))return;
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)local[i]+=weaponPose.m[j][i]*v[j];
 DevFlameRotate(local);
 for(int i=0;i<3;i++){viewDelta[i]=0;for(int j=0;j<3;j++)viewDelta[i]+=weaponPose.m[i][j]*local[j];}
 viewDelta[1]*=4.0/3.0;
}
