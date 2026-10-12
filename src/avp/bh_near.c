#include "3dc.h"

#include "inline.h"
#include "module.h"

#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"

#include "dynblock.h"

#include "pheromon.h"
#include "bh_pred.h"
#include "bh_alien.h"
#include "bh_near.h"
#include "bh_far.h"
#include "bh_marin.h"
#include "pfarlocs.h"
#include "targeting.h"
#include "weapons.h"
#define UseLocalAssert Yes
#include "ourasert.h"

#include "psnd.h"
#include "psndplat.h"
#include "dxlog.h"
#include "showcmds.h"
#include "ai_sight.h"
#include "los.h"
#include "bh_gener.h"
#include "pldnet.h"
#include "particle.h"
#include "bh_corpse.h"
#include "scream.h"

extern int NormalFrameTime;
extern int ShowHiveState;
extern ACTIVESOUNDSAMPLE ActiveSounds[];

/* Patrick 27/6/97 *********************************
Prototypes for behaviour functions
****************************************************/
static void AlienNearState_Approach(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Attack(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Wander(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Wait(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Avoidance(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Hunt(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Retreat(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Pounce(STRATEGYBLOCK *sbPtr);
static void AlienNearState_Jump(STRATEGYBLOCK *sbPtr);
void AlienNearState_Taunting(STRATEGYBLOCK *sbPtr);
void AlienNearState_Dormant(STRATEGYBLOCK *sbPtr);
void AlienNearState_Awakening(STRATEGYBLOCK *sbPtr);
static int GoToJump(STRATEGYBLOCK *sbPtr);
int TargetIsFiringFlamethrowerAtAlien(STRATEGYBLOCK *sbPtr);
static int StartAlienTaunt(STRATEGYBLOCK *sbPtr);
void StartAlienMovementSequence(STRATEGYBLOCK *sbPtr);
void AlienHandleMovingAnimation(STRATEGYBLOCK *sbPtr);

extern void Execute_Alien_Dying(STRATEGYBLOCK *sbPtr);

int AlienIsAbleToClimb(STRATEGYBLOCK *sbPtr);
int AlienIsAbleToStand(STRATEGYBLOCK *sbPtr);
static int StartAlienPounce(STRATEGYBLOCK *sbPtr);
static int AlienHasPathToTarget(STRATEGYBLOCK *sbPtr);

extern ATTACK_DATA Alien_Special_Gripping_Attack;

/* Patrick 27/6/97 *********************************
Functions...
****************************************************/

int AlienStandingTauntList[] = {
	(int)ASSS_Taunt,
	(int)ASSS_Taunt2,
	(int)ASSS_Taunt3,
	(int)ASSS_Fear,
	-1,
};

void Alien_ElectToCrawl(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;

	LOCALASSERT(sbPtr);	
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	switch(alienStatusPointer->Type) {
		case AT_Standard:
		default:
			if ((FastRandom()%65535)<16384) {
				alienStatusPointer->PreferToCrouch=1;
			} else {
				alienStatusPointer->PreferToCrouch=0;
			}
			break;
		case AT_Predalien:
			alienStatusPointer->PreferToCrouch=0;
			break;
		case AT_Praetorian:
			if ((FastRandom()%65535)<16384) {
				alienStatusPointer->PreferToCrouch=~alienStatusPointer->PreferToCrouch;
			}
			break;
	}

}

void Alien_ElectToPounce(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;

	LOCALASSERT(sbPtr);	
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	if (alienStatusPointer->PounceDetected==0) {
		/* Can't pounce, won't pounce. */
		alienStatusPointer->EnablePounce=0;
		return;
	}
	
	switch(alienStatusPointer->Type) {
		case AT_Standard:
		default:
			if ((FastRandom()%65535)<16384) {
				alienStatusPointer->EnablePounce=1;
			} else {
				alienStatusPointer->EnablePounce=0;
			}
			break;
		case AT_Predalien:
			if ((FastRandom()%65535)<16384) {
				alienStatusPointer->EnablePounce=0;
			} else {
				alienStatusPointer->EnablePounce=1;
			}
			break;
		case AT_Praetorian:
			if (alienStatusPointer->PreferToCrouch) {
				/* Why not? */
				alienStatusPointer->EnablePounce=1;
			} else if ((FastRandom()%65535)<16384) {
				alienStatusPointer->EnablePounce=0;
			} else {
				alienStatusPointer->EnablePounce=1;
			}
			break;
	}
}


/* Navigation cannot detach an already attached alien merely because an AI
 * state changes. Pounces/jumps retain their explicit detach operations. */
static void AlienNavigationGravity(STRATEGYBLOCK *sb,int allowClimb)
{
 DYNAMICSBLOCK *d=sb->DynPtr;
 if(((ALIEN_STATUS_BLOCK*)sb->SBdataptr)->motion.launchLocked || !AlienIsAbleToClimb(sb)){d->UseStandardGravity=1;return;}
 if(allowClimb || (!d->UseStandardGravity &&
    (d->GravityDirection.vy<63000 || d->OrientMat.mat22<63000)))d->UseStandardGravity=0;
 else d->UseStandardGravity=1;
}
static int AlienCorpseVisible(STRATEGYBLOCK *sb,STRATEGYBLOCK *corpse)
{
 VECTORCH eye,point,direction,oldPoint=LOS_Point,oldNormal=LOS_ObjectNormal;
 DISPLAYBLOCK *oldHit=LOS_ObjectHitPtr;
 SECTION_DATA *oldSection=LOS_HModel_Section;
 int oldLambda=LOS_Lambda,visible;
 if(!sb->SBdptr || !corpse->SBdptr)return 0;
 GetTargetingPointOfObject(sb->SBdptr,&eye);
 GetTargetingPointOfObject(corpse->SBdptr,&point);
 direction.vx=point.vx-eye.vx;direction.vy=point.vy-eye.vy;direction.vz=point.vz-eye.vz;
 if(!direction.vx && !direction.vy && !direction.vz)return 1;
 Normalise(&direction);
 /* The invisible dead player must not conceal its own physical corpse. */
 FindPolygonInLineOfSight_TwoIgnores(&direction,&eye,0,sb->SBdptr,Player);
 visible=LOS_ObjectHitPtr==corpse->SBdptr;
 LOS_Point=oldPoint;LOS_ObjectNormal=oldNormal;LOS_ObjectHitPtr=oldHit;
 LOS_HModel_Section=oldSection;LOS_Lambda=oldLambda;return visible;
}
static int AlienCorpseAlreadyClaimed(STRATEGYBLOCK *sb,STRATEGYBLOCK *corpse)
{
 int i;
 for(i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK *other=ActiveStBlockList[i];ALIEN_STATUS_BLOCK *a;
  if(!other || other==sb || other->I_SBtype!=I_BehaviourAlien || !other->SBdptr ||
     !other->SBdataptr || other->SBDamageBlock.Health<=0)continue;
  a=(ALIEN_STATUS_BLOCK*)other->SBdataptr;
  if(!a->Target && (a->motion.corpsePhase==1 || a->motion.corpsePhase==2) &&
     NAME_ISEQUAL(a->motion.corpseName,corpse->SBname))return 1;
 }
 return 0;
}
/* One reachable feeder per body; everyone else uses normal wander/avoidance.
 * Timers are simulated seconds, not random trials per rendered frame. */
static int AlienAfterPlayerDeath(STRATEGYBLOCK *sb)
{
 ALIEN_STATUS_BLOCK *a=(ALIEN_STATUS_BLOCK*)sb->SBdataptr;
 ALIEN_MOTION_RUNTIME *r=&a->motion;
 PLAYER_STATUS *player=Player?(PLAYER_STATUS*)Player->ObStrategyBlock->SBdataptr:NULL;
 STRATEGYBLOCK *corpse;
 VECTORCH direction,target;
 int distance,range;
 if(AvP.Network!=I_No_Network || !player || player->IsAlive || a->Target ||
    a->BehaviourState==ABS_Dying || a->BehaviourState==ABS_Dormant ||
    a->BehaviourState==ABS_Awakening){
  if(r->aftermath){
   r->aftermath=0;r->pace=ONE_FIXED;r->corpsePhase=0;r->corpseRetry=0;
   r->corpseName[0]=0;
   if(a->BehaviourState!=ABS_Dying && a->BehaviourState!=ABS_Dormant && a->BehaviourState!=ABS_Awakening)
    StartAlienMovementSequence(sb);
  }
  return 0;
 }
 if(!r->aftermath){
  r->aftermath=1;r->paceTimer=0;r->corpseRetry=0;
  NPC_InitWanderData(&a->wanderData);NPC_InitMovementData(&a->moveData);
  InitWaypointManager(&a->waypointManager);
 }
 if(NormalFrameTime<=0)return r->corpsePhase==2;
 r->paceTimer-=NormalFrameTime;
 if(r->paceTimer<=0){
  r->pace=(FastRandom()&1)?ONE_FIXED*35/100:ONE_FIXED*60/100;
  r->paceTimer=3*ONE_FIXED+(FastRandom()&65535)*3;
 }
 corpse=player->MyCorpse;
 /* Resolve the current body each frame, never retain a potentially freed pointer. */
 if(!corpse || corpse->I_SBtype!=I_BehaviourNetCorpse || !corpse->DynPtr ||
    !corpse->SBdataptr || !corpse->SBdptr ||
    ((NETCORPSEDATABLOCK*)corpse->SBdataptr)->timer<ONE_FIXED ||
    ((NETCORPSEDATABLOCK*)corpse->SBdataptr)->GibbFactor>ONE_FIXED/2){
  r->corpsePhase=0;return 0;
 }
 if(!NAME_ISEQUAL(r->corpseName,corpse->SBname)){
  COPY_NAME(r->corpseName,corpse->SBname);r->corpsePhase=0;r->corpseRetry=0;
  r->feedTime=0;r->feedStall=0;r->bitten=0;
 }
 direction.vx=corpse->DynPtr->Position.vx-sb->DynPtr->Position.vx;
 direction.vy=corpse->DynPtr->Position.vy-sb->DynPtr->Position.vy;
 direction.vz=corpse->DynPtr->Position.vz-sb->DynPtr->Position.vz;
 distance=Magnitude(&direction);range=16000;
 r->corpseRetry-=NormalFrameTime;
 if(!r->corpsePhase){
  if(r->corpseRetry>0)return 0;
  r->corpseRetry=ONE_FIXED/5;
  if(distance>range || !HModelSequence_Exists(&a->HModelController,HMSQT_AlienStand,ASSS_Feed) ||
     AlienCorpseAlreadyClaimed(sb,corpse) || !AlienCorpseVisible(sb,corpse))return 0;
  r->corpsePhase=1;r->feedBestDistance=distance;r->feedStall=0;
  NPC_InitMovementData(&a->moveData);InitWaypointManager(&a->waypointManager);
 }
 if(r->corpsePhase==3)return 0;
 if(r->corpseRetry<=0){
  r->corpseRetry=ONE_FIXED/5;
  if(distance>range || !AlienCorpseVisible(sb,corpse)){
   r->corpsePhase=0;r->corpseRetry=5*ONE_FIXED;return 0;
  }
 }
 if(r->corpsePhase==1){
  double horizontal=(double)direction.vx*direction.vx+(double)direction.vz*direction.vz;
  if(horizontal>1800.0*1800.0 || abs(direction.vy)>2500 || !sb->DynPtr->IsInContactWithFloor){
   r->feedStall+=NormalFrameTime;
   if(distance+100<r->feedBestDistance){r->feedBestDistance=distance;r->feedStall=0;}
   if(r->feedStall>2*ONE_FIXED){
    r->corpsePhase=0;r->corpseRetry=5*ONE_FIXED;
    NPC_InitWanderData(&a->wanderData);return 0;
   }
   target=corpse->DynPtr->Position;
   AlienNavigationGravity(sb,1);a->IAmCrouched=AlienShouldBeCrawling(sb);
   AlienHandleMovingAnimation(sb);
   NPCGetMovementDirection(sb,&direction,&target,&a->waypointManager);
   NPCSetVelocity(sb,&direction,a->MaxSpeed);
   return 1;
  }
  /* Stay on the same surface; do not snap the NPC to the body or world up. */
  if(sb->DynPtr->OrientMat.mat22<63000){
   r->corpsePhase=0;r->corpseRetry=5*ONE_FIXED;return 0;
  }
  r->corpsePhase=2;r->feedTime=0;r->feedTimer=0;r->feedSound=0;a->IAmCrouched=0;
  SetAlienShapeAnimSequence_Core(sb,HMSQT_AlienStand,ASSS_Feed,2*ONE_FIXED,ONE_FIXED/4);
 }
 if(r->corpsePhase==2){
  if(distance>4000){r->corpsePhase=0;r->corpseRetry=5*ONE_FIXED;return 0;}
  NPCOrientateToVector(sb,&direction,NPC_TURNRATE/2,NULL);
  r->feedTime+=NormalFrameTime;
   if(AlienFeedSoundEvent(r,a->HModelController.sequence_timer,
       a->HModelController.keyframe_flags,a->HModelController.Tweening)){
    /* Independent one-shots: vocal/fire handles must not suppress the jaw. */
    Sound_Play(SID_ALIEN_JAW_ATTACK,"dh",&sb->DynPtr->Position);
    /* These five loaded native samples are the engine's flesh-ripping
     * family; SID_RIP is a legacy unloaded slot in stock data. */
    Sound_Play(SID_BODY_BEING_HACKED_UP_0+(r->feedTime/(2*ONE_FIXED))%5,
        "dv",&corpse->DynPtr->Position,96);
   }
  if(!r->bitten && r->feedTime>=ONE_FIXED &&
     (a->HModelController.keyframe_flags || r->feedTime>=2*ONE_FIXED)){
   NETCORPSEDATABLOCK *body=(NETCORPSEDATABLOCK*)corpse->SBdataptr;
   SECTION_DATA *head=GetThisSectionData(body->HModelController.section_data,"head");
   r->bitten=1;
   if(head && !(head->flags&section_data_notreal))
    CauseDamageToHModel(&body->HModelController,corpse,
      &TemplateAmmo[AMMO_ALIEN_BITE_KILLSECTION].MaxDamage[AvP.Difficulty],ONE_FIXED,head,NULL,NULL,0);
  }
  if(r->feedTime>=6*ONE_FIXED){
   r->corpsePhase=3;NPC_InitWanderData(&a->wanderData);
   StartAlienMovementSequence(sb);return 0;
  }
  return 1;
 }
 return 0;
}

void NearAlienBehaviour(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;
	char *descriptor;

	LOCALASSERT(sbPtr);	
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);	

	if (ShowSlack) {
		int synthSpeed,setSpeed,slack;
		VECTORCH offset;
		extern int SlackTotal;
		extern int SlackSize;
				
		offset.vx=(sbPtr->DynPtr->Position.vx-sbPtr->DynPtr->PrevPosition.vx);
		offset.vy=(sbPtr->DynPtr->Position.vy-sbPtr->DynPtr->PrevPosition.vy);
		offset.vz=(sbPtr->DynPtr->Position.vz-sbPtr->DynPtr->PrevPosition.vz);
		
		synthSpeed=Magnitude(&offset);
		synthSpeed=DIV_FIXED(synthSpeed,NormalFrameTime);
		setSpeed=Magnitude(&sbPtr->DynPtr->LinVelocity);
		
		if (setSpeed) {		
			slack=(ONE_FIXED-(DIV_FIXED(synthSpeed,setSpeed)));
			SlackTotal+=slack;
			SlackSize++;
		}
		#if 0
		PrintDebuggingText("MaxSpeed = %d, SynthSpeed = %d, SetSpeed = %d, Slack %d\n",alienStatusPointer->MaxSpeed,synthSpeed,setSpeed,slack);
		#endif
	}


    {
        double dx=(double)sbPtr->DynPtr->Position.vx-sbPtr->DynPtr->PrevPosition.vx;
        double dy=(double)sbPtr->DynPtr->Position.vy-sbPtr->DynPtr->PrevPosition.vy;
        double dz=(double)sbPtr->DynPtr->Position.vz-sbPtr->DynPtr->PrevPosition.vz;
        AlienMotionObserve(&alienStatusPointer->motion,sqrt(dx*dx+dy*dy+dz*dz),NormalFrameTime);
    }
    AlienLaunchObserve(&alienStatusPointer->motion,sbPtr->DynPtr->IsInContactWithFloor,
        DotProduct(&sbPtr->DynPtr->LinImpulse,&sbPtr->DynPtr->GravityDirection)<-100,
        NormalFrameTime);
	/* zero alien's velocity */
	{
		DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
		dynPtr->LinVelocity.vx = dynPtr->LinVelocity.vy = dynPtr->LinVelocity.vz = 0;
	}

	InitWaypointSystem(alienStatusPointer->EnableWaypoints);
    if(AlienAfterPlayerDeath(sbPtr))return;
    {
        DYNAMICSBLOCK *dyn=sbPtr->DynPtr;
        int navigating=alienStatusPointer->BehaviourState==ABS_Approach ||
            alienStatusPointer->BehaviourState==ABS_Hunt ||
            alienStatusPointer->BehaviourState==ABS_Wander ||
            alienStatusPointer->BehaviourState==ABS_Retreat ||
            alienStatusPointer->BehaviourState==ABS_Avoidance;
        if(AlienRouteStalled(&alienStatusPointer->motion,dyn->Position.vx,
            dyn->Position.vy,dyn->Position.vz,NormalFrameTime,navigating)) {
            NPC_InitMovementData(&alienStatusPointer->moveData);
            NPC_InitWanderData(&alienStatusPointer->wanderData);
            InitWaypointManager(&alienStatusPointer->waypointManager);
            Initialise_AvoidanceManager(sbPtr,&alienStatusPointer->avoidanceManager);
            alienStatusPointer->huntingModule=NULL;
            alienStatusPointer->CurveTimeOut=0;
            alienStatusPointer->NearStateTimer=0;
            alienStatusPointer->BehaviourState=ABS_Wander;
        }
    }


	if (alienStatusPointer->incidentFlag) {
		Alien_ElectToPounce(sbPtr);
		Alien_ElectToCrawl(sbPtr);
	}

	switch (alienStatusPointer->BehaviourState)
	{		
		case ABS_Wait:
		{
			AlienNearState_Wait(sbPtr);
			descriptor="Waiting";
			break;		
		}		
		case ABS_Approach:
		{
			AlienNearState_Approach(sbPtr);
			descriptor="Approaching";
			break;
		}
		case ABS_Jump:
		{
			AlienNearState_Jump(sbPtr);
			descriptor="Jumping";
			break;
		}
		case ABS_Hunt:
		{
			AlienNearState_Hunt(sbPtr);
			descriptor="Hunting";
			break;
		}
		case ABS_Wander:
		{
			AlienNearState_Wander(sbPtr);
			descriptor="Wandering";
			break;
		}
		case ABS_Retreat:
		{
			AlienNearState_Retreat(sbPtr);
			descriptor="Retreating";
			break;
		}
		case ABS_Attack:
		{
			AlienNearState_Attack(sbPtr);
			descriptor="Attacking";
			break;
		}
		case ABS_Pounce:
		{
			AlienNearState_Pounce(sbPtr);
			descriptor="Pouncing";
			break;
		}
		case ABS_Avoidance:
		{
			AlienNearState_Avoidance(sbPtr);
			descriptor="Avoiding";
			break;		
		}		
		case ABS_Dying:
		{
			Execute_Alien_Dying(sbPtr);
			descriptor="Dying";
			break;
		}
		case ABS_Dormant:
		{
			AlienNearState_Dormant(sbPtr);
			descriptor="Dormant";
			break;		
		}		
		case ABS_Awakening:
		{
			AlienNearState_Awakening(sbPtr);
			descriptor="Awakening";
			break;		
		}		
		case ABS_Taunting:
		{
			AlienNearState_Taunting(sbPtr);
			descriptor="Taunting";
			break;		
		}		
		default:
		{
			GLOBALASSERT(1==0);		
  			break;
  		}
  	}

	if (ShowHiveState) {
		/* Alien position print. */

		MODULE *thisModule = sbPtr->containingModule;
		
		LOCALASSERT(thisModule);

		PrintDebuggingText("Near %s alien is in module %d, %s\n",descriptor,thisModule->m_index,thisModule->name);

	}

	#if 0
	//now dealt with in AddPlayerAndObjectUpdateMessages
	/*  */
	if (AvP.Network != I_No_Network)
	{
		AddNetMsg_AlienAIState(sbPtr);
	}
	/*  */
	#endif
}	

int GetAlienCrawlMode(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;
	
	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	/* Are we on a ceiling or wall? */
	if(sbPtr->DynPtr->OrientMat.mat22<63000) {
		return((int)ACSS_Standard);
	}

	/* Wounding! */
	/* Crawl Hurt if you've lost any leg sections, or both feet. */
	if ((alienStatusPointer->Wounds&(section_flag_left_leg|section_flag_right_leg))
		||((alienStatusPointer->Wounds&section_flag_left_foot)
		&&(alienStatusPointer->Wounds&section_flag_right_foot))) {
	
		if (HModelSequence_Exists(&alienStatusPointer->HModelController,HMSQT_AlienCrawl,ACSS_Crawl_Hurt)) {
			return((int)ACSS_Crawl_Hurt);
		} else {
			return((int)ACSS_Standard);
		}
	}
	/* Else, try to scamper. */

	if (HModelSequence_Exists(&alienStatusPointer->HModelController,HMSQT_AlienCrawl,ACSS_Scamper)) {
		return((int)ACSS_Scamper);
	} else {
		return((int)ACSS_Standard);
	}

}

void AlienHandleStandingAnimation(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);
	/* Make sure we're playing a 'standing still' sequence. */

	if (alienStatusPointer->IAmCrouched) {
		if ((alienStatusPointer->HModelController.Sequence_Type!=HMSQT_AlienCrouch)
			||(alienStatusPointer->HModelController.Sub_Sequence!=ACSS_Standard)) {
			SetAlienShapeAnimSequence_Core(sbPtr,HMSQT_AlienCrouch,ACSS_Standard,-1,(ONE_FIXED>>3));
		}
	} else {
		if ((alienStatusPointer->HModelController.Sequence_Type!=HMSQT_AlienStand)
			||(alienStatusPointer->HModelController.Sub_Sequence!=ASSS_Standard)) {
			SetAlienShapeAnimSequence_Core(sbPtr,HMSQT_AlienStand,ASSS_Standard,-1,(ONE_FIXED>>3));
		}
	}

}

void AlienHandleMovingAnimation(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;
	int crawlMode;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);
	/* Make sure we're playing a 'moving' sequence, if we're actually moving. */


    /* Windowed, hysteretic motion state: a zero-displacement frame at high
     * FPS or a polygon seam must not restart stand/run/crawl tweening. */
    if(!alienStatusPointer->motion.moving){
        AlienHandleStandingAnimation(sbPtr);return;
    }
    if(alienStatusPointer->motion.aftermath && !alienStatusPointer->Target &&
       !alienStatusPointer->HModelController.Tweening &&
       (alienStatusPointer->HModelController.Sequence_Type==HMSQT_AlienRun ||
        alienStatusPointer->HModelController.Sequence_Type==HMSQT_AlienCrawl)){
        int duration=DIV_FIXED(alienStatusPointer->IAmCrouched?ONE_FIXED>>2:ONE_FIXED>>1,
                               alienStatusPointer->motion.pace);
        #if WOUNDING_SPEED_EFFECTS
        {
            int factor=GetAlienSpeedFactor_ForSequence(sbPtr,
                alienStatusPointer->HModelController.Sequence_Type,
                alienStatusPointer->HModelController.Sub_Sequence);
            if(factor!=ONE_FIXED)duration=DIV_FIXED(duration,factor);
        }
        #endif
        if(duration!=alienStatusPointer->HModelController.Seconds_For_Sequence)
            HModel_ChangeSpeed(&alienStatusPointer->HModelController,duration);
    }
	/* Right, now play the correct sort of sequence. */	

	if(alienStatusPointer->IAmCrouched) {
		crawlMode=GetAlienCrawlMode(sbPtr);
		if ((alienStatusPointer->HModelController.Sequence_Type!=HMSQT_AlienCrawl)
			||(alienStatusPointer->HModelController.Sub_Sequence!=crawlMode)) {
			/* Replace this when I implement speed properly! */
			StartAlienMovementSequence(sbPtr);
		}
	} else {
		if ((alienStatusPointer->HModelController.Sequence_Type!=HMSQT_AlienRun)
			||(alienStatusPointer->HModelController.Sub_Sequence!=ARSS_Standard)) {
			/* Replace this when I implement speed properly! */
			StartAlienMovementSequence(sbPtr);
		}
	}

}

void StartAlienMovementSequence(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;
	int crawlMode,duration;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

    duration=ONE_FIXED>>1;
    if(alienStatusPointer->motion.aftermath && !alienStatusPointer->Target)
        duration=DIV_FIXED(duration,alienStatusPointer->motion.pace);
	if(alienStatusPointer->IAmCrouched) {
		crawlMode=GetAlienCrawlMode(sbPtr);
		SetAlienShapeAnimSequence(sbPtr,HMSQT_AlienCrawl,crawlMode,duration>>1);
	} else {
		SetAlienShapeAnimSequence(sbPtr,HMSQT_AlienRun,(int)ARSS_Standard,duration);					
	}
	RecomputeAlienSpeed(sbPtr);

}

void Force_Alien_Running_Sequence(STRATEGYBLOCK *sbPtr) {

	int redo;
	ALIEN_STATUS_BLOCK *alienStatusPointer;
	
	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);
	
	redo=0;

	switch(alienStatusPointer->HModelController.Sequence_Type) {
		case ((int)HMSQT_AlienCrawl):
		{
			int crawlMode;
			
			crawlMode=GetAlienCrawlMode(sbPtr);

			if (alienStatusPointer->HModelController.Sub_Sequence!=crawlMode) {
				redo=1;
			}
			break;
		}
		case ((int)HMSQT_AlienRun):
		{
			if (alienStatusPointer->HModelController.Sub_Sequence!=(int)ARSS_Standard) {
				redo=1;
			}
			break;
		}
		case ((int)HMSQT_AlienStand):
		{
			//if (alienStatusPointer->HModelController.Sub_Sequence!=(int)ASSS_Standard) {
				redo=1;
			//}
			break;
		}
		case ((int)HMSQT_AlienCrouch):
		{
			//if (alienStatusPointer->HModelController.Sub_Sequence!=(int)ACrSS_Standard) {
				redo=1;
			//}
			break;
		}
		default:
		{
			GLOBALASSERT(0);
			break;
		}
	}

	if (redo) {

		StartAlienMovementSequence(sbPtr);

	}

	GLOBALASSERT(alienStatusPointer->HModelController.Playing);	

}

void StartAlienAttackSequence(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	ATTACK_DATA *thisAttack;
	int collidingWithTarget;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	LOCALASSERT(alienStatusPointer->Target);

	/* Test for gripping attack! */
	collidingWithTarget=0;
	{
		struct collisionreport *nextReport;
		nextReport = sbPtr->DynPtr->CollisionReportPtr;

		while(nextReport)
		{		
 			if(nextReport->ObstacleSBPtr)
 			{	
				if (nextReport->ObstacleSBPtr==alienStatusPointer->Target) {
					collidingWithTarget=1;
				}
			} 		
 			nextReport = nextReport->NextCollisionReportPtr;
		}
	}

	if (HModelSequence_Exists(&alienStatusPointer->HModelController,Alien_Special_Gripping_Attack.Sequence_Type,
		Alien_Special_Gripping_Attack.Sub_Sequence)) {
		VECTORCH offset,mypos;
		int dot;
		
		GetTargetingPointOfObject_Far(alienStatusPointer->Target,&offset);
		GetTargetingPointOfObject_Far(sbPtr,&mypos);
		offset.vx-=mypos.vx;
		offset.vy-=mypos.vy;
		offset.vz-=mypos.vz;
		Normalise(&offset);

		if (sbPtr->DynPtr->UseStandardGravity) {
			sbPtr->DynPtr->GravityDirection.vx=0;
			sbPtr->DynPtr->GravityDirection.vy=65536;
			sbPtr->DynPtr->GravityDirection.vz=0;
		}

		dot=DotProduct(&offset,&sbPtr->DynPtr->GravityDirection);

		if ((dot>16962)&&(collidingWithTarget)) { /* 75 degs... */
			thisAttack=&Alien_Special_Gripping_Attack;
		} else {
			thisAttack=GetAlienAttackSequence(&alienStatusPointer->HModelController,alienStatusPointer->Wounds,
				alienStatusPointer->IAmCrouched);
		}
	} else {
		thisAttack=GetAlienAttackSequence(&alienStatusPointer->HModelController,alienStatusPointer->Wounds,
			alienStatusPointer->IAmCrouched);
	}
	
	GLOBALASSERT(thisAttack);

	SetAlienShapeAnimSequence_Core(sbPtr,thisAttack->Sequence_Type,thisAttack->Sub_Sequence,
		thisAttack->Sequence_Length,thisAttack->TweeningTime);

	alienStatusPointer->current_attack=thisAttack;

}

static enum AMMO_ID GetAttackDamageType(STRATEGYBLOCK *sbPtr,int flagnum) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	if (alienStatusPointer->current_attack==NULL) {
		return(AMMO_NONE);
	}

	/* Fix praetorian damage here! */

	switch (alienStatusPointer->Type) {
		case AT_Standard:
		default:
			/* No change. */
			return(alienStatusPointer->current_attack->flag_damage[flagnum]);
			break;
		case AT_Predalien:
			{
				switch (alienStatusPointer->current_attack->flag_damage[flagnum]) {
					case AMMO_NPC_ALIEN_CLAW:
					default:
						return(AMMO_NPC_PREDALIEN_CLAW);
						break;
					case AMMO_NPC_ALIEN_TAIL:
						return(AMMO_NPC_PREDALIEN_TAIL);
						break;
					case AMMO_NPC_ALIEN_BITE:
						return(AMMO_NPC_PREDALIEN_BITE);
						break;
				}
			}
			break;
		case AT_Praetorian:
			{
				switch (alienStatusPointer->current_attack->flag_damage[flagnum]) {
					case AMMO_NPC_ALIEN_CLAW:
					default:
						return(AMMO_NPC_PRAETORIAN_CLAW);
						break;
					case AMMO_NPC_ALIEN_TAIL:
						return(AMMO_NPC_PRAETORIAN_TAIL);
						break;
					case AMMO_NPC_ALIEN_BITE:
						return(AMMO_NPC_PRAETORIAN_BITE);
						break;
				}
			}
			break;
	}

	/* What? */	
	return(AMMO_NONE);

}

#if 0
static void DoAlienAIAttackSound(STRATEGYBLOCK *sbPtr) {

	DYNAMICSBLOCK *dynPtr;
	SOUNDINDEX soundIndex;
	ALIEN_STATUS_BLOCK *alienStatusPointer;

	GLOBALASSERT(sbPtr);
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(dynPtr);

	/* This one is for ALIEN SWIPE SOUND. */

	soundIndex=SID_NOSOUND;

	PlayAlienSound((int)alienStatusPointer->Type,ASC_Swipe,0,
		NULL,&sbPtr->DynPtr->Position);

	#if 0	
	if (AvP.Network != I_No_Network)
	{
		soundIndex=ActiveSounds[alienStatusPointer->soundHandle2].soundIndex;
		if (soundIndex!=SID_NOSOUND) {
			AddNetMsg_SpotAlienSound(soundIndex,pitch,&dynPtr->Position);
		}
	}
	#endif

}
#endif

static void DoAlienAIRandomHiss(STRATEGYBLOCK *sbPtr) {

	DYNAMICSBLOCK *dynPtr;
	SOUNDINDEX soundIndex;
	ALIEN_STATUS_BLOCK *alienStatusPointer;

	GLOBALASSERT(sbPtr);
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(dynPtr);

	soundIndex=SID_NOSOUND;

	/* This one is for ALIEN GENERAL SCREAM. */

	soundIndex=SID_NOSOUND;

	if (alienStatusPointer->soundHandle2==SOUND_NOACTIVEINDEX) {
		PlayAlienSound((int)alienStatusPointer->Type,ASC_Scream_General,0,
			&alienStatusPointer->soundHandle2,&sbPtr->DynPtr->Position);

		if (AvP.Network != I_No_Network)
		{
			AddNetMsg_SpotAlienSound((int)ASC_Scream_General,(int)alienStatusPointer->Type,0,&dynPtr->Position);
		}
	}

}

static void DoAlienAITauntHiss(STRATEGYBLOCK *sbPtr) {

	DYNAMICSBLOCK *dynPtr;
	SOUNDINDEX soundIndex;
	ALIEN_STATUS_BLOCK *alienStatusPointer;

	GLOBALASSERT(sbPtr);
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(dynPtr);

	soundIndex=SID_NOSOUND;

	/* This one is for ALIEN TAUNT. */

	soundIndex=SID_NOSOUND;

	if (alienStatusPointer->soundHandle2==SOUND_NOACTIVEINDEX) {
		PlayAlienSound((int)alienStatusPointer->Type,ASC_Taunt,0,
			&alienStatusPointer->soundHandle2,&sbPtr->DynPtr->Position);

		if (AvP.Network != I_No_Network)
		{
			AddNetMsg_SpotAlienSound((int)ASC_Taunt,(int)alienStatusPointer->Type,0,&dynPtr->Position);
		}
	}
}


static void AlienNearDamageShell(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	int workingflags,flagnum,a;
	int dist,dodamage;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* Damage shell! */
	workingflags=alienStatusPointer->HModelController.keyframe_flags>>1;
	flagnum=0;

	dist = VectorDistance(&(dynPtr->Position),&(alienStatusPointer->Target->DynPtr->Position));
	if (dist<ALIEN_ATTACKRANGE) {
		dodamage=1;
	} else {
		dodamage=0;
	}
	
	for (a=0; a<NUM_ATTACKFLAGS; a++) {
	
		if (workingflags&1) {
		
	    	/* Do the alien attack sound */
			#if 0 /* There are now sounds on the sequences. */
			DoAlienAIAttackSound(sbPtr);
			#endif

			/* Oops, check range first. */
			if (dodamage) {
				extern DISPLAYBLOCK *HtoHDamageToHModel(STRATEGYBLOCK *sbPtr, DAMAGE_PROFILE *damage, int multiple, STRATEGYBLOCK *source, VECTORCH *attack_dir);
	
				if (alienStatusPointer->Target->SBdptr) {
					VECTORCH rel_pos,attack_dir;
	
					rel_pos.vx=alienStatusPointer->Target->DynPtr->Position.vx-sbPtr->DynPtr->Position.vx;
					rel_pos.vy=alienStatusPointer->Target->DynPtr->Position.vy-sbPtr->DynPtr->Position.vy;
					rel_pos.vz=alienStatusPointer->Target->DynPtr->Position.vz-sbPtr->DynPtr->Position.vz;

					GetDirectionOfAttack(alienStatusPointer->Target,&rel_pos,&attack_dir);

					if (alienStatusPointer->Target->SBdptr->HModelControlBlock) {
						HtoHDamageToHModel(alienStatusPointer->Target,&TemplateAmmo[GetAttackDamageType(sbPtr,flagnum)].MaxDamage[AvP.Difficulty], ONE_FIXED, sbPtr, &attack_dir);
					} else {
						
						CauseDamageToObject(alienStatusPointer->Target,&TemplateAmmo[GetAttackDamageType(sbPtr,flagnum)].MaxDamage[AvP.Difficulty], ONE_FIXED,&attack_dir);
					}		
				} else {
					VECTORCH rel_pos,attack_dir;
	
					rel_pos.vx=alienStatusPointer->Target->DynPtr->Position.vx-sbPtr->DynPtr->Position.vx;
					rel_pos.vy=alienStatusPointer->Target->DynPtr->Position.vy-sbPtr->DynPtr->Position.vy;
					rel_pos.vz=alienStatusPointer->Target->DynPtr->Position.vz-sbPtr->DynPtr->Position.vz;
	
					GetDirectionOfAttack(alienStatusPointer->Target,&rel_pos,&attack_dir);
	
					CauseDamageToObject(alienStatusPointer->Target,&TemplateAmmo[GetAttackDamageType(sbPtr,flagnum)].MaxDamage[AvP.Difficulty], ONE_FIXED,&attack_dir);
				}
			}
		}
		/* Prepare next flag. */
		workingflags>>=1;
		flagnum++;
	}

}

int Alien_Special_Pounce_Condition(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr=sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);
	LOCALASSERT(alienStatusPointer->Target);

	if (sbPtr->containingModule->m_aimodule->m_waypoints==NULL) {
		/* If in an unwaypointed module... */
		if ((alienStatusPointer->Target->DynPtr->Position.vy-dynPtr->Position.vy)<(-2000)) {
			/* And the target is directly above us... */
			return(1);
		}
	}

	return(0);
}

#define ALIEN_CURVETOPLAYERDIST 8000
static void AlienNearState_Approach(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	VECTORCH targetPos;
	int approachingAirDuct = 0;
	int curveToPlayer = 0;
	int distanceToPlayer;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr=sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	if (alienStatusPointer->Target==NULL || !alienStatusPointer->Target->DynPtr) {
		/* Approach what? */
		AlienHandleMovingAnimation(sbPtr);
		alienStatusPointer->BehaviourState = ABS_Hunt;
		InitWaypointManager(&alienStatusPointer->waypointManager);
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		return;
	} 	

    distanceToPlayer=VectorDistance(&dynPtr->Position,&alienStatusPointer->Target->DynPtr->Position);
	/* Update sequence! */
	AlienHandleMovingAnimation(sbPtr);
 	
 	/* target acquisition ? */
	{
		if(VectorDistance(&(alienStatusPointer->Target->DynPtr->Position),&(dynPtr->Position)) < ALIEN_CURVETOPLAYERDIST)		  
		{
			curveToPlayer = 1;	
			//targetPos = alienStatusPointer->Target->DynPtr->Position;					
			GetTargetingPointOfObject_Far(alienStatusPointer->Target, &targetPos);
			/* do climb on walls, etc */
			if (AlienIsAbleToClimb(sbPtr)) {
				AlienNavigationGravity(sbPtr,1);
			} else {
				dynPtr->UseStandardGravity=1;
			}
		}
		else
		{
			curveToPlayer = 0;	
			NPCGetMovementTarget(sbPtr, alienStatusPointer->Target, &targetPos, &approachingAirDuct,1);
			/* Keep wall attachment across navigation state changes. */
            AlienNavigationGravity(sbPtr,0);
		}
	}

	if (sbPtr->containingModule->m_aimodule->m_waypoints!=NULL) {
		/* Never curve in a waypoint module, you might hurt yourself. */
		curveToPlayer=0;
	}

	if(curveToPlayer)
	{
		/* curve to the player... */
		/* translate target into alien's local space */

		//textprint("curving alien \n");

		{
			MATRIXCH toLocalSpaceMatrix = dynPtr->OrientMat;
			TransposeMatrixCH(&toLocalSpaceMatrix);
			
			targetPos.vx -= dynPtr->Position.vx;
			targetPos.vy -= dynPtr->Position.vy;
			targetPos.vz -= dynPtr->Position.vz;
			RotateVector(&targetPos,&toLocalSpaceMatrix);
		}

		/* tracking movement */
		{
			int distanceToTarget = Magnitude(&targetPos);

		    if (dynPtr->IsInContactWithFloor)
	        {
				int offset;

				if (alienStatusPointer->CurveTimeOut<=0)
				{
					alienStatusPointer->CurveLength = distanceToTarget;
					alienStatusPointer->CurveRadius = ((FastRandom()&16383)-8192)*2;
					alienStatusPointer->CurveTimeOut= ONE_FIXED*3;
				}
				else alienStatusPointer->CurveTimeOut-=NormalFrameTime;

				offset = 
					MUL_FIXED
					(
						alienStatusPointer->CurveRadius,
						GetCos((1024*(distanceToTarget)/alienStatusPointer->CurveLength)&4095)
					);

				dynPtr->LinVelocity.vx = 
					WideMulNarrowDiv
					(
						alienStatusPointer->MaxSpeed,
						targetPos.vx,
						distanceToTarget
					)
					-WideMulNarrowDiv
					(
						offset,
						targetPos.vz,
						distanceToTarget
					);
					
					
				dynPtr->LinVelocity.vz = 
					WideMulNarrowDiv
					(
						alienStatusPointer->MaxSpeed,
						targetPos.vz,
						distanceToTarget
					)+
					WideMulNarrowDiv
					(
						offset,
						targetPos.vx,
						distanceToTarget
					);
				#if 0
				if ( (targetPos.vy < -ALIEN_ATTACKDISTANCE_MIN)
				   &&(targetPos.vx*targetPos.vx+targetPos.vz*targetPos.vz < 2*ALIEN_ATTACKDISTANCE_MIN*ALIEN_ATTACKDISTANCE_MIN)
				   &&(DotProduct(&dynPtr->GravityDirection,(VECTORCH*)&dynPtr->OrientMat.mat21)>64500) 
				   &&(sbPtr->containingModule==playerPherModule))
				#else
				if ((dynPtr->UseStandardGravity==0)&&(sbPtr->containingModule==playerPherModule)) {
					int dot;
					//VECTORCH velocityDirection=dynPtr->LinVelocity;
					VECTORCH velocityDirection;
					GetTargetingPointOfObject_Far(alienStatusPointer->Target, &velocityDirection);
					velocityDirection.vx -= dynPtr->Position.vx;
					velocityDirection.vy -= dynPtr->Position.vy;
					velocityDirection.vz -= dynPtr->Position.vz;

					Normalise(&velocityDirection);
					dot=DotProduct(&dynPtr->GravityDirection,&velocityDirection);
					if (dot<-60000 && AlienLaunchBegin(&alienStatusPointer->motion,dynPtr->IsInContactWithFloor))
				#endif
				{
					/* patrick 29/7/97: I have added the extra condition of not being in the same module as
					the player, so that alien does not jump at entry-points */
					dynPtr->TimeNotInContactWithFloor = 0;
					dynPtr->UseStandardGravity=1;
					dynPtr->LinImpulse.vx -= MUL_FIXED(dynPtr->GravityDirection.vx,ALIEN_JUMPVELOCITY);
					dynPtr->LinImpulse.vy -= MUL_FIXED(dynPtr->GravityDirection.vy,ALIEN_JUMPVELOCITY);
					dynPtr->LinImpulse.vz -= MUL_FIXED(dynPtr->GravityDirection.vz,ALIEN_JUMPVELOCITY);
				}
				#if 1
				}
				#endif
			 	RotateVector(&dynPtr->LinVelocity,&dynPtr->OrientMat);
				
				/* align to velocity */
				NPCOrientateToVector(sbPtr,&dynPtr->LinVelocity,NPC_TURNRATE,NULL);
			}
		}
	}
	else
	{
		/* use standard NPC direction finding... */
		VECTORCH velocityDirection = {0,0,0};

		//textprint("non curving alien \n");

		NPCGetMovementDirection(sbPtr, &velocityDirection, &targetPos,&alienStatusPointer->waypointManager);
		if (AlienIsEncouragedToCrawl()&&(alienStatusPointer->EnableWaypoints)) {
			if (AlienIsAbleToClimb(sbPtr)) {
				AlienNavigationGravity(sbPtr,1);
			} else {
				dynPtr->UseStandardGravity=1;
			}
		}
		NPCSetVelocity(sbPtr, &velocityDirection, alienStatusPointer->MaxSpeed);
		alienStatusPointer->CurveTimeOut=0; /* forces curve init next time curving is used */

		/* Consider ripping off the wall. */
		if ((dynPtr->UseStandardGravity==0)&&(sbPtr->containingModule==playerPherModule)) {
			int dot;
			
			dot=DotProduct(&dynPtr->GravityDirection,&velocityDirection);

			if (dot<-60000 && AlienLaunchBegin(&alienStatusPointer->motion,dynPtr->IsInContactWithFloor)) {
			
				dynPtr->TimeNotInContactWithFloor = 0;
				dynPtr->UseStandardGravity=1;
				dynPtr->LinImpulse.vx -= MUL_FIXED(dynPtr->GravityDirection.vx,ALIEN_JUMPVELOCITY);
				dynPtr->LinImpulse.vy -= MUL_FIXED(dynPtr->GravityDirection.vy,ALIEN_JUMPVELOCITY);
				dynPtr->LinImpulse.vz -= MUL_FIXED(dynPtr->GravityDirection.vz,ALIEN_JUMPVELOCITY);
			}
		}
	}
			

	/* state change and sequence change tests must go here, because they test the approaching
	airduct flag, which is set during target aquisition */
		
	/* check if we should be crouched or standing up */
	{
		int redo=0;

		if(alienStatusPointer->IAmCrouched)
		{
			/* curently crouched */
			if((!approachingAirDuct)&&(!AlienShouldBeCrawling(sbPtr)))
			{
				/* should be running*/
				alienStatusPointer->IAmCrouched = 0;
				redo=1;
			}
		}
		else
		{
			/* currently standing */
			if((approachingAirDuct)||(AlienShouldBeCrawling(sbPtr)))
			{
				/* should be crawling */
				alienStatusPointer->IAmCrouched = 1;
				redo=1;
			}
		}

		if (redo) {
			StartAlienMovementSequence(sbPtr);
		}
	}

	/* Hang on, should we be doing this? */
	if (sbPtr->SBDamageBlock.IsOnFire==0) {
		if ((distanceToPlayer<30000)
			&&(sbPtr->DynPtr->OrientMat.mat22>=63000)
			&&(sbPtr->DynPtr->IsInContactWithFloor)
			) {

			/* 150% flamethrower 'range'. */
			if (TargetIsFiringFlamethrowerAtAlien(sbPtr)) {
				/* Pause and taunt. */
				if (StartAlienTaunt(sbPtr)) {
					/* My work is done. */
					return;
				}
			}
		}
	}

	/* should we change back to attack, maybe? */
	{

		if ((distanceToPlayer<=ALIEN_POUNCE_STARTMAXRANGE) 
			&&(distanceToPlayer>=ALIEN_POUNCE_MINRANGE)) {
			/* Might want to pounce. */
			if ((alienStatusPointer->EnablePounce)||(Alien_Special_Pounce_Condition(sbPtr))) {
				if (StartAlienPounce(sbPtr)) {
					/* Success. */
					return;
				}
			}
		} else if (distanceToPlayer>ALIEN_POUNCE_STARTMAXRANGE) {
			if (alienStatusPointer->incidentFlag) {
				int prob;

				switch (alienStatusPointer->Type) {
					case AT_Standard:
					default:
						prob=ALIEN_JUMPINESS;
						break;
					case AT_Predalien:
						prob=PREDALIEN_JUMPINESS;
						break;
					case AT_Praetorian:
						prob=PRAETORIAN_JUMPINESS;
						break;
				}

				if ((FastRandom()&65535)<prob) {
					if (GoToJump(sbPtr)) {
						/* Boop! */
						return;
					}
				}
			}
		}

		if(distanceToPlayer<=ALIEN_ATTACKDISTANCE_MIN) {
		
			if (AlienIsAllowedToAttack(sbPtr)) {
								 
				/* state and sequence change */
				alienStatusPointer->BehaviourState = ABS_Attack;
				alienStatusPointer->NearStateTimer=ALIEN_ATTACKTIME;
				/* Can alien stand up? */
				if(alienStatusPointer->IAmCrouched) {
					if (!AlienShouldBeCrawling(sbPtr)) {
						/* Better stand up now then. */
						alienStatusPointer->IAmCrouched = 0;
					}
				}
				/* Now, */

				StartAlienAttackSequence(sbPtr);

				/* this is because we have already set a velocity this frame */
				dynPtr->LinVelocity.vx = dynPtr->LinVelocity.vy = dynPtr->LinVelocity.vz = 0;
				return;
			}
		}
	}

	/* is player visible?: if not, go to timed wait... */
	if(!AlienIsAwareOfTarget(sbPtr))
	{
		alienStatusPointer->BehaviourState = ABS_Wait;
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		InitWaypointManager(&alienStatusPointer->waypointManager);
		if(alienStatusPointer->IAmCrouched) SetAlienShapeAnimSequence(sbPtr,HMSQT_AlienCrouch,(int)ACrSS_Standard,ONE_FIXED>>1);		
		else SetAlienShapeAnimSequence(sbPtr,HMSQT_AlienStand,(int)ASSS_Standard,ONE_FIXED>>1);
		/* this is because we have already set a velocity this frame */
		dynPtr->LinVelocity.vx = dynPtr->LinVelocity.vy = dynPtr->LinVelocity.vz = 0;
		return;
	} else if (!AlienHasPathToTarget(sbPtr)) {
		/* Go to hunt. */
		alienStatusPointer->BehaviourState = ABS_Hunt;
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		InitWaypointManager(&alienStatusPointer->waypointManager);
	}
	
	/* test here for impeding collisions, and not being able to reach target... */
	#if ALL_NEW_AVOIDANCE_ALIEN
	{
		if (New_NPC_IsObstructed(sbPtr,&alienStatusPointer->avoidanceManager)) {
			/* Go to all new avoidance. */
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			InitWaypointManager(&alienStatusPointer->waypointManager);
			return;
		}
	}
	#else
	{
		STRATEGYBLOCK *destructableObject = NULL;
		NPC_OBSTRUCTIONREPORT obstruction;
		NPC_IsObstructed(sbPtr,&(alienStatusPointer->moveData),&obstruction,&destructableObject);
		if(obstruction.environment)
		{
			#if 1
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			return;
			#endif
		} else if(obstruction.destructableObject) {
			LOCALASSERT(destructableObject);
			CauseDamageToObject(destructableObject,&TemplateAmmo[AMMO_ALIEN_OBSTACLE_CLEAR].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);
		} else if ((obstruction.anySingleObstruction)&&(!obstruction.otherCharacter)) {
			/* Try for a nearer target? */
			alienStatusPointer->Target=NULL;
		}
	}
	
	{
		VECTORCH velocityDirection = dynPtr->LinVelocity;
		Normalise(&velocityDirection);

		if(NPC_CannotReachTarget(&(alienStatusPointer->moveData), &targetPos, &velocityDirection))
		{
			/* go to avoidance */
			NPC_OBSTRUCTIONREPORT obstruction = {1,0,0};
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);						
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			/* no sequence change required */
			return;
		}
	}
	#endif

	/* still here?... make some noise */
	if ((FastRandom()&127)<20) {
		DoAlienAIRandomHiss(sbPtr);
	}
}

