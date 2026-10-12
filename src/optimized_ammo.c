#include "3dc.h"
#include "gamedef.h"
#include "equipmnt.h"
#include "molecular_acid.h"
#include "optimized_ammo.h"
#include <stdint.h>
#include <string.h>
int OptimizedAmmo=0;
unsigned int OptimizedAmmoDeficit[OPTIMIZED_AMMO_CHANNELS];
_Static_assert(MAX_NO_OF_WEAPON_TEMPLATES*2<=60,"Reserve channel layout");
void OptimizedAmmoReset(void){memset(OptimizedAmmoDeficit,0,sizeof OptimizedAmmoDeficit);}
int OptimizedAmmoHasResiduals(void){
 for(int i=0;i<OPTIMIZED_AMMO_CHANNELS;i++)if(OptimizedAmmoDeficit[i])return 1;
 return 0;
}
static int reserveChannel(int weapon,int secondary){
 if(weapon<0 || weapon>=MAX_NO_OF_WEAPON_TEMPLATES)return -1;
 if(weapon==WEAPON_GRENADELAUNCHER && !secondary){
  switch(GrenadeLauncherData.SelectedAmmo){
   case AMMO_FLARE_GRENADE:return 61;
   case AMMO_PROXIMITY_GRENADE:return 62;
   case AMMO_FRAGMENTATION_GRENADE:return 63;
   default:return 60;
  }
 }
 return weapon*2+(secondary!=0);
}
static uint64_t reserveAmount(int channel,unsigned short magazines,unsigned int capacity){
 unsigned int *d=&OptimizedAmmoDeficit[channel];
 if(!magazines || *d>=capacity)*d=0;
 return (uint64_t)magazines*capacity-*d;
}
static void storeReserve(int channel,uint64_t amount,unsigned int capacity,unsigned short *magazines){
 uint64_t count=(amount+capacity-1)/capacity;
 /* A stack uses the widened runtime representation; callers own gameplay caps. */
 if(count>65535){count=65535;amount=count*capacity;}
 *magazines=(unsigned short)count;
 OptimizedAmmoDeficit[channel]=(unsigned int)(count*capacity-amount);
}
void OptimizedAmmoReload(int channel,unsigned int capacity,unsigned int *rounds,
                         unsigned short *magazines,int conserve){
 if(channel<0 || channel>=OPTIMIZED_AMMO_CHANNELS || !capacity)return;
 uint64_t reserve=reserveAmount(channel,*magazines,capacity);
 if(!reserve || *rounds>=capacity)return;
 if(conserve){
  unsigned int take=capacity-*rounds;
  if(reserve<take)take=(unsigned int)reserve;
  *rounds+=take;
  storeReserve(channel,reserve-take,capacity,magazines);
 }else{
  /* Vanilla discards the loaded residue. If this game previously used the
   * option, its final partial magazine must not become a free full one. */
  *rounds=*magazines==1?(unsigned int)reserve:capacity;
  --*magazines;
  if(!*magazines)OptimizedAmmoDeficit[channel]=0;
 }
}
void ReloadWeaponMagazine(PLAYER_WEAPON_DATA *w,int secondary){
 if(!w || w->WeaponIDNumber<0 || w->WeaponIDNumber>=MAX_NO_OF_WEAPON_TEMPLATES)return;
 int ammo=secondary?TemplateWeapon[w->WeaponIDNumber].SecondaryAmmoID:TemplateWeapon[w->WeaponIDNumber].PrimaryAmmoID;
 if(w->WeaponIDNumber==WEAPON_TWO_PISTOLS)ammo=AMMO_MARINE_PISTOL_PC;
 if(w->WeaponIDNumber==WEAPON_GRENADELAUNCHER && !secondary)ammo=GrenadeLauncherData.SelectedAmmo;
 if(ammo<0 || ammo>=MAX_NO_OF_AMMO_TEMPLATES)return;
 unsigned int capacity=AcidMagazineRounds(ammo,TemplateAmmo[ammo].AmmoPerMagazine);
 unsigned int *rounds=secondary?&w->SecondaryRoundsRemaining:&w->PrimaryRoundsRemaining;
 unsigned short *mags=secondary?&w->SecondaryMagazinesRemaining:&w->PrimaryMagazinesRemaining;
 if(AvP.Network!=I_No_Network){
  if(capacity && *mags && *rounds<capacity){*rounds=capacity;--*mags;}
  return;
 }
 OptimizedAmmoReload(reserveChannel(w->WeaponIDNumber,secondary),capacity,rounds,mags,OptimizedAmmo==1);
}
void OptimizedAmmoSplitPistols(PLAYER_WEAPON_DATA *single,PLAYER_WEAPON_DATA *dual){
 if(AvP.Network!=I_No_Network)return;
 int c=WEAPON_MARINE_PISTOL*2,d=WEAPON_TWO_PISTOLS*2;
 unsigned int cap=AcidMagazineRounds(AMMO_MARINE_PISTOL_PC,TemplateAmmo[AMMO_MARINE_PISTOL_PC].AmmoPerMagazine);
 if(!cap)return;
 reserveAmount(c,single->PrimaryMagazinesRemaining,cap);
 OptimizedAmmoDeficit[d]=dual->PrimaryMagazinesRemaining?OptimizedAmmoDeficit[c]:0;
 OptimizedAmmoDeficit[d+1]=0;
}
void OptimizedAmmoMergePistols(PLAYER_WEAPON_DATA *single,PLAYER_WEAPON_DATA *dual){
 if(AvP.Network!=I_No_Network)return;
 int c=WEAPON_MARINE_PISTOL*2,d=WEAPON_TWO_PISTOLS*2;
 unsigned int cap=AcidMagazineRounds(AMMO_MARINE_PISTOL_PC,TemplateAmmo[AMMO_MARINE_PISTOL_PC].AmmoPerMagazine);
 if(!cap)return;
 uint64_t total=reserveAmount(d,dual->PrimaryMagazinesRemaining,cap)+reserveAmount(d+1,dual->SecondaryMagazinesRemaining,cap);
 uint64_t limit=(uint64_t)AcidResourceCapacity(99)*cap;
 if(total>limit)total=limit;
 storeReserve(c,total,cap,&single->PrimaryMagazinesRemaining);
}
void OptimizedAmmoCopyWeapon(int weapon,unsigned int *dst,const unsigned int *src){
 if(weapon<0 || weapon>=MAX_NO_OF_WEAPON_TEMPLATES)return;
 dst[weapon*2]=src[weapon*2];dst[weapon*2+1]=src[weapon*2+1];
 if(weapon==WEAPON_GRENADELAUNCHER)for(int i=60;i<64;i++)dst[i]=src[i];
}
