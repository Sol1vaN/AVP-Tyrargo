#ifndef TYRARGO_NPC_INVENTORY_H
#define TYRARGO_NPC_INVENTORY_H
#include "equipmnt.h"
#include "optimized_ammo.h"
#include "molecular_acid.h"
/* No pointers are serialized. Native legacy marine save blocks stay unchanged. */
typedef struct {
 char pickupName[SB_NAME_LENGTH],pickupSkipName[SB_NAME_LENGTH];
 int pickupProgress,pickupDistance,pickupSkipTimer;
 int initialized,activeSlot,maxHealth,maxArmour,pickupTimer,weaponTimer;
 PLAYER_WEAPON_DATA weapons[MAX_NO_OF_WEAPON_SLOTS];
 GRENADE_LAUNCHER_DATA grenades;
 unsigned int deficits[OPTIMIZED_AMMO_CHANNELS];
 int pickupRemainders[ACID_PICKUP_CHANNELS];
 VECTORCH patrolGoal,tacticalGoal;int patrolTimer,patrolPhase,tacticalTimer,speed,moveStyle,retreating,movementCrouch;
 int visited[16],visitCursor,jumpTimer,airborneSpeed;VECTORCH airborneDirection;
 /* Appended extension; the schema-245 prefix stays byte compatible. */
 int loadoutReady,additionalPicked,loadoutTimer;unsigned int loadoutMask;
 int paceTimer,desiredPace;VECTORCH paceDirection;
 int perceptionTimer;int sightTimer,enemyMemory;VECTORCH enemyPosition;char enemyName[SB_NAME_LENGTH];
 /* Appended schema-251 weapon clocks; old squad prefixes remain unchanged. */
 int shotDelay[MAX_NO_OF_WEAPON_SLOTS][2],reloadDelay;
 unsigned int fuelCarryLow,fuelCarryHigh;
} NPC_MARINE_SQUAD;
void NPCInventoryReset(void);
int NPCInventoryCanResupply(STRATEGYBLOCK *sb);
int NPCInventoryApproach(STRATEGYBLOCK *sb,VECTORCH *goal);
void NPCInventoryInit(STRATEGYBLOCK *sb);
void NPCInventorySync(STRATEGYBLOCK *sb);
int NPCInventoryGrenadeType(STRATEGYBLOCK *sb);
int NPCInventoryReload(STRATEGYBLOCK *sb);
int NPCInventoryChooseWeapon(STRATEGYBLOCK *sb,int range);
int NPCInventoryWanted(STRATEGYBLOCK *sb,STRATEGYBLOCK *item);
int NPCInventoryPickup(STRATEGYBLOCK *sb,STRATEGYBLOCK *item);
STRATEGYBLOCK *NPCInventoryNearby(STRATEGYBLOCK *sb);
int NPCInventoryNativeWeapon(int weapon);
int NPCInventoryPlayerWeapon(int weapon);
#endif