static void AlienNearState_Attack(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* The following should be #if 0, but I'll not check that in until the player is unclimbable. */
	#if 1
	/* don't climb on walls, etc */
	AlienNavigationGravity(sbPtr,0);
	#endif
	/* Nowadays, other things can be stuck to walls... sigh. */

	if (alienStatusPointer->Target==NULL) {
		/* Attack what? */
		alienStatusPointer->BehaviourState = ABS_Hunt;
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		InitWaypointManager(&alienStatusPointer->waypointManager);
		return;
	} 	

	/* patrick 13/6/97: a little addition:
	Orientate towards player, just to make sure we're facing */
	if (dynPtr->UseStandardGravity) {
		VECTORCH orientationDirn;
		int i;
		orientationDirn.vx = alienStatusPointer->Target->DynPtr->Position.vx - sbPtr->DynPtr->Position.vx;
		orientationDirn.vy = 0;
		orientationDirn.vz = alienStatusPointer->Target->DynPtr->Position.vz - sbPtr->DynPtr->Position.vz;
		i = NPCOrientateToVector(sbPtr, &orientationDirn,NPC_TURNRATE,NULL);
	} else {
		/* Replace this! */
		VECTORCH orientationDirn;
		int i,dot;
		orientationDirn.vx = alienStatusPointer->Target->DynPtr->Position.vx - sbPtr->DynPtr->Position.vx;
		orientationDirn.vy = alienStatusPointer->Target->DynPtr->Position.vy - sbPtr->DynPtr->Position.vy;
		orientationDirn.vz = alienStatusPointer->Target->DynPtr->Position.vz - sbPtr->DynPtr->Position.vz;

		dot = -(DotProduct(&dynPtr->GravityDirection,&orientationDirn));
		/* Hold that thought. */
		orientationDirn.vx -= MUL_FIXED(dot,dynPtr->GravityDirection.vx);
		orientationDirn.vy -= MUL_FIXED(dot,dynPtr->GravityDirection.vy);
		orientationDirn.vz -= MUL_FIXED(dot,dynPtr->GravityDirection.vz);

		#if 0
		{
			VECTORCH point;
			/* Wacky test. */
			GetTargetingPointOfObject_Far(sbPtr,&point);
			Normalise(&orientationDirn);
			MakeParticle(&point,&orientationDirn,PARTICLE_PREDATOR_BLOOD);
	
		}
		#endif

		i = NPCOrientateToVector(sbPtr, &orientationDirn,NPC_TURNRATE,NULL);
	}

	/* change back to approach?: don't need to directly test if we should go to wander, as 
	approach state will do this... */
	{
		int distanceToPlayer = VectorDistance(&(dynPtr->Position),&(alienStatusPointer->Target->DynPtr->Position));
		if((distanceToPlayer>ALIEN_ATTACKDISTANCE_MAX))
		{					 
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			alienStatusPointer->BehaviourState = ABS_Approach;

			StartAlienMovementSequence(sbPtr);
			InitWaypointManager(&alienStatusPointer->waypointManager);

			alienStatusPointer->NearStateTimer = 0;
			alienStatusPointer->CurveTimeOut = 0;
			return;
		}
	}
	
	/* Fall over check? */
		
	if(!(alienStatusPointer->IAmCrouched)) {
		if (AlienShouldBeCrawling(sbPtr)) {
			/* Something has changed.  Fall over. */

			/* That implies going back to approach, btw. */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			alienStatusPointer->BehaviourState = ABS_Approach;

			StartAlienMovementSequence(sbPtr);
			InitWaypointManager(&alienStatusPointer->waypointManager);

			alienStatusPointer->NearStateTimer = 0;
			alienStatusPointer->CurveTimeOut = 0;
			return;
		}
	}

	/* alien can inflict nastiness on the target */
	alienStatusPointer->NearStateTimer-=NormalFrameTime;

	AlienNearDamageShell(sbPtr);

	if (alienStatusPointer->HModelController.keyframe_flags&1) {

		/* Retest for moving closer? */
		int distanceToPlayer = VectorDistance(&(dynPtr->Position),&(alienStatusPointer->Target->DynPtr->Position));
		if((distanceToPlayer>ALIEN_ATTACKDISTANCE_MIN))
		{					 
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			alienStatusPointer->BehaviourState = ABS_Approach;

			StartAlienMovementSequence(sbPtr);
			InitWaypointManager(&alienStatusPointer->waypointManager);

			alienStatusPointer->NearStateTimer = 0;
			alienStatusPointer->CurveTimeOut = 0;
			return;
		}

		StartAlienAttackSequence(sbPtr);
		alienStatusPointer->NearStateTimer = ALIEN_ATTACKTIME;
		/* Shrug. */
	}

	if ((FastRandom()&127)<20) {
		DoAlienAIRandomHiss(sbPtr);
	}

}


