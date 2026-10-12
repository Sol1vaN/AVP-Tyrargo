/* Kill attribution, allied orders and nearby sentry operation. */
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "stratdef.h"
#include "pvisible.h"
#include "gamedef.h"
#include "dynblock.h"
#include "bh_types.h"
#include "bh_ais.h"
#include "bh_marin.h"
#include "bh_agun.h"
#include "savegame.h"
#include "npc_support.h"
#include "scream.h"
#include "psnd.h"
#include "pldnet.h"
#include "pldghost.h"
#include "bh_pred.h"
#include "los.h"
#include "weapons.h"
#include "targeting.h"
#include "particle.h"
#include "molecular_acid.h"
#include "bh_alien.h"
#include "bh_gener.h"
extern int NormalFrameTime;
#include <string.h>
#include <stddef.h>
extern void Marine_Enter_Taunt_State(STRATEGYBLOCK *sb);
extern void Marine_Enter_Wait_State(STRATEGYBLOCK *sb);
extern void Predator_Enter_Taunt_State(STRATEGYBLOCK *sb);
extern void InitMission(STRATEGYBLOCK *sb,MARINE_MISSION mission);
static int huntTraceTimer,huntTraceCount;
static struct {AIMODULE*module;int count;} huntTraces[256];
void NPCSupportFrameBegin(void){
 NPCSupportPatrolFrame();
 huntTraceTimer-=NormalFrameTime;if(huntTraceTimer>0)return;huntTraceTimer=ONE_FIXED/2;huntTraceCount=0;
 for(int i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK*s=ActiveStBlockList[i];if((s->I_SBtype!=I_BehaviourAlien && s->I_SBtype!=I_BehaviourPredatorAlien) || !s->SBdataptr || !s->containingModule || NPC_IsDead(s))continue;
  AIMODULE*m=s->containingModule->m_aimodule;if(!m)continue;int j=0;for(;j<huntTraceCount;j++)if(huntTraces[j].module==m)break;
  if(j==huntTraceCount){if(j>=256)continue;huntTraces[j].module=m;huntTraces[j].count=0;huntTraceCount++;}huntTraces[j].count++;
 }
}
AIMODULE*NPCSupportAlienHuntModule(STRATEGYBLOCK*sb){
 AIMODULE*best=NULL;int score=2147483647;
 for(int i=0;i<huntTraceCount;i++){
  VECTORCH point=huntTraces[i].module->m_world;int d=VectorDistance(&sb->DynPtr->Position,&point);
  /* Strategic traces select a search AREA. They never select a firing target
     or bypass the species' visibility/cloaking checks. */
  int value=d/(1+huntTraces[i].count);if(value<score){score=value;best=huntTraces[i].module;}
 }return best;
}
static NPC_DAMAGE_SOURCE currentSource;
static int useHeld,useEdge;
static int IsMarine(int type){return type==I_BehaviourMarine || type==I_BehaviourSeal;}
static int IsCelebratingSpecies(int type){return IsMarine(type)||type==I_BehaviourPredator;}
static int IsVictimSpecies(int type){return IsMarine(type)||type==I_BehaviourPredator || type==I_BehaviourAlien || type==I_BehaviourPredatorAlien || type==I_BehaviourMarinePlayer || type==I_BehaviourAlienPlayer || type==I_BehaviourPredatorPlayer;}
static int IsProjectile(int type){switch(type){
 case I_BehaviourGrenade:case I_BehaviourFlareGrenade:case I_BehaviourPulseGrenade:case I_BehaviourFragmentationGrenade:
 case I_BehaviourProximityGrenade:case I_BehaviourClusterGrenade:case I_BehaviourRocket:
 case I_BehaviourFlameProjectile:case I_BehaviourMolotov:case I_BehaviourFrisbee:
 case I_BehaviourFrisbeeEnergyBolt:case I_BehaviourNPCPredatorDisc:
 case I_BehaviourPredatorEnergyBolt:case I_BehaviourPPPlasmaBolt:case I_BehaviourSpeargunBolt:return 1;
 default:return 0;
}}
NPC_DAMAGE_SOURCE NPCSupportPushSource(STRATEGYBLOCK *sb){
 NPC_DAMAGE_SOURCE previous=currentSource;memset(&currentSource,0,sizeof(currentSource));
 if(sb && AvP.Network==I_No_Network){
  if(IsCelebratingSpecies(sb->I_SBtype))memcpy(currentSource.name,sb->SBname,SB_NAME_LENGTH);
  else if(IsProjectile(sb->I_SBtype))memcpy(currentSource.name,sb->combatOwner,SB_NAME_LENGTH);
 }
 return previous;
}
NPC_DAMAGE_SOURCE NPCSupportCurrentSource(void){return currentSource;}
NPC_DAMAGE_SOURCE NPCSupportPushOwner(NPC_DAMAGE_SOURCE owner){
 NPC_DAMAGE_SOURCE previous=currentSource;currentSource=owner;
 if(AvP.Network!=I_No_Network)memset(&currentSource,0,sizeof(currentSource));return previous;
}
NPC_DAMAGE_SOURCE NPCSupportPushBurnSource(STRATEGYBLOCK *sb){
 NPC_DAMAGE_SOURCE owner;memcpy(owner.name,sb->combatBurnOwner,SB_NAME_LENGTH);return NPCSupportPushOwner(owner);
}
void NPCSupportIgnite(STRATEGYBLOCK *sb){
 if(!sb->SBDamageBlock.IsOnFire)memcpy(sb->combatBurnOwner,currentSource.name,SB_NAME_LENGTH);
}
void NPCSupportPopSource(NPC_DAMAGE_SOURCE previous){currentSource=previous;}
void NPCSupportInheritSource(STRATEGYBLOCK *sb){memcpy(sb->combatOwner,currentSource.name,SB_NAME_LENGTH);}
NPC_DAMAGE_EVENT NPCSupportBeforeDamage(STRATEGYBLOCK *sb){
 NPC_DAMAGE_EVENT e={0};e.source=currentSource;
 if(sb){e.onFire=sb->SBDamageBlock.IsOnFire;e.type=sb->I_SBtype;e.alive=IsVictimSpecies(e.type)&&sb->SBdataptr&&!NPC_IsDead(sb);}
 return e;
}
void NPCSupportAfterDamage(STRATEGYBLOCK *sb,NPC_DAMAGE_EVENT e){
 if(sb && !e.onFire && sb->SBDamageBlock.IsOnFire)memcpy(sb->combatBurnOwner,e.source.name,SB_NAME_LENGTH);
 if(!e.alive || !sb || sb->combatKillReported || !NPC_IsDead(sb) || AvP.Network!=I_No_Network)return;
 sb->combatKillReported=1;
 NPC_DAMAGE_SOURCE empty={0};if(!memcmp(e.source.name,empty.name,SB_NAME_LENGTH))return;
 STRATEGYBLOCK *killer=FindSBWithName(e.source.name);
 if(!killer || killer==sb || !killer->SBdataptr || NPC_IsDead(killer))return;
 int kt=killer->I_SBtype;
 if((IsMarine(kt) && (e.type==I_BehaviourAlien || e.type==I_BehaviourPredatorAlien || e.type==I_BehaviourPredator || e.type==I_BehaviourAlienPlayer || e.type==I_BehaviourPredatorPlayer)) ||
 (kt==I_BehaviourPredator && (IsMarine(e.type) || e.type==I_BehaviourAlien || e.type==I_BehaviourPredatorAlien || e.type==I_BehaviourAlienPlayer || e.type==I_BehaviourMarinePlayer))){
  if(killer->supportKillsPending<2147483647)killer->supportKillsPending++;
 }
}
int NPCSupportCanCommand(STRATEGYBLOCK *sb){
 if(!sb || sb->I_SBtype!=I_BehaviourMarine || !sb->SBdataptr || !sb->DynPtr || NPC_IsDead(sb) || AvP.Network!=I_No_Network || AvP.PlayerType!=I_Marine)return 0;
 MARINE_STATUS_BLOCK *m=sb->SBdataptr;
 return m->My_Weapon && m->My_Weapon->ARealMarine && !m->Android && m->Mission!=MM_NonCom && m->Mission!=MM_RunAroundOnFire;
}
void NPCSupportUseInput(int pressed){useEdge=pressed&&!useHeld;useHeld=!!pressed;}
void NPCSupportCommand(STRATEGYBLOCK *sb){
 if(!useEdge || !NPCSupportCanCommand(sb))return;useEdge=0;
 MARINE_STATUS_BLOCK *m=sb->SBdataptr;
 sb->supportFollow=!sb->supportFollow;NPCNavReset(&m->localNav);NPCInventoryInit(sb);m->squad.patrolTimer=m->squad.tacticalTimer=0;
 m->supportSource=m->supportDestination=m->supportNext=NULL;m->supportRouteTimer=0;
 InitWaypointManager(&m->waypointManager);NPC_InitMovementData(&m->moveData);
 m->wanderData.currentModule=NPC_NOWANDERMODULE;
 if(!m->Target || NPC_IsDead(m->Target))InitMission(sb,m->Mission);
 if(m->soundHandle2!=SOUND_NOACTIVEINDEX)Sound_Stop(m->soundHandle2);
 PlayMarineCommandSound(sb->supportFollow,&m->soundHandle2,&sb->DynPtr->Position);
}
static void Celebrate(STRATEGYBLOCK *sb){
 if(!sb->supportKillsPending || NPC_IsDead(sb) || !sb->SBdptr)return;
 if(!SoundSys_IsOn()){sb->supportKillsPending--;return;}
 if(IsMarine(sb->I_SBtype)){
  MARINE_STATUS_BLOCK *m=sb->SBdataptr;
  if(m->Android){sb->supportKillsPending=0;return;}
   if(m->soundHandle2!=SOUND_NOACTIVEINDEX)return;
  int peaceful=(!m->Target || NPC_IsDead(m->Target)) && (m->behaviourState==MBS_Waiting || m->behaviourState==MBS_Wandering || m->behaviourState==MBS_Sentry);
  if(peaceful)Marine_Enter_Taunt_State(sb);
  if(m->soundHandle2==SOUND_NOACTIVEINDEX)PlayMarineScream(m->Voice,SC_Taunt,m->VoicePitch,&m->soundHandle2,&sb->DynPtr->Position);
  if(m->soundHandle2!=SOUND_NOACTIVEINDEX)sb->supportKillsPending--;
 }else if(sb->I_SBtype==I_BehaviourPredator){
  PREDATOR_STATUS_BLOCK *p=sb->SBdataptr;
  if(p->soundHandle!=SOUND_NOACTIVEINDEX)return;
  if((!p->Target || NPC_IsDead(p->Target)) && p->behaviourState!=PBS_Taunting)Predator_Enter_Taunt_State(sb);
  if(p->soundHandle==SOUND_NOACTIVEINDEX)PlayPredatorSound(0,PSC_Taunt,0,&p->soundHandle,&sb->DynPtr->Position);
  if(p->soundHandle!=SOUND_NOACTIVEINDEX)sb->supportKillsPending--;
 }
}
#include "acid_population_impl.inc"
void NPCSupportUpdate(STRATEGYBLOCK *sb){
 if(!sb || !sb->SBdataptr || !sb->DynPtr || AvP.Network!=I_No_Network)return;
 AcidPopulationUpdate(sb);
 if(IsCelebratingSpecies(sb->I_SBtype))Celebrate(sb);
 if(!NPCSupportCanCommand(sb) || !sb->SBdptr)return;
 sb->supportSentryTimer-=NormalFrameTime;if(sb->supportSentryTimer>0)return;sb->supportSentryTimer=ONE_FIXED/2;
 for(int i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK *gun=ActiveStBlockList[i];
  if(gun->I_SBtype!=I_BehaviourAutoGun || !gun->SBdataptr || !gun->DynPtr || !gun->SBdptr)continue;
  AUTOGUN_STATUS_BLOCK *g=gun->SBdataptr;
  if(g->behaviourState!=I_inactive || g->ammo<=0 || gun->SBflags.please_destroy_me)continue;
  if(VectorDistance(&sb->DynPtr->Position,&gun->DynPtr->Position)>3000)continue;
  VECTORCH eye,end,direction;GetTargetingPointOfObject_Far(sb,&eye);GetTargetingPointOfObject_Far(gun,&end);
  direction=end;SubVector(&eye,&direction);int distance=Magnitude(&direction);
  if(distance){Normalise(&direction);FindPolygonInLineOfSight_TwoIgnores(&direction,&eye,0,sb->SBdptr,gun->SBdptr);}
  if(!distance || !LOS_ObjectHitPtr || LOS_Lambda>=distance-30)RequestState(gun,1,0);
 }
}
void NPCSupportReset(void){extern void ParticleCollisionCacheReset(void);ParticleCollisionCacheReset();NPCSupportPatrolReset();memset(&currentSource,0,sizeof(currentSource));useHeld=useEdge=0;acidPopulationRemainder=0;NPCInventoryReset();huntTraceTimer=huntTraceCount=0;}
/* Optional extension: native player/NPC/projectile save layouts stay intact. */
typedef struct {SAVE_BLOCK_HEADER header;int schema;char name[SB_NAME_LENGTH];NPC_MARINE_SQUAD squad;} SQUAD_SAVE;
typedef struct {SAVE_BLOCK_HEADER header;int schema;char name[SB_NAME_LENGTH];char owner[SB_NAME_LENGTH];char burnOwner[SB_NAME_LENGTH];int following,kills,reported;} SUPPORT_SAVE;
void NPCSupportSave(void){
 NPCSupportPatrolSave();
 for(int i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK *s=ActiveStBlockList[i];
  AcidPopulationSave(s);
  if(NPCSupportCanCommand(s)){
   NPCInventoryInit(s);NPCInventorySync(s);SQUAD_SAVE*b=GetPointerForSaveBlock(sizeof(*b));memset(b,0,sizeof(*b));
   b->header.type=SaveBlock_NPCSupport;b->header.size=sizeof(*b);b->schema=246;memcpy(b->name,s->SBname,SB_NAME_LENGTH);b->squad=((MARINE_STATUS_BLOCK*)s->SBdataptr)->squad;
  }
  NPC_DAMAGE_SOURCE empty={0};
  if(!s->supportFollow && !s->supportKillsPending && !memcmp(s->combatOwner,empty.name,SB_NAME_LENGTH) && !memcmp(s->combatBurnOwner,empty.name,SB_NAME_LENGTH))continue;
  SUPPORT_SAVE *b=GetPointerForSaveBlock(sizeof(*b));memset(b,0,sizeof(*b));
  b->header.type=SaveBlock_NPCSupport;b->header.size=sizeof(*b);b->schema=243;
  memcpy(b->name,s->SBname,SB_NAME_LENGTH);memcpy(b->owner,s->combatOwner,SB_NAME_LENGTH);
  memcpy(b->burnOwner,s->combatBurnOwner,SB_NAME_LENGTH);b->following=s->supportFollow;b->kills=s->supportKillsPending;b->reported=s->combatKillReported;
 }
}
void NPCSupportLoad(SAVE_BLOCK_HEADER *header){
 if(NPCSupportPatrolLoad(header))return;
 if(AcidPopulationLoad(header))return;
 if(header->size>=sizeof(SAVE_BLOCK_HEADER)+sizeof(int) && *(int*)(header+1)==244){ParticleSupportLoadOwners(header);return;}
 size_t oldSize=offsetof(SQUAD_SAVE,squad)+offsetof(NPC_MARINE_SQUAD,loadoutReady);
 if((header->size==sizeof(SQUAD_SAVE) && *(int*)(header+1)==246) || (header->size==oldSize && *(int*)(header+1)==245)){
  SQUAD_SAVE*b=(SQUAD_SAVE*)header;STRATEGYBLOCK*s=FindSBWithName(b->name);
  if(NPCSupportCanCommand(s) && b->squad.initialized==1 && b->squad.activeSlot>=-1 && b->squad.activeSlot<MAX_NO_OF_WEAPON_SLOTS){
   NPCInventoryInit(s);NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)s->SBdataptr)->squad;
    for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++)if(q->weapons[i].WeaponIDNumber!=b->squad.weapons[i].WeaponIDNumber)return;
    memset(q,0,sizeof(*q));memcpy(q,&b->squad,header->size==oldSize?offsetof(NPC_MARINE_SQUAD,loadoutReady):sizeof(*q));q->jumpTimer=q->tacticalTimer=q->patrolTimer=0;
  }return;
 }
 if(header->size!=sizeof(SUPPORT_SAVE))return;SUPPORT_SAVE *b=(SUPPORT_SAVE*)header;
 if(b->schema!=243 || b->following<0 || b->following>1 || b->kills<0)return;
 STRATEGYBLOCK *s=FindSBWithName(b->name);if(!s)return;
 s->supportFollow=NPCSupportCanCommand(s)?b->following:0;
 memcpy(s->combatBurnOwner,b->burnOwner,SB_NAME_LENGTH);s->supportKillsPending=b->kills;s->combatKillReported=!!b->reported;
 if(IsProjectile(s->I_SBtype))memcpy(s->combatOwner,b->owner,SB_NAME_LENGTH);
}

