#ifndef _usr_io_h_
#define _usr_io_h_ 1
#include <stdint.h>
extern int AlwaysRunBySpecies[3];
extern int AlwaysRun_Temp,AlwaysRunSpecies_Temp;
int PlayerWantsToRun(int species,int modifierHeld);
extern unsigned char PredatorPreviousVisionBindings[2];
extern int PredatorVisionCycleDirection;
/*-------------- Patrick 21/10/96 -----------------
  Header File for User Input functions, structures,
  etc.  This is basically all platform dependant,
  and console versions should have they're own
  versions of the functions prototyped here.
  (win 95 source is in avp/win95/usr_io.c).  
  -------------------------------------------------*/ 


/*-------------- Patrick 21/10/96 -----------------
  Configuration structure for Win95 keyboard
  reading stuff.  Console versions can proabably
  use the same structure, but don't have to.
    
  NB Entries in the configuration structure 
  correspond to request flags in the player status
  block.  However, each request flag does not 
  necessarily need a config entry (eg lie down,
  which is only activated by special moves). If you
  don't want to implement a particular request flag
  functionality, leave it out of the config, and don't
  set it in ReadPlayerGameInput.
  -------------------------------------------------*/ 
#include "avp_menus.h"
#if 0
typedef struct player_input_configuration
{
	unsigned char Forward;
	unsigned char Backward;
	unsigned char Left;
	unsigned char Right;
	
	unsigned char Strafe;
	unsigned char StrafeLeft;
	unsigned char StrafeRight;
	
	unsigned char LookUp;
	unsigned char LookDown;
	unsigned char CentreView;
	
	unsigned char Walk;
	unsigned char Crouch;
	unsigned char Jump;
	
	unsigned char Operate;

	unsigned char NextWeapon;
	unsigned char PreviousWeapon;
	unsigned char FirePrimaryWeapon;
	unsigned char FireSecondaryWeapon;

	#ifdef __cplusplus
	// C++ only stuff, added by DHM 17/3/98 to ease rewrite of the 
	// key configuration screens:
	// (definitions are in REBITEMS.HPP)
	unsigned char GetMethod( enum KeyConfigItems theEffect ) const;
	void SetMethod
	(
		enum KeyConfigItems theEffect,
		unsigned char newMethod
	);
	#endif

}PLAYER_INPUT_CONFIGURATION;
#endif
enum PLAYER_INPUT_ID
{
	PLAYER_INPUT_FORWARD,
	PLAYER_INPUT_BACKWARD,
	PLAYER_INPUT_LEFT,
	PLAYER_INPUT_RIGHT,

	PLAYER_INPUT_STRAFE,
	PLAYER_INPUT_STRAFELEFT,
	PLAYER_INPUT_STRAFERIGHT,

	PLAYER_INPUT_LOOKUP,
	PLAYER_INPUT_LOOKDOWN,
	PLAYER_INPUT_CENTREVIEW,

	PLAYER_INPUT_WALK,
	PLAYER_INPUT_CROUCH,
	PLAYER_INPUT_JUMP,

	PLAYER_INPUT_OPERATE,
	PLAYER_INPUT_CHANGEVISION,

	PLAYER_INPUT_NEXTWEAPON,
	PLAYER_INPUT_PREVIOUSWEAPON,
	PLAYER_INPUT_FIREPRIMARYWEAPON,
	PLAYER_INPUT_FIRESECONDARYWEAPON,

	PLAYER_INPUT_ZOOMIN,
	PLAYER_INPUT_ZOOMOUT,

	PLAYER_INPUT_BONUSABILITY,

	MAX_NO_OF_PLAYER_INPUTS
};

#if PREDATOR_DEMO||DEATHMATCH_DEMO
#define NUMBER_OF_PREDATOR_INPUTS 30
#else
#define NUMBER_OF_PREDATOR_INPUTS 30
#endif

#if MARINE_DEMO||DEATHMATCH_DEMO
#define NUMBER_OF_MARINE_INPUTS	28
#else
#define NUMBER_OF_MARINE_INPUTS	28
#endif

#if ALIEN_DEMO||DEATHMATCH_DEMO
#define NUMBER_OF_ALIEN_INPUTS 21
#else
#define NUMBER_OF_ALIEN_INPUTS 22
#endif

typedef struct fixed_input_configuration
{
	unsigned char Weapon1;
	unsigned char Weapon2;
	unsigned char Weapon3;
	unsigned char Weapon4;
	unsigned char Weapon5;
	unsigned char Weapon6;
	unsigned char Weapon7;
	unsigned char Weapon8;
	unsigned char Weapon9;
	unsigned char Weapon10;
	unsigned char PauseGame;
		
}FIXED_INPUT_CONFIGURATION;

/* These five utility commands are shared by Predator, Marine and Alien. */
#define NUMBER_OF_GLOBAL_COMMAND_BINDINGS 5
typedef struct global_command_input_configuration
{
	unsigned char Screenshot;
	unsigned char QuickSave;
	unsigned char QuickLoad;
	unsigned char SaveGameMenu;
	unsigned char LoadGameMenu;
} GLOBAL_COMMAND_INPUT_CONFIGURATION;