static void AlienNearState_Wander(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	VECTORCH velocityDirection = {0,0,0};

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* Relaxed patrol can crawl; existing wall attachment is preserved. */
    AlienNavigationGravity(sbPtr,alienStatusPointer->motion.aftermath);
    alienStatusPointer->IAmCrouched=AlienShouldBeCrawling(sbPtr);
	
	/* check if we should be crouched or standing up */

	/* Update sequence! */
	AlienHandleMovingAnimation(sbPtr);

	/* Do something else? */

	#if 0
	if (alienStatusPointer->Mission==AM_GlobalHunt) {
	
		alienStatusPointer->BehaviourState = ABS_Hunt;  		
		alienStatusPointer->NearStateTimer = 0;
		InitWaypointManager(&alienStatusPointer->waypointManager);
		/* no sequence change required */
		return;

	}
	#endif

	/* should we change to approach state? */
	if(!alienStatusPointer->motion.recoveryTime && AlienHasPathToTarget(sbPtr))
	{
		/* doesn't require a sequence change */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;
		alienStatusPointer->CurveTimeOut = 0;
		InitWaypointManager(&alienStatusPointer->waypointManager);
		if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarine) {
			WarnMarineOfAttack(alienStatusPointer->Target,sbPtr);
		}
		return;
	} else if (!alienStatusPointer->motion.recoveryTime && AlienIsAwareOfTarget(sbPtr)) {
		/* Go to hunt. */
		alienStatusPointer->BehaviourState = ABS_Hunt;
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		InitWaypointManager(&alienStatusPointer->waypointManager);
	}
	
	/* Try to reaquire target? */
	if(!alienStatusPointer->motion.recoveryTime)alienStatusPointer->Target=NULL;

	/* wander target aquisition: if no target, or moved module */
	LOCALASSERT(sbPtr->containingModule);
	if(alienStatusPointer->wanderData.currentModule==NPC_NOWANDERMODULE)
	{
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		NPC_FindAIWanderTarget(sbPtr,&(alienStatusPointer->wanderData),&(alienStatusPointer->moveData),1);
	}
	else if(alienStatusPointer->wanderData.currentModule!=sbPtr->containingModule->m_aimodule->m_index)
	{
		NPC_FindAIWanderTarget(sbPtr,&(alienStatusPointer->wanderData),&(alienStatusPointer->moveData),1);
	}
	
	/* if we still haven't got one, go to wait */
	if(alienStatusPointer->wanderData.currentModule==NPC_NOWANDERMODULE)
	{
		alienStatusPointer->BehaviourState = ABS_Wait;
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		if(alienStatusPointer->IAmCrouched) SetAlienShapeAnimSequence(sbPtr,HMSQT_AlienCrouch,(int)ACrSS_Standard,ONE_FIXED>>1);		
		else SetAlienShapeAnimSequence(sbPtr,HMSQT_AlienStand,(int)ASSS_Standard,ONE_FIXED>>1);
        return; /* No target means no stale worldPosition velocity. */
	}
	
    if(alienStatusPointer->motion.aftermath){
        alienStatusPointer->NearStateTimer+=NormalFrameTime;
        if(alienStatusPointer->NearStateTimer>=2*ONE_FIXED && !alienStatusPointer->motion.moving){
            NPC_InitWanderData(&alienStatusPointer->wanderData);
            alienStatusPointer->NearStateTimer=0;
            alienStatusPointer->BehaviourState=ABS_Wait;
            return;
        }
        if(alienStatusPointer->motion.moving)alienStatusPointer->NearStateTimer=0;
    }
	/* ok: should have a current target at this stage... */
	NPCGetMovementDirection(sbPtr, &velocityDirection, &(alienStatusPointer->wanderData.worldPosition),&alienStatusPointer->waypointManager);
	NPCSetVelocity(sbPtr, &velocityDirection, alienStatusPointer->MaxSpeed);	

	/* test here for impeding collisions, and not being able to reach target... */
	#if ALL_NEW_AVOIDANCE_ALIEN
	{
		if (New_NPC_IsObstructed(sbPtr,&alienStatusPointer->avoidanceManager)) {
			/* Go to all new avoidance. */
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			InitWaypointManager(&alienStatusPointer->waypointManager);
			return;
		}
	}
	#else
	{
		STRATEGYBLOCK *destructableObject = NULL;
		NPC_OBSTRUCTIONREPORT obstruction;
		NPC_IsObstructed(sbPtr,&(alienStatusPointer->moveData),&obstruction,&destructableObject);
		if((obstruction.environment)||(obstruction.otherCharacter))
		{
			#if 1
			/* go to avoidance */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);						
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			/* no sequence change required */
			return;
			#endif
		}
		if(obstruction.destructableObject)
		{
			LOCALASSERT(destructableObject);
			CauseDamageToObject(destructableObject,&TemplateAmmo[AMMO_ALIEN_OBSTACLE_CLEAR].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);
		}
	}

	if(NPC_CannotReachTarget(&(alienStatusPointer->moveData), &(alienStatusPointer->wanderData.worldPosition), &velocityDirection))
	{
		/* go to avoidance */
		NPC_OBSTRUCTIONREPORT obstruction = {1,0,0};
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);						
		alienStatusPointer->BehaviourState = ABS_Avoidance;  		
		alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
		/* no sequence change required */
		return;
	}
	#endif
}