static int SmartgunSpecies(STRATEGYBLOCK*s){
 if(s->I_SBtype==I_BehaviourNetGhost)return s->SBdataptr?((NETGHOSTDATABLOCK*)s->SBdataptr)->type:-1;
 if(Player && s==Player->ObStrategyBlock)return AvP.PlayerType==I_Marine?I_BehaviourMarinePlayer:AvP.PlayerType==I_Alien?I_BehaviourAlienPlayer:I_BehaviourPredatorPlayer;
 return s->I_SBtype;
}
int NPCSupportSmartgunHostile(STRATEGYBLOCK*shooter,STRATEGYBLOCK*target){
 if(!shooter || !target || target==shooter || !target->SBdataptr)return 0;
 int a=SmartgunSpecies(shooter),b=SmartgunSpecies(target);
 if(a==I_BehaviourMarine || a==I_BehaviourMarinePlayer){
  if((b==I_BehaviourMarine || b==I_BehaviourSeal) && IsMarine(target->I_SBtype)){MARINE_STATUS_BLOCK*m=target->SBdataptr;if(!m->Android && !(m->My_Weapon && m->My_Weapon->Android))return 0;}
  if(b==I_BehaviourMarine && target->I_SBtype==I_BehaviourNetGhost)return 0;
  if(b==I_BehaviourMarinePlayer && (a==I_BehaviourMarine || AvP.Network==I_No_Network || netGameData.gameType==NGT_Coop || netGameData.gameType==NGT_CoopDeathmatch || netGameData.gameType==NGT_LastManStanding))return 0;
 }
 if(AvP.Network!=I_No_Network && target->I_SBtype==I_BehaviourNetGhost && netGameData.gameType==NGT_Coop && (b==I_BehaviourMarinePlayer||b==I_BehaviourAlienPlayer||b==I_BehaviourPredatorPlayer))return 0;
 return 1;
}
int NPCSupportPredatorRevealed(STRATEGYBLOCK*target){
 if(!target || target->I_SBtype!=I_BehaviourPredator || !target->SBdataptr)return 0;
 PREDATOR_STATUS_BLOCK*p=target->SBdataptr;
 /* The laser/burning body is a visual cue. Callers still require physical LOS. */
 return p->Pred_Laser_On || p->behaviourState==PBS_Attacking || target->SBDamageBlock.IsOnFire;
}
int NPCSupportSentryGoal(STRATEGYBLOCK*sb,VECTORCH*goal){
 if(!NPCSupportCanCommand(sb))return 0;MARINE_STATUS_BLOCK*m=sb->SBdataptr;
 if(m->sentryGoalTimer>0){STRATEGYBLOCK*gun=FindSBWithName(m->sentryGoalName);if(gun && gun->I_SBtype==I_BehaviourAutoGun && gun->DynPtr && gun->SBdataptr && ((AUTOGUN_STATUS_BLOCK*)gun->SBdataptr)->behaviourState==I_inactive && ((AUTOGUN_STATUS_BLOCK*)gun->SBdataptr)->ammo>0 && VectorDistance(&sb->DynPtr->Position,&gun->DynPtr->Position)<12000){*goal=gun->DynPtr->Position;return 1;}return 0;}
 m->sentryGoalTimer=ONE_FIXED/2;memset(m->sentryGoalName,0,SB_NAME_LENGTH);STRATEGYBLOCK*best=NULL;int distance=12000;
 for(int i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK*gun=ActiveStBlockList[i];if(gun->I_SBtype!=I_BehaviourAutoGun || !gun->DynPtr || !gun->SBdataptr || gun->SBflags.please_destroy_me)continue;
  AUTOGUN_STATUS_BLOCK*g=gun->SBdataptr;if(g->behaviourState!=I_inactive || g->ammo<=0)continue;
  int d=VectorDistance(&sb->DynPtr->Position,&gun->DynPtr->Position);if(d>=distance)continue;
  int reserved=0;for(int j=0;j<NumActiveStBlocks;j++){STRATEGYBLOCK*ally=ActiveStBlockList[j];if(ally!=sb && NPCSupportCanCommand(ally) && VectorDistance(&ally->DynPtr->Position,&gun->DynPtr->Position)+500<d){reserved=1;break;}}
  if(!reserved){best=gun;distance=d;}
 }
 if(!best)return 0;memcpy(m->sentryGoalName,best->SBname,SB_NAME_LENGTH);*goal=best->DynPtr->Position;return 1;
}

#include "npc_patrol_impl.inc"