extern GLOBAL_COMMAND_INPUT_CONFIGURATION GlobalCommandPrimaryConfig;
extern GLOBAL_COMMAND_INPUT_CONFIGURATION GlobalCommandSecondaryConfig;
extern const GLOBAL_COMMAND_INPUT_CONFIGURATION DefaultGlobalCommandPrimaryConfig;
extern const GLOBAL_COMMAND_INPUT_CONFIGURATION DefaultGlobalCommandSecondaryConfig;

/* Weapon slots are engine slots (1 through 11), not fixed physical keys. */
#define NUMBER_OF_WEAPON_BINDINGS 11
#define NUMBER_OF_MARINE_WEAPON_BINDS 9
#define NUMBER_OF_PREDATOR_WEAPON_BINDS 6
#define NUMBER_OF_ALIEN_WEAPON_BINDS 0

typedef struct weapon_input_configuration
{
	unsigned char Weapon[NUMBER_OF_WEAPON_BINDINGS];
} WEAPON_INPUT_CONFIGURATION;


typedef struct
{
	unsigned char Forward;
	unsigned char Backward;
	unsigned char Left;
	unsigned char Right;
	
	unsigned char Strafe;
	unsigned char StrafeLeft;
	unsigned char StrafeRight;
	unsigned char LookUp;

	unsigned char LookDown;
	unsigned char CentreView;
	unsigned char Walk;
	unsigned char Crouch;

	unsigned char Jump;
	unsigned char Operate;
	unsigned char FirePrimaryWeapon;
	unsigned char FireSecondaryWeapon;

	union
	{
		unsigned char NextWeapon; // Predator & Marine
		unsigned char AlternateVision;	// Alien
	} a;
		
	union
	{
		unsigned char PreviousWeapon; // Predator & Marine
		unsigned char Taunt;	  	  // Alien
	} b;
	
	union
	{
		unsigned char FlashbackWeapon; // Predator & Marine
		unsigned char Alien_MessageHistory; // Alien
	} c;

	union
	{
		unsigned char Cloak;			// Predator
		unsigned char ImageIntensifier; // Marine
		unsigned char Alien_Say;		// Alien
	} d;

	union
	{
		unsigned char CycleVisionMode;	// Predator
		unsigned char ThrowFlare;	   	// Marine
		unsigned char Alien_SpeciesSay;	// Alien
	} e;
	
	union
	{
		unsigned char ZoomIn;			// Predator
		unsigned char Jetpack;			// Marine
		unsigned char Alien_ShowScores;	// Alien
	} f;
	
	union
	{									
		unsigned char ZoomOut;			// Predator
		unsigned char MarineTaunt;		// Marine
	} g;
	
	union
	{
		unsigned char GrapplingHook;    // Predator	
		unsigned char Marine_MessageHistory; // Marine
	} h;
	
	union
	{
		unsigned char RecallDisc; // Predator
		unsigned char Marine_Say; // Marine
	} i;
	
	union
	{
		unsigned char PredatorTaunt;	 // Predator
		unsigned char Marine_SpeciesSay; // Marine
	} j;
	
	union
	{
		unsigned char Predator_MessageHistory; // Predator
		unsigned char Marine_ShowScores; // Marine
	} k;

	unsigned char Predator_Say;
	unsigned char Predator_SpeciesSay;
	unsigned char Predator_ShowScores;
	unsigned char ReloadWeapon; /* Marine reload; retained for legacy Predator bind migration. */
	unsigned char ReloadBindingProfileMarker; /* Profile migration marker. */
} PLAYER_INPUT_CONFIGURATION;

extern int SmartJetpackEnabled;
extern int SmartZoomEnabled;

/* Predator reuses its retired reload byte for the optional Zoom holding bind. */
#define PREDATOR_ZOOM_HOLD_BIND(inputConfig) ((inputConfig)->ReloadWeapon)

#define RELOAD_BINDING_PROFILE_MARKER 0xA7

typedef struct
{
	/* analogue stuff */
	unsigned int MouseXSensitivity;
	unsigned int MouseYSensitivity;

	unsigned int VAxisIsMovement; // else it's looking
	unsigned int HAxisIsTurning; // else it's sidestepping

	unsigned int FlipVerticalAxis;

	/* general stuff */
	unsigned int AutoCentreOnMovement;

}CONTROL_METHODS;

typedef struct
{
	/* joystick stuff */
	unsigned int JoystickEnabled;
	
	unsigned int JoystickVAxisIsMovement; // else it's looking
	unsigned int JoystickHAxisIsTurning;  // else it's sidestepping
	unsigned int JoystickFlipVerticalAxis;

	unsigned int JoystickPOVVAxisIsMovement; // else it's looking
	unsigned int JoystickPOVHAxisIsTurning;	 // else it's sidestepping
	unsigned int JoystickPOVFlipVerticalAxis;

	unsigned int JoystickRudderEnabled;		  
	unsigned int JoystickRudderAxisIsTurning; // else it's sidestepping

	unsigned int JoystickTrackerBallEnabled;
	unsigned int JoystickTrackerBallFlipVerticalAxis;
	unsigned int JoystickTrackerBallHorizontalSensitivity;
	unsigned int JoystickTrackerBallVerticalSensitivity;

}JOYSTICK_CONTROL_METHODS;

#define DEFAULT_MOUSEX_SENSITIVITY 15
#define DEFAULT_MOUSEY_SENSITIVITY 15
/* Keep the existing slider range independent of the default sensitivity. */
#define MAX_MOUSE_SENSITIVITY 192
#define DEFAULT_MOUSE_ZOOM_SENSITIVITY_TENTHS 13

#define DEFAULT_TRACKERBALL_HORIZONTAL_SENSITIVITY 32
#define DEFAULT_TRACKERBALL_VERTICAL_SENSITIVITY 32

/* Global Variables */
#ifdef __cplusplus
	// Linkage wrapping added by DHM 17/3/98:
	extern "C"
	{
#endif

	/* Globals */
	extern PLAYER_INPUT_CONFIGURATION MarineInputPrimaryConfig;
	extern PLAYER_INPUT_CONFIGURATION MarineInputSecondaryConfig;
	extern PLAYER_INPUT_CONFIGURATION AlienInputPrimaryConfig;
	extern PLAYER_INPUT_CONFIGURATION AlienInputSecondaryConfig;

extern WEAPON_INPUT_CONFIGURATION MarineWeaponPrimaryConfig;
extern WEAPON_INPUT_CONFIGURATION MarineWeaponSecondaryConfig;
extern WEAPON_INPUT_CONFIGURATION PredatorWeaponPrimaryConfig;
extern WEAPON_INPUT_CONFIGURATION PredatorWeaponSecondaryConfig;
extern WEAPON_INPUT_CONFIGURATION AlienWeaponPrimaryConfig;
extern WEAPON_INPUT_CONFIGURATION AlienWeaponSecondaryConfig;

extern WEAPON_INPUT_CONFIGURATION DefaultMarineWeaponPrimaryConfig;
extern WEAPON_INPUT_CONFIGURATION DefaultMarineWeaponSecondaryConfig;
extern WEAPON_INPUT_CONFIGURATION DefaultPredatorWeaponPrimaryConfig;
extern WEAPON_INPUT_CONFIGURATION DefaultPredatorWeaponSecondaryConfig;
extern WEAPON_INPUT_CONFIGURATION DefaultAlienWeaponPrimaryConfig;
extern WEAPON_INPUT_CONFIGURATION DefaultAlienWeaponSecondaryConfig;
	extern PLAYER_INPUT_CONFIGURATION PredatorInputPrimaryConfig;
	extern PLAYER_INPUT_CONFIGURATION PredatorInputSecondaryConfig;
	extern PLAYER_INPUT_CONFIGURATION DefaultMarineInputPrimaryConfig;
	extern PLAYER_INPUT_CONFIGURATION DefaultMarineInputSecondaryConfig;
	extern PLAYER_INPUT_CONFIGURATION DefaultAlienInputPrimaryConfig;
	extern PLAYER_INPUT_CONFIGURATION DefaultAlienInputSecondaryConfig;
	extern PLAYER_INPUT_CONFIGURATION DefaultPredatorInputPrimaryConfig;
	extern PLAYER_INPUT_CONFIGURATION DefaultPredatorInputSecondaryConfig;
	extern CONTROL_METHODS ControlMethods;
	extern CONTROL_METHODS DefaultControlMethods;
	extern JOYSTICK_CONTROL_METHODS JoystickControlMethods;
	extern JOYSTICK_CONTROL_METHODS DefaultJoystickControlMethods;

	/* Direct mouse camera deltas, in 16.16 AvP angle units for this frame. */
	extern int64_t MouseRawYawDelta;
	extern int64_t MouseRawPitchDelta;

	/* Extra zoom multiplier, stored in tenths: 0.0 through 2.0, default 1.0. */
	extern int MouseZoomSensitivityTenths;

	/* Fraction below a legacy 1/4096 turn unit, retained for camera rendering. */
	extern int64_t MouseRawPitchFraction;

	/* Per-profile vertical look range: 0 = original limits, 1 = +/-90 degrees. */
	extern int FullVerticalPitchEnabled;

	/* Prototypes */
	extern void InitPlayerGameInput(STRATEGYBLOCK* sbPtr);
	extern void ReadPlayerGameInput(STRATEGYBLOCK* sbPtr);

	/*
		DHM 1/4/98: Added prototypes for these functions; changing so
		that filename to use is passed to them, rather than being hardcoded
		as "avp.key" so that we can have multiple keyconfig files
	*/
	extern void LoadKeyConfiguration(void);
	extern void SaveKeyConfiguration(void);

	extern void LoadAKeyConfiguration(char* Filename);
	extern void SaveAKeyConfiguration(char* Filename);
void LoadDefaultPrimaryConfigs(void);

#ifdef __cplusplus
	};
	// ...linkage wrapping added by DHM 17/3/98
#endif

#endif