static void AlienNearState_Wait(STRATEGYBLOCK *sbPtr)
{
	/* wait until near state timer runs out, then wander:
	alternatively, if we can attack the player, go straight to approach */
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* do climb on walls, etc */
	if (AlienIsAbleToClimb(sbPtr)) {
		AlienNavigationGravity(sbPtr,1);
	} else {
		dynPtr->UseStandardGravity=1;
	}

	AlienHandleStandingAnimation(sbPtr);

    if(alienStatusPointer->motion.recoveryTime>0)return;
	/* test for attack */
	if(AlienHasPathToTarget(sbPtr))
	{
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;
		InitWaypointManager(&alienStatusPointer->waypointManager);

		StartAlienMovementSequence(sbPtr);

		alienStatusPointer->CurveTimeOut = 0;
		alienStatusPointer->NearStateTimer = 0;	
		if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarine) {
			WarnMarineOfAttack(alienStatusPointer->Target,sbPtr);
		}
		return;
	} else if (AlienIsAwareOfTarget(sbPtr)) {
		/* Go to hunt. */
		alienStatusPointer->BehaviourState = ABS_Hunt;
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		InitWaypointManager(&alienStatusPointer->waypointManager);
	}

	/* still waiting: decrement timer */
	alienStatusPointer->NearStateTimer-=NormalFrameTime;
	
	if(alienStatusPointer->NearStateTimer<=0)
	{
		/* waiting has expired: go to wander */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		NPC_InitWanderData(&(alienStatusPointer->wanderData));
		alienStatusPointer->BehaviourState = ABS_Wander;
		InitWaypointManager(&alienStatusPointer->waypointManager);

		StartAlienMovementSequence(sbPtr);

		alienStatusPointer->NearStateTimer = 0;		
	}
}

static void AlienNearState_Avoidance(STRATEGYBLOCK *sbPtr)
{
	int terminateState = 0;
	ALIEN_STATUS_BLOCK *alienStatusPointer;

	LOCALASSERT(sbPtr);
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);	          		
		
	#if ALL_NEW_AVOIDANCE_ALIEN
	/* New avoidance kernel. */

	/* Update sequence! */
	AlienHandleMovingAnimation(sbPtr);

	NPCSetVelocity(sbPtr, &(alienStatusPointer->avoidanceManager.avoidanceDirection), (alienStatusPointer->MaxSpeed));
	/* Velocity CANNOT be zero, unless deliberately so! */	
	{
		AVOIDANCE_RETURN_CONDITION rc;
		
		rc=AllNewAvoidanceKernel(sbPtr,&alienStatusPointer->avoidanceManager);

		if (rc!=AvRC_Avoidance) {
			terminateState=1;
		}
	}
	#else
	/* set velocity */
	LOCALASSERT((alienStatusPointer->moveData.avoidanceDirn.vx!=0)||
				(alienStatusPointer->moveData.avoidanceDirn.vy!=0)||
				(alienStatusPointer->moveData.avoidanceDirn.vz!=0));

	NPCSetVelocity(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn), (alienStatusPointer->MaxSpeed));

	/* Update sequence! */
	AlienHandleMovingAnimation(sbPtr);

	/* next, decrement state timer */
	alienStatusPointer->NearStateTimer -= NormalFrameTime;
	if(alienStatusPointer->NearStateTimer <= 0) terminateState = 1;
	/* and check for an impeding collision */
	{
		STRATEGYBLOCK *destructableObject;
		NPC_OBSTRUCTIONREPORT obstruction;
		NPC_IsObstructed(sbPtr,&(alienStatusPointer->moveData),&obstruction,&destructableObject);
		if(obstruction.anySingleObstruction)
		{
			/* return to approach */
			terminateState = 1;
		}
	}
	#endif
	
	if(terminateState)
	{
		if(AlienHasPathToTarget(sbPtr))
		{
			/* switch to approach */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			alienStatusPointer->BehaviourState = ABS_Approach;  		
			InitWaypointManager(&alienStatusPointer->waypointManager);
			alienStatusPointer->NearStateTimer = 0;
			if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarine) {
				WarnMarineOfAttack(alienStatusPointer->Target,sbPtr);
			}
			/* no sequence change required */
		} else if (AlienIsAwareOfTarget(sbPtr)) {
			/* Go to hunt. */
			alienStatusPointer->BehaviourState = ABS_Hunt;
			InitWaypointManager(&alienStatusPointer->waypointManager);
			alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		}
		else
		{
			/* switch to wander */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			NPC_InitWanderData(&(alienStatusPointer->wanderData));
			alienStatusPointer->BehaviourState = ABS_Wander;  		
			alienStatusPointer->NearStateTimer = 0;
			InitWaypointManager(&alienStatusPointer->waypointManager);
			/* no sequence change required */
		}
	}
}

static void AlienNearState_Retreat(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	VECTORCH velocityDirection = {0,0,0};

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* don't climb on walls, etc */
	dynPtr->UseStandardGravity=1;
	
	/* check if we should be crouched or standing up */

	/* Update sequence! */
	AlienHandleMovingAnimation(sbPtr);

	/* should we change to approach state? */
	if(AlienIsAwareOfTarget(sbPtr))
	{
		/* doesn't require a sequence change */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;
		InitWaypointManager(&alienStatusPointer->waypointManager);
		alienStatusPointer->CurveTimeOut = 0;
		if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarine) {
			WarnMarineOfAttack(alienStatusPointer->Target,sbPtr);
		}
		/* Bloodthirsty, these aliens... */
		return;
	}

	/* Retreat target aquisition. */
	{
		AIMODULE *targetModule;
		FARENTRYPOINT *thisEp = (FARENTRYPOINT *)0;

		targetModule = NearNPC_GetTargetAIModuleForRetreat(sbPtr, &(alienStatusPointer->moveData));

		if (targetModule) {
			//textprint("Target module is %s\n",targetModule->name);
			textprint("Target AI module found, %x.\n",(int)targetModule);
		} else {
			textprint("Target module is NULL!\n");
		}

		if ((targetModule==sbPtr->containingModule->m_aimodule)
			|| (targetModule==NULL)) {
			/* Hey, it'll drop through. */
			alienStatusPointer->BehaviourState = ABS_Hunt;
			alienStatusPointer->CurveTimeOut = 0;
			InitWaypointManager(&alienStatusPointer->waypointManager);
			return;
		}
		
		GLOBALASSERT(targetModule);
		
		thisEp=GetAIModuleEP(targetModule,sbPtr->containingModule->m_aimodule);
		if (!thisEp) {
			//LOGDXFMT(("This assert is a busted adjacency!\nNo EP between %s and %s.",targetModule->name,sbPtr->containingModule->name));
			LOGDXFMT(("This assert is a busted adjacency!"));
			GLOBALASSERT(thisEp);
		}
		/* If that fired, there's a farped adjacency. */
	
		alienStatusPointer->wanderData.worldPosition=thisEp->position;
		alienStatusPointer->wanderData.worldPosition.vx+=targetModule->m_world.vx;
		alienStatusPointer->wanderData.worldPosition.vy+=targetModule->m_world.vy;
		alienStatusPointer->wanderData.worldPosition.vz+=targetModule->m_world.vz;
		
	}
	
	/* ok: should have a current target at this stage... */
	NPCGetMovementDirection(sbPtr, &velocityDirection, &(alienStatusPointer->wanderData.worldPosition),&alienStatusPointer->waypointManager);
	NPCSetVelocity(sbPtr, &velocityDirection, alienStatusPointer->MaxSpeed);	

	/* test here for impeding collisions, and not being able to reach target... */
	#if ALL_NEW_AVOIDANCE_ALIEN
	{
		if (New_NPC_IsObstructed(sbPtr,&alienStatusPointer->avoidanceManager)) {
			/* Go to all new avoidance. */
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			InitWaypointManager(&alienStatusPointer->waypointManager);
			return;
		}
	}
	#else
	{
		STRATEGYBLOCK *destructableObject = NULL;
		NPC_OBSTRUCTIONREPORT obstruction;
		NPC_IsObstructed(sbPtr,&(alienStatusPointer->moveData),&obstruction,&destructableObject);
		if((obstruction.environment)||(obstruction.otherCharacter))
		{
			#if 1
			/* go to avoidance */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);						
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			/* no sequence change required */
			return;
			#endif
		}
		if(obstruction.destructableObject)
		{
			LOCALASSERT(destructableObject);
			CauseDamageToObject(destructableObject,&TemplateAmmo[AMMO_ALIEN_OBSTACLE_CLEAR].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);
		}
	}

	if(NPC_CannotReachTarget(&(alienStatusPointer->moveData), &(alienStatusPointer->wanderData.worldPosition), &velocityDirection))
	{
		/* go to avoidance */
		NPC_OBSTRUCTIONREPORT obstruction = {1,0,0};
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);						
		alienStatusPointer->BehaviourState = ABS_Avoidance;  		
		alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
		/* no sequence change required */
		return;
	}
	#endif
}

