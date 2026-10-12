#include "npc_aim_math.h"
static void MarineAimTick(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;
 if(m->My_Weapon->id!=MNPCW_Smartgun || !m->Target || !m->Target->DynPtr || !NPCSupportSmartgunHostile(sb,m->Target)){m->smartAimValid=0;return;}
 VECTORCH target,origin;NPCGetTargetPosition(&target,m->Target);
 if(m->My_Gunflash_Section)origin=m->My_Gunflash_Section->World_Offset;else GetTargetingPointOfObject_Far(sb,&origin);
 SubVector(&origin,&target);double n=sqrt((double)target.vx*target.vx+(double)target.vy*target.vy+(double)target.vz*target.vz);if(n<1)return;
 double desired[3]={target.vx/n,target.vy/n,target.vz/n};
 if(!m->smartAimValid || memcmp(m->smartAimTarget,m->Target->SBname,SB_NAME_LENGTH)){
  m->smartAim[0]=sb->DynPtr->OrientMat.mat31/(double)ONE_FIXED;m->smartAim[1]=0;m->smartAim[2]=sb->DynPtr->OrientMat.mat33/(double)ONE_FIXED;
  n=hypot(m->smartAim[0],m->smartAim[2]);if(n<.01){m->smartAim[0]=0;m->smartAim[2]=1;}else{m->smartAim[0]/=n;m->smartAim[2]/=n;}
  memcpy(m->smartAimTarget,m->Target->SBname,SB_NAME_LENGTH);m->smartAimValid=1;
 }
 NPCAimConverge(m->smartAim,desired,NormalFrameTime/(double)ONE_FIXED,TemplateWeapon[WEAPON_SMARTGUN].SmartTargetSpeed);
}
static VECTORCH MarineAimDirection(STRATEGYBLOCK*sb,VECTORCH*origin){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;VECTORCH direction=m->weaponTarget;
 if(m->Target && m->Target->DynPtr)NPCGetTargetPosition(&direction,m->Target);
 SubVector(origin,&direction);Normalise(&direction);return direction;
}
static MATRIXCH MarineAimMatrix(STRATEGYBLOCK*sb,VECTORCH*origin){MATRIXCH mat;VECTORCH d=MarineAimDirection(sb,origin);MakeMatrixFromDirection(&d,&mat);return mat;}
static void MarineMissShot(STRATEGYBLOCK*sb,STRATEGYBLOCK*target,VECTORCH*origin,MATRIXCH*art,enum AMMO_ID ammo,int rounds){
 MATRIXCH aim=target?MarineAimMatrix(sb,origin):*art;ProjectNPCShot(sb,target,origin,&aim,ammo,rounds);
}
static void MarineSmartRounds(STRATEGYBLOCK*sb,int rounds){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;if(!m->smartAimValid || !NPCSupportSmartgunHostile(sb,m->Target))return;
 VECTORCH centre={(int)(m->smartAim[0]*ONE_FIXED),(int)(m->smartAim[1]*ONE_FIXED),(int)(m->smartAim[2]*ONE_FIXED)};MATRIXCH aim;MakeMatrixFromDirection(&centre,&aim);
 /* Separate native rounds in a small cone, never a multiplied perfect hit. */
 for(int i=0;i<rounds;i++){
  double angle=(FastRandom()&65535)*(6.283185307179586/65536.),radius=sqrt((FastRandom()&65535)/65535.)*.006;
  int x=(int)(cos(angle)*radius*ONE_FIXED),y=(int)(sin(angle)*radius*ONE_FIXED);
  VECTORCH d={centre.vx+MUL_FIXED(aim.mat11,x)+MUL_FIXED(aim.mat21,y),centre.vy+MUL_FIXED(aim.mat12,x)+MUL_FIXED(aim.mat22,y),centre.vz+MUL_FIXED(aim.mat13,x)+MUL_FIXED(aim.mat23,y)};Normalise(&d);
  CastLOSProjectile(sb,&m->My_Gunflash_Section->World_Offset,&d,MarineWeaponAmmo(sb),1,0);
 }
}
static int MarineProjectileAim(STRATEGYBLOCK*sb,VECTORCH*origin,int grenade,MATRIXCH*mat){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;VECTORCH target=m->weaponTarget;if(m->Target && m->Target->DynPtr)NPCGetTargetPosition(&target,m->Target);
 double offset[3]={(double)target.vx-origin->vx,(double)target.vy-origin->vy,(double)target.vz-origin->vz};DYNAMICSBLOCK*d=m->Target?m->Target->DynPtr:NULL;
 double velocity[3]={d?(double)d->LinVelocity.vx+d->LinImpulse.vx:0,d?(double)d->LinVelocity.vy+d->LinImpulse.vy:0,d?(double)d->LinVelocity.vz+d->LinImpulse.vz:0},dir[3],flight;
 if(!NPCAimSolve(offset,velocity,grenade==1?70000:grenade==2?20000:80000,grenade==1?GRAVITY_STRENGTH:0,dir,&flight))return 0;
 VECTORCH direction={(int)(dir[0]*ONE_FIXED),(int)(dir[1]*ONE_FIXED),(int)(dir[2]*ONE_FIXED)};Normalise(&direction);MakeMatrixFromDirection(&direction,mat);return 1;
}
static STRATEGYBLOCK*MarineLaunchRocket(STRATEGYBLOCK*sb,VECTORCH*origin){MATRIXCH aim;if(!MarineShotReady(sb,0) || !MarineProjectileAim(sb,origin,0,&aim))return NULL;STRATEGYBLOCK*shot=CreateRocketKernel(origin,&aim,0);if(shot)MarineShotCommitted(sb,0);return shot;}
static STRATEGYBLOCK*MarineLaunchGrenade(STRATEGYBLOCK*sb,AVP_BEHAVIOUR_TYPE type,VECTORCH*origin){MATRIXCH aim;if(!MarineShotReady(sb,0) || !MarineProjectileAim(sb,origin,1,&aim))return NULL;STRATEGYBLOCK*shot=CreateGrenadeKernel(type,origin,&aim,0);if(shot)MarineShotCommitted(sb,0);return shot;}
static STRATEGYBLOCK*MarineLaunchFrisbee(STRATEGYBLOCK*sb,VECTORCH*origin){MATRIXCH aim;if(!MarineShotReady(sb,0) || !MarineProjectileAim(sb,origin,2,&aim))return NULL;STRATEGYBLOCK*shot=CreateFrisbeeKernel(origin,&aim,0);if(shot)MarineShotCommitted(sb,0);return shot;}
