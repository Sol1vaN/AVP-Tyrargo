#ifndef TYRARGO_MOLECULAR_ACID_H
#define TYRARGO_MOLECULAR_ACID_H
#ifdef __cplusplus
extern "C" {
#endif
#define ACID_PICKUP_CHANNELS 256
extern int MolecularAcidTier;
extern int AcidPickupRemainders[ACID_PICKUP_CHANNELS];
void AcidSetTier(int tier);
void AcidResetPickups(void);
int AcidActive(void);
int AcidResourceCapacity(int base);
/* Whole rounds, not fractional cartridges. Energy/armour use 16.16 directly. */
int AcidMagazineRounds(int ammoID,int baseFixed);
int AcidMagazinePickup(int ammoID,int baseCount);
int AcidStartingArmour(int species,int basePoints);
int AcidSpawnLimit(int base);
int AcidSpawnInterval(int base);
int AcidPickupBonus(int channel,int amount);
int AcidCompletion(void);
const char *AcidMultiplierLabel(int tier);
#ifdef __cplusplus
}
#endif
#endif