static void AlienNearState_Hunt(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	int approachingAirDuct = 0;
	VECTORCH velocityDirection = {0,0,0};

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* don't climb on walls, etc */
	AlienNavigationGravity(sbPtr,0);

	/* check if we should be crouched or standing up */

	{
		int redo=0;

		if(alienStatusPointer->IAmCrouched)
		{
			/* curently crouched */
			if((!approachingAirDuct)&&(!AlienShouldBeCrawling(sbPtr)))
			{
				/* should be running*/
				alienStatusPointer->IAmCrouched = 0;
				redo=1;
			}
		}
		else
		{
			/* currently standing */
			if((approachingAirDuct)||(AlienShouldBeCrawling(sbPtr)))
			{
				/* should be crawling */
				alienStatusPointer->IAmCrouched = 1;
				redo=1;
			}
		}

		if (redo) {
			StartAlienMovementSequence(sbPtr);
		}
	}

	AlienHandleMovingAnimation(sbPtr);

	if (alienStatusPointer->IAmCrouched) {
		/* Are we climbing? */
		if (AlienIsAbleToClimb(sbPtr)) {
			AlienNavigationGravity(sbPtr,1);
		} else {
			AlienNavigationGravity(sbPtr,0);
		}
	} else {
		AlienNavigationGravity(sbPtr,0);
	}

	/* should we change to approach state? */
	if(AlienHasPathToTarget(sbPtr))
	{
		/* doesn't require a sequence change */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;
		InitWaypointManager(&alienStatusPointer->waypointManager);
		alienStatusPointer->CurveTimeOut = 0;
		if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarine) {
			WarnMarineOfAttack(alienStatusPointer->Target,sbPtr);
		}
		/* Bloodthirsty, these aliens... */
		return;
	}
	
	/* Hunting target aquisition. */
	if (sbPtr->containingModule!=alienStatusPointer->my_containing_module) {
		alienStatusPointer->huntingModule=NULL;
	}
	/* Check again. */
	if (alienStatusPointer->huntingModule!=NULL) {
		FARENTRYPOINT *thisEp = (FARENTRYPOINT *)0;

		thisEp=GetAIModuleEP(alienStatusPointer->huntingModule,sbPtr->containingModule->m_aimodule);
		if (!thisEp) {
			alienStatusPointer->huntingModule=NULL;
		}
	}

	if (alienStatusPointer->huntingModule==NULL) {
		AIMODULE *targetModule;

		if (alienStatusPointer->Mission==AM_GlobalHunt) {
			targetModule = FarNPC_GetTargetAIModuleForGlobalHunt(sbPtr);
		} else {
			targetModule = FarNPC_GetTargetAIModuleForGlobalHunt(sbPtr);
		}

		if (targetModule) {
			//textprint("Target module is %s\n",targetModule->name);
			textprint("Target AI module for hunt found, %x.\n",(int)targetModule);
		} else {
			textprint("Target module is NULL!\n");
		}

		if (targetModule==NULL) {
			/* Better have a handler for this. */
			#if 0
			alienStatusPointer->BehaviourState = ABS_Dormant;
			alienStatusPointer->CurveTimeOut = 0;
			if (HModelSequence_Exists(&alienStatusPointer->HModelController,HMSQT_AlienStand,ASSS_Dormant)) {
				SetAlienShapeAnimSequence_Core(sbPtr,HMSQT_AlienStand,ASSS_Dormant,-1,ONE_FIXED);
			} else {
				SetAlienShapeAnimSequence_Core(sbPtr,HMSQT_AlienStand,ASSS_Standard,ONE_FIXED,(ONE_FIXED>>2));
			}
			#else
			alienStatusPointer->BehaviourState = ABS_Wander;
			alienStatusPointer->CurveTimeOut = 0;
			#endif
			return;
		}

		if (targetModule==sbPtr->containingModule->m_aimodule) {
			/* We should have arrived - get a new target? */
			if (alienStatusPointer->Target==NULL) {
				/* Oops - nobody about. */
				NPC_InitMovementData(&(alienStatusPointer->moveData));
				NPC_InitWanderData(&(alienStatusPointer->wanderData));
				alienStatusPointer->BehaviourState = ABS_Wander;
				alienStatusPointer->CurveTimeOut = 0;
				InitWaypointManager(&alienStatusPointer->waypointManager);
			} else {
				alienStatusPointer->Target=NULL;
				alienStatusPointer->BehaviourState = ABS_Hunt;
				InitWaypointManager(&alienStatusPointer->waypointManager);
				alienStatusPointer->CurveTimeOut = 0;
			}
			return;
		}
		
		GLOBALASSERT(targetModule);

		alienStatusPointer->huntingModule=targetModule;
	}

	{
		FARENTRYPOINT *thisEp = (FARENTRYPOINT *)0;

		thisEp=GetAIModuleEP(alienStatusPointer->huntingModule,sbPtr->containingModule->m_aimodule);
		if (!thisEp) {
			LOGDXFMT(("This assert is a busted adjacency!\nNo EP between %s and %s.",(*(alienStatusPointer->huntingModule->m_module_ptrs))->name,sbPtr->containingModule->name));
			//LOGDXFMT(("This assert is a busted adjacency!"));
			GLOBALASSERT(thisEp);
		}
		/* If that fired, there's a farped adjacency. */
	
		alienStatusPointer->wanderData.worldPosition=thisEp->position;
		alienStatusPointer->wanderData.worldPosition.vx+=alienStatusPointer->huntingModule->m_world.vx;
		alienStatusPointer->wanderData.worldPosition.vy+=alienStatusPointer->huntingModule->m_world.vy;
		alienStatusPointer->wanderData.worldPosition.vz+=alienStatusPointer->huntingModule->m_world.vz;
		
	}
	
	/* ok: should have a current target at this stage... */
	NPCGetMovementDirection(sbPtr, &velocityDirection, &(alienStatusPointer->wanderData.worldPosition),&alienStatusPointer->waypointManager);
	NPCSetVelocity(sbPtr, &velocityDirection, alienStatusPointer->MaxSpeed);	

	/* test here for impeding collisions, and not being able to reach target... */
	#if ALL_NEW_AVOIDANCE_ALIEN
	{
		if (New_NPC_IsObstructed(sbPtr,&alienStatusPointer->avoidanceManager)) {
			/* Go to all new avoidance. */
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			InitWaypointManager(&alienStatusPointer->waypointManager);
			return;
		}
	}
	#else
	{
		STRATEGYBLOCK *destructableObject = NULL;
		NPC_OBSTRUCTIONREPORT obstruction;
		NPC_IsObstructed(sbPtr,&(alienStatusPointer->moveData),&obstruction,&destructableObject);
		if((obstruction.environment)||(obstruction.otherCharacter))
		{
			#if 1
			/* go to avoidance */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);						
			alienStatusPointer->BehaviourState = ABS_Avoidance;  		
			alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
			/* no sequence change required */
			return;
			#endif
		}
		if(obstruction.destructableObject)
		{
			LOCALASSERT(destructableObject);
			CauseDamageToObject(destructableObject,&TemplateAmmo[AMMO_ALIEN_OBSTACLE_CLEAR].MaxDamage[AvP.Difficulty], ONE_FIXED,NULL);
		}
	}

	if(NPC_CannotReachTarget(&(alienStatusPointer->moveData), &(alienStatusPointer->wanderData.worldPosition), &velocityDirection))
	{
		/* go to avoidance */
		NPC_OBSTRUCTIONREPORT obstruction = {1,0,0};
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		NPCGetAvoidanceDirection(sbPtr, &(alienStatusPointer->moveData.avoidanceDirn),&obstruction);						
		alienStatusPointer->BehaviourState = ABS_Avoidance;  		
		alienStatusPointer->NearStateTimer = NPC_AVOIDTIME;
		/* no sequence change required */
		return;
	}
	#endif
}

/* Patrick 27/6/97 *********************************
Some support functions for the alien:
Hopefully, these are fairly obvious...
****************************************************/
void SetAlienShapeAnimSequence_Core(STRATEGYBLOCK *sbPtr,HMODEL_SEQUENCE_TYPES type, int subtype, int length, int tweeningtime)
{

	ALIEN_STATUS_BLOCK *alienStatus=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);

	GLOBALASSERT(length!=0);

	alienStatus->last_anim_length=length;
	if (alienStatus->last_anim_length==-1) {
		/* Stops divisions by zero. */
		alienStatus->last_anim_length=ONE_FIXED;
	}

	/* Consider wounding. */
	#if WOUNDING_SPEED_EFFECTS
	{
		int factor;
		
		factor=GetAlienSpeedFactor_ForSequence(sbPtr,type,subtype);

		if (factor!=ONE_FIXED) {
			length=DIV_FIXED(length,factor);
		}
	
	}
	#endif

	if (tweeningtime<=0) {
		InitHModelSequence(&alienStatus->HModelController,(int)type,subtype,length);
	} else {
		InitHModelTweening(&alienStatus->HModelController, tweeningtime, (int)type,subtype,length, 1);
	}

	if (AvP.Network != I_No_Network)
	{
		AddNetMsg_AlienAISeqChange(sbPtr,type,subtype,length,tweeningtime);
	}

	alienStatus->HModelController.Playing=1;
	/* Might be unset... */
}

void SetAlienShapeAnimSequence(STRATEGYBLOCK *sbPtr,HMODEL_SEQUENCE_TYPES type, int subtype, int length) {

	SetAlienShapeAnimSequence_Core(sbPtr,type,subtype,length,(ONE_FIXED>>2));

}

int AlienHasNoArms(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatus=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);
	/* Check wounds. */
	
	/* Crawl if you've lost both arm sections. */	
	if (alienStatus->Wounds&(section_flag_left_arm)) {
		if (alienStatus->Wounds&(section_flag_right_arm)) {
			return(1);
		}
	}
	return(0);
}

int AlienIsAbleToStand(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatus=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);
	/* Check wounds. */
	
	/* Crawl if you've lost any leg sections. */	
	if (alienStatus->Wounds&(section_flag_left_leg|section_flag_right_leg)) return(0);
	/* Also crawl if you've lost both feet. */
	if ((alienStatus->Wounds&section_flag_left_foot)
		&&(alienStatus->Wounds&section_flag_right_foot)) return(0);

	return(1);
}

int AlienShouldBeCrawling(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatus=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);

	/* Hysteresis on wall/floor transitions prevents alternating gaits. */
    if(sbPtr->DynPtr->OrientMat.mat22<(alienStatus->IAmCrouched?64000:63000))return 1;
	if(sbPtr->containingModule->m_flags & MODULEFLAG_AIRDUCT) return 1;
	/* Wounding! */

	if (AlienIsAbleToStand(sbPtr)==0) {
		return(1);
	}

	if (alienStatus->BehaviourState==ABS_Attack) {
		return(0);
		/* Second lowest priority. */
	}

	/* New thing, 22/6/99 CDF. */
	if (AlienHasNoArms(sbPtr)) {
		/* This just looks silly. */
		return(0);
	}

	if ((AlienIsEncouragedToCrawl())&&(alienStatus->EnableWaypoints)) {
		return(1);
	}

	/* Does a praetorian just feel like it? */
	if (alienStatus->PreferToCrouch) {
		return(1);
	}

	return(0);
}

int AlienIsAbleToClimb(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatus=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);
	
	if (alienStatus->Type==AT_Praetorian) {
		/* Praetorian Guard can't climb.  Sorry. */
		return(0);
	}

	/* Can only climb if alien has both hands... */
	if ((alienStatus->Wounds&section_flag_left_hand)
		||(alienStatus->Wounds&section_flag_right_hand)) {
		return(0);
	}

	return 1;

}

int AlienIsAwareOfTarget(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);

	GLOBALASSERT(sbPtr->containingModule);
	/* test for player being cloaked */
	if (alienStatusPointer->Target==Player->ObStrategyBlock)
	{
		GLOBALASSERT(alienStatusPointer->Target->containingModule);
		#if 0
		{
			PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
			LOCALASSERT(playerStatusPtr);

			if((playerStatusPtr->cloakOn==1)&&(playerStatusPtr->cloakPositionGivenAway==0)) {
				return 0;
			}
		}
		#endif
		/* test for player being an alien */
		if(AvP.PlayerType==I_Alien) {
			return 0;
		}
		#if 0
		/* Expensive path test? */
		if (GetNextModuleForLink_Core(sbPtr->containingModule->m_aimodule,
			alienStatusPointer->Target->containingModule->m_aimodule,4,1,1)) {
			return(1);
		} else {
			return(0);
		}
		#endif
	} else {
		/* NPCs test... */
		if (alienStatusPointer->Target==NULL) {
			/* Can't be aware of nothing! */
			return 0;
		} else {
			GLOBALASSERT(alienStatusPointer->Target->containingModule);
			/* Module visibility test? */
			#if 1
			if ((IsModuleVisibleFromModule(sbPtr->containingModule,
				alienStatusPointer->Target->containingModule))) {
				return(1);
			} else {
				return(0);
			}
			#else
			if (GetNextModuleForLink_Core(sbPtr->containingModule->m_aimodule,
				alienStatusPointer->Target->containingModule->m_aimodule,4,1,1)) {
				return(1);
			} else {
				return(0);
			}
			#endif
		}
	}

	return 1;
}

static int AlienHasPathToTarget(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);

	GLOBALASSERT(sbPtr->containingModule);
	/* test for player being cloaked */
	if (alienStatusPointer->Target==Player->ObStrategyBlock)
	{
		GLOBALASSERT(alienStatusPointer->Target->containingModule);
		{
			#if 0
			PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
			LOCALASSERT(playerStatusPtr);

			if((playerStatusPtr->cloakOn==1)&&(playerStatusPtr->cloakPositionGivenAway==0)) {
				return 0;
			}
			#endif
		}
		/* test for player being an alien */
		if(AvP.PlayerType==I_Alien) {
			return 0;
		}
		/* Expensive path test? */
		if (GetNextModuleForLink_Core(sbPtr->containingModule->m_aimodule,
			alienStatusPointer->Target->containingModule->m_aimodule,4,1,1)) {
			return(1);
		} else {
			return(0);
		}

	} else {
		/* NPCs test... */
		if (alienStatusPointer->Target==NULL) {
			/* Can't be aware of nothing! */
			return 0;
		} else {
			GLOBALASSERT(alienStatusPointer->Target->containingModule);
			/* Module visibility test? */
			#if 0
			if ((IsModuleVisibleFromModule(sbPtr->containingModule,
				alienStatusPointer->Target->containingModule))) {
				return(1);
			} else {
				return(0);
			}
			#else
			if (GetNextModuleForLink_Core(sbPtr->containingModule->m_aimodule,
				alienStatusPointer->Target->containingModule->m_aimodule,4,1,1)) {
				return(1);
			} else {
				return(0);
			}
			#endif
		}
	}

	return 1;
}

ATTACK_DATA *AlienIsAbleToPounce(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	ATTACK_DATA *thisAttack;
	VECTORCH targetPoint;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);
	
	/* First rule.  Must be on a flat floor. */
	if(sbPtr->DynPtr->OrientMat.mat22<63000) {
		return(NULL);
	}
	if(sbPtr->DynPtr->IsInContactWithFloor==0) {
		return(NULL);
	}
	/* And not in an airduct. */
	if(sbPtr->containingModule->m_flags & MODULEFLAG_AIRDUCT) {
		return(NULL);
	}
    if(alienStatusPointer->motion.launchLocked)return NULL;
	/* Wounding! */

	/* Can't pounce if you've lost any leg sections. */	
	if (alienStatusPointer->Wounds&(section_flag_left_leg|section_flag_right_leg)) {
		return(NULL);
	}
	/* Also not if you've lost both feet. */
	if ((alienStatusPointer->Wounds&section_flag_left_foot)
		&&(alienStatusPointer->Wounds&section_flag_right_foot)) {
		return(NULL);
	}

	/* Also not if you're badly hurt. */
	{
		int factor;

		factor=GetAlienSpeedFactor_ForSequence(sbPtr,HMSQT_AlienRun,(int)ARSS_Standard);

		if (factor<((ONE_FIXED*3)/4)) {
			return(NULL);
		}
	}
	/* Status of target? */
	if (alienStatusPointer->Target==NULL) {
		/* Don't waste my time. */
		return(NULL);
	}
	if (alienStatusPointer->Target->SBdptr==NULL) {
		/* Ditto.  Target is far. */
		return(NULL);
	}

	GetTargetingPointOfObject(alienStatusPointer->Target->SBdptr,&targetPoint);

	/* Do we have a clear line of sight? */

	if (!(IsThisObjectVisibleFromThisPosition_WithIgnore(sbPtr->SBdptr,alienStatusPointer->Target->SBdptr,&targetPoint,NPC_MAX_VIEWRANGE))) {
		return(NULL);
	}
	

	/* Finally, test the sequence. */
	thisAttack=GetAlienPounceAttack(&alienStatusPointer->HModelController,alienStatusPointer->Wounds,
		alienStatusPointer->IAmCrouched);
	
	if ((thisAttack==NULL)&&(alienStatusPointer->IAmCrouched==0)) {
		/* Try again crouched. Pook. */
	thisAttack=GetAlienPounceAttack(&alienStatusPointer->HModelController,alienStatusPointer->Wounds,
		1);
	}

	return(thisAttack);
		
}

