#ifndef TYRARGO_OPTIMIZED_AMMO_H
#define TYRARGO_OPTIMIZED_AMMO_H
#ifdef __cplusplus
extern "C" {
#endif
/* Reserve counts include the last partial magazine. Deficits use the same
 * 16.16 units as the weapon, so even partially consumed fuel is conserved.
 * Separate storage preserves the layout of legacy player/save structures. */
#define OPTIMIZED_AMMO_CHANNELS 64
extern int OptimizedAmmo;
extern unsigned int OptimizedAmmoDeficit[OPTIMIZED_AMMO_CHANNELS];
struct player_weapon_data;
void OptimizedAmmoReset(void);
int OptimizedAmmoHasResiduals(void);
void OptimizedAmmoReload(int channel, unsigned int capacity, unsigned int *rounds,
                         unsigned short *magazines, int conserve);
void ReloadWeaponMagazine(struct player_weapon_data *weapon, int secondary);
void OptimizedAmmoSplitPistols(struct player_weapon_data *single, struct player_weapon_data *dual);
void OptimizedAmmoMergePistols(struct player_weapon_data *single, struct player_weapon_data *dual);
void OptimizedAmmoCopyWeapon(int weapon, unsigned int *destination, const unsigned int *source);
#ifdef __cplusplus
}
#endif
#endif
