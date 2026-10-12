#ifndef AVP_PLAYER_FOV_H
#define AVP_PLAYER_FOV_H

/* Horizontal FOV at 4:3. Hor+ preserves vertical coverage on wider displays. */
#define PLAYER_FOV_MINIMUM 60
#define PLAYER_FOV_MARINE_PREDATOR_MINIMUM 90
#define PLAYER_FOV_MARINE_PREDATOR_MAXIMUM 115
#define PLAYER_FOV_MARINE_DEFAULT 90
#define PLAYER_FOV_PREDATOR_DEFAULT 90
#define PLAYER_FOV_ALIEN_DEFAULT 127
#define PLAYER_FOV_DEFAULT PLAYER_FOV_MARINE_DEFAULT
#define PLAYER_FOV_MAXIMUM 130

/* First-person weapon projection is independent of the gameplay camera. */
#define WEAPON_FOV_MINIMUM 70
#define WEAPON_FOV_MARINE_PREDATOR_DEFAULT 90
#define WEAPON_FOV_ALIEN_DEFAULT 90
#define WEAPON_FOV_DEFAULT WEAPON_FOV_MARINE_PREDATOR_DEFAULT
#define WEAPON_FOV_MAXIMUM 95
#define WEAPON_FOV_ALIEN_MAXIMUM 90
/* Preserve legacy profile decoding before applying the current limits. */
#define WEAPON_FOV_PROFILE_MINIMUM 60
#define WEAPON_FOV_PROFILE_MAXIMUM 130
/* Neutral world lens for the coupled style, independent of profile defaults. */
#define WEAPON_FOV_WORLD_REFERENCE 90

enum WEAPON_FOV_STYLE
{
	WEAPON_FOV_STYLE_EXACT,
	WEAPON_FOV_STYLE_STRETCHED
};

#ifdef __cplusplus
extern "C" {
#endif

extern int PlayerFOV;
extern int PlayerFOVMarinePredator;
extern int PlayerFOVAlien;
/* The world FOV control is intentionally scoped to the active player species. */
extern int PlayerFOVMinimumForCurrentSpecies(void);
extern int PlayerFOVDefaultForCurrentSpecies(void);
extern int PlayerFOVMaximumForCurrentSpecies(void);
extern void ClampPlayerFOVForCurrentSpecies(void);
extern void RestorePlayerFOVForCurrentSpecies(void);
extern void RefreshPlayerFOV(void);
extern int WeaponFOVMaximumForCurrentSpecies(void);
extern int WeaponFOVDefaultForCurrentSpecies(void);
extern int WeaponFOV;
extern int WeaponFOVMarine;
extern int WeaponFOVPredator;
extern int WeaponFOVAlien;
extern void SynchroniseWeaponFOVForCurrentSpecies(void);
extern void RestoreWeaponFOVForCurrentSpecies(void);
extern int WeaponFOVStyle;
/* Returns the temporary VDB projection distance for one viewmodel draw. */
extern double WeaponFOVProjectionForViewSize(int viewWidth, int viewHeight);

#ifdef __cplusplus
}
#endif

#endif