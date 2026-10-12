#include "event_latency.h"
#include "optimized_ammo.h"
#include "molecular_acid.h"
/* Maintains the equipmnt the player has, 
rounds fired etc etc etc*/

#include "3dc.h"
#include "inline.h"
#include "module.h"

#include "stratdef.h"
#include "gamedef.h" 
#include "gameplat.h"	
#include "bh_types.h"
#include "bh_weap.h"
#include "equipmnt.h"
#include "dynblock.h"
#include "pvisible.h"
#include "language.h"
#include "huddefs.h"
#include "psnd.h"
#include "weapons.h"
#include "inventry.h"
#include "los.h"
#include "npc_precision.h"
#include "bh_marin.h"
#include "npc_support.h"
#include "targeting.h"
extern int NormalFrameTime;

/* for win95 net game support */
#include "pldnet.h"
#include "pldghost.h"

#include "avp_userprofile.h"

#define UseLocalAssert Yes
#include "ourasert.h"

static PLAYER_STATUS *InventoryRecipient;
static STRATEGYBLOCK *InventoryNPC;
static void RemovePickedUpObjectForActor(STRATEGYBLOCK*,STRATEGYBLOCK*);
static STRATEGYBLOCK *InventoryActor(void){return InventoryNPC?InventoryNPC:Player->ObStrategyBlock;}
static void InventoryMessage(const char *text){if(!InventoryNPC)NewOnScreenMessage(text);}
void InitialisePlayersInventory(PLAYER_STATUS *playerStatusPtr);
void MaintainPlayersInventory(void);
void SetPlayerSecurityClearance(STRATEGYBLOCK *sbPtr, unsigned int securityLevel);

static int AbleToPickupAmmo(enum AMMO_ID ammoID);
static int AbleToPickupWeapon(enum WEAPON_ID weaponID);
static int AbleToPickupHealth(int healthID);
static int AbleToPickupArmour(int armourID);
static int AbleToPickupMTrackerUpgrade(int mtrackerID);
void RemovePickedUpObject(STRATEGYBLOCK *objectPtr);
static int AbleToPickupFieldCharge(int chargeID);
int AutoWeaponChangeOn = TRUE;

PLAYER_STARTING_EQUIPMENT StartingEquipment;

int RecallDisc_Charge=400000;

#define MEDICOMP_MAX_AMMO	(ONE_FIXED*4)

#define SPECIALIST_PISTOLS	(netGameData.specialistPistols)

static void TryPlayerPickup(STRATEGYBLOCK *collidedWith)
{
			INANIMATEOBJECT_STATUSBLOCK* objStatPtr = collidedWith->SBdataptr;
			
			/*Make sure the object hasn't already been picked up this frame*/
			if(collidedWith->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0)
			{
				/* now test for inanimate objects we can pick up... */
                EVENT_TICK pickupSpan=EventLatencyBegin();
                EVENT_TICK pickupId=EventLatencyId(collidedWith->SBname);
                int pickupType=objStatPtr->typeId,pickupSubtype=objStatPtr->subType,pickupAccepted=0;
				switch(objStatPtr->typeId)
				{
					case(IOT_Weapon):
					{
						if (AbleToPickupWeapon(objStatPtr->subType)) {
							{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }	
						 	/*Message now done in able to pickup function*/
						 //	InventoryMessage(GetTextString(TemplateWeapon[objStatPtr->subType].Name));
						}
						break;
					}
					case(IOT_Ammo):
					{
						if (AbleToPickupAmmo(objStatPtr->subType)) {
							{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }
							InventoryMessage(GetTextString(TemplateAmmo[objStatPtr->subType].ShortName));
						}
						break;
					}
					case(IOT_Health):
					{
						if (AbleToPickupHealth(objStatPtr->subType)) {
							{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }
							InventoryMessage(GetTextString(TEXTSTRING_INGAME_MEDIKIT));
						}
						break;
					}
					case(IOT_Armour):
					{
						if (AbleToPickupArmour(objStatPtr->subType)) {
							{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }
							InventoryMessage(GetTextString(TEXTSTRING_INGAME_ARMOUR));
						}
						break;
					}
					case(IOT_Key):
					{
//						SetPlayerSecurityClearance(InventoryActor(),objStatPtr->subType);						
						{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }
						break;
					}
					case(IOT_BoxedSentryGun):
					{
						{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }						
						break;
					}
					case(IOT_IRGoggles):
					{
						{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }						
						break;
					}
					case(IOT_DataTape):
					{
						{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }						
						break;
					}
      				case(IOT_MTrackerUpgrade):
					{
						if (AbleToPickupMTrackerUpgrade(objStatPtr->subType))				
							{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }						
						break;
					}
      				case(IOT_PheromonePod):
					{
					 	((PLAYER_STATUS *)(InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr))->MTrackerType++;	
						{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }						
					}
					case IOT_SpecialPickupObject:
					{
						{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }						
						
						break;
					}
					case IOT_FieldCharge:
					{
						if (AbleToPickupFieldCharge(objStatPtr->subType)) {
							{ pickupAccepted=1; RemovePickedUpObject(collidedWith); }
						}
						break;
					}

					default: ;
				}
                EventLatencyEnd(pickupSpan,"pickup handling",pickupType,pickupSubtype,pickupId,0,pickupAccepted);
			}
}
static void MaintainPlayersInventoryMeasuredBody(void)
{
	DYNAMICSBLOCK *dynPtr = InventoryActor()->DynPtr;
	struct collisionreport *nextReport;

	LOCALASSERT(dynPtr);
	nextReport = dynPtr->CollisionReportPtr;
	
	/* walk the collision report list, looking for collisions against inanimate objects */
	while(nextReport)
	{
		STRATEGYBLOCK* collidedWith = nextReport->ObstacleSBPtr;

		/* check collison report for valid object */
		if((collidedWith) && (collidedWith->I_SBtype == I_BehaviourInanimateObject))
		{
            TryPlayerPickup(collidedWith);
		} else if((collidedWith) && (collidedWith->I_SBtype == I_BehaviourNetGhost)) {
			NETGHOSTDATABLOCK* ghostData = collidedWith->SBdataptr;

			if (collidedWith->SBflags.please_destroy_me==0) {
				if (ghostData->type==I_BehaviourInanimateObject) {
					if (ghostData->IOType==IOT_Ammo) {
						if (AbleToPickupAmmo(ghostData->subtype)) {
							AddNetMsg_LocalObjectDestroyed_Request(collidedWith);
							ghostData->IOType=IOT_Non;
							/* So it's not picked up again... */
							InventoryMessage(GetTextString(TemplateAmmo[ghostData->subtype].ShortName));
						}
					}
					if (ghostData->IOType==IOT_Weapon) {
						if (AbleToPickupWeapon(ghostData->subtype)) {
							AddNetMsg_LocalObjectDestroyed_Request(collidedWith);
							ghostData->IOType=IOT_Non;
							/* So it's not picked up again... */
						 	/*Message now done in able to pickup function*/
						   //	InventoryMessage(GetTextString(TemplateWeapon[ghostData->subtype].Name));
						}
					}
				}
			}
		}

		nextReport = nextReport->NextCollisionReportPtr;
	}

}


/* Reaching a shelf must not require moving the torso collision sphere upwards.
 * Retain native collision pickups; extend weapons/ammo only within arm reach,
 * with an eye-to-item ray so walls and the underside of shelves still block. */
static void MaintainReachPickups(void)
{
 extern STRATEGYBLOCK *ActiveStBlockList[];extern int NumActiveStBlocks;
 DYNAMICSBLOCK *d=InventoryActor()->DynPtr;int i;
 if(AvP.PlayerType!=I_Marine || AvP.Network!=I_No_Network)return;
 for(i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK *sb=ActiveStBlockList[i];INANIMATEOBJECT_STATUSBLOCK *o;
  VECTORCH eye,direction,oldPoint,oldNormal;int distance,oldLambda,visible;
  DISPLAYBLOCK *oldHit;SECTION_DATA *oldSection;
  if(!sb || !sb->DynPtr || !sb->SBdptr || sb->I_SBtype!=I_BehaviourInanimateObject || sb->SBflags.please_destroy_me)continue;
  o=(INANIMATEOBJECT_STATUSBLOCK*)sb->SBdataptr;
  if(o->respawnTimer || (o->typeId!=IOT_Weapon && o->typeId!=IOT_Ammo))continue;
  if(!NPCPickupWithinReach((double)sb->DynPtr->Position.vx-d->Position.vx,
       (double)sb->DynPtr->Position.vy-d->Position.vy,(double)sb->DynPtr->Position.vz-d->Position.vz,
       Player->ObMinY,Player->ObMaxY))continue;
  eye=d->Position;eye.vy+=Player->ObMinY+200;
  direction.vx=sb->DynPtr->Position.vx-eye.vx;direction.vy=sb->DynPtr->Position.vy-eye.vy;direction.vz=sb->DynPtr->Position.vz-eye.vz;
  distance=Magnitude(&direction);if(!distance){TryPlayerPickup(sb);continue;}
  Normalise(&direction);
  oldPoint=LOS_Point;oldNormal=LOS_ObjectNormal;oldHit=LOS_ObjectHitPtr;oldSection=LOS_HModel_Section;oldLambda=LOS_Lambda;
  FindPolygonInLineOfSight_TwoIgnores(&direction,&eye,0,Player,sb->SBdptr);
  visible=LOS_Lambda>=distance;
  LOS_Point=oldPoint;LOS_ObjectNormal=oldNormal;LOS_ObjectHitPtr=oldHit;LOS_HModel_Section=oldSection;LOS_Lambda=oldLambda;
  if(visible)TryPlayerPickup(sb);
 }
}
void MaintainPlayersInventory(void)
{
 EVENT_TICK span=EventLatencyBegin();MaintainPlayersInventoryMeasuredBody();MaintainReachPickups();
 EventLatencyEnd(span,"player inventory",0,0,0,0,0);
}

void LoadAllWeapons(PLAYER_STATUS *playerStatusPtr) {

   	int slot = MAX_NO_OF_WEAPON_SLOTS;

	/* Sorry, doesn't do pulse grenades yet. */

	do {
		PLAYER_WEAPON_DATA *wdPtr;
		TEMPLATE_WEAPON_DATA *twPtr;
		TEMPLATE_AMMO_DATA *templateAmmoPtr;

		wdPtr = &playerStatusPtr->WeaponSlot[--slot];
		if(wdPtr->WeaponIDNumber<0 || wdPtr->WeaponIDNumber>=MAX_NO_OF_WEAPON_TEMPLATES)continue;
	    twPtr = &TemplateWeapon[wdPtr->WeaponIDNumber];
		templateAmmoPtr = &TemplateAmmo[twPtr->PrimaryAmmoID];

		if ((wdPtr->PrimaryRoundsRemaining==0)
			&&(wdPtr->PrimaryMagazinesRemaining>0)
			&&(twPtr->PrimaryAmmoID!=AMMO_NONE)) {
			ReloadWeaponMagazine(wdPtr, 0);
		}

		if ((wdPtr->SecondaryRoundsRemaining==0)
			&&(wdPtr->SecondaryMagazinesRemaining>0)
			&&(twPtr->SecondaryAmmoID!=AMMO_NONE)) {
			TEMPLATE_AMMO_DATA *secondaryTemplateAmmoPtr;

			secondaryTemplateAmmoPtr = &TemplateAmmo[twPtr->SecondaryAmmoID];
			
			ReloadWeaponMagazine(wdPtr, 1);
		}

    } while(slot);

}
	
/* Fresh level only; saved/campaign inventory is restored afterwards unchanged.
 * Magazine-fed weapons get larger magazines (LoadAllWeapons), not more of them.
 * Fixed-capacity weapons retain the scaled initial reserve from probe 209. */
static void AcidInitialInventory(PLAYER_STATUS *ps)
{
 OptimizedAmmoReset();
 if(!AcidActive())return;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  PLAYER_WEAPON_DATA *w=&ps->WeaponSlot[i];
  if(AvP.PlayerType==I_Marine){
   if(w->WeaponIDNumber==WEAPON_PULSERIFLE || w->WeaponIDNumber==WEAPON_SADAR || w->WeaponIDNumber==WEAPON_FRISBEE_LAUNCHER)
    w->PrimaryMagazinesRemaining=AcidResourceCapacity(w->PrimaryMagazinesRemaining);
   if(w->WeaponIDNumber==WEAPON_PULSERIFLE)
    w->SecondaryRoundsRemaining=AcidResourceCapacity(w->SecondaryRoundsRemaining/ONE_FIXED)*ONE_FIXED;
  }else if(AvP.PlayerType==I_Predator && w->WeaponIDNumber==WEAPON_PRED_RIFLE){
   w->PrimaryRoundsRemaining=AcidResourceCapacity(w->PrimaryRoundsRemaining/ONE_FIXED)*ONE_FIXED;
  }
 }
}

void InitialisePlayersInventory(PLAYER_STATUS *playerStatusPtr)
{
	PLAYER_WEAPON_DATA *weaponDataPtr = &playerStatusPtr->WeaponSlot[WEAPON_SLOT_1];

	enum WEAPON_ID *PlayerWeaponKey;
    
	switch(AvP.PlayerType) {
		case I_Marine:
			PlayerWeaponKey=&MarineWeaponKey[0];
			break;
		case I_Alien:
			PlayerWeaponKey=&AlienWeaponKey[0];
			break;
		case I_Predator:
			PlayerWeaponKey=&PredatorWeaponKey[0];
			break;
	}
	

    {
    	int slot = MAX_NO_OF_WEAPON_SLOTS;
        do
        {
    		PLAYER_WEAPON_DATA *wdPtr = &playerStatusPtr->WeaponSlot[--slot];

   			//wdPtr->WeaponIDNumber = NULL_WEAPON;
   			wdPtr->WeaponIDNumber = PlayerWeaponKey[slot];

 		   	wdPtr->PrimaryRoundsRemaining=0;
            wdPtr->PrimaryMagazinesRemaining=0;
 		   	wdPtr->SecondaryRoundsRemaining=0;
            wdPtr->SecondaryMagazinesRemaining=0;
 			wdPtr->CurrentState = WEAPONSTATE_IDLE;
            wdPtr->StateTimeOutCounter=0;
			wdPtr->PositionOffset.vx = 0;
			wdPtr->PositionOffset.vy = 0;
			wdPtr->PositionOffset.vz = 0;
			wdPtr->DirectionOffset.EulerX = 0;
			wdPtr->DirectionOffset.EulerY = 0;
			wdPtr->DirectionOffset.EulerZ = 0;
			wdPtr->Possessed=0;
			InitThisWeapon(wdPtr); // To set the null pointers.
        }
		while(slot);
	}
	
	GrenadeLauncherData.StandardRoundsRemaining=0;	
	GrenadeLauncherData.StandardMagazinesRemaining=0;
	GrenadeLauncherData.FlareRoundsRemaining=0;
	GrenadeLauncherData.FlareMagazinesRemaining=0;
	GrenadeLauncherData.ProximityRoundsRemaining=0;
	GrenadeLauncherData.ProximityMagazinesRemaining=0;
	GrenadeLauncherData.FragmentationRoundsRemaining=0;
	GrenadeLauncherData.FragmentationMagazinesRemaining=0;
	GrenadeLauncherData.SelectedAmmo = AMMO_GRENADE;
														  	
	/* 'swap' to weapon at beginning of game */
    playerStatusPtr->SelectedWeaponSlot = WEAPON_SLOT_1;
	playerStatusPtr->PreviouslySelectedWeaponSlot = WEAPON_SLOT_1;
    playerStatusPtr->SwapToWeaponSlot = WEAPON_SLOT_1;
	weaponDataPtr->CurrentState = WEAPONSTATE_SWAPPING_IN;
	weaponDataPtr->StateTimeOutCounter = 0;

	/* Set to initial motion tracker type */

	playerStatusPtr->MTrackerType = 0;

	/* Initially not facehugged */

	playerStatusPtr->MyFaceHugger=NULL;

	/* Nor dead. */

	playerStatusPtr->MyCorpse=NULL;

	ThisDiscMode=I_Seek_Track;
	SmartgunMode=I_Track;

	/* switch on player type */
	switch(AvP.PlayerType)
	{
		int a;

		case I_Marine:
		{
			/*if in a multiplayer game , check to see if player is a specialist marine*/
			if(AvP.Network != I_No_Network)
			{
				switch(netGameData.myCharacterSubType)
				{

					case NGSCT_Smartgun :
						if (SPECIALIST_PISTOLS) {
							/* Conditional pistol! */
							a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
		            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
							}
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=0;
							}
						} else {
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
							}
						}
						a=SlotForThisWeapon(WEAPON_SMARTGUN);
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						break;

					case NGSCT_Flamer :
						if (SPECIALIST_PISTOLS) {
							/* Conditional pistol! */
							a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
		            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
							}
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=0;
							}
						} else {
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
							}
						}
						a=SlotForThisWeapon(WEAPON_FLAMETHROWER);
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						break;

					case NGSCT_Sadar :
						if (SPECIALIST_PISTOLS) {
							/* Conditional pistol! */
							a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
		            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
							}
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=0;
							}
						} else {
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
							}
						}
						a=SlotForThisWeapon(WEAPON_SADAR);
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						break;
	
					case NGSCT_GrenadeLauncher :
						if (SPECIALIST_PISTOLS) {
							/* Conditional pistol! */
							a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
		            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
							}
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=0;
							}
						} else {
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
							}
						}
						a=SlotForThisWeapon(WEAPON_GRENADELAUNCHER);
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=4;
						GrenadeLauncherData.StandardMagazinesRemaining=4;
						GrenadeLauncherData.FlareMagazinesRemaining=4;
						GrenadeLauncherData.ProximityMagazinesRemaining=4;
						GrenadeLauncherData.FragmentationMagazinesRemaining=4;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						break;

					case NGSCT_Minigun :
						if (SPECIALIST_PISTOLS) {
							/* Conditional pistol! */
							a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
		            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
							}
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=0;
							}
						} else {
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
							}
						}
						a=SlotForThisWeapon(WEAPON_MINIGUN);
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						break;
					
					case NGSCT_PulseRifle :
						if (SPECIALIST_PISTOLS) {
							/* Conditional pistol! */
							a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
		            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
							}
						}
						/* Conditional cudgel! */
						a=SlotForThisWeapon(WEAPON_CUDGEL);
						if (a!=-1) {
	            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						}
						a=SlotForThisWeapon(WEAPON_PULSERIFLE);
            			//pulse rifle marines get more ammo
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
            			playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=(ONE_FIXED*15);
            			playerStatusPtr->WeaponSlot[a].SecondaryMagazinesRemaining=0;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;

						break;
					case NGSCT_Frisbee :
						if (SPECIALIST_PISTOLS) {
							/* Conditional pistol! */
							a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
		            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=10;
							}
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=0;
							}
						} else {
							a=SlotForThisWeapon(WEAPON_CUDGEL);
							if (a!=-1) {
		            			playerStatusPtr->WeaponSlot[a].Possessed=1;
							}
						}
						a=SlotForThisWeapon(WEAPON_FRISBEE_LAUNCHER);
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						break;
					case NGSCT_Pistols :
						a=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
						if (a!=-1) {
	            			playerStatusPtr->WeaponSlot[a].Possessed=1;
	            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=50;
						}
						a=SlotForThisWeapon(WEAPON_TWO_PISTOLS);
						if (a!=-1) {
	            			playerStatusPtr->WeaponSlot[a].Possessed=1;
	            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=25;
	            			playerStatusPtr->WeaponSlot[a].SecondaryMagazinesRemaining=25;
						}
						break;
					case NGSCT_General :
					default :
						/* Conditional cudgel! */
						a=SlotForThisWeapon(WEAPON_CUDGEL);
						if (a!=-1) {
	            			playerStatusPtr->WeaponSlot[a].Possessed=1;
						}
						a=SlotForThisWeapon(WEAPON_PULSERIFLE);
            			playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
            			playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=(ONE_FIXED*5);
            			playerStatusPtr->WeaponSlot[a].SecondaryMagazinesRemaining=0;
            			playerStatusPtr->WeaponSlot[a].Possessed=1;

						break;
									
				}
    			
    			//make sure the player starts with his appropriate weapon selected
//    			playerStatusPtr->SelectedWeaponSlot = a;
				playerStatusPtr->PreviouslySelectedWeaponSlot = a;
    			playerStatusPtr->SwapToWeaponSlot = a;
				
			}
			else
			{
				a=SlotForThisWeapon(WEAPON_PULSERIFLE);
				if (GRENADE_MODE) {
	            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
	            	playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=(ONE_FIXED*99);
    	        	playerStatusPtr->WeaponSlot[a].SecondaryMagazinesRemaining=0;
				} else {
	            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
	            	playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=(ONE_FIXED*5);
    	        	playerStatusPtr->WeaponSlot[a].SecondaryMagazinesRemaining=0;
				}
            	playerStatusPtr->WeaponSlot[a].Possessed=1;
				/* Conditional cudgel! */
				a=SlotForThisWeapon(WEAPON_CUDGEL);
				if (a!=-1) {
           			playerStatusPtr->WeaponSlot[a].Possessed=1;
				}
			}
			#if 0
			/* Keep smartgun and flamer... for demo, lose SADAR, grenadelauncher and minigun. */
			a=SlotForThisWeapon(WEAPON_SADAR);
            playerStatusPtr->WeaponSlot[a].Possessed=-1;
			/* KJL 15:47:30 26/11/98 - grenade launched back in for multiplayer demo */
//			a=SlotForThisWeapon(WEAPON_GRENADELAUNCHER);
//          playerStatusPtr->WeaponSlot[a].Possessed=-1;
			a=SlotForThisWeapon(WEAPON_MINIGUN);
            playerStatusPtr->WeaponSlot[a].Possessed=-1;
			#endif

			a=SlotForThisWeapon(WEAPON_BEAMCANNON);
            playerStatusPtr->WeaponSlot[a].Possessed=0;

    		break;	
    	}
       	case I_Predator:
		{
			a=SlotForThisWeapon(WEAPON_PRED_WRISTBLADE);
			playerStatusPtr->WeaponSlot[a].Possessed=1;
			
			if(StartingEquipment.predator_pistol)
			{
				a=SlotForThisWeapon(WEAPON_PRED_PISTOL);
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=ONE_FIXED;
            	playerStatusPtr->WeaponSlot[a].SecondaryRoundsRemaining=ONE_FIXED;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if(StartingEquipment.predator_num_spears)
			{
				a=SlotForThisWeapon(WEAPON_PRED_RIFLE);
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=(ONE_FIXED*StartingEquipment.predator_num_spears);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if(StartingEquipment.predator_plasmacaster)
			{
				a=SlotForThisWeapon(WEAPON_PRED_SHOULDERCANNON);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=5;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if(StartingEquipment.predator_disc)
			{
				a=SlotForThisWeapon(WEAPON_PRED_DISC);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=1;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			if(StartingEquipment.predator_medicomp)
			{
				a=SlotForThisWeapon(WEAPON_PRED_MEDICOMP);
            	playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
            	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=MEDICOMP_MAX_AMMO;
				playerStatusPtr->WeaponSlot[a].Possessed=1;
			}
			a=SlotForThisWeapon(WEAPON_PRED_STAFF);
            playerStatusPtr->WeaponSlot[a].PrimaryMagazinesRemaining=0;
			playerStatusPtr->WeaponSlot[a].Possessed=0;

			break;
		}
		case I_Alien:
		{
			a=SlotForThisWeapon(WEAPON_ALIEN_CLAW);
			playerStatusPtr->WeaponSlot[a].Possessed=1;
			a=SlotForThisWeapon(WEAPON_ALIEN_GRAB);
			playerStatusPtr->WeaponSlot[a].Possessed=0;
			//a=SlotForThisWeapon(WEAPON_ALIEN_SPIT);
            //playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining=100*65536;
			//playerStatusPtr->WeaponSlot[a].Possessed=1;
			break;
		}
		default:
			LOCALASSERT(1==0);
			break;							
        
    }
    
	/*check jetpack and grappling hook*/
    playerStatusPtr->JetpackEnabled = StartingEquipment.marine_jetpack;
    playerStatusPtr->GrapplingHookEnabled = StartingEquipment.predator_grappling_hook;	

	AcidInitialInventory(playerStatusPtr);
	LoadAllWeapons(playerStatusPtr);
}


/* fn returns zero if unable to add ammo to inventory,
 eg, of the wrong type */

static int AbleToPickupAmmoBase(enum AMMO_ID ammoID);
/* Apply deterministic pickup totals in the base path. This wrapper keeps
 * rejected pickups atomic and clamps the alternate integer reserves. */
static int AbleToPickupAmmo(enum AMMO_ID ammoID)
{
 if(!AcidActive()||ammoID==AMMO_PRED_DISC)return AbleToPickupAmmoBase(ammoID);
 PLAYER_STATUS *ps=(PLAYER_STATUS*)(InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
 PLAYER_WEAPON_DATA before[MAX_NO_OF_WEAPON_SLOTS];
 GRENADE_LAUNCHER_DATA grenades=GrenadeLauncherData;
 memcpy(before,ps->WeaponSlot,sizeof before);
 int accepted=AbleToPickupAmmoBase(ammoID);
 if(!accepted){memcpy(ps->WeaponSlot,before,sizeof before);GrenadeLauncherData=grenades;return 0;}
 /* Keep integer reserves bounded, including alternate grenade/pistol stores.
  * Fractional pickup fields remain in saves for compatibility, but are unused. */
 int cap=AcidResourceCapacity(99);
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  PLAYER_WEAPON_DATA *w=&ps->WeaponSlot[i];
  if(w->PrimaryMagazinesRemaining>cap)w->PrimaryMagazinesRemaining=cap;
  if(w->SecondaryMagazinesRemaining>cap)w->SecondaryMagazinesRemaining=cap;
 }
 #define ACID_GRENADE(member) if(GrenadeLauncherData.member>cap)GrenadeLauncherData.member=cap
 ACID_GRENADE(StandardMagazinesRemaining);
 ACID_GRENADE(FlareMagazinesRemaining);
 ACID_GRENADE(ProximityMagazinesRemaining);
 ACID_GRENADE(FragmentationMagazinesRemaining);
 #undef ACID_GRENADE
 return 1;
}

static int AbleToPickupAmmoBase(enum AMMO_ID ammoID)
{
	int weaponSlot = -1;

	LOCALASSERT(ammoID>=0);
	LOCALASSERT(ammoID<MAX_NO_OF_AMMO_TEMPLATES);

	switch(AvP.PlayerType)
	{
		case I_Marine:
       	{
			
			if(AvP.Network != I_No_Network)
			{
				/*if in a multiplayer game , check to see if player is a specialist marine*/
				switch(netGameData.myCharacterSubType)
				{
					case NGSCT_PulseRifle :
						if (ammoID!=AMMO_10MM_CULW && 
						   ammoID!=AMMO_PULSE_GRENADE
						   && (ammoID!=AMMO_MARINE_PISTOL_PC)
						   ) {
						   	return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(ammoID==AMMO_MARINE_PISTOL_PC)) {
							return 0;
						}

						break;

					case NGSCT_Smartgun :
						if (ammoID!=AMMO_SMARTGUN
						   && (ammoID!=AMMO_MARINE_PISTOL_PC)
							) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(ammoID==AMMO_MARINE_PISTOL_PC)) {
							return 0;
						}
						break;

					case NGSCT_Flamer :
						if (ammoID!=AMMO_FLAMETHROWER
						   && (ammoID!=AMMO_MARINE_PISTOL_PC)
							) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(ammoID==AMMO_MARINE_PISTOL_PC)) {
							return 0;
						}
						break;

					case NGSCT_Sadar :
						if (ammoID!=AMMO_SADAR_TOW
						   && (ammoID!=AMMO_MARINE_PISTOL_PC)
							) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(ammoID==AMMO_MARINE_PISTOL_PC)) {
							return 0;
						}
						break;
	
					case NGSCT_GrenadeLauncher :
						if (ammoID!=AMMO_GRENADE
						   && (ammoID!=AMMO_MARINE_PISTOL_PC)
							) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(ammoID==AMMO_MARINE_PISTOL_PC)) {
							return 0;
						}
						break;

					case NGSCT_Minigun :
						if (ammoID!=AMMO_MINIGUN
						   && (ammoID!=AMMO_MARINE_PISTOL_PC)
							) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(ammoID==AMMO_MARINE_PISTOL_PC)) {
							return 0;
						}
						break;

					case NGSCT_Frisbee :
						if (ammoID!=AMMO_FRISBEE
						   && (ammoID!=AMMO_MARINE_PISTOL_PC)
							) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(ammoID==AMMO_MARINE_PISTOL_PC)) {
							return 0;
						}
						break;

					case NGSCT_Pistols :
						if (ammoID!=AMMO_MARINE_PISTOL_PC) {
							return 0;
						}
						break;
					
					case NGSCT_General :
						break; //do nothing
					default :
						break;
				}
			}
			
			if (GRENADE_MODE) {
				if (ammoID!=AMMO_PULSE_GRENADE) {
					return(0);
				}
			}
			
			switch(ammoID)
			{
				case AMMO_10MM_CULW:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_PULSERIFLE);
					break;
				}
				case AMMO_SHOTGUN:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_AUTOSHOTGUN);
					break;
				}
				case AMMO_SMARTGUN:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_SMARTGUN);
					break;
				}
				case AMMO_FLAMETHROWER:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_FLAMETHROWER);
					break;
				}
				case AMMO_PLASMA:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_PLASMAGUN);
					break;
				}
				case AMMO_SADAR_TOW:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_SADAR);
					break;
				}
				case AMMO_FRISBEE:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_FRISBEE_LAUNCHER);
					break;
				}
				case AMMO_MARINE_PISTOL_PC:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
					break;
				}
				case AMMO_GRENADE: 
				{
					weaponSlot = SlotForThisWeapon(WEAPON_GRENADELAUNCHER);
					GrenadeLauncherData.StandardMagazinesRemaining+=1;
					GrenadeLauncherData.FlareMagazinesRemaining+=1;
					GrenadeLauncherData.ProximityMagazinesRemaining+=1;
					GrenadeLauncherData.FragmentationMagazinesRemaining+=1;
					break;
				}
				case AMMO_MINIGUN:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_MINIGUN);
					break;
				}
				case AMMO_PULSE_GRENADE:
				{
					/* Quick fix... */
					weaponSlot = SlotForThisWeapon(WEAPON_PULSERIFLE);
					{
						PLAYER_WEAPON_DATA *weaponPtr;
						    
					    /* access the extra data hanging off the strategy block */
						PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
					    GLOBALASSERT(playerStatusPtr);
	    	
				    	/* init a pointer to the weapon's data */
				    	weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);
						
						#if 0
						if(weaponPtr->SecondaryMagazinesRemaining == 99)
						{
							/* no room! */
							return 0;
						}
						else
						{
							weaponPtr->SecondaryMagazinesRemaining+=1; /* KJL 12:54:37 03/10/97 - need extra data field in ammo templates */
						
							if(weaponPtr->SecondaryMagazinesRemaining > 99)
								weaponPtr->SecondaryMagazinesRemaining = 99;
						}
						#else
						if(weaponPtr->SecondaryRoundsRemaining >= AcidResourceCapacity(99)*ONE_FIXED)
						{
							/* no room! */
							return 0;
						}

						weaponPtr->SecondaryRoundsRemaining+=AcidResourceCapacity(5)*ONE_FIXED;
						if (weaponPtr->SecondaryRoundsRemaining>(AcidResourceCapacity(99)*ONE_FIXED)) {
							weaponPtr->SecondaryRoundsRemaining=(AcidResourceCapacity(99)*ONE_FIXED);
						}
						#endif
					}		
					/* successful */
					return 1;
					break;
				}
				default:
					break;
			}
			
			break;
		}
       	case I_Predator:
		{
			switch(ammoID)
			{
				case AMMO_PRED_RIFLE:
				{
					weaponSlot = SlotForThisWeapon(WEAPON_PRED_RIFLE);
					break;
				}
				case AMMO_PRED_DISC:
				{
					PLAYER_WEAPON_DATA *weaponPtr;
					PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);

				    GLOBALASSERT(playerStatusPtr);
					weaponSlot = SlotForThisWeapon(WEAPON_PRED_DISC);

				    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);

					if ((weaponPtr->PrimaryMagazinesRemaining!=0)
						||(weaponPtr->PrimaryRoundsRemaining!=0)) {
						/* You have a disc: you can't have another! */
						return(0);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case I_Alien:
		{
			break;
		}

		default:
			LOCALASSERT(1==0);
			break;
	}

	/* if unable to find the correct weapon slot for the ammo */
	if (weaponSlot == -1) return 0;
	
	{
		PLAYER_WEAPON_DATA *weaponPtr;
	    
	    /* access the extra data hanging off the strategy block */
		PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	    GLOBALASSERT(playerStatusPtr);
	    	
	    /* init a pointer to the weapon's data */
	    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);
		
		if(weaponPtr->PrimaryMagazinesRemaining >= AcidResourceCapacity(99))
		{
			/* no room! */
			return 0;
		}
		else
		{
			if (ammoID==AMMO_SADAR_TOW) {
				weaponPtr->PrimaryMagazinesRemaining+=AcidMagazinePickup(ammoID,4);
			} else if (ammoID==AMMO_MARINE_PISTOL_PC) {
				if ((AvP.Network != I_No_Network)&&(PISTOL_INFINITE_AMMO)) {
					weaponPtr->PrimaryMagazinesRemaining=10;
					/* If you have Two Pistols, load that, too. */
					{
						PLAYER_WEAPON_DATA *twoPistolPtr;
						int twoPistolSlot;

						twoPistolSlot=SlotForThisWeapon(WEAPON_TWO_PISTOLS);
						if (twoPistolSlot!=-1) {
							twoPistolPtr=&(playerStatusPtr->WeaponSlot[twoPistolSlot]);
							if (twoPistolPtr) {
								twoPistolPtr->PrimaryMagazinesRemaining=5;
								twoPistolPtr->SecondaryMagazinesRemaining=5;
							}
						}
					}
				} else {
					weaponPtr->PrimaryMagazinesRemaining+=10;
					/* If you have Two Pistols, load that, too. */
					{
						PLAYER_WEAPON_DATA *twoPistolPtr;
						int twoPistolSlot;

						twoPistolSlot=SlotForThisWeapon(WEAPON_TWO_PISTOLS);
						if (twoPistolSlot!=-1) {
							twoPistolPtr=&(playerStatusPtr->WeaponSlot[twoPistolSlot]);
							if (twoPistolPtr) {
								twoPistolPtr->PrimaryMagazinesRemaining+=5;
								twoPistolPtr->SecondaryMagazinesRemaining+=5;
							}
						}
					}
				}
			} else if (ammoID==AMMO_10MM_CULW) {
				weaponPtr->PrimaryMagazinesRemaining+=AcidMagazinePickup(ammoID,5);
			} else if (ammoID==AMMO_PRED_RIFLE) {
				/* Add to Rounds. */
				weaponPtr->PrimaryRoundsRemaining+=ONE_FIXED*AcidResourceCapacity(SPEARS_PER_PICKUP);
				if (weaponPtr->PrimaryRoundsRemaining>(ONE_FIXED*AcidResourceCapacity(MAX_SPEARS))) {
					weaponPtr->PrimaryRoundsRemaining=(ONE_FIXED*AcidResourceCapacity(MAX_SPEARS));
				}
			} else if (ammoID==AMMO_PRED_DISC) {
				/* Disc case... */
				if ((weaponPtr->PrimaryMagazinesRemaining==0)
					&& (weaponPtr->PrimaryRoundsRemaining==0)) {
					weaponPtr->PrimaryRoundsRemaining+=ONE_FIXED;
					/* Autoswap to disc here? */
					AutoSwapToDisc();
				} else {
					weaponPtr->PrimaryMagazinesRemaining+=1;
				}
			} else {
				weaponPtr->PrimaryMagazinesRemaining+=AcidMagazinePickup(ammoID,1); /* KJL 12:54:37 03/10/97 - need extra data field in ammo templates */
			}

			if(weaponPtr->PrimaryMagazinesRemaining > AcidResourceCapacity(99))
				weaponPtr->PrimaryMagazinesRemaining = AcidResourceCapacity(99);
		}
	}

	/* successful */
	return 1;

}


