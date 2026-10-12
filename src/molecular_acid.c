/* Molecular acid is a campaign modifier over I_Hard, never an index into
 * the four legacy AI/damage/statistics tables. All arithmetic is integer. */
#include "3dc.h"
#include "gamedef.h"
#include "equipmnt.h"
#include "molecular_acid.h"
#include <stdint.h>
#include <limits.h>
#include <string.h>
int MolecularAcidTier;
int AcidPickupRemainders[ACID_PICKUP_CHANNELS];
static const int resourceTenths[4]={10,15,20,30};
static const int spawnTenths[4]={10,15,20,25};
void AcidResetPickups(void){memset(AcidPickupRemainders,0,sizeof AcidPickupRemainders);}
void AcidSetTier(int tier){MolecularAcidTier=(tier>=1&&tier<=3)?tier:0;AcidResetPickups();if(MolecularAcidTier)AvP.Difficulty=I_Hard;}
int AcidActive(void){return MolecularAcidTier>=1&&MolecularAcidTier<=3&&AvP.Network==I_No_Network&&AvP.Difficulty==I_Hard;}
static int bounded(int64_t n){return n>INT_MAX?INT_MAX:(int)n;}
int AcidResourceCapacity(int base){return AcidActive()&&base>0?bounded((int64_t)base*resourceTenths[MolecularAcidTier]/10):base;}
int AcidSpawnLimit(int base){return AcidActive()&&base>0?bounded(((int64_t)base*spawnTenths[MolecularAcidTier]+9)/10):base;}
int AcidSpawnInterval(int base){if(!AcidActive()||base<=0)return base;int n=(int)((int64_t)base*10/spawnTenths[MolecularAcidTier]);return n>0?n:1;}
int AcidPickupBonus(int channel,int amount){
 if(!AcidActive()||amount<=0||channel<0||channel>=ACID_PICKUP_CHANNELS)return 0;
 int64_t n=(int64_t)amount*(resourceTenths[MolecularAcidTier]-10)+AcidPickupRemainders[channel];
 AcidPickupRemainders[channel]=(int)(n%10);return bounded(n/10);
}
int AcidCompletion(void){return AcidActive()?5:AvP.Difficulty+1;}
const char *AcidMultiplierLabel(int tier){static const char *labels[]={"x1","x1","x1.5","x2"};return labels[tier>=1&&tier<=3?tier:1];}

/* Only magazine-fed player ammunition is expanded. Never change the shared
 * templates: NPC damage, multiplayer and later non-Acid games stay stock. */
int AcidMagazineRounds(int ammoID,int baseFixed)
{
 switch(ammoID){
 case AMMO_SMARTGUN: case AMMO_FLAMETHROWER: case AMMO_MINIGUN:
 case AMMO_GRENADE: case AMMO_FLARE_GRENADE:
 case AMMO_FRAGMENTATION_GRENADE: case AMMO_PROXIMITY_GRENADE:
 case AMMO_MARINE_PISTOL_PC:
  return AcidActive()?AcidResourceCapacity(baseFixed/ONE_FIXED)*ONE_FIXED:baseFixed;
 default:return baseFixed;
 }
}
int AcidMagazinePickup(int ammoID,int baseCount)
{
 if(AcidActive() && baseCount>0 &&
    (ammoID==AMMO_10MM_CULW || ammoID==AMMO_SADAR_TOW || ammoID==AMMO_FRISBEE))
  return MolecularAcidTier+2; /* Total per pickup: 3 / 4 / 5, not a bonus. */
 return baseCount;
}
int AcidStartingArmour(int species,int basePoints)
{
 if(AcidActive() && species==I_Predator)return AcidResourceCapacity(15*ONE_FIXED);
 if(species==I_Marine)return AcidResourceCapacity(basePoints)*ONE_FIXED;
 return basePoints*ONE_FIXED;
}
