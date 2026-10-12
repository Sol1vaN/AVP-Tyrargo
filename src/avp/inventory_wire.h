#ifndef TYRARGO_INVENTORY_WIRE_H
#define TYRARGO_INVENTORY_WIRE_H
#include <stddef.h>
#include <string.h>
#include "molecular_acid.h"
/* Keep the original byte-based player save representation. Runtime reserves
 * are 16-bit; the existing Acid block carries the expanded values. */
typedef struct legacy_player_weapon_data
{
	enum WEAPON_ID	 	WeaponIDNumber;
	/* eg.==MARINE_WEAPON_PULSE. This can be used to access the TemplateWeapon[] data, graphics, etc. */
 
    enum WEAPON_STATE 	CurrentState;
 
    int				 	StateTimeOutCounter; /* in 16.16 */   
        
    unsigned int		PrimaryRoundsRemaining; /* in 'current' magazine */
    unsigned int		SecondaryRoundsRemaining; /* in 'current' magazine */
    unsigned char		PrimaryMagazinesRemaining;
    unsigned char		SecondaryMagazinesRemaining;

	VECTORCH			PositionOffset; 
    EULER				DirectionOffset;

	SHAPEANIMATIONCONTROLLER ShpAnimCtrl;
    TXACTRLBLK			*TxAnimCtrl;
     
    /* general flags */
    signed int		Possessed :2; /* meaning you are carrying the weapon, not that an evil spirit etc etc. */

} LEGACY_PLAYER_WEAPON_DATA;
typedef struct 
{
	enum AMMO_ID	SelectedAmmo;

	unsigned int	StandardRoundsRemaining;
    unsigned char	StandardMagazinesRemaining;

	unsigned int	FlareRoundsRemaining;
    unsigned char	FlareMagazinesRemaining;

	unsigned int	ProximityRoundsRemaining;
    unsigned char	ProximityMagazinesRemaining;

	unsigned int	FragmentationRoundsRemaining;
    unsigned char	FragmentationMagazinesRemaining;

} LEGACY_GRENADE_LAUNCHER_DATA;
typedef char InventoryWeaponWireSize[(sizeof(LEGACY_PLAYER_WEAPON_DATA)==sizeof(PLAYER_WEAPON_DATA))?1:-1];
typedef char InventoryWeaponWireOffset[(offsetof(LEGACY_PLAYER_WEAPON_DATA,PositionOffset)==offsetof(PLAYER_WEAPON_DATA,PositionOffset))?1:-1];
typedef char InventoryGrenadeWireSize[(sizeof(LEGACY_GRENADE_LAUNCHER_DATA)==sizeof(GRENADE_LAUNCHER_DATA))?1:-1];
static void InventorySaveGrenades(LEGACY_GRENADE_LAUNCHER_DATA *out,const GRENADE_LAUNCHER_DATA *in)
{
 memcpy(out,in,sizeof(*out));
 #define WIRE_GRENADE(field) out->field=(unsigned char)(in->field>255?255:in->field)
 WIRE_GRENADE(StandardMagazinesRemaining);WIRE_GRENADE(FlareMagazinesRemaining);
 WIRE_GRENADE(ProximityMagazinesRemaining);WIRE_GRENADE(FragmentationMagazinesRemaining);
 #undef WIRE_GRENADE
}
static void InventoryLoadGrenades(GRENADE_LAUNCHER_DATA *out,const LEGACY_GRENADE_LAUNCHER_DATA *in)
{
 memcpy(out,in,sizeof(*out));
 out->StandardMagazinesRemaining=in->StandardMagazinesRemaining;
 out->FlareMagazinesRemaining=in->FlareMagazinesRemaining;
 out->ProximityMagazinesRemaining=in->ProximityMagazinesRemaining;
 out->FragmentationMagazinesRemaining=in->FragmentationMagazinesRemaining;
}
typedef struct {
 int remainder[ACID_PICKUP_CHANNELS];
 unsigned short magazines[MAX_NO_OF_WEAPON_SLOTS][2];
 unsigned short grenades[4];
} ACID_INVENTORY_PAYLOAD;
static void InventoryExportAcid(ACID_INVENTORY_PAYLOAD *out,const PLAYER_STATUS *ps)
{
 int i;memset(out,0,sizeof(*out));memcpy(out->remainder,AcidPickupRemainders,sizeof out->remainder);
 for(i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  out->magazines[i][0]=ps->WeaponSlot[i].PrimaryMagazinesRemaining;
  out->magazines[i][1]=ps->WeaponSlot[i].SecondaryMagazinesRemaining;
 }
 out->grenades[0]=GrenadeLauncherData.StandardMagazinesRemaining;
 out->grenades[1]=GrenadeLauncherData.FlareMagazinesRemaining;
 out->grenades[2]=GrenadeLauncherData.ProximityMagazinesRemaining;
 out->grenades[3]=GrenadeLauncherData.FragmentationMagazinesRemaining;
}
static void InventoryImportAcid(const ACID_INVENTORY_PAYLOAD *in,PLAYER_STATUS *ps)
{
 int i,cap=AcidResourceCapacity(99);
 for(i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  ps->WeaponSlot[i].PrimaryMagazinesRemaining=in->magazines[i][0]>cap?cap:in->magazines[i][0];
  ps->WeaponSlot[i].SecondaryMagazinesRemaining=in->magazines[i][1]>cap?cap:in->magazines[i][1];
 }
 #define READ_GRENADE(field,index) GrenadeLauncherData.field=in->grenades[index]>cap?cap:in->grenades[index]
 READ_GRENADE(StandardMagazinesRemaining,0);READ_GRENADE(FlareMagazinesRemaining,1);
 READ_GRENADE(ProximityMagazinesRemaining,2);READ_GRENADE(FragmentationMagazinesRemaining,3);
 #undef READ_GRENADE
}
#endif