/* fn returns zero if unable to add weapon to inventory,
 eg, of the wrong type */

static int AbleToPickupWeapon(enum WEAPON_ID weaponID)
{
	BOOL WillSwitchToThisWeapon = FALSE;
	int weaponSlot = -1;
    /* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
    GLOBALASSERT(playerStatusPtr);

	LOCALASSERT(weaponID>=0);
	LOCALASSERT(weaponID<MAX_NO_OF_WEAPON_TEMPLATES);

	
	switch(AvP.PlayerType)
	{
		case I_Marine:
       	{
			
			if(AvP.Network != I_No_Network)
			{
				/*if in a multiplayer game , check to see if player is a specialist marine*/
				switch(netGameData.myCharacterSubType)
				{
					case NGSCT_PulseRifle :
						if ((weaponID!=WEAPON_PULSERIFLE)
							&&(weaponID!=WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(weaponID==WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						break;

					case NGSCT_Smartgun :
						if ((weaponID!=WEAPON_SMARTGUN) 
							&&(weaponID!=WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(weaponID==WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						break;

					case NGSCT_Flamer :
						if ((weaponID!=WEAPON_FLAMETHROWER) 
							&&(weaponID!=WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(weaponID==WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						break;

					case NGSCT_Sadar :
						if ((weaponID!=WEAPON_SADAR)
							&&(weaponID!=WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(weaponID==WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						break;
	
					case NGSCT_GrenadeLauncher :
						if ((weaponID!=WEAPON_GRENADELAUNCHER) 
							&&(weaponID!=WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(weaponID==WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						break;

					case NGSCT_Minigun :
						if ((weaponID!=WEAPON_MINIGUN)
							&&(weaponID!=WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(weaponID==WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						break;
					
					case NGSCT_Frisbee :
						if ((weaponID!=WEAPON_FRISBEE_LAUNCHER) 
							&&(weaponID!=WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						if ((!(SPECIALIST_PISTOLS))&&(weaponID==WEAPON_MARINE_PISTOL)) {
							return 0;
						}
						break;

					case NGSCT_Pistols :
						if (weaponID!=WEAPON_MARINE_PISTOL) {
							return 0;
						}
						break;

					case NGSCT_General :
						break; //do nothing
					default :
						break;
				}
			}
			
			if (GRENADE_MODE) {
				if(weaponID!=WEAPON_PULSERIFLE) {
					return(0);
				}
			}

			switch(weaponID)
			{
				case WEAPON_PULSERIFLE:
				case WEAPON_AUTOSHOTGUN:
				case WEAPON_SMARTGUN:
				case WEAPON_FLAMETHROWER:
				case WEAPON_PLASMAGUN:
				case WEAPON_SADAR:
				case WEAPON_GRENADELAUNCHER:
				case WEAPON_MINIGUN:
				{
					/* The marine weapons all map to the 
					  weapon slot of the same number */
					weaponSlot = SlotForThisWeapon(weaponID);
					break;
				}
				case WEAPON_MARINE_PISTOL:
				{
					int pistolSlot;

					pistolSlot=SlotForThisWeapon(WEAPON_MARINE_PISTOL);
	    			
	    			if (playerStatusPtr->WeaponSlot[pistolSlot].Possessed==1) {
						weaponSlot = SlotForThisWeapon(WEAPON_TWO_PISTOLS);
					} else {
						weaponSlot = SlotForThisWeapon(WEAPON_MARINE_PISTOL);
					}
					break;
				}
				default:
					weaponSlot = SlotForThisWeapon(weaponID);
					break;
			}
			
			break;
		}
       	case I_Predator:
		{
			weaponSlot = SlotForThisWeapon(weaponID);
			break;
		}
		case I_Alien:
		{
			weaponSlot = SlotForThisWeapon(weaponID);
			break;
		}

		default:
			LOCALASSERT(1==0);
			break;
	}

	
	
	/* if unable to find the correct weapon slot */
	if (weaponSlot == -1) return 0;
	
	{
		PLAYER_WEAPON_DATA *weaponPtr;
		TEMPLATE_WEAPON_DATA *twPtr;

	    	
	    /* init a pointer to the weapon's data */
	    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);
		
		#if 0
		/* if weapon slot isn't empty, unable to pickup weapon */
		if (weaponPtr->Possessed!=0) return 0;

		/* add weapon to inventory */
		weaponPtr->Possessed = 1;
		#else

		/* Select new weapons. */
		if (weaponPtr->Possessed==0) {
			if(AutoWeaponChangeOn)
			{
        		playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo=(weaponSlot+1);
				WillSwitchToThisWeapon = TRUE;
			}
		}

		twPtr=&TemplateWeapon[weaponID];

		if(weaponID==WEAPON_PULSERIFLE) //get some secondary ammo (grenades) as well
		{
			if (AbleToPickupAmmo(twPtr->SecondaryAmmoID)) {
				/* AbleToPickupAmmo should load some grenades... */
				#if 0
				TEMPLATE_AMMO_DATA *templateAmmoPtr;

				/* Load weapon. */
				templateAmmoPtr = &TemplateAmmo[twPtr->SecondaryAmmoID];

				if ((weaponPtr->SecondaryRoundsRemaining==0)&&(weaponPtr->SecondaryMagazinesRemaining>0)) {
					ReloadWeaponMagazine(weaponPtr, 1);
				}
				#endif
			} 
		}

		if (twPtr->PrimaryAmmoID!=AMMO_NONE) {

			if (AbleToPickupAmmo(twPtr->PrimaryAmmoID)) {
				TEMPLATE_AMMO_DATA *templateAmmoPtr;

				weaponPtr->Possessed = 1;
				/* Load weapon. */
				templateAmmoPtr = &TemplateAmmo[twPtr->PrimaryAmmoID];

				if ((weaponPtr->PrimaryRoundsRemaining==0)&&(weaponPtr->PrimaryMagazinesRemaining>0)) {
					ReloadWeaponMagazine(weaponPtr, 0);
				}
				if (weaponPtr->WeaponIDNumber==WEAPON_TWO_PISTOLS) {
					if ((weaponPtr->SecondaryRoundsRemaining==0)&&(weaponPtr->SecondaryMagazinesRemaining>0)) {
						ReloadWeaponMagazine(weaponPtr, 1);
					}
				}
				if(!WillSwitchToThisWeapon)
				{
					/*
					Only show pickup weapon message if this isn't a new weapon.
					(To avoid message duplication when the automatic weapon switching is done)
					*/
					InventoryMessage(GetTextString(TemplateWeapon[weaponID].Name));
				}
				
				return(1);
			} else {
				if (weaponPtr->Possessed==1) {
					/* Get a weapon, but no ammo */
					weaponPtr->Possessed = 1;
					if(!WillSwitchToThisWeapon)
					{
						/*
						Only show pickup weapon message if this isn't a new weapon.
						(To avoid message duplication when the automatic weapon switching is done)
						*/
						InventoryMessage(GetTextString(TemplateWeapon[weaponID].Name));
					}
					return(1);
				} else {
					return(0);
				}
			}
		
		} else {
			/* if weapon slot isn't empty, unable to pickup weapon */
			if (weaponPtr->Possessed!=0) return 0;
			/* add weapon to inventory */
			weaponPtr->Possessed = 1;
			/* No ammo to load. */
		}
		
		#endif

	}

	if(!WillSwitchToThisWeapon)
	{
		/*
		Only show pickup weapon message if this isn't a new weapon.
		(To avoid message duplication when the automatic weapon switching is done)
		*/
		InventoryMessage(GetTextString(TemplateWeapon[weaponID].Name));
	}
	/* successful */
	return 1;

}

/* KJL 15:59:48 03/10/97 - enum for health pickups required */
static int AbleToPickupHealth(int healthID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	{
		NPC_DATA *NpcData;
		NPC_TYPES PlayerType;

		switch(AvP.PlayerType) 
		{
			case(I_Marine):
			{
				switch (AvP.Difficulty) {
					case I_Easy:
						PlayerType=I_PC_Marine_Easy;
						break;
					default:
					case I_Medium:
						PlayerType=I_PC_Marine_Medium;
						break;
					case I_Hard:
						PlayerType=I_PC_Marine_Hard;
						break;
					case I_Impossible:
						PlayerType=I_PC_Marine_Impossible;
						break;
				}
				break;
			}
			case(I_Predator):
			{
				/* KJL 17:25:18 28/11/98 - no medipacks for Predator! */
				return 0;

//				PlayerType=I_PC_Predator;
				break;
			}
			case(I_Alien):
			{
				/* CDF 24/2/99 No medipacks for aliens, either! */
				return 0;

//				PlayerType=I_PC_Alien;
				break;
			}
			default:
			{
				LOCALASSERT(1==0);
				break;
			}
		}

		NpcData=GetThisNpcData(PlayerType);
		LOCALASSERT(NpcData);

		if (InventoryActor()->SBDamageBlock.IsOnFire) {
			InventoryActor()->SBDamageBlock.IsOnFire=0;
		} else if (InventoryActor()->SBDamageBlock.Health==NpcData->StartingStats.Health<<ONE_FIXED_SHIFT) {
			/* Already at max. */
			return(0);
		}

		InventoryActor()->SBDamageBlock.Health=NpcData->StartingStats.Health<<ONE_FIXED_SHIFT;
		playerStatusPtr->Health=InventoryActor()->SBDamageBlock.Health;
	}
	return 1;
}

/* KJL 15:59:48 03/10/97 - enum for armour pickups required */
static int AbleToPickupArmour(int armourID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	GLOBALASSERT(playerStatusPtr);
	
	{
		NPC_DATA *NpcData;
		NPC_TYPES PlayerType;

		switch(AvP.PlayerType) 
		{
			case(I_Marine):
			{
				switch (AvP.Difficulty) {
					case I_Easy:
						PlayerType=I_PC_Marine_Easy;
						break;
					default:
					case I_Medium:
						PlayerType=I_PC_Marine_Medium;
						break;
					case I_Hard:
						PlayerType=I_PC_Marine_Hard;
						break;
					case I_Impossible:
						PlayerType=I_PC_Marine_Impossible;
						break;
				}
				break;
			}
			case(I_Predator):
			{
				/* KJL 17:25:38 28/11/98 - no armour pickups for Predator! */
		 //		PlayerType=I_PC_Predator;
				return 0;
				break;
			}
			case(I_Alien):
			{
				/* CDF 24/2/99 ...or for the alien. */
				return(0);
//				PlayerType=I_PC_Alien;
				break;
			}
			default:
			{
				LOCALASSERT(1==0);
				break;
			}
		}

		NpcData=GetThisNpcData(PlayerType);
		LOCALASSERT(NpcData);

		if (InventoryActor()->SBDamageBlock.Armour>=(AcidResourceCapacity(NpcData->StartingStats.Armour)<<ONE_FIXED_SHIFT)) {
			/* Already at max. */
			return(0);
		}
		
		InventoryActor()->SBDamageBlock.Armour=AcidResourceCapacity(NpcData->StartingStats.Armour)<<ONE_FIXED_SHIFT;
		playerStatusPtr->Armour=InventoryActor()->SBDamageBlock.Armour;
	}
	return 1;
}

/* Andy 21/04/97 - enum for motion tracker upgrades required */
static int AbleToPickupMTrackerUpgrade(int mtrackerID)
{
	/* access the extra data hanging off the strategy block */
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

  /* For now, allow the player to pick up any type of upgrade */
	
	playerStatusPtr->MTrackerType = mtrackerID;
	return 1;
}




/*--------------------------Patrick 11/3/97---------------------------
  A couple of functions to set and check player's security clearances:
  * The 'set' function takes the security level as an UNSIGNED INTEGER 
    parameter
  * The 'Return' function also takes the security level as an UNSIGNED 
    INTEGER parameter, and returns 1 if the player has clearance to the
    specified level, or 0 if not...

  NB valid security levels are 0 to 31.
  --------------------------------------------------------------------*/

void SetPlayerSecurityClearance(STRATEGYBLOCK *sbPtr, unsigned int securityLevel)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	GLOBALASSERT(playerStatusPtr);	
	GLOBALASSERT(securityLevel<32);

	/* completely horrible hack: if you're the predator, and level 1 is passed,
	you get all levels except for one. Of course. Obvious isn't it. */
	
	if((AvP.PlayerType==I_Predator)&&(securityLevel==1))
	{
		playerStatusPtr->securityClearances|=0xfffffffe;
	}
	else playerStatusPtr->securityClearances|=(1<<securityLevel);
}

int ReturnPlayerSecurityClearance(STRATEGYBLOCK *sbPtr, unsigned int securityLevel)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	GLOBALASSERT(playerStatusPtr);	
	GLOBALASSERT(securityLevel<32);
	
	return(playerStatusPtr->securityClearances&(1<<securityLevel));
}


/* Removes objects picked up by the player: either destroys them for a single
player game, or for a multiplayer game, sets them to respawn */
static void RemovePickedUpObjectForActor(STRATEGYBLOCK *objectPtr,STRATEGYBLOCK *actor)
{
	INANIMATEOBJECT_STATUSBLOCK* objStatPtr = objectPtr->SBdataptr;
	GLOBALASSERT(objectPtr->I_SBtype == I_BehaviourInanimateObject);

	/* NPC collection uses the normal positional/EFX audio path. */
 if(actor && actor!=Player->ObStrategyBlock && actor->DynPtr){
  SOUNDINDEX sound=objStatPtr->typeId==IOT_Weapon?SID_MARINE_PICKUP_WEAPON:objStatPtr->typeId==IOT_Ammo?SID_MARINE_PICKUP_AMMO:objStatPtr->typeId==IOT_Armour?SID_MARINE_PICKUP_ARMOUR:SID_PICKUP;
  Sound_Play(sound,"d",&actor->DynPtr->Position);
 }else switch(objStatPtr->typeId)
	{
		case(IOT_Weapon):
			switch (AvP.PlayerType) {
				case I_Alien:
				default:
					Sound_Play(SID_PICKUP,NULL);
					break;
				case I_Marine:
					Sound_Play(SID_MARINE_PICKUP_WEAPON,NULL);
					break;
				case I_Predator:
					Sound_Play(SID_PREDATOR_PICKUP_WEAPON,NULL);
					break;
			}
			break;
		case(IOT_Ammo):
			Sound_Play(SID_MARINE_PICKUP_AMMO,NULL);
			break;
		case(IOT_Armour):
			Sound_Play(SID_MARINE_PICKUP_ARMOUR,NULL);
			break;
		case(IOT_FieldCharge):
			Sound_Play(SID_PREDATOR_PICKUP_FIELDCHARGE,NULL);
			break;
		default:
			Sound_Play(SID_PICKUP,NULL);
			break;
	}

	if (objStatPtr->ghosted_object) {
		/* Must be a runtime pickup... */
		AddNetMsg_LocalObjectDestroyed(objectPtr);

		DestroyAnyStrategyBlock(objectPtr);
		return;
	}

	//see if object has a target that should be notified upon being picked up
	if(objStatPtr->event_target)
	{
		if(objStatPtr->event_target->triggering_event & ObjectEventFlag_PickedUp)
		{
			RequestState(objStatPtr->event_target->event_target_sbptr,objStatPtr->event_target->request,0);
		}
	}
	
	if(AvP.Network==I_No_Network) DestroyAnyStrategyBlock(objectPtr);
	else
	{
		AddNetMsg_ObjectPickedUp(&objectPtr->SBname[0]);

		KillInanimateObjectForRespawn(objectPtr);
	}

}

int SlotForThisWeapon(enum WEAPON_ID weaponID) {

	PLAYER_STATUS *psptr;
	int a;

	psptr=(PLAYER_STATUS *)(InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	for (a=0; a<MAX_NO_OF_WEAPON_SLOTS; a++) {
		if (psptr->WeaponSlot[a].WeaponIDNumber==weaponID) {
			break;
		}
	}
	if (a!=MAX_NO_OF_WEAPON_SLOTS) {
		return(a);
	} else {
		return(-1);
	}
}

static int AbleToPickupFieldCharge(int chargeID)
{
	/* access the extra data hanging off the strategy block */
	NPC_TYPES PlayerType;
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	switch(AvP.PlayerType) 
	{
		case(I_Marine):
		{
			return(0);
			break;
		}
		case(I_Predator):
		{
			switch (AvP.Difficulty) {
				case I_Easy:
					PlayerType=I_PC_Predator_Easy;
					break;
				default:
				case I_Medium:
					PlayerType=I_PC_Predator_Medium;
					break;
				case I_Hard:
					PlayerType=I_PC_Predator_Hard;
					break;
				case I_Impossible:
					PlayerType=I_PC_Predator_Impossible;
					break;
			}
			break;
		}
		case(I_Alien):
		{
			return(0);
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}

	{
		int a;
		/* Add a medicomp 'round'. */

		a=SlotForThisWeapon(WEAPON_PRED_MEDICOMP);
		if (playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining<(MEDICOMP_MAX_AMMO)) {
	       	playerStatusPtr->WeaponSlot[a].PrimaryRoundsRemaining+=(ONE_FIXED);
		}
	}

	if (playerStatusPtr->FieldCharge==AcidResourceCapacity(PLAYERCLOAK_MAXENERGY)) {
		/* Got max already. */
		return(0);
	} else {
		playerStatusPtr->FieldCharge=AcidResourceCapacity(PLAYERCLOAK_MAXENERGY);
		return(1);
	}
}

void Convert_Disc_To_Pickup(STRATEGYBLOCK *sbPtr) {

	DYNAMICSBLOCK *dynPtr = sbPtr->DynPtr;
    PC_PRED_DISC_BEHAV_BLOCK *bbPtr = (PC_PRED_DISC_BEHAV_BLOCK * ) sbPtr->SBdataptr;
	SECTION_DATA *disc_section;
	INANIMATEOBJECT_STATUSBLOCK* objectStatusPtr;
	/* Transmogrify a disc behaviour to a disc ammo pickup! */

	disc_section=GetThisSectionData(bbPtr->HModelController.section_data,"disk");
	GLOBALASSERT(disc_section);

	/* Sort out dynamics block */
	dynPtr->LinVelocity.vx=0;
	dynPtr->LinVelocity.vy=0;
	dynPtr->LinVelocity.vz=0;

	dynPtr->LinImpulse.vx=0;
	dynPtr->LinImpulse.vy=0;
	dynPtr->LinImpulse.vz=0;

	dynPtr->OrientMat=disc_section->SecMat;
	dynPtr->Position=disc_section->World_Offset;
	MatrixToEuler(&dynPtr->OrientMat,&dynPtr->OrientEuler);

	sbPtr->shapeIndex=disc_section->sempai->ShapeNum;

	if (sbPtr->SBdptr) {
		sbPtr->SBdptr->ObShape=disc_section->sempai->ShapeNum;
		sbPtr->SBdptr->ObShapeData=disc_section->sempai->Shape;
		sbPtr->SBdptr->HModelControlBlock=NULL;
	}
	/* Create data block! */

	objectStatusPtr = (void *)AllocateMem(sizeof(INANIMATEOBJECT_STATUSBLOCK));

	objectStatusPtr->respawnTimer = 0; 
			
	/* these should be loaded */
	objectStatusPtr->typeId = IOT_Ammo;
	objectStatusPtr->subType = (int)AMMO_PRED_DISC;
	
	/* set default indestructibility */
	objectStatusPtr->Indestructable = Yes;
	objectStatusPtr->event_target=NULL;
	objectStatusPtr->fragments=NULL;
	objectStatusPtr->num_frags=0;
	objectStatusPtr->inan_tac=NULL;
	objectStatusPtr->ghosted_object=1;
	objectStatusPtr->explosionTimer=0;
	objectStatusPtr->lifespanTimer=0;

	/* Now swap it round... */
	Sound_Stop(bbPtr->soundHandle);
	Dispel_HModel(&bbPtr->HModelController);
	DeallocateMem(bbPtr);
	sbPtr->SBdataptr=(void *)objectStatusPtr;
	sbPtr->I_SBtype=I_BehaviourInanimateObject;

}

int ObjectIsDiscPickup(STRATEGYBLOCK *sbPtr) {

	if (sbPtr->I_SBtype == I_BehaviourInanimateObject) {

		INANIMATEOBJECT_STATUSBLOCK* objStatPtr = sbPtr->SBdataptr;
		
		/* Make sure the object hasn't already been picked up this frame */
		if(sbPtr->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0)
		{
			if (objStatPtr->typeId==IOT_Ammo) {
				if (objStatPtr->subType==AMMO_PRED_DISC) {
					return(1);
				}
			}			
		}
	} else if (sbPtr->I_SBtype == I_BehaviourNetGhost) {
		NETGHOSTDATABLOCK* ghostData = sbPtr->SBdataptr;

		if (sbPtr->SBflags.please_destroy_me==0) {
			if (ghostData->type==I_BehaviourInanimateObject) {
				if (ghostData->IOType==IOT_Ammo) {
					if (ghostData->subtype==AMMO_PRED_DISC) {
						return(1);
					}
				}
			}
		}
	}
	
	return(0);

}

void Recall_Disc(void) {

	/* If you are a predator with NO discs, pick up the nearest one? */
	PLAYER_WEAPON_DATA *weaponPtr;
	int weaponSlot = -1;
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (InventoryRecipient ? (void*)InventoryRecipient : InventoryActor()->SBdataptr);
    GLOBALASSERT(playerStatusPtr);

	if (AvP.PlayerType!=I_Predator) return;

	if (playerStatusPtr->FieldCharge<RecallDisc_Charge) return;
	
	weaponSlot = SlotForThisWeapon(WEAPON_PRED_DISC);

	if (weaponSlot == -1) return;
	
    /* init a pointer to the weapon's data */
    weaponPtr = &(playerStatusPtr->WeaponSlot[weaponSlot]);

	if ((weaponPtr->PrimaryMagazinesRemaining==0)
		&& (weaponPtr->PrimaryRoundsRemaining==0)) {

		/* We have no discs: try to find a pickup. */
		STRATEGYBLOCK *nearest, *candidate;
		int neardist,a;

		nearest=NULL;
		neardist=10000000;

		for (a=0; a<NumActiveStBlocks; a++) {
			candidate=ActiveStBlockList[a];
	
			if (candidate->DynPtr) {

				/* Are we the right type? */
				if (ObjectIsDiscPickup(candidate)) {
					VECTORCH offset;
					int dist;

					offset.vx=InventoryActor()->DynPtr->Position.vx-candidate->DynPtr->Position.vx;
					offset.vy=InventoryActor()->DynPtr->Position.vy-candidate->DynPtr->Position.vy;
					offset.vz=InventoryActor()->DynPtr->Position.vz-candidate->DynPtr->Position.vz;

					dist=Approximate3dMagnitude(&offset);
					
					if (dist<neardist) {
						nearest=candidate;
						neardist=dist;
					}
				} else if (candidate->I_SBtype==I_BehaviourPredatorDisc_SeekTrack) {
					VECTORCH offset;
					int dist;

					offset.vx=InventoryActor()->DynPtr->Position.vx-candidate->DynPtr->Position.vx;
					offset.vy=InventoryActor()->DynPtr->Position.vy-candidate->DynPtr->Position.vy;
					offset.vz=InventoryActor()->DynPtr->Position.vz-candidate->DynPtr->Position.vz;

					dist=Approximate3dMagnitude(&offset);
					
					if (dist<neardist) {
						nearest=candidate;
						neardist=dist;
					}
				}
			}
		}
		/* Do we have a disc? */
		if (nearest) {
			/* Attempt to pick it up. */
			if (nearest->I_SBtype == I_BehaviourInanimateObject) {

				INANIMATEOBJECT_STATUSBLOCK* objStatPtr = nearest->SBdataptr;
		
				/* Make sure the object hasn't already been picked up this frame */
				GLOBALASSERT(nearest->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0);
				GLOBALASSERT(objStatPtr->typeId==IOT_Ammo);
				GLOBALASSERT(objStatPtr->subType==AMMO_PRED_DISC);

				if (AbleToPickupAmmo(objStatPtr->subType)) {
					RemovePickedUpObject(nearest);
					playerStatusPtr->FieldCharge-=RecallDisc_Charge;
					CurrentGameStats_ChargeUsed(RecallDisc_Charge);
					#if 0
					InventoryMessage("RECALLED DISC");
					#endif
					Sound_Play(SID_PREDATOR_DISK_RECOVERED,"h");
					AutoSwapToDisc_OutOfSequence();
				}

			} else if (nearest->I_SBtype == I_BehaviourNetGhost) {
				NETGHOSTDATABLOCK* ghostData = nearest->SBdataptr;

				GLOBALASSERT(nearest->SBflags.please_destroy_me==0);
				GLOBALASSERT(ghostData->type==I_BehaviourInanimateObject);
				GLOBALASSERT(ghostData->IOType==IOT_Ammo);
				GLOBALASSERT(ghostData->subtype==AMMO_PRED_DISC);

				if (AbleToPickupAmmo(ghostData->subtype)) {
					AddNetMsg_LocalObjectDestroyed_Request(nearest);
					ghostData->IOType=IOT_Non;
					/* So it's not picked up again... */
					playerStatusPtr->FieldCharge-=RecallDisc_Charge;
					CurrentGameStats_ChargeUsed(RecallDisc_Charge);
					#if 0
					InventoryMessage("RECALLED DISC");
					#endif
					Sound_Play(SID_PREDATOR_DISK_RECOVERED,"h");
					AutoSwapToDisc_OutOfSequence();
				}

			} else if (nearest->I_SBtype == I_BehaviourPredatorDisc_SeekTrack) {

			    PC_PRED_DISC_BEHAV_BLOCK *bbPtr = (PC_PRED_DISC_BEHAV_BLOCK * ) nearest->SBdataptr;

				if (AbleToPickupAmmo(AMMO_PRED_DISC)) {
					/* Pick it up. */
						
					AutoSwapToDisc_OutOfSequence();
				
					Sound_Stop(bbPtr->soundHandle);
					Sound_Play(SID_PREDATOR_DISK_RECOVERED,"h");

					if(AvP.Network != I_No_Network)	AddNetMsg_LocalObjectDestroyed(nearest);

				    DestroyAnyStrategyBlock(nearest);	
				
				}

			} else {
				GLOBALASSERT(0);
			}
		} else {
			#if 0
			InventoryMessage("FOUND NO DISCS");
			#endif
		}
	}
}

int ObjectIsPlayersDisc(STRATEGYBLOCK *sbPtr) {

	if (sbPtr->I_SBtype == I_BehaviourInanimateObject) {

		INANIMATEOBJECT_STATUSBLOCK* objStatPtr = sbPtr->SBdataptr;
		
		/* Make sure the object hasn't already been picked up this frame */
		if(sbPtr->SBflags.please_destroy_me==0 && objStatPtr->respawnTimer==0)
		{
			if (objStatPtr->typeId==IOT_Ammo) {
				if (objStatPtr->subType==AMMO_PRED_DISC) {
					return(1);
				}
			}			
		}
	} else if (sbPtr->I_SBtype==I_BehaviourPredatorDisc_SeekTrack) {
		/* Erm... just return? */
		return(1);
	}

	return(0);
}

void RemoveAllThisPlayersDiscs(void) {

	STRATEGYBLOCK *candidate;
	int a;

	/* All discs that are NOT ghosted must 'belong' to this player. */

	for (a=0; a<NumActiveStBlocks; a++) {
		candidate=ActiveStBlockList[a];

		if (candidate->DynPtr) {

			/* Are we the right type? */
			if (ObjectIsPlayersDisc(candidate)) {
				AddNetMsg_LocalObjectDestroyed(candidate);

				DestroyAnyStrategyBlock(candidate);
			}
		}
	}
}

#include "npc_inventory_impl.inc"

void RemovePickedUpObject(STRATEGYBLOCK*objectPtr){RemovePickedUpObjectForActor(objectPtr,Player->ObStrategyBlock);}