static int StartAlienPounce(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	ATTACK_DATA *thisAttack;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	/* Check validity, and do it. */

	thisAttack=AlienIsAbleToPounce(sbPtr);

	if (thisAttack==NULL) {
		/* Can't do it. */
		return(0);
	}

	SetAlienShapeAnimSequence_Core(sbPtr,thisAttack->Sequence_Type,thisAttack->Sub_Sequence,
		thisAttack->Sequence_Length,thisAttack->TweeningTime);
	
	alienStatusPointer->HModelController.Looped=0;
	alienStatusPointer->HModelController.LoopAfterTweening=0;

	alienStatusPointer->current_attack=thisAttack;

	alienStatusPointer->BehaviourState = ABS_Pounce;
	alienStatusPointer->NearStateTimer=0;
	
	return(1);
}

static void CheckPounceIntegrity(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	VECTORCH targetPoint;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	if (alienStatusPointer->Target==NULL) {
		/* Attack what? */
		alienStatusPointer->BehaviourState = ABS_Hunt;
		alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
		return;
	} 	

	if ((alienStatusPointer->Target->SBdptr==NULL)
		||((alienStatusPointer->Target->DynPtr->Position.vy-sbPtr->DynPtr->Position.vy)>1000)) {

		/* Yuck.  Target is far, or too far below you. */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;

		StartAlienMovementSequence(sbPtr);

		alienStatusPointer->NearStateTimer = 0;
		alienStatusPointer->CurveTimeOut = 0;
		return;
	}

	GetTargetingPointOfObject(alienStatusPointer->Target->SBdptr,&targetPoint);

	/* Do we have a clear line of sight? */

	if (!(IsThisObjectVisibleFromThisPosition_WithIgnore(sbPtr->SBdptr,alienStatusPointer->Target->SBdptr,&targetPoint,NPC_MAX_VIEWRANGE))) {
		/* Can't see! */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;

		StartAlienMovementSequence(sbPtr);

		alienStatusPointer->NearStateTimer = 0;
		alienStatusPointer->CurveTimeOut = 0;
		return;
	}

	
	/* Orientate towards player, just to make sure we're facing */
	{
		VECTORCH orientationDirn;
		int i;
		orientationDirn.vx = alienStatusPointer->Target->DynPtr->Position.vx - sbPtr->DynPtr->Position.vx;
		orientationDirn.vy = 0;
		orientationDirn.vz = alienStatusPointer->Target->DynPtr->Position.vz - sbPtr->DynPtr->Position.vz;
		i = NPCOrientateToVector(sbPtr, &orientationDirn,NPC_TURNRATE,NULL);
		
		if (i==0) {
			/* Still not right.  Be careful. */
			alienStatusPointer->HModelController.StopAfterTweening=1;
		} else {
			/* Okay for the moment. */
			alienStatusPointer->HModelController.StopAfterTweening=0;
		}
	
	}
	
	/* change back to approach?: don't need to directly test if we should go to wander, as 
	approach state will do this... */
	{
		int distanceToPlayer = VectorDistance(&(dynPtr->Position),&(alienStatusPointer->Target->DynPtr->Position));
		if((distanceToPlayer>ALIEN_POUNCE_MAXRANGE)) {					 

			NPC_InitMovementData(&(alienStatusPointer->moveData));
			alienStatusPointer->BehaviourState = ABS_Approach;

			StartAlienMovementSequence(sbPtr);

			alienStatusPointer->NearStateTimer = 0;
			alienStatusPointer->CurveTimeOut = 0;
			return;
		}
	}
	
	/* Are we still able to pounce? */
		
	/* Not if you've lost any leg sections. */	
	if ( (alienStatusPointer->Wounds&(section_flag_left_leg|section_flag_right_leg)) 
		/* Also not if you've lost both feet. */
		|| ((alienStatusPointer->Wounds&section_flag_left_foot)
		&&(alienStatusPointer->Wounds&section_flag_right_foot)) ) {
	
		/* Can't stand either.  Fall over. */
		/* That implies going back to approach, btw. */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;

		StartAlienMovementSequence(sbPtr);

		alienStatusPointer->NearStateTimer = 0;
		alienStatusPointer->CurveTimeOut = 0;
	}

}

static void ApplyPounceImpulse(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	VECTORCH pounceVector,targetPoint;
	int dist,speed,factor;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);
	LOCALASSERT(alienStatusPointer->Target);	

    if(!AlienLaunchBegin(&alienStatusPointer->motion,dynPtr->IsInContactWithFloor))return;
	GetTargetingPointOfObject(alienStatusPointer->Target->SBdptr,&targetPoint);
	
	dist = VectorDistance(&(dynPtr->Position),&targetPoint);

	/* Apply a correction based on range. */
	targetPoint.vy-=(dist>>3);

	pounceVector.vx=targetPoint.vx-dynPtr->Position.vx;
	pounceVector.vy=targetPoint.vy-dynPtr->Position.vy;
	pounceVector.vz=targetPoint.vz-dynPtr->Position.vz;

	Normalise(&pounceVector);
	/* Must jump at least a little bit upwards. */
	if (pounceVector.vy>-10000) {
		pounceVector.vy=-10000;
	}	

	switch (alienStatusPointer->Type) {
		case AT_Standard:
		default:
			speed=ALIEN_JUMP_SPEED;
			break;
		case AT_Predalien:
			speed=PREDALIEN_JUMP_SPEED;
			break;
		case AT_Praetorian:
			speed=PRAETORIAN_JUMP_SPEED;
			break;
	}

	factor=GetAlienSpeedFactor_ForSequence(sbPtr,HMSQT_AlienRun,(int)ARSS_Standard);
	speed=MUL_FIXED(speed,factor);

	pounceVector.vx=MUL_FIXED(speed,pounceVector.vx);
	pounceVector.vy=MUL_FIXED(speed,pounceVector.vy);
	pounceVector.vz=MUL_FIXED(speed,pounceVector.vz);

	sbPtr->DynPtr->LinImpulse=pounceVector;

}

static void AlienNearState_Pounce(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* don't climb on walls, etc */
	dynPtr->UseStandardGravity=1;

	/* Firstly, are we actually pouncing yet? */
	/* NearStateTimer is a status flag. */

	if (alienStatusPointer->NearStateTimer==0) {
		/* Still tweening? */
		if (alienStatusPointer->HModelController.Tweening==Controller_NoTweening) {
			/* We've finished!  Are we facing right? */
			VECTORCH orientationDirn;
			int i;

			orientationDirn.vx = alienStatusPointer->Target->DynPtr->Position.vx - sbPtr->DynPtr->Position.vx;
			orientationDirn.vy = 0;
			orientationDirn.vz = alienStatusPointer->Target->DynPtr->Position.vz - sbPtr->DynPtr->Position.vz;
			i = NPCOrientateToVector(sbPtr, &orientationDirn,NPC_TURNRATE,NULL);
			
			if (i==0) {
				/* Still not right!  Wait for proper facing. */
				alienStatusPointer->HModelController.Playing=0;
				CheckPounceIntegrity(sbPtr);
				return;
			} else {
				/* Okay, pounce! */
	
				ApplyPounceImpulse(sbPtr);

				alienStatusPointer->HModelController.Playing=1;
				alienStatusPointer->NearStateTimer=1;
			}
		} else {
			/* Yup, still tweening.  Check state validity. */
			CheckPounceIntegrity(sbPtr);
		}
	} else {
		/* We must be in the pounce.  Can't break out of this until the anim finishes. */

		if ((alienStatusPointer->HModelController.Tweening==Controller_NoTweening)
			&&(alienStatusPointer->HModelController.sequence_timer==(ONE_FIXED-1))) {
			/* You know what?  Just did! */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			alienStatusPointer->BehaviourState = ABS_Approach;

			StartAlienMovementSequence(sbPtr);
			InitWaypointManager(&alienStatusPointer->waypointManager);

			alienStatusPointer->NearStateTimer = 0;
			alienStatusPointer->CurveTimeOut = 0;
			/* Choose whether to pounce again. */
			Alien_ElectToPounce(sbPtr);
			return;

		}

		AlienNearDamageShell(sbPtr);
	
	}
	
}

static int CheckJumpingAbility(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

    if(!sbPtr->DynPtr->IsInContactWithFloor || alienStatusPointer->motion.launchLocked)return 0;
	if (alienStatusPointer->JumpDetected==0) {
		/* No jump sequence.  Always a bad start. */
		return(0);
	}

	/* Must be upright. */
	if(sbPtr->DynPtr->OrientMat.mat22<63000) {
		return(0);
	}

	/* Only the leg test here, so far. */	
	if ( (alienStatusPointer->Wounds&(section_flag_left_leg|section_flag_right_leg)) 
		|| ((alienStatusPointer->Wounds&section_flag_left_foot)
		&&(alienStatusPointer->Wounds&section_flag_right_foot)) ) {
		return(0);
	} else {
		return(1);
	}
	
}

static int GoToJump(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	int speed,factor;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	if (CheckJumpingAbility(sbPtr)==0) {
		/* bleah. */
		return(0);
	}

    if(!AlienLaunchBegin(&alienStatusPointer->motion,dynPtr->IsInContactWithFloor))return 0;
	/* Should be able to jump now! */

	dynPtr->LinVelocity.vx=0;
	dynPtr->LinVelocity.vy=0;
	dynPtr->LinVelocity.vz=0;

	dynPtr->LinImpulse.vx=0;
	dynPtr->LinImpulse.vy=-10000;
	dynPtr->LinImpulse.vz=20000;
	
	RotateVector(&dynPtr->LinImpulse,&dynPtr->OrientMat);
	Normalise(&dynPtr->LinImpulse);

	switch (alienStatusPointer->Type) {
		case AT_Standard:
		default:
			speed=ALIEN_JUMP_SPEED;
			break;
		case AT_Predalien:
			speed=PREDALIEN_JUMP_SPEED;
			break;
		case AT_Praetorian:
			speed=PRAETORIAN_JUMP_SPEED;
			break;
	}
	factor=GetAlienSpeedFactor_ForSequence(sbPtr,HMSQT_AlienRun,(int)ARSS_Standard);
	speed=MUL_FIXED(speed,factor);

	dynPtr->LinImpulse.vx=MUL_FIXED(speed,dynPtr->LinImpulse.vx);
	dynPtr->LinImpulse.vy=MUL_FIXED(speed,dynPtr->LinImpulse.vy);
	dynPtr->LinImpulse.vz=MUL_FIXED(speed,dynPtr->LinImpulse.vz);
	
	alienStatusPointer->BehaviourState = ABS_Jump;
	alienStatusPointer->NearStateTimer = 0;
	SetAlienShapeAnimSequence(sbPtr,HMSQT_AlienRun,(int)ARSS_Jump,ONE_FIXED>>1);					

	return(1);
}

static void AlienNearState_Jump(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;
	struct collisionreport *nextReport;
	int terminateState=0;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);
	nextReport = dynPtr->CollisionReportPtr;

	/* Pretty simple one, this.  Just fly through the air, until you hit something. */

	dynPtr->UseStandardGravity=1;
	/* Just to be on the safe side. */

	while(nextReport)
	{		
		if (nextReport->ObstacleSBPtr==NULL) {
			/* This is the environment. */
			terminateState=1;
		} else {
			switch (nextReport->ObstacleSBPtr->I_SBtype) {
				/* Futureproofing. */
				default:
					/* Oh, what the heck. */
					terminateState=1;
					break;
			}
		}
		nextReport = nextReport->NextCollisionReportPtr;
	}
	
	if (dynPtr->IsInContactWithFloor) {
		terminateState=1;
	}

	if (terminateState) {
		/* should be crawling. */
		alienStatusPointer->IAmCrouched = 1;

		StartAlienMovementSequence(sbPtr);
		InitWaypointManager(&alienStatusPointer->waypointManager);

		/* Hang on for dear life... */
		if (AlienIsAbleToClimb(sbPtr)) {
			AlienNavigationGravity(sbPtr,1);
		} else {
			dynPtr->UseStandardGravity=1;
		}
		/* And change back to approach. */
		NPC_InitMovementData(&(alienStatusPointer->moveData));
		alienStatusPointer->BehaviourState = ABS_Approach;
		alienStatusPointer->NearStateTimer = 0;
		alienStatusPointer->CurveTimeOut = 0;
		return;

	}
}

/* Tyrargo campaign safeguard, separate from the AvPx Skirmish ID fix.
 * Use perception distance, not the 3000-unit melee range. The regression save
 * places the exposed dormant alien 14720 units away from the player.
 * No module-visibility shortcut: each successful ray must hit this alien first. */
