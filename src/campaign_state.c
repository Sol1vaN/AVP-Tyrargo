/* Durable player resources only: no strategy, animation or physics pointers.
 * Transfer is one-shot; the resulting mission-entry snapshot survives retries. */
#include "optimized_ammo.h"
#include "molecular_acid.h"
#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "equipmnt.h"
#include "avp_envinfo.h"
#include "avp_userprofile.h"
#include "bonusabilities.h"
#include "campaign_state.h"
#include "inventory_wire.h"
#include <string.h>
int CampaignModeFromPreference(int schema,int value){
 if(schema==203 && value>=0 && value<=1)return value?CAMPAIGN_RETAIN:CAMPAIGN_OFF;
 if(schema==207 && value>=CAMPAIGN_OFF && value<=CAMPAIGN_RETAIN)return value;
 return CAMPAIGN_OFF;
}
int RetainPlayerState=0;
extern int MarineEpisodeToPlay,PredatorEpisodeToPlay,AlienEpisodeToPlay;
typedef struct {
 int id,possessed; unsigned int primary,secondary;
 unsigned short magazines,secondaryMagazines;
} CAMPAIGN_WEAPON;
typedef struct {
 int valid,species,episode,environment,difficulty,acidTier;
 int acidRemainders[ACID_PICKUP_CHANNELS];
 unsigned int ammoDeficit[OPTIMIZED_AMMO_CHANNELS];
 int health,armour,energy,charge,tracker,jetpack,grapple;
 CAMPAIGN_WEAPON weapon[MAX_NO_OF_WEAPON_SLOTS];
 GRENADE_LAUNCHER_DATA grenades;
} CAMPAIGN_SNAPSHOT;
static CAMPAIGN_SNAPSHOT transfer,missionEntry,loadDefault;
static int transferArmed,restartArmed,loadingSavedGame;
static int CampaignEpisode(void){
 switch(AvP.PlayerType){case I_Marine:return MarineEpisodeToPlay;
 case I_Predator:return PredatorEpisodeToPlay;case I_Alien:return AlienEpisodeToPlay;default:return -1;}
}
static int retainEnabled(void){
 return RetainPlayerState==CAMPAIGN_RETAIN && AvP.Network==I_No_Network && CheatMode_Active==CHEATMODE_NONACTIVE;
}
static int matches(const CAMPAIGN_SNAPSHOT *c,int checkEnvironment){
 return c->valid && c->species==AvP.PlayerType && c->episode==CampaignEpisode() &&
  (!checkEnvironment || c->environment==AvP.CurrentEnv) && c->difficulty==AvP.Difficulty &&
  c->acidTier==(AcidActive()?MolecularAcidTier:0);
}
static void capture(CAMPAIGN_SNAPSHOT *c,STRATEGYBLOCK *sb,int episode){
 memset(c,0,sizeof *c);
 if(!retainEnabled() || !sb || !sb->SBdataptr)return;
 PLAYER_STATUS *ps=(PLAYER_STATUS*)sb->SBdataptr;
 if(!ps->IsAlive || sb->SBDamageBlock.Health<=0)return;
 c->species=AvP.PlayerType;c->episode=episode;c->environment=AvP.CurrentEnv;c->difficulty=AvP.Difficulty;
 c->acidTier=AcidActive()?MolecularAcidTier:0;
 memcpy(c->acidRemainders,AcidPickupRemainders,sizeof c->acidRemainders);
 memcpy(c->ammoDeficit,OptimizedAmmoDeficit,sizeof c->ammoDeficit);
 c->health=sb->SBDamageBlock.Health;c->armour=sb->SBDamageBlock.Armour;
 c->energy=ps->Energy;c->charge=ps->FieldCharge;
 c->tracker=ps->MTrackerType;c->jetpack=ps->JetpackEnabled;c->grapple=ps->GrapplingHookEnabled;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  PLAYER_WEAPON_DATA *w=&ps->WeaponSlot[i];CAMPAIGN_WEAPON *v=&c->weapon[i];
  v->id=w->WeaponIDNumber;v->possessed=w->Possessed;
  v->primary=w->PrimaryRoundsRemaining;v->secondary=w->SecondaryRoundsRemaining;
  v->magazines=w->PrimaryMagazinesRemaining;v->secondaryMagazines=w->SecondaryMagazinesRemaining;
 }
 c->grenades=GrenadeLauncherData;c->valid=1;
}
static void restore(const CAMPAIGN_SNAPSHOT *c,STRATEGYBLOCK *sb){
 PLAYER_STATUS *ps=(PLAYER_STATUS*)sb->SBdataptr;int hasGrenades=0;
 if(AcidActive())memcpy(AcidPickupRemainders,c->acidRemainders,sizeof AcidPickupRemainders);
 sb->SBDamageBlock.Health=ps->Health=c->health;sb->SBDamageBlock.Armour=ps->Armour=c->armour;
 ps->Energy=c->energy;ps->FieldCharge=c->charge;
 if(c->tracker>(int)ps->MTrackerType)ps->MTrackerType=c->tracker;
 ps->JetpackEnabled|=c->jetpack;ps->GrapplingHookEnabled|=c->grapple;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  PLAYER_WEAPON_DATA *w=&ps->WeaponSlot[i];
  if(w->WeaponIDNumber==NULL_WEAPON || w->Possessed==-1)continue;
  for(int j=0;j<MAX_NO_OF_WEAPON_SLOTS;j++){
   const CAMPAIGN_WEAPON *v=&c->weapon[j];
   if(v->id!=w->WeaponIDNumber || v->possessed!=1)continue;
   OptimizedAmmoCopyWeapon(w->WeaponIDNumber,OptimizedAmmoDeficit,c->ammoDeficit);
   w->Possessed=1;w->PrimaryRoundsRemaining=v->primary;w->SecondaryRoundsRemaining=v->secondary;
   w->PrimaryMagazinesRemaining=v->magazines;w->SecondaryMagazinesRemaining=v->secondaryMagazines;
   if(w->WeaponIDNumber==WEAPON_GRENADELAUNCHER)hasGrenades=1;
   break;
  }
 }
 if(hasGrenades)GrenadeLauncherData=c->grenades;
 /* Destination restrictions/new equipment and readying animation still apply. */
}
void CampaignCapturePlayer(void){
 int basic,episode=CampaignEpisode();memset(&transfer,0,sizeof transfer);transferArmed=0;
 if(!retainEnabled() || !AvP.LevelCompleted || !Player)return;
 switch(AvP.PlayerType){case I_Marine:basic=MAX_NO_OF_BASIC_MARINE_EPISODES;break;
 case I_Predator:basic=MAX_NO_OF_BASIC_PREDATOR_EPISODES;break;
 case I_Alien:basic=MAX_NO_OF_BASIC_ALIEN_EPISODES;break;default:return;}
 if(episode<0 || episode>=basic-1)return;
 capture(&transfer,Player->ObStrategyBlock,episode+1);
}
void CampaignArmNextLevel(void){
 transferArmed=retainEnabled() && matches(&transfer,0);
 if(!transferArmed)transfer.valid=0;
}
void CampaignBeginLevel(void){
 /* A fresh episode selection must not inherit a previous run of the same map. */
 memset(&missionEntry,0,sizeof missionEntry);memset(&loadDefault,0,sizeof loadDefault);
 restartArmed=loadingSavedGame=0;
 if(!transferArmed)transfer.valid=0;
}
void CampaignArmRestart(void){
 restartArmed=!loadingSavedGame && retainEnabled() && matches(&missionEntry,1);
}
void CampaignRestorePlayer(struct strategyblock *sb){
 const CAMPAIGN_SNAPSHOT *c=NULL;
 if(retainEnabled() && !loadingSavedGame){
  if(restartArmed && matches(&missionEntry,1))c=&missionEntry;
  else if(transferArmed && matches(&transfer,0))c=&transfer;
 }
 restartArmed=transferArmed=0;
 if(sb && sb->SBdataptr){
  if(c)restore(c,sb);
  /* Capture AFTER applying the incoming inventory, and before gameplay. */
  capture(loadingSavedGame?&loadDefault:&missionEntry,sb,CampaignEpisode());
 }
 transfer.valid=0;
}
void CampaignPrepareSavedGame(void){
 memset(&missionEntry,0,sizeof missionEntry);memset(&loadDefault,0,sizeof loadDefault);
 transfer.valid=0;restartArmed=transferArmed=0;loadingSavedGame=1;
}
void CampaignFinishSavedGame(int success){
 loadingSavedGame=0;
 if(!success){missionEntry=loadDefault;return;}
 if(!matches(&missionEntry,1)){
  /* Older saves have no entry snapshot. Use their living saved inventory as
   * the retry baseline rather than replacing it with vanilla equipment. */
  capture(&missionEntry,Player?Player->ObStrategyBlock:NULL,CampaignEpisode());
  if(!missionEntry.valid)missionEntry=loadDefault;
 }
}
typedef struct {int schema;CAMPAIGN_SNAPSHOT entry;} CAMPAIGN_ENTRY_PAYLOAD;
int CampaignEntryPayloadSize(void){return sizeof(CAMPAIGN_ENTRY_PAYLOAD);}
int CampaignHasEntry(void){return retainEnabled() && matches(&missionEntry,1);}
int CampaignExportEntry(void *data,int size){
 if(!data || size!=sizeof(CAMPAIGN_ENTRY_PAYLOAD) || !CampaignHasEntry())return 0;
 CAMPAIGN_ENTRY_PAYLOAD payload;memset(&payload,0,sizeof payload);
 payload.schema=242;payload.entry=missionEntry;memcpy(data,&payload,sizeof payload);return 1;
}
int CampaignImportEntry(const void *data,int size){
 CAMPAIGN_ENTRY_PAYLOAD payload;
 if(!data || size!=sizeof payload)return 0;
 memcpy(&payload,data,sizeof payload);
 if(payload.schema==216){
  /* Legacy snapshot counts share the same padded struct size. Convert the
   * old adjacent bytes before interpreting them as independent 16-bit counts. */
  for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
   const unsigned char *bytes=(const unsigned char*)&payload.entry.weapon[i].magazines;
   unsigned int primary=bytes[0],secondary=bytes[1];
   payload.entry.weapon[i].magazines=primary;payload.entry.weapon[i].secondaryMagazines=secondary;
  }
  LEGACY_GRENADE_LAUNCHER_DATA legacy;memcpy(&legacy,&payload.entry.grenades,sizeof legacy);
  InventoryLoadGrenades(&payload.entry.grenades,&legacy);
 }else if(payload.schema!=242)return 0;
 if(!matches(&payload.entry,1) || payload.entry.health<=0 || payload.entry.armour<0)return 0;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  const CAMPAIGN_WEAPON *w=&payload.entry.weapon[i];
  if(w->id<NULL_WEAPON || w->id>=MAX_NO_OF_WEAPON_TEMPLATES || w->possessed < -1 || w->possessed>1)return 0;
 }
 missionEntry=payload.entry;return 1;
}