#define ALIEN_DORMANT_WAKE_RANGE 16000
extern int Alien_TargetFilter(STRATEGYBLOCK *candidate);
extern DISPLAYBLOCK PlayersWeapon;
static int AlienDormantPlayerIsNearbyAndVisible(STRATEGYBLOCK *sbPtr)
{
 ALIEN_STATUS_BLOCK *status;
 STRATEGYBLOCK *playerSB;
 VECTORCH eye,points[3],oldPoint,oldNormal;
 DISPLAYBLOCK *oldHit;
 SECTION_DATA *oldSection,*root,*head;
 int oldLambda,visible=0,count=1,i;
 double dx,dy,dz;
 if(!sbPtr || !sbPtr->DynPtr || !(status=(ALIEN_STATUS_BLOCK*)sbPtr->SBdataptr) ||
    NormalFrameTime<=0 || AvP.Network!=I_No_Network || !sbPtr->SBdptr || !Player ||
    !(playerSB=Player->ObStrategyBlock) || !playerSB->DynPtr || !playerSB->SBdataptr ||
    !((PLAYER_STATUS*)playerSB->SBdataptr)->IsAlive || !Alien_TargetFilter(playerSB))return 0;
 /* Reuse the dormant state's idle timer: no save-format change.
  * Five checks per simulated second, independent of rendering FPS. */
 if(status->NearStateTimer>ONE_FIXED/5)status->NearStateTimer=ONE_FIXED/5;
 status->NearStateTimer-=NormalFrameTime;
 if(status->NearStateTimer>0)return 0;
 status->NearStateTimer=ONE_FIXED/5;
 dx=(double)playerSB->DynPtr->Position.vx-sbPtr->DynPtr->Position.vx;
 dy=(double)playerSB->DynPtr->Position.vy-sbPtr->DynPtr->Position.vy;
 dz=(double)playerSB->DynPtr->Position.vz-sbPtr->DynPtr->Position.vz;
 if(dx*dx+dy*dy+dz*dz>(double)ALIEN_DORMANT_WAKE_RANGE*ALIEN_DORMANT_WAKE_RANGE)return 0;
 GetTargetingPointOfObject(Player,&eye);
 /* This proves the current pose before reading section world positions.
  * A curled-up chest can be hidden while the head/body remains exposed. */
 GetTargetingPointOfObject(sbPtr->SBdptr,&points[0]);
 root=sbPtr->SBdptr->HModelControlBlock?sbPtr->SBdptr->HModelControlBlock->section_data:NULL;
 head=root?GetThisSectionData(root,"head"):NULL;
 if(head && (head->flags&section_data_initialised) && !(head->flags&section_data_notreal))
  points[count++]=head->World_Offset;
 if(root && root!=head && (root->flags&section_data_initialised) && !(root->flags&section_data_notreal))
  points[count++]=root->World_Offset;
 oldPoint=LOS_Point;oldNormal=LOS_ObjectNormal;oldHit=LOS_ObjectHitPtr;
 oldSection=LOS_HModel_Section;oldLambda=LOS_Lambda;
 for(i=0;i<count && !visible;i++){
  VECTORCH direction;
  direction.vx=points[i].vx-eye.vx;direction.vy=points[i].vy-eye.vy;direction.vz=points[i].vz-eye.vz;
  if(!direction.vx && !direction.vy && !direction.vz)continue;
  Normalise(&direction);
  /* The player's own first-person model cannot conceal a world-space enemy.
   * Doors, walls, glass and all other objects retain the normal AI LOS rules. */
  FindPolygonInLineOfSight_TwoIgnores(&direction,&eye,0,Player,&PlayersWeapon);
  visible=(LOS_ObjectHitPtr==sbPtr->SBdptr && LOS_Lambda<=ALIEN_DORMANT_WAKE_RANGE);
 }
 LOS_Point=oldPoint;LOS_ObjectNormal=oldNormal;LOS_ObjectHitPtr=oldHit;
 LOS_HModel_Section=oldSection;LOS_Lambda=oldLambda;
 return visible;
}

void AlienNearState_Dormant(STRATEGYBLOCK *sbPtr)
{
	/* wait until near state timer runs out, then wander:
	alternatively, if we can attack the player, go straight to approach */
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;

	/* Also used for FAR! */

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	/* don't climb on walls, etc */
	dynPtr->UseStandardGravity=1;
	/* Zero velocity. */
	dynPtr->LinVelocity.vx = 0;
	dynPtr->LinVelocity.vy = 0;
	dynPtr->LinVelocity.vz = 0;

	if (!sbPtr->SBdptr) {
		if (alienStatusPointer->HModelController.Playing) {
			/* Try to call this as little as possible. */
			ProveHModel_Far(&alienStatusPointer->HModelController,sbPtr);
			if (alienStatusPointer->HModelController.Tweening==0) {
				alienStatusPointer->HModelController.Playing=0;
			}
		}
	} else {
		alienStatusPointer->HModelController.Playing=1;
	}

	if (AlienDormantPlayerIsNearbyAndVisible(sbPtr)) {
		Alien_Awaken(sbPtr);
		return;
	}

	/* Brushing Test. */
	{
		struct collisionreport *nextReport;
		nextReport = sbPtr->DynPtr->CollisionReportPtr;

		while(nextReport)
		{		
 			if(nextReport->ObstacleSBPtr)
 			{	
				if((nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourMarine)||
				   ((nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourMarinePlayer)
				   &&(AvP.PlayerType!=I_Alien))||
				   (nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourAlienPlayer)||
				   (nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourPredatorPlayer)||
				   (nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourNetGhost)||
				   (nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourXenoborg)||
				   (nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourPredatorAlien)||
				   (nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourQueenAlien)||
				   (nextReport->ObstacleSBPtr->I_SBtype==I_BehaviourFaceHugger))
					{
					Alien_Awaken(sbPtr);
				}
			} 		
 			nextReport = nextReport->NextCollisionReportPtr;
		}
	}


}

void AlienNearState_Awakening(STRATEGYBLOCK *sbPtr)
{
	/* wait until near state timer runs out, then wander:
	alternatively, if we can attack the player, go straight to approach */
	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;

	/* Also used for FAR! */

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr = sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	if (!sbPtr->SBdptr) {
		ProveHModel_Far(&alienStatusPointer->HModelController,sbPtr);
	}

	/* don't climb on walls, etc */
	dynPtr->UseStandardGravity=1;
	/* Zero velocity. */
	dynPtr->LinVelocity.vx = 0;
	dynPtr->LinVelocity.vy = 0;
	dynPtr->LinVelocity.vz = 0;

	if ((alienStatusPointer->HModelController.Tweening==Controller_NoTweening)
		&&(alienStatusPointer->HModelController.sequence_timer==(ONE_FIXED-1))) {

		Alien_GoToApproach(sbPtr);
	}


}

void AlienNearState_Taunting(STRATEGYBLOCK *sbPtr)
{

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	DYNAMICSBLOCK *dynPtr;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	dynPtr=sbPtr->DynPtr;
	LOCALASSERT(alienStatusPointer);
	LOCALASSERT(dynPtr);

	if (!sbPtr->SBdptr) {
		ProveHModel_Far(&alienStatusPointer->HModelController,sbPtr);
	}

	/* Orientate towards target, to avoid looking stupid */
	if (alienStatusPointer->Target) {

		VECTORCH orientationDirn;
		orientationDirn.vx = alienStatusPointer->Target->DynPtr->Position.vx - sbPtr->DynPtr->Position.vx;
		orientationDirn.vy = 0;
		orientationDirn.vz = alienStatusPointer->Target->DynPtr->Position.vz - sbPtr->DynPtr->Position.vz;
		NPCOrientateToVector(sbPtr, &orientationDirn,NPC_TURNRATE,NULL);
	}
	
	/* If finished, exit.  If on fire, go mental. */
	if ((HModelAnimation_IsFinished(&alienStatusPointer->HModelController))
		||(sbPtr->SBDamageBlock.IsOnFire)) {
		/* Exit state somehow. */
		InitWaypointManager(&alienStatusPointer->waypointManager);

		if(AlienHasPathToTarget(sbPtr))
		{
			/* Go to approach. */
			StartAlienMovementSequence(sbPtr);
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			alienStatusPointer->BehaviourState = ABS_Approach;
			alienStatusPointer->CurveTimeOut = 0;
			if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarine) {
				WarnMarineOfAttack(alienStatusPointer->Target,sbPtr);
			}
			return;
		} else if (AlienIsAwareOfTarget(sbPtr)) {
			/* Go to hunt. */
			StartAlienMovementSequence(sbPtr);
			alienStatusPointer->BehaviourState = ABS_Hunt;
			alienStatusPointer->NearStateTimer = ALIEN_NEARWAITTIME;
			return;
		} else {
		
			/* Go to wander */
			NPC_InitMovementData(&(alienStatusPointer->moveData));
			NPC_InitWanderData(&(alienStatusPointer->wanderData));
			alienStatusPointer->BehaviourState = ABS_Wander;
	
			StartAlienMovementSequence(sbPtr);
	
			alienStatusPointer->NearStateTimer = 0;		
			return;
		}
	} else {
		return;
	}

}

int TargetIsFiringFlamethrowerAtAlien(STRATEGYBLOCK *sbPtr)
{
	ALIEN_STATUS_BLOCK *alienStatusPointer;

	LOCALASSERT(sbPtr);	
	alienStatusPointer = (ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	if (alienStatusPointer->Target==NULL) {
		return(0);
	}

	/* First, let's see what the target is. */
	if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarinePlayer) {
		/* Is the player firing a flamethrower? */
		PLAYER_WEAPON_DATA *weaponPtr;
		PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	    GLOBALASSERT(playerStatusPtr);
	
		if (AvP.PlayerType!=I_Marine) {
			return(0);
		}
	    weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	
		if (weaponPtr->WeaponIDNumber != WEAPON_FLAMETHROWER) {
			return(0);
		}
		if (weaponPtr->CurrentState != WEAPONSTATE_FIRING_PRIMARY) {
			return(0);
		}
	} else if (alienStatusPointer->Target->I_SBtype==I_BehaviourMarine) {
		MARINE_STATUS_BLOCK *marineStatusPointer;    

		LOCALASSERT(sbPtr);
		marineStatusPointer = (MARINE_STATUS_BLOCK *)(alienStatusPointer->Target->SBdataptr);
		LOCALASSERT(marineStatusPointer);	          		
		
		if ((marineStatusPointer->My_Weapon->id!=MNPCW_Flamethrower)
			&&(marineStatusPointer->My_Weapon->id!=MNPCW_MFlamer)) {
			return(0);
		}
		if (marineStatusPointer->behaviourState!=MBS_Firing) {
			return(0);
		}
	} else {
		/* Gotta insert a case for netghosts. */
		return(0);
	}

	/* Next, let's see if the alien and the target are both in the other's front arc. */
	
	{

		VECTORCH sourcepos,targetpos,offset;
		MATRIXCH WtoL;
		
		WtoL=sbPtr->DynPtr->OrientMat;
		GetTargetingPointOfObject_Far(sbPtr,&sourcepos);
		GetTargetingPointOfObject_Far(alienStatusPointer->Target,&targetpos);
 	
		offset.vx=sourcepos.vx-targetpos.vx;
		offset.vy=sourcepos.vy-targetpos.vy;
		offset.vz=sourcepos.vz-targetpos.vz;
	
		TransposeMatrixCH(&WtoL);
		RotateVector(&offset,&WtoL);

		if ( (offset.vz <0) 
			&& (offset.vz <  offset.vx) 
			&& (offset.vz < -offset.vx) 
			&& (offset.vz <  offset.vy) 
			&& (offset.vz < -offset.vy) ) {
	
			/* 90 horizontal, 90 vertical... continue. */
		} else {
			return(0);
		}
		
		/* Now test it for the other way round. */

		WtoL=alienStatusPointer->Target->DynPtr->OrientMat;
		GetTargetingPointOfObject_Far(alienStatusPointer->Target,&sourcepos);
		GetTargetingPointOfObject_Far(sbPtr,&targetpos);
 	
		offset.vx=sourcepos.vx-targetpos.vx;
		offset.vy=sourcepos.vy-targetpos.vy;
		offset.vz=sourcepos.vz-targetpos.vz;
	
		TransposeMatrixCH(&WtoL);
		RotateVector(&offset,&WtoL);

		if ( (offset.vz <0) 
			&& (offset.vz <  offset.vx) 
			&& (offset.vz < -offset.vx) 
			&& (offset.vz <  offset.vy) 
			&& (offset.vz < -offset.vy) ) {
	
			/* 90 horizontal, 90 vertical... continue. */
		} else {
			return(0);
		}

	}
	
	/* If here, then it must be true! */
	return(1);
}

static int StartAlienTaunt(STRATEGYBLOCK *sbPtr) {

	ALIEN_STATUS_BLOCK *alienStatusPointer;    
	int a,b,numTaunts,sub_sequence;

	LOCALASSERT(sbPtr);
	alienStatusPointer=(ALIEN_STATUS_BLOCK *)(sbPtr->SBdataptr);    
	LOCALASSERT(alienStatusPointer);

	/* Check validity, and do it. */

	if ((AlienIsAbleToStand(sbPtr))==0) {
		/* First the crouch case. */
		if (HModelSequence_Exists(&alienStatusPointer->HModelController,(int)HMSQT_AlienCrouch,ACrSS_Taunt)) {
			SetAlienShapeAnimSequence_Core(sbPtr,(int)HMSQT_AlienCrouch,ACrSS_Taunt,-1,(ONE_FIXED>>3));
	
			alienStatusPointer->HModelController.Looped=0;
			alienStatusPointer->HModelController.LoopAfterTweening=0;

			alienStatusPointer->BehaviourState = ABS_Taunting;
			alienStatusPointer->NearStateTimer=0;
	
			if ((FastRandom()&127)<10) {
				DoAlienAITauntHiss(sbPtr);
			}
			return(1);
		} else {
			/* No sequence - can't do it. */
			return(0);
		}		
	}
	
	/* By this point, we must be able to stand. */
	numTaunts=0;	
	
	/* How many taunts are there? */

	a=0;
	while (AlienStandingTauntList[a]!=-1) {
		if (HModelSequence_Exists(&alienStatusPointer->HModelController,(int)HMSQT_AlienStand,AlienStandingTauntList[a])) {
			numTaunts++;
		}
		a++;
	}
	
	if (numTaunts<1) {
		/* Can't do it. */
		return(0);
	}

	b=FastRandom()%numTaunts;
	sub_sequence=-1;
	a=0;

	while (sub_sequence==-1) {
		if (HModelSequence_Exists(&alienStatusPointer->HModelController,(int)HMSQT_AlienStand,AlienStandingTauntList[a])) {
			if (b==0) {
				/* This one! */
				sub_sequence=AlienStandingTauntList[a];
			} else {
				b--;
			}
		}
		a++;
		GLOBALASSERT(AlienStandingTauntList[a]!=-1);
		GLOBALASSERT(b>=0);
	}
	/* The things I do for transparent code... */

	SetAlienShapeAnimSequence_Core(sbPtr,(int)HMSQT_AlienStand,sub_sequence,-1,(ONE_FIXED>>3));
	
	alienStatusPointer->HModelController.Looped=0;
	alienStatusPointer->HModelController.LoopAfterTweening=0;

	alienStatusPointer->BehaviourState = ABS_Taunting;
	alienStatusPointer->NearStateTimer=0;
	
	if ((FastRandom()&127)<10) {
		DoAlienAITauntHiss(sbPtr);
	}
	
	return(1);
}
