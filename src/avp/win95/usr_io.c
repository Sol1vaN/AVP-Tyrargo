/*------------------------------- Patrick 21/10/96 ------------------------------
  Source for reading player inputs.
  Note that whilst ReadUserInput() reads raw input data, the functions in this
  file map those inputs onto the player movement structures (defined in Player
  Status).  This is, of course, entirely platform dependant.  Consoles will
  need their own equivalent functions.... 

  -------------------------------------------------------------------------------*/ 
#include "3dc.h"
#include "module.h"
#include "inline.h"

#include "stratdef.h"
#include "gamedef.h"
#include "gameplat.h"

#include "bh_types.h"
#include "ourasert.h"
#include "comp_shp.h"

#include "pmove.h"
#include "usr_io.h"
#include "dev_viewmodel.h"
#include "hud.h"
#include "messagehistory.h"

#include "iofocus.h"

#include "paintball.h"
#include "ahudgadg.hpp"
#include "avp_menus.h"
#include "psndproj.h"
#include <stdint.h>
#include <limits.h>

/* Profile preference per species: false = walk, modifier held = run.
 * True = run, modifier held = walk. No speed, acceleration or physics changes. */
int AlwaysRunBySpecies[3]={0,0,0};
int PlayerWantsToRun(int species,int modifierHeld)
{
    int alwaysRun=(species>=I_Marine && species<=I_Alien) ? AlwaysRunBySpecies[species]==1 : 0;
    return alwaysRun != (modifierHeld!=0);
}

extern int InGameMenusAreRunning(void);
extern void AvP_TriggerInGameMenus(void);
extern void AvP_OpenInGameSaveMenu(void);
extern void AvP_OpenInGameLoadMenu(void);
extern void QuickSave(void);
extern void QuickLoad(void);
extern void RequestScreenShot(void);
extern void Recall_Disc(void);
extern void ShowMultiplayerScores(void);


extern int NormalFrameTime;
extern unsigned char DebouncedKeyboardInput[];

FIXED_INPUT_CONFIGURATION FixedInputConfig =
{
	KEY_1,				// Weapon1;
	KEY_2,				// Weapon2;
	KEY_3,				// Weapon3;
	KEY_4,				// Weapon4;
	KEY_5,				// Weapon5;
	KEY_6,				// Weapon6;
	KEY_7,				// Weapon7;
	KEY_8,				// Weapon8;
	KEY_9,				// Weapon9;
	KEY_0,				// Weapon10;
	KEY_ESCAPE,			// PauseGame;

};

GLOBAL_COMMAND_INPUT_CONFIGURATION GlobalCommandPrimaryConfig =
{ 102, 95, 98, 92, 93 };
GLOBAL_COMMAND_INPUT_CONFIGURATION GlobalCommandSecondaryConfig =
{ 255, 255, 255, 255, 255 };
const GLOBAL_COMMAND_INPUT_CONFIGURATION DefaultGlobalCommandPrimaryConfig =
{ 102, 95, 98, 92, 93 };
const GLOBAL_COMMAND_INPUT_CONFIGURATION DefaultGlobalCommandSecondaryConfig =
{ 255, 255, 255, 255, 255 };

static int GlobalCommandPressed(unsigned char primary, unsigned char secondary)
{
	return ((primary != KEY_VOID && DebouncedKeyboardInput[primary])
		|| (secondary != KEY_VOID && DebouncedKeyboardInput[secondary]));
}
void CheckScreenshotInput(void)
{
    if(GlobalCommandPressed(GlobalCommandPrimaryConfig.Screenshot,GlobalCommandSecondaryConfig.Screenshot))
        RequestScreenShot();
}


PLAYER_INPUT_CONFIGURATION MarineInputPrimaryConfig;
PLAYER_INPUT_CONFIGURATION MarineInputSecondaryConfig;
PLAYER_INPUT_CONFIGURATION PredatorInputPrimaryConfig;
PLAYER_INPUT_CONFIGURATION PredatorInputSecondaryConfig;
PLAYER_INPUT_CONFIGURATION AlienInputPrimaryConfig;
PLAYER_INPUT_CONFIGURATION AlienInputSecondaryConfig;

/* Per-species, rebindable direct weapon slots. The first ten retain the
   historical 1..0 defaults; the Marine-only eleventh slot starts unbound. */
WEAPON_INPUT_CONFIGURATION MarineWeaponPrimaryConfig;
WEAPON_INPUT_CONFIGURATION MarineWeaponSecondaryConfig;
WEAPON_INPUT_CONFIGURATION PredatorWeaponPrimaryConfig;
WEAPON_INPUT_CONFIGURATION PredatorWeaponSecondaryConfig;
WEAPON_INPUT_CONFIGURATION AlienWeaponPrimaryConfig;
WEAPON_INPUT_CONFIGURATION AlienWeaponSecondaryConfig;

WEAPON_INPUT_CONFIGURATION DefaultMarineWeaponPrimaryConfig = {{4, 5, 7, 9, 8, 6, 10, 2, 3, 255, 255}};
WEAPON_INPUT_CONFIGURATION DefaultMarineWeaponSecondaryConfig = {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}};
WEAPON_INPUT_CONFIGURATION DefaultPredatorWeaponPrimaryConfig = {{2, 3, 4, 7, 6, 5, 255, 255, 255, 255, 255}};
WEAPON_INPUT_CONFIGURATION DefaultPredatorWeaponSecondaryConfig = {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}};
WEAPON_INPUT_CONFIGURATION DefaultAlienWeaponPrimaryConfig = {{KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_VOID}};
WEAPON_INPUT_CONFIGURATION DefaultAlienWeaponSecondaryConfig = {{KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID, KEY_VOID}};

#if 1 // English
PLAYER_INPUT_CONFIGURATION DefaultMarineInputPrimaryConfig =
{
 /* Bindings calibrated in user test; no profile option metadata. */
 33, 29, 255, 255, 255, 11, 14, 255, 255, 255, 53, 55, 52, 16, 119, 121, {15}, {27}, {255}, {124}, {123}, {59}, {120}, {91}, {30}, {35}, {42}, 255, 255, 255, 28, 167
};
PLAYER_INPUT_CONFIGURATION DefaultPredatorInputPrimaryConfig =
{
 /* Bindings calibrated in user test; no profile option metadata. */
 33, 29, 255, 255, 255, 11, 14, 255, 255, 255, 53, 55, 52, 16, 119, 121, {15}, {27}, {255}, {KEY_Z}, {KEY_C}, {123}, {124}, {255}, {28}, {120}, {91}, 30, 35, 42, 255, 167
};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputPrimaryConfig =
{
 /* Bindings calibrated in user test; no profile option metadata. */
 33, 29, 255, 255, 255, 11, 14, 255, 255, 255, 53, 55, 52, 16, 119, 121, {124}, {120}, {91}, {30}, {35}, {42}, {255}, {255}, {255}, {255}, {255}, 255, 255, 255, 255, 167
};
#elif 0	// Dutch
PLAYER_INPUT_CONFIGURATION DefaultMarineInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Z, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_ASTERISK,		// NextWeapon;
	KEY_DIACRITIC_UMLAUT,// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;

	KEY_MINUS,	  		// ImageIntensifier;
	KEY_FSTOP,    		// ThrowFlare;
	KEY_DIACRITIC_ACUTE,	  	// Jetpack;
	KEY_PLUS,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
PLAYER_INPUT_CONFIGURATION DefaultPredatorInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Z, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;
	
	KEY_ASTERISK,		// NextWeapon;
	KEY_DIACRITIC_UMLAUT,// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;
	
	KEY_FSTOP,	 		// Cloak;
	KEY_MINUS,	 		// CycleVisionMode;
	KEY_PAGEUP,			// ZoomIn;
	KEY_PAGEDOWN,		// ZoomOut;
	KEY_DIACRITIC_ACUTE,	  	// GrapplingHook
	KEY_COMMA,			// RecallDisk
	KEY_PLUS,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Z, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_MINUS,			// AlternateVision;
	KEY_FSTOP,				// Taunt;
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
#elif 0	// French
PLAYER_INPUT_CONFIGURATION DefaultMarineInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_A, 				// LookUp;
	KEY_W, 				// LookDown;
	KEY_Q,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_DOLLAR,  		// NextWeapon;
	KEY_DIACRITIC_CARET,  		// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;

	KEY_EXCLAMATION,	  		// ImageIntensifier;
	KEY_COLON,    		// ThrowFlare;
	KEY_U_GRAVE,	  	// Jetpack;
	KEY_M,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
PLAYER_INPUT_CONFIGURATION DefaultPredatorInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_A, 				// LookUp;
	KEY_W, 				// LookDown;
	KEY_Q,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;
	
	KEY_DOLLAR,		// NextWeapon;
	KEY_DIACRITIC_CARET, 			// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;
	
	KEY_COLON,	 		// Cloak;
	KEY_EXCLAMATION,	 		// CycleVisionMode;
	KEY_PAGEUP,			// ZoomIn;
	KEY_PAGEDOWN,		// ZoomOut;
	KEY_U_GRAVE,	  	// GrapplingHook
	KEY_SEMICOLON,			// RecallDisk
	KEY_M,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_A, 				// LookUp;
	KEY_W, 				// LookDown;
	KEY_Q,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_EXCLAMATION,			// AlternateVision;
	KEY_SEMICOLON,				// Taunt;
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
#elif 0	 // German
PLAYER_INPUT_CONFIGURATION DefaultMarineInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Y, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_PLUS,  		// NextWeapon;
	KEY_U_UMLAUT,  		// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;

	KEY_MINUS,	  		// ImageIntensifier;
	KEY_FSTOP,    		// ThrowFlare;
	KEY_A_UMLAUT,	  	// Jetpack;
	KEY_O_UMLAUT,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
PLAYER_INPUT_CONFIGURATION DefaultPredatorInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Y, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;
	
	KEY_PLUS,		// NextWeapon;
	KEY_U_UMLAUT, 			// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;
	
	KEY_FSTOP,	 		// Cloak;
	KEY_MINUS,	 		// CycleVisionMode;
	KEY_PAGEUP,			// ZoomIn;
	KEY_PAGEDOWN,		// ZoomOut;
	KEY_A_UMLAUT,	  	// GrapplingHook
	KEY_COMMA,			// RecallDisk
	KEY_O_UMLAUT,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Y, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_MINUS,			// AlternateVision;
	KEY_FSTOP,				// Taunt;
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
#elif 0	// Spanish
PLAYER_INPUT_CONFIGURATION DefaultMarineInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Z, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_PLUS,  		// NextWeapon;
	KEY_DIACRITIC_GRAVE,  		// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;

	KEY_MINUS,	  		// ImageIntensifier;
	KEY_FSTOP,    		// ThrowFlare;
	KEY_DIACRITIC_ACUTE,	  	// Jetpack;
	KEY_N_TILDE,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
PLAYER_INPUT_CONFIGURATION DefaultPredatorInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Z, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;
	
	KEY_PLUS,		// NextWeapon;
	KEY_DIACRITIC_GRAVE, 			// PreviousWeapon;
	KEY_BACKSPACE,		// FlashbackWeapon;
	
	KEY_FSTOP,	 		// Cloak;
	KEY_MINUS,	 		// CycleVisionMode;
	KEY_PAGEUP,			// ZoomIn;
	KEY_PAGEDOWN,		// ZoomOut;
	KEY_DIACRITIC_ACUTE,	  	// GrapplingHook
	KEY_COMMA,			// RecallDisk
	KEY_N_TILDE,		// Taunt
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputPrimaryConfig =
{
	KEY_UP,				// Forward;
	KEY_DOWN,			// Backward;
	KEY_NUMPAD4, 		// Left;
	KEY_NUMPAD6, 		// Right;

	KEY_RIGHTALT,		// Strafe;
	KEY_LEFT,	 		// StrafeLeft;
	KEY_RIGHT,	 		// StrafeRight;

	KEY_Q, 				// LookUp;
	KEY_Z, 				// LookDown;
	KEY_A,				// CentreView;

	KEY_LEFTSHIFT,		// Walk;
	KEY_RIGHTCTRL, 		// Crouch;
	KEY_RIGHTSHIFT,		// Jump;

	KEY_SPACE,			// Operate;

	KEY_LMOUSE, 		// FirePrimaryWeapon;
	KEY_RMOUSE, 		// FireSecondaryWeapon;

	KEY_MINUS,			// AlternateVision;
	KEY_FSTOP,				// Taunt;
	KEY_F1,
	KEY_F11,
	KEY_F12,
	KEY_TAB,
};
#endif
PLAYER_INPUT_CONFIGURATION DefaultMarineInputSecondaryConfig =
{
 /* Bindings calibrated in user test; no profile option metadata. */
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, 255, 255, 255, 255, 167
};




PLAYER_INPUT_CONFIGURATION DefaultPredatorInputSecondaryConfig =
{
 /* Bindings calibrated in user test; no profile option metadata. */
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, 255, 255, 255, 255, 167
};

PLAYER_INPUT_CONFIGURATION DefaultAlienInputSecondaryConfig =
{
 /* Bindings calibrated in user test; no profile option metadata. */
 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, {255}, 255, 255, 255, 255, 167
};


CONTROL_METHODS ControlMethods =
{
	/* analogue stuff */
	DEFAULT_MOUSEX_SENSITIVITY, //unsigned int MouseXSensitivity;
	DEFAULT_MOUSEY_SENSITIVITY, //unsigned int MouseYSensitivity;

	0,//unsigned int VAxisIsMovement :1; // else it's looking
	1,//unsigned int HAxisIsTurning :1; // else it's sidestepping

	0,//unsigned int FlipVerticalAxis :1;
	
	/* general stuff */
	0,//unsigned int AutoCentreOnMovement :1;
};

CONTROL_METHODS DefaultControlMethods =
{
	/* analogue stuff */
	DEFAULT_MOUSEX_SENSITIVITY, //unsigned int MouseXSensitivity;
	DEFAULT_MOUSEY_SENSITIVITY, //unsigned int MouseYSensitivity;

	0,//unsigned int VAxisIsMovement :1; // else it's looking
	1,//unsigned int HAxisIsTurning :1; // else it's sidestepping

	0,//unsigned int FlipVerticalAxis :1;
	
	/* general stuff */
	0,//unsigned int AutoCentreOnMovement :1;
};

JOYSTICK_CONTROL_METHODS JoystickControlMethods =
{
	0,//unsigned int JoystickEnabled;
	
	1,//unsigned int JoystickVAxisIsMovement; // else it's looking
	1,//unsigned int JoystickHAxisIsTurning;  // else it's sidestepping
	0,//unsigned int JoystickFlipVerticalAxis;

	0,//unsigned int JoystickPOVVAxisIsMovement; // else it's looking
	0,//unsigned int JoystickPOVHAxisIsTurning;	 // else it's sidestepping
	0,//unsigned int JoystickPOVFlipVerticalAxis;

	0,//unsigned int JoystickRudderEnabled;		  
	0,//unsigned int JoystickRudderAxisIsTurning; // else it's sidestepping
	
	0,//unsigned int JoystickTrackerBallEnabled;
	0,//unsigned int JoystickTrackerBallFlipVerticalAxis;
	DEFAULT_TRACKERBALL_HORIZONTAL_SENSITIVITY,//unsigned int JoystickTrackerBallHorizontalSensitivity;
	DEFAULT_TRACKERBALL_VERTICAL_SENSITIVITY,//unsigned int JoystickTrackerBallVerticalSensitivity;

};
JOYSTICK_CONTROL_METHODS DefaultJoystickControlMethods =
{
	0,//unsigned int JoystickEnabled;
	
	1,//unsigned int JoystickVAxisIsMovement; // else it's looking
	1,//unsigned int JoystickHAxisIsTurning;  // else it's sidestepping
	0,//unsigned int JoystickFlipVerticalAxis;

	0,//unsigned int JoystickPOVVAxisIsMovement; // else it's looking
	0,//unsigned int JoystickPOVHAxisIsTurning;	 // else it's sidestepping
	0,//unsigned int JoystickPOVFlipVerticalAxis;

	0,//unsigned int JoystickRudderEnabled;		  
	0,//unsigned int JoystickRudderAxisIsTurning; // else it's sidestepping
	
	0,//unsigned int JoystickTrackerBallEnabled;
	0,//unsigned int JoystickTrackerBallFlipVerticalAxis;
	DEFAULT_TRACKERBALL_HORIZONTAL_SENSITIVITY,//unsigned int JoystickTrackerBallHorizontalSensitivity;
	DEFAULT_TRACKERBALL_VERTICAL_SENSITIVITY,//unsigned int JoystickTrackerBallVerticalSensitivity;
};

/* Extern for global keyboard buffer */
extern unsigned char KeyboardInput[];
extern unsigned char DebouncedKeyboardInput[];
int SmartJetpackEnabled;
int SmartZoomEnabled;
unsigned char PredatorPreviousVisionBindings[2]={KEY_X,KEY_VOID};
int PredatorVisionCycleDirection=1;
static int SmartJetpackToggleOn;
/* Edge state for one notification per press/release of Zoom holding. */
static int SmartZoomHoldingNotified;
extern int GotJoystick;
extern int GotMouse;
extern float MouseRawDeltaX;
extern float MouseRawDeltaY;

int64_t MouseRawYawDelta;
int64_t MouseRawPitchDelta;

/* Extra user multiplier for FOV-scaled raw camera input, in tenths. */
int MouseZoomSensitivityTenths = DEFAULT_MOUSE_ZOOM_SENSITIVITY_TENTHS;

/* Return the requested zoom direction for one of the four Smart Zoom actions.
   Debounced input preserves the native one-step-per-press zoom behaviour. */
static int PredatorSmartZoomDirectionForBinding(unsigned char binding, PLAYER_INPUT_CONFIGURATION *primaryInput, PLAYER_INPUT_CONFIGURATION *secondaryInput, int useDebouncedInput)
{
	int zoomInMatch;
	int zoomOutMatch;
	int bindingIsActive;

	if (binding==KEY_VOID) return 0;
	bindingIsActive = useDebouncedInput ? DebouncedKeyboardInput[binding] : KeyboardInput[binding];
	if (!bindingIsActive) return 0;
	zoomInMatch = (binding==primaryInput->f.ZoomIn || binding==secondaryInput->f.ZoomIn);
	zoomOutMatch = (binding==primaryInput->g.ZoomOut || binding==secondaryInput->g.ZoomOut);
	/* A key assigned to both directions is ambiguous, so retain its normal action. */
	if (zoomInMatch==zoomOutMatch) return 0;
	return zoomInMatch ? 1 : -1;
}

/* Convert an SDL relative count directly to the engine's 16.16 angle unit.
 * This deliberately does not divide by frame time: camera travel depends on
 * physical mouse distance, as in Quake 3's direct mouse path. */
static int64_t MouseCountsToAngleFixed(float counts, float sensitivity)
{
	const double result = (double)counts * (double)sensitivity * 2000.0;

	if (result >= (double)INT64_MAX) return INT64_MAX;
	if (result <= (double)INT64_MIN) return INT64_MIN;
	return (result >= 0.0) ? (int64_t)(result + 0.5) : (int64_t)(result - 0.5);
}

/* initialise the player input structure(s) in the player_status block */
void InitPlayerGameInput(STRATEGYBLOCK* sbPtr)
{
	PLAYER_STATUS *playerStatusPtr;

    /* get the player status block ... */
    playerStatusPtr = (PLAYER_STATUS *) (sbPtr->SBdataptr);
    LOCALASSERT(playerStatusPtr);
	

	/* analogue type inputs */
	playerStatusPtr->Mvt_MotionIncrement = 0;
	playerStatusPtr->Mvt_TurnIncrement = 0;
	playerStatusPtr->Mvt_PitchIncrement = 0;
	playerStatusPtr->Mvt_AnalogueTurning = 0;
	playerStatusPtr->Mvt_AnaloguePitching = 0;
	playerStatusPtr->Mvt_SideStepIncrement = 0;

	/* request flags */
	playerStatusPtr->Mvt_InputRequests.Mask = 0;
	playerStatusPtr->Mvt_InputRequests.Mask2 = 0;

	/* Use the species preference, also when no gameplay input is accepted. */
	playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster = PlayerWantsToRun(AvP.PlayerType,0);

}


/* Only slots published by the species' bind menu may request direct selection.
   Legacy/internal slots must not overwrite a valid user weapon binding. */
static int DirectWeaponSlotIsPublishedForPlayer(int playerType, int weaponSlot)
{
	switch (playerType)
	{
		case I_Marine:
			return (weaponSlot >= 0 && weaponSlot <= 8);

		case I_Predator:
			return (weaponSlot >= 0 && weaponSlot <= 5);

		case I_Alien:
			return 0;

		default:
			return 0;
	}
}
/* This function maps raw inputs onto the players movement attributes in
   the player_status block.  It is called from the ExecuteFreeMovement
   function.
   NB Currently, only keyboard input is supported. */
void ReadPlayerGameInput(STRATEGYBLOCK* sbPtr)
{
    int devEditKeys = 0;
    unsigned char devSavedKeys[6],flameSavedKeys[12];
    int flameKeysActive=0;
    const int flameKeys[6]={KEY_W,KEY_S,KEY_A,KEY_D,KEY_Q,KEY_E};

	PLAYER_INPUT_CONFIGURATION *primaryInput;
	PLAYER_INPUT_CONFIGURATION *secondaryInput;
	WEAPON_INPUT_CONFIGURATION *primaryWeaponInput;
	WEAPON_INPUT_CONFIGURATION *secondaryWeaponInput;
	PLAYER_STATUS *playerStatusPtr;
	int smartJetpackJumpHeld;
	int smartJetpackTogglePressed;
	int smartJetpackAvailable;
	int smartPredatorZoomHolding;
	int smartPredatorZoomInPressed;
	int smartPredatorZoomOutPressed;
	int smartPredatorNextZoomDirection;
	int smartPredatorPreviousZoomDirection;
	int smartPredatorVisionZoomDirection;
	int smartPredatorFlareZoomDirection;
	int smartPredatorSuppressNext;
	int smartPredatorSuppressPrevious;
	int smartPredatorSuppressVision;
	int smartPredatorSuppressFlare;

    /* get the player status block ... */
    playerStatusPtr = (PLAYER_STATUS *) (sbPtr->SBdataptr);
    LOCALASSERT(playerStatusPtr);
	
	/* start off by initialising the inputs */
	InitPlayerGameInput(sbPtr);
    PredatorVisionCycleDirection=1;
	MouseRawYawDelta = 0;
	MouseRawPitchDelta = 0;

	switch (AvP.PlayerType)
	{
		default:
		case I_Marine:
		{
			primaryInput = &MarineInputPrimaryConfig;
			secondaryInput = &MarineInputSecondaryConfig;
			primaryWeaponInput = &MarineWeaponPrimaryConfig;
			secondaryWeaponInput = &MarineWeaponSecondaryConfig;
			break;
		}
		case I_Predator:
		{
			primaryInput = &PredatorInputPrimaryConfig;
			secondaryInput = &PredatorInputSecondaryConfig;
			primaryWeaponInput = &PredatorWeaponPrimaryConfig;
			secondaryWeaponInput = &PredatorWeaponSecondaryConfig;
			break;
		}
		case I_Alien:
		{
			primaryInput = &AlienInputPrimaryConfig;
			secondaryInput = &AlienInputSecondaryConfig;
			primaryWeaponInput = &AlienWeaponPrimaryConfig;
			secondaryWeaponInput = &AlienWeaponSecondaryConfig;
			break;
		}
	}

	/* Turning Smart Zoom off must make its modifier inert immediately, without
	 * emitting a stale release notification. */
	if (AvP.PlayerType!=I_Predator || !SmartZoomEnabled)
		SmartZoomHoldingNotified=0;

	if ( IOFOCUS_AcceptControls() && !InGameMenusAreRunning())
	{
        if(playerStatusPtr->IsAlive && AvP.PlayerType==I_Marine &&
           playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_FLAMETHROWER && DevFlameEditing()){
            flameKeysActive=1;
            DevFlameKeys(KeyboardInput[KEY_W],KeyboardInput[KEY_S],KeyboardInput[KEY_A],KeyboardInput[KEY_D],KeyboardInput[KEY_Q],KeyboardInput[KEY_E],NormalFrameTime);
            for(int i=0;i<6;i++){flameSavedKeys[i]=KeyboardInput[flameKeys[i]];flameSavedKeys[6+i]=DebouncedKeyboardInput[flameKeys[i]];KeyboardInput[flameKeys[i]]=DebouncedKeyboardInput[flameKeys[i]]=0;}
        }
        if (playerStatusPtr->IsAlive && DevViewmodelHandleKeys(AvP.PlayerType,
            playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot].WeaponIDNumber,
            KeyboardInput[KEY_W],KeyboardInput[KEY_S],DebouncedKeyboardInput[KEY_Z],NormalFrameTime)) {
            devEditKeys=1;
            devSavedKeys[0]=KeyboardInput[KEY_W];devSavedKeys[1]=KeyboardInput[KEY_S];devSavedKeys[2]=KeyboardInput[KEY_Z];
            devSavedKeys[3]=DebouncedKeyboardInput[KEY_W];devSavedKeys[4]=DebouncedKeyboardInput[KEY_S];devSavedKeys[5]=DebouncedKeyboardInput[KEY_Z];
            KeyboardInput[KEY_W]=KeyboardInput[KEY_S]=KeyboardInput[KEY_Z]=0;
            DebouncedKeyboardInput[KEY_W]=DebouncedKeyboardInput[KEY_S]=DebouncedKeyboardInput[KEY_Z]=0;
        }

		smartJetpackJumpHeld = (KeyboardInput[primaryInput->Jump]
			|| KeyboardInput[secondaryInput->Jump]);
		smartJetpackTogglePressed = 0;
		/* A map grants the actual capability through StartingEquipment.
		   Smart Jetpack must remain completely inert without that equipment. */
		smartJetpackAvailable = (AvP.PlayerType==I_Marine
			&& SmartJetpackEnabled
			&& playerStatusPtr->JetpackEnabled);
		if (smartJetpackAvailable)
		{
			/* Only the dedicated Jetpack bind changes mode.  Jump is deliberately
			   excluded so intermittent Jump taps can never toggle the jetpack. */
			smartJetpackTogglePressed = (DebouncedKeyboardInput[primaryInput->f.Jetpack]
				|| DebouncedKeyboardInput[secondaryInput->f.Jetpack]);
			if (smartJetpackTogglePressed)
			{
				SmartJetpackToggleOn = !SmartJetpackToggleOn;
				PlaySmartJetpackToggleSound(SmartJetpackToggleOn);
				GADGET_NewOnScreenMessage(GetTextString(SmartJetpackToggleOn ? TEXTSTRING_SMART_JETPACK_MODE_ON_NEW : TEXTSTRING_SMART_JETPACK_MODE_OFF_NEW));
			}
		}
		else
		{
			SmartJetpackToggleOn = 0;
		}

		smartPredatorZoomHolding = 0;
		smartPredatorZoomInPressed = 0;
		smartPredatorZoomOutPressed = 0;
		smartPredatorNextZoomDirection = 0;
		smartPredatorPreviousZoomDirection = 0;
		smartPredatorVisionZoomDirection = 0;
		smartPredatorFlareZoomDirection = 0;
		smartPredatorSuppressNext = 0;
		smartPredatorSuppressPrevious = 0;
		smartPredatorSuppressVision = 0;
		smartPredatorSuppressFlare = 0;
		if (AvP.PlayerType==I_Predator && SmartZoomEnabled)
		{
			smartPredatorZoomHolding = (PREDATOR_ZOOM_HOLD_BIND(primaryInput)!=KEY_VOID && KeyboardInput[PREDATOR_ZOOM_HOLD_BIND(primaryInput)])
				|| (PREDATOR_ZOOM_HOLD_BIND(secondaryInput)!=KEY_VOID && KeyboardInput[PREDATOR_ZOOM_HOLD_BIND(secondaryInput)]);
			if (smartPredatorZoomHolding!=SmartZoomHoldingNotified)
			{
				GADGET_NewOnScreenMessage(GetTextString(smartPredatorZoomHolding
					? TEXTSTRING_SMART_ZOOM_ACTIVATED_NEW
					: TEXTSTRING_SMART_ZOOM_DEACTIVATED_NEW));
				SmartZoomHoldingNotified=smartPredatorZoomHolding;
			}
			if (smartPredatorZoomHolding)
			{
				/* Smart Zoom overlays only these four published Predator actions. */
				smartPredatorNextZoomDirection = PredatorSmartZoomDirectionForBinding(primaryInput->a.NextWeapon,primaryInput,secondaryInput,0);
				if (!smartPredatorNextZoomDirection) smartPredatorNextZoomDirection = PredatorSmartZoomDirectionForBinding(secondaryInput->a.NextWeapon,primaryInput,secondaryInput,0);
				smartPredatorPreviousZoomDirection = PredatorSmartZoomDirectionForBinding(primaryInput->b.PreviousWeapon,primaryInput,secondaryInput,0);
				if (!smartPredatorPreviousZoomDirection) smartPredatorPreviousZoomDirection = PredatorSmartZoomDirectionForBinding(secondaryInput->b.PreviousWeapon,primaryInput,secondaryInput,0);
				smartPredatorVisionZoomDirection = PredatorSmartZoomDirectionForBinding(primaryInput->d.Cloak,primaryInput,secondaryInput,0);
				if (!smartPredatorVisionZoomDirection) smartPredatorVisionZoomDirection = PredatorSmartZoomDirectionForBinding(secondaryInput->d.Cloak,primaryInput,secondaryInput,0);
				smartPredatorFlareZoomDirection = PredatorSmartZoomDirectionForBinding(primaryInput->e.CycleVisionMode,primaryInput,secondaryInput,0);
				if (!smartPredatorFlareZoomDirection) smartPredatorFlareZoomDirection = PredatorSmartZoomDirectionForBinding(secondaryInput->e.CycleVisionMode,primaryInput,secondaryInput,0);

				smartPredatorZoomInPressed = (PredatorSmartZoomDirectionForBinding(primaryInput->a.NextWeapon,primaryInput,secondaryInput,1)>0 || PredatorSmartZoomDirectionForBinding(secondaryInput->a.NextWeapon,primaryInput,secondaryInput,1)>0 || PredatorSmartZoomDirectionForBinding(primaryInput->b.PreviousWeapon,primaryInput,secondaryInput,1)>0 || PredatorSmartZoomDirectionForBinding(secondaryInput->b.PreviousWeapon,primaryInput,secondaryInput,1)>0 || PredatorSmartZoomDirectionForBinding(primaryInput->d.Cloak,primaryInput,secondaryInput,1)>0 || PredatorSmartZoomDirectionForBinding(secondaryInput->d.Cloak,primaryInput,secondaryInput,1)>0 || PredatorSmartZoomDirectionForBinding(primaryInput->e.CycleVisionMode,primaryInput,secondaryInput,1)>0 || PredatorSmartZoomDirectionForBinding(secondaryInput->e.CycleVisionMode,primaryInput,secondaryInput,1)>0);
				smartPredatorZoomOutPressed = (PredatorSmartZoomDirectionForBinding(primaryInput->a.NextWeapon,primaryInput,secondaryInput,1)<0 || PredatorSmartZoomDirectionForBinding(secondaryInput->a.NextWeapon,primaryInput,secondaryInput,1)<0 || PredatorSmartZoomDirectionForBinding(primaryInput->b.PreviousWeapon,primaryInput,secondaryInput,1)<0 || PredatorSmartZoomDirectionForBinding(secondaryInput->b.PreviousWeapon,primaryInput,secondaryInput,1)<0 || PredatorSmartZoomDirectionForBinding(primaryInput->d.Cloak,primaryInput,secondaryInput,1)<0 || PredatorSmartZoomDirectionForBinding(secondaryInput->d.Cloak,primaryInput,secondaryInput,1)<0 || PredatorSmartZoomDirectionForBinding(primaryInput->e.CycleVisionMode,primaryInput,secondaryInput,1)<0 || PredatorSmartZoomDirectionForBinding(secondaryInput->e.CycleVisionMode,primaryInput,secondaryInput,1)<0);
				smartPredatorSuppressNext = (smartPredatorNextZoomDirection!=0);
				smartPredatorSuppressPrevious = (smartPredatorPreviousZoomDirection!=0);
				smartPredatorSuppressVision = (smartPredatorVisionZoomDirection!=0);
				smartPredatorSuppressFlare = (smartPredatorFlareZoomDirection!=0);
			}
		}
		/* now do forward,backward,left,right,up and down 
		   IMPORTANT:  The request flag and the movement 
		   increment must BOTH be set!
		*/
		if(KeyboardInput[primaryInput->Forward]
		 ||KeyboardInput[secondaryInput->Forward])
		{
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
			playerStatusPtr->Mvt_MotionIncrement = ONE_FIXED;
		}	
		if(KeyboardInput[primaryInput->Backward]
		 ||KeyboardInput[secondaryInput->Backward])
		{
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
			playerStatusPtr->Mvt_MotionIncrement = -ONE_FIXED;
		}
		if(KeyboardInput[primaryInput->Left]
		 ||KeyboardInput[secondaryInput->Left])
		{
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
			playerStatusPtr->Mvt_TurnIncrement = -ONE_FIXED;
		}
		if(KeyboardInput[primaryInput->Right]
		 ||KeyboardInput[secondaryInput->Right])
		{
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
			playerStatusPtr->Mvt_TurnIncrement = ONE_FIXED;
		}

		if(KeyboardInput[primaryInput->StrafeLeft]
		 ||KeyboardInput[secondaryInput->StrafeLeft])
		{
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
			playerStatusPtr->Mvt_SideStepIncrement = -ONE_FIXED;
		}
		if(KeyboardInput[primaryInput->StrafeRight]
		 ||KeyboardInput[secondaryInput->StrafeRight])
		{	
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
			playerStatusPtr->Mvt_SideStepIncrement = ONE_FIXED;
		}
        playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Faster = PlayerWantsToRun(AvP.PlayerType,
            (primaryInput->Walk!=KEY_VOID && KeyboardInput[primaryInput->Walk]) ||
            (secondaryInput->Walk!=KEY_VOID && KeyboardInput[secondaryInput->Walk]));
		
		if(KeyboardInput[primaryInput->Strafe]
		 ||KeyboardInput[secondaryInput->Strafe])
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe = 1;

		if(KeyboardInput[primaryInput->Crouch]
		 ||KeyboardInput[secondaryInput->Crouch])
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Crouch = 1;
		
		if(smartJetpackJumpHeld
		 && !(smartJetpackAvailable && SmartJetpackToggleOn))
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jump = 1;

		if(KeyboardInput[primaryInput->Operate]
		 ||KeyboardInput[secondaryInput->Operate])
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Operate = 1;

		/* check for character specific abilities */
		if (playerStatusPtr->IsAlive)
		switch (AvP.PlayerType)
		{
			case I_Marine:
			{
				if(KeyboardInput[primaryInput->d.ImageIntensifier]
				 ||KeyboardInput[secondaryInput->d.ImageIntensifier])
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ChangeVision = 1;

				if(DebouncedKeyboardInput[primaryInput->e.ThrowFlare]
				 ||DebouncedKeyboardInput[secondaryInput->e.ThrowFlare])
					ThrowAFlare();

				#if !(MARINE_DEMO||DEATHMATCH_DEMO)
				if (smartJetpackAvailable)
				{
					/* A tap of Jetpack selects the mode; holding Jump supplies thrust. */
					if (SmartJetpackToggleOn && smartJetpackJumpHeld)
						playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jetpack = 1;
				}
				/* In Smart mode without equipment, the dedicated Jetpack bind is null.
				   With Smart mode disabled, preserve the original vanilla key path. */
				else if (!SmartJetpackEnabled && (KeyboardInput[primaryInput->f.Jetpack]
				 ||KeyboardInput[secondaryInput->f.Jetpack]))
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Jetpack = 1;
				#endif
				
				if(KeyboardInput[primaryInput->g.MarineTaunt]
				 ||KeyboardInput[secondaryInput->g.MarineTaunt])
					StartPlayerTaunt();
				
				if(DebouncedKeyboardInput[primaryInput->h.Marine_MessageHistory]
				 ||DebouncedKeyboardInput[secondaryInput->h.Marine_MessageHistory])
					MessageHistory_DisplayPrevious();
					
				if(DebouncedKeyboardInput[primaryInput->i.Marine_Say]
				 ||DebouncedKeyboardInput[secondaryInput->i.Marine_Say])
					BringDownConsoleWithSayTypedIn();

				if(DebouncedKeyboardInput[primaryInput->j.Marine_SpeciesSay]
				 ||DebouncedKeyboardInput[secondaryInput->j.Marine_SpeciesSay])
					BringDownConsoleWithSaySpeciesTypedIn();

				if(KeyboardInput[primaryInput->k.Marine_ShowScores]
				 ||KeyboardInput[secondaryInput->k.Marine_ShowScores])
					ShowMultiplayerScores();
					

				break;
			}
			case I_Predator:
			{
				extern int CameraZoomLevel;
				
				if(!smartPredatorSuppressVision && (KeyboardInput[primaryInput->d.Cloak]
				
				 ||KeyboardInput[secondaryInput->d.Cloak]))
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ChangeVision = 1;
				
				if(!smartPredatorSuppressFlare && (DebouncedKeyboardInput[primaryInput->e.CycleVisionMode]
				
				 ||DebouncedKeyboardInput[secondaryInput->e.CycleVisionMode]))
                    playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CycleVisionMode = 1;
                /* Separate debounced action: one step/sound per press. The
                   existing forward action wins if both are pressed together. */
                if (!playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CycleVisionMode &&
                    ((PredatorPreviousVisionBindings[0]!=KEY_VOID && DebouncedKeyboardInput[PredatorPreviousVisionBindings[0]]) ||
                     (PredatorPreviousVisionBindings[1]!=KEY_VOID && DebouncedKeyboardInput[PredatorPreviousVisionBindings[1]])))
                {
                    PredatorVisionCycleDirection=-1;
                    playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CycleVisionMode=1;
                }

				#if !(PREDATOR_DEMO||DEATHMATCH_DEMO)
				if(DebouncedKeyboardInput[primaryInput->h.GrapplingHook]
				 ||DebouncedKeyboardInput[secondaryInput->h.GrapplingHook])
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_GrapplingHook = 1;
				#endif

				if (SmartZoomEnabled)
				{
					/* Preserve native zoom steps: one shared-action press changes one
					   level while its Smart Zoom modifier is held. */
					if (smartPredatorZoomHolding)
					{
						if (smartPredatorZoomInPressed && !smartPredatorZoomOutPressed && CameraZoomLevel<3) CameraZoomLevel++;
						else if (smartPredatorZoomOutPressed && !smartPredatorZoomInPressed && CameraZoomLevel>0) CameraZoomLevel--;
					}
				}
				else
				{
					if(DebouncedKeyboardInput[primaryInput->f.ZoomIn]
					 ||DebouncedKeyboardInput[secondaryInput->f.ZoomIn])
					{
						if (CameraZoomLevel<3) CameraZoomLevel++;
					}
					if(DebouncedKeyboardInput[primaryInput->g.ZoomOut]
					 ||DebouncedKeyboardInput[secondaryInput->g.ZoomOut])
					{
						if (CameraZoomLevel>0) CameraZoomLevel--;
					}
				}
				
				MaintainZoomingLevel();
				
				if(KeyboardInput[primaryInput->j.PredatorTaunt]
				 ||KeyboardInput[secondaryInput->j.PredatorTaunt])
					StartPlayerTaunt();

				if(KeyboardInput[primaryInput->i.RecallDisc]
				 ||KeyboardInput[secondaryInput->i.RecallDisc])
					Recall_Disc();
					
				if(DebouncedKeyboardInput[primaryInput->k.Predator_MessageHistory]
				 ||DebouncedKeyboardInput[secondaryInput->k.Predator_MessageHistory])
					MessageHistory_DisplayPrevious();
					
				if(DebouncedKeyboardInput[primaryInput->Predator_Say]
				 ||DebouncedKeyboardInput[secondaryInput->Predator_Say])
					BringDownConsoleWithSayTypedIn();

				if(DebouncedKeyboardInput[primaryInput->Predator_SpeciesSay]
				 ||DebouncedKeyboardInput[secondaryInput->Predator_SpeciesSay])
					BringDownConsoleWithSaySpeciesTypedIn();

				if(KeyboardInput[primaryInput->Predator_ShowScores]
				 ||KeyboardInput[secondaryInput->Predator_ShowScores])
					ShowMultiplayerScores();

				break;
			}

			case I_Alien:
			{
				if(KeyboardInput[primaryInput->a.AlternateVision]
				 ||KeyboardInput[secondaryInput->a.AlternateVision])
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ChangeVision = 1;

				if(KeyboardInput[primaryInput->b.Taunt]
				 ||KeyboardInput[secondaryInput->b.Taunt])
					StartPlayerTaunt();
	
				if(DebouncedKeyboardInput[primaryInput->c.Alien_MessageHistory]
				 ||DebouncedKeyboardInput[secondaryInput->c.Alien_MessageHistory])
					MessageHistory_DisplayPrevious();
					
				if(DebouncedKeyboardInput[primaryInput->d.Alien_Say]
				 ||DebouncedKeyboardInput[secondaryInput->d.Alien_Say])
					BringDownConsoleWithSayTypedIn();

				if(DebouncedKeyboardInput[primaryInput->e.Alien_SpeciesSay]
				 ||DebouncedKeyboardInput[secondaryInput->e.Alien_SpeciesSay])
					BringDownConsoleWithSaySpeciesTypedIn();

				if(KeyboardInput[primaryInput->f.Alien_ShowScores]
				 ||KeyboardInput[secondaryInput->f.Alien_ShowScores])
					ShowMultiplayerScores();

				break;
			}
		}
		
		if(DebouncedKeyboardInput[FixedInputConfig.PauseGame])
			AvP_TriggerInGameMenus();

		/* Global utility binds are edge-triggered and species-independent. */

		if (AvP.Network == I_No_Network)
		{
			if (GlobalCommandPressed(GlobalCommandPrimaryConfig.QuickSave,GlobalCommandSecondaryConfig.QuickSave)) QuickSave();
			if (GlobalCommandPressed(GlobalCommandPrimaryConfig.QuickLoad,GlobalCommandSecondaryConfig.QuickLoad)) QuickLoad();
			if (GlobalCommandPressed(GlobalCommandPrimaryConfig.SaveGameMenu,GlobalCommandSecondaryConfig.SaveGameMenu)) AvP_OpenInGameSaveMenu();
			else if (GlobalCommandPressed(GlobalCommandPrimaryConfig.LoadGameMenu,GlobalCommandSecondaryConfig.LoadGameMenu)) AvP_OpenInGameLoadMenu();
		}
	//		playerStatusPtr->Mvt_InputRequests.Flags.Rqst_QuitGame = 1;
//			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_PauseGame = 1;

		if(!PaintBallMode.IsOn)
		{
			if(KeyboardInput[primaryInput->FirePrimaryWeapon]
			 ||KeyboardInput[secondaryInput->FirePrimaryWeapon])
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FirePrimaryWeapon = 1;
			
			if(KeyboardInput[primaryInput->LookUp]
			 ||KeyboardInput[secondaryInput->LookUp])
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_PitchIncrement = -ONE_FIXED;
			}
			else if(KeyboardInput[primaryInput->LookDown]
			 ||KeyboardInput[secondaryInput->LookDown])
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_PitchIncrement = ONE_FIXED;
			}
  			
			if(KeyboardInput[primaryInput->CentreView]
			 ||KeyboardInput[secondaryInput->CentreView])
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_CentreView = 1;

			if(!smartPredatorSuppressNext && (KeyboardInput[primaryInput->a.NextWeapon]

			 ||KeyboardInput[secondaryInput->a.NextWeapon]))
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_NextWeapon = 1;
			
			if(!smartPredatorSuppressPrevious && (KeyboardInput[primaryInput->b.PreviousWeapon]
			
			 ||KeyboardInput[secondaryInput->b.PreviousWeapon]))
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_PreviousWeapon = 1;

			if(DebouncedKeyboardInput[primaryInput->c.FlashbackWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->c.FlashbackWeapon])
			{
				if (playerStatusPtr->PreviouslySelectedWeaponSlot!=playerStatusPtr->SelectedWeaponSlot)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = playerStatusPtr->PreviouslySelectedWeaponSlot+1;
				}
			}

			if ((AvP.PlayerType==I_Marine)
				&& (DebouncedKeyboardInput[primaryInput->ReloadWeapon]
				 || DebouncedKeyboardInput[secondaryInput->ReloadWeapon]))
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_ReloadWeapon = 1;
			}
			
			if(KeyboardInput[primaryInput->FireSecondaryWeapon]
			 ||KeyboardInput[secondaryInput->FireSecondaryWeapon])
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FireSecondaryWeapon = 1;
			
			/* Direct selection is per species and completely rebindable.
			   Ignore unpublished internal slots so a stale KEY_0 cannot replace
			   the user's valid weapon request with an empty slot. */
			for (int weaponSlot=0; weaponSlot<NUMBER_OF_WEAPON_BINDINGS; weaponSlot++)
			{
				if (DirectWeaponSlotIsPublishedForPlayer(AvP.PlayerType, weaponSlot)
				 && ((primaryWeaponInput->Weapon[weaponSlot] != KEY_VOID
					  && KeyboardInput[primaryWeaponInput->Weapon[weaponSlot]])
					 || (secondaryWeaponInput->Weapon[weaponSlot] != KEY_VOID
					  && KeyboardInput[secondaryWeaponInput->Weapon[weaponSlot]])))
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_WeaponNo = weaponSlot+1;
				}
			}
			
		}
		#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO||DEATHMATCH_DEMO)
		else // Cool - paintball mode				`
		{
			if(DebouncedKeyboardInput[primaryInput->a.NextWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->a.NextWeapon])
			{
				PaintBallMode_ChangeSelectedDecalID(+1);
			}
			
			if(DebouncedKeyboardInput[primaryInput->b.PreviousWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->b.PreviousWeapon])
			{
				PaintBallMode_ChangeSelectedDecalID(-1);
			}
				
			if(KeyboardInput[primaryInput->LookUp]
			 ||KeyboardInput[secondaryInput->LookUp])
			{
				PaintBallMode_ChangeSize(+1);
			}
			
			if(KeyboardInput[primaryInput->LookDown]
			 ||KeyboardInput[secondaryInput->LookDown])
			{
				PaintBallMode_ChangeSize(-1);
			}
			
			if(KeyboardInput[primaryInput->CentreView]
			 ||KeyboardInput[secondaryInput->CentreView])
			{
				PaintBallMode_Rotate();				
			}
						  
			if(DebouncedKeyboardInput[FixedInputConfig.Weapon1])
			{
				PaintBallMode_ChangeSubclass(+1);
			}
			
		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon2])
			{
				PaintBallMode_ChangeSubclass(-1);
			}

		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon3])
			{
				PaintBallMode.DecalIsInverted = ~PaintBallMode.DecalIsInverted;
			}

		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon4])
			{
				PaintBallMode_Randomise();
			}

		  	if(DebouncedKeyboardInput[FixedInputConfig.Weapon10])
			{
				extern void save_preplaced_decals();
				save_preplaced_decals();
			}
	

			if(DebouncedKeyboardInput[primaryInput->FirePrimaryWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->FirePrimaryWeapon])
			{
				PaintBallMode_AddDecal();
			}
			if(DebouncedKeyboardInput[primaryInput->FireSecondaryWeapon]
			 ||DebouncedKeyboardInput[secondaryInput->FireSecondaryWeapon])
			{
				PaintBallMode_RemoveDecal();
			}
			
		}
		#endif
	}
	/* end of block conditional on input focus */


	/* KJL 10:16:49 04/29/97 - mouse control */




	if (GotMouse)
	{
		extern int MouseVelX;
		extern int MouseVelY;
		extern float CameraZoomScale;
		float mouseZoomFovScale = 1.0f;
		int zoomSensitivityTenths = MouseZoomSensitivityTenths;

		/* Q3-style: use the live FOV scale only for raw camera yaw/pitch. */
		if (zoomSensitivityTenths < 0) zoomSensitivityTenths = 0;
		if (zoomSensitivityTenths > 20) zoomSensitivityTenths = 20;
		if (CameraZoomScale < 1.0f)
		{
			mouseZoomFovScale = CameraZoomScale * ((float)zoomSensitivityTenths / 10.0f);
		}


		if(ControlMethods.HAxisIsTurning)
		{
			if(MouseRawDeltaX<0.0f)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				MouseRawYawDelta = MouseCountsToAngleFixed(MouseRawDeltaX, (float)ControlMethods.MouseXSensitivity * mouseZoomFovScale);
			}
			else if(MouseRawDeltaX>0.0f)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				MouseRawYawDelta = MouseCountsToAngleFixed(MouseRawDeltaX, (float)ControlMethods.MouseXSensitivity * mouseZoomFovScale);
			}

			/* Preserve legacy keyboard/strafe behavior. Mouse camera input now
			 * bypasses Mvt_TurnIncrement, so this is not its flick limit. */
			if(playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe)
			{
				if(playerStatusPtr->Mvt_TurnIncrement < -ONE_FIXED)
					playerStatusPtr->Mvt_TurnIncrement = -ONE_FIXED;
				if(playerStatusPtr->Mvt_TurnIncrement > ONE_FIXED)
					playerStatusPtr->Mvt_TurnIncrement = ONE_FIXED;
			}
		}
		else // it's sidestep
		{
			if(MouseVelX<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
				playerStatusPtr->Mvt_SideStepIncrement = ((int)MouseVelX)*ControlMethods.MouseXSensitivity;
			}
			else if(MouseVelX>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
				playerStatusPtr->Mvt_SideStepIncrement = ((int)MouseVelX)*ControlMethods.MouseXSensitivity;
			}
	   		
	   		if(playerStatusPtr->Mvt_SideStepIncrement < -ONE_FIXED)
	   			playerStatusPtr->Mvt_SideStepIncrement = -ONE_FIXED;
	   		if(playerStatusPtr->Mvt_SideStepIncrement > ONE_FIXED)
	   			playerStatusPtr->Mvt_SideStepIncrement = ONE_FIXED;
			

		}

		if(ControlMethods.VAxisIsMovement)
		{
			
			if(MouseVelY<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
			 	playerStatusPtr->Mvt_MotionIncrement = -((int)MouseVelY)*ControlMethods.MouseYSensitivity;
			}
			else if(MouseVelY>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
			 	playerStatusPtr->Mvt_MotionIncrement = -((int)MouseVelY)*ControlMethods.MouseYSensitivity;
			}
	   	
	   		if(playerStatusPtr->Mvt_MotionIncrement < -ONE_FIXED)
	   			playerStatusPtr->Mvt_MotionIncrement = -ONE_FIXED;
	   		if(playerStatusPtr->Mvt_MotionIncrement > ONE_FIXED)
	   			playerStatusPtr->Mvt_MotionIncrement = ONE_FIXED;
		}
		else // it's looking
		{
			float newMouseDeltaY;

			if (ControlMethods.FlipVerticalAxis) newMouseDeltaY = -MouseRawDeltaY;
			else newMouseDeltaY = MouseRawDeltaY;

			if(newMouseDeltaY<0.0f)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				MouseRawPitchDelta = MouseCountsToAngleFixed(newMouseDeltaY, (float)ControlMethods.MouseYSensitivity * mouseZoomFovScale);
			}
			else if(newMouseDeltaY>0.0f)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				MouseRawPitchDelta = MouseCountsToAngleFixed(newMouseDeltaY, (float)ControlMethods.MouseYSensitivity * mouseZoomFovScale);
			}
		}
	}

	/* KJL 18:27:34 04/29/97 - joystick control */
	if (GotJoystick)
	{
		#define JOYSTICK_DEAD_ZONE 12000
		extern JOYINFOEX JoystickData;
		extern JOYCAPS JoystickCaps;
		
		
		int yAxis = (32768-JoystickData.dwYpos)*2;
		int xAxis = (JoystickData.dwXpos-32768)*2;
		
		if(JoystickControlMethods.JoystickVAxisIsMovement)
		{
			if(JoystickControlMethods.JoystickFlipVerticalAxis) yAxis=-yAxis;

			if(yAxis>JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
				playerStatusPtr->Mvt_MotionIncrement = yAxis;
			}	
			else if(yAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
				playerStatusPtr->Mvt_MotionIncrement = yAxis;
			}
		}
		else // looking up/down
		{
			if(!JoystickControlMethods.JoystickFlipVerticalAxis) yAxis=-yAxis;

			if(yAxis>JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = yAxis;
			}
			else if(yAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = yAxis;
			}
		}

		if (JoystickControlMethods.JoystickHAxisIsTurning)
		{
			if(xAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = xAxis;
			}
			else if(xAxis>JOYSTICK_DEAD_ZONE)
			{			  
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = xAxis;
			}
		}
		else // strafing
		{
			if(xAxis<-JOYSTICK_DEAD_ZONE)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
				playerStatusPtr->Mvt_SideStepIncrement = xAxis;
			}
			else if(xAxis>JOYSTICK_DEAD_ZONE)
			{			  
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
				playerStatusPtr->Mvt_SideStepIncrement = xAxis;
			}
		}
		
		/* check for rudder */
		if ((JoystickCaps.wCaps & JOYCAPS_HASR) && JoystickControlMethods.JoystickRudderEnabled)
		{
			int rAxis = (JoystickData.dwRpos-32768)*2;
			if (JoystickControlMethods.JoystickRudderAxisIsTurning)
			{
				if(rAxis>JOYSTICK_DEAD_ZONE)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}
				else if(rAxis<-JOYSTICK_DEAD_ZONE)
				{			  
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}
			}
			else
			{
				if(rAxis>JOYSTICK_DEAD_ZONE)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe = 1;
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}	
				else if(rAxis<-JOYSTICK_DEAD_ZONE)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Strafe = 1;
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = rAxis;
				}	
			}
		}

		/* check joystick buttons */
		#if 0 
		if(JoystickData.dwButtons & JOY_BUTTON1)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FirePrimaryWeapon = 1;
		else if(JoystickData.dwButtons & JOY_BUTTON2)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_FireSecondaryWeapon = 1;
		else if(JoystickData.dwButtons & JOY_BUTTON3)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_NextWeapon = 1;
		else if(JoystickData.dwButtons & JOY_BUTTON4)
			playerStatusPtr->Mvt_InputRequests.Flags.Rqst_PreviousWeapon = 1;
		#endif

		/* Point Of View Hat */
		if (JoystickData.dwPOV<36000)
		{
			int theta = ((JoystickData.dwPOV * 4096) /36000);
			int verticalAxis = GetCos(theta);
			int horizontalAxis = GetSin(theta);

			if (JoystickControlMethods.JoystickPOVFlipVerticalAxis)
			{
				verticalAxis = -verticalAxis;
			}

			if (JoystickControlMethods.JoystickPOVVAxisIsMovement)
			{
				if(verticalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward = 1;
					playerStatusPtr->Mvt_MotionIncrement = verticalAxis;
				}								  
				else if(verticalAxis<0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward = 1;
					playerStatusPtr->Mvt_MotionIncrement = verticalAxis;
				}
			}
			else
			{
				if(verticalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
					playerStatusPtr->Mvt_PitchIncrement -= verticalAxis;
				}								  
				else if(verticalAxis<0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
					playerStatusPtr->Mvt_PitchIncrement -= verticalAxis;
				}
			}
			if (JoystickControlMethods.JoystickPOVHAxisIsTurning)
			{
				if(horizontalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = horizontalAxis;
				}
				else if(horizontalAxis<0)
				{			  
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
					playerStatusPtr->Mvt_AnalogueTurning = 1;
					playerStatusPtr->Mvt_TurnIncrement = horizontalAxis;
				}
			}
			else // strafing
			{
				if(horizontalAxis>0)
				{
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight = 1;
					playerStatusPtr->Mvt_SideStepIncrement = horizontalAxis;
				}
				else if(horizontalAxis<0)
				{			  
					playerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft = 1;
					playerStatusPtr->Mvt_SideStepIncrement = horizontalAxis;
				}
			}
		}
		if (JoystickControlMethods.JoystickTrackerBallEnabled)
		{
			int trackerballH = JoystickData.dwUpos - 32768;
			int trackerballV = JoystickData.dwVpos - 32768;

			if (JoystickControlMethods.JoystickTrackerBallFlipVerticalAxis)
			{
				trackerballV = -trackerballV;
			}

			if(trackerballH<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnLeft = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = trackerballH*JoystickControlMethods.JoystickTrackerBallHorizontalSensitivity;
			   
			}
			else if(trackerballH>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_TurnRight = 1;
				playerStatusPtr->Mvt_AnalogueTurning = 1;
				playerStatusPtr->Mvt_TurnIncrement = trackerballH*JoystickControlMethods.JoystickTrackerBallHorizontalSensitivity;

			}
			if(trackerballV<0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookUp = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = trackerballV*JoystickControlMethods.JoystickTrackerBallVerticalSensitivity;
			}
			else if(trackerballV>0)
			{
				playerStatusPtr->Mvt_InputRequests.Flags.Rqst_LookDown = 1;
				playerStatusPtr->Mvt_AnaloguePitching = 1;
				playerStatusPtr->Mvt_PitchIncrement = trackerballV*JoystickControlMethods.JoystickTrackerBallVerticalSensitivity;;
			}

		}

		#if 0
		textprint("%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n",
			JoystickData.dwXpos,
			JoystickData.dwYpos,
			JoystickData.dwZpos,
			JoystickData.dwRpos,
			JoystickData.dwUpos,
			JoystickData.dwVpos,
			JoystickData.dwButtons,
			JoystickData.dwPOV);
		#endif
	}

	/* KJL 10:55:22 10/9/97 - HUD transparency */
	#if 0
	{
		extern signed int HUDTranslucencyLevel;

		if (KeyboardInput[KEY_F1])
		{
			HUDTranslucencyLevel-=NormalFrameTime>>9;
			if (HUDTranslucencyLevel<0) HUDTranslucencyLevel=0;
		}
		else if (KeyboardInput[KEY_F2])
		{
			HUDTranslucencyLevel+=NormalFrameTime>>9;
			if (HUDTranslucencyLevel>255) HUDTranslucencyLevel=255;
		}
	}
	#endif
	/* KJL 10:55:32 10/9/97 - screen size */
	#if 0
	if(KeyboardInput[KEY_F3])
		MakeViewingWindowLarger();
	else if(KeyboardInput[KEY_F4])
		MakeViewingWindowSmaller();
	#endif
	#if 0
	if (DebouncedKeyboardInput[KEY_F3])
	{
		MessageHistory_DisplayPrevious();
	}
	#endif
    /* The open console owns ESC: collapse it without opening pause or
     * executing/submitting the current input line. */
    if (DebouncedKeyboardInput[KEY_ESCAPE] && IOFOCUS_AcceptTyping()) {
        IOFOCUS_Toggle();
        DebouncedKeyboardInput[KEY_ESCAPE]=0;
    }
    else if (DebouncedKeyboardInput[KEY_GRAVE]) IOFOCUS_Toggle();

    if(flameKeysActive)for(int i=0;i<6;i++){KeyboardInput[flameKeys[i]]=flameSavedKeys[i];DebouncedKeyboardInput[flameKeys[i]]=flameSavedKeys[6+i];}
    if (devEditKeys) {
        KeyboardInput[KEY_W]=devSavedKeys[0];KeyboardInput[KEY_S]=devSavedKeys[1];KeyboardInput[KEY_Z]=devSavedKeys[2];
        DebouncedKeyboardInput[KEY_W]=devSavedKeys[3];DebouncedKeyboardInput[KEY_S]=devSavedKeys[4];DebouncedKeyboardInput[KEY_Z]=devSavedKeys[5];
    }
}

void LoadKeyConfiguration(void)
{
	#if ALIEN_DEMO
	LoadAKeyConfiguration("alienavpkey.cfg");
	#else
	LoadAKeyConfiguration("avpkey.cfg");
	#endif

}

void SaveKeyConfiguration(void)
{
	#if ALIEN_DEMO
	SaveAKeyConfiguration("alienavpkey.cfg");
	#else
	SaveAKeyConfiguration("avpkey.cfg");
	#endif
}

void LoadAKeyConfiguration(char* Filename)
{
	#if 0
	FILE* file=fopen(Filename,"rb");
	if(!file)
	{
		MarineInputPrimaryConfig = DefaultMarineInputPrimaryConfig;
		MarineInputSecondaryConfig = DefaultMarineInputSecondaryConfig;
		PredatorInputPrimaryConfig = DefaultPredatorInputPrimaryConfig;
		PredatorInputSecondaryConfig = DefaultPredatorInputSecondaryConfig;
		AlienInputPrimaryConfig = DefaultAlienInputPrimaryConfig;
		AlienInputSecondaryConfig = DefaultAlienInputSecondaryConfig;
		return;
	}
	fread(&MarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&MarineInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&PredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&PredatorInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&AlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&AlienInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	fread(&ControlMethods,sizeof(CONTROL_METHODS),1,file);
	fread(&JoystickControlMethods,sizeof(JOYSTICK_CONTROL_METHODS),1,file);
	
	fclose(file);
	#endif
}

void SaveAKeyConfiguration(char* Filename)
{
	#if 0
	FILE* file=fopen(Filename,"wb");
	if(!file) return;

	fwrite(&MarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&MarineInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&PredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&PredatorInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&AlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&AlienInputSecondaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	fwrite(&ControlMethods,sizeof(CONTROL_METHODS),1,file);
	fwrite(&JoystickControlMethods,sizeof(JOYSTICK_CONTROL_METHODS),1,file);

	fclose(file);
	#endif
}

void SaveDefaultPrimaryConfigs(void)
{
	FILE *file = OpenGameFile("default.cfg", FILEMODE_WRITEONLY, FILETYPE_CONFIG);
	if(!file) return;

	fwrite(&DefaultMarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&DefaultPredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fwrite(&DefaultAlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	fclose(file);
}
void LoadDefaultPrimaryConfigs(void)
{
	FILE *file = OpenGameFile("default.cfg", FILEMODE_READONLY, FILETYPE_CONFIG);
	if(!file) return;

	fread(&DefaultMarineInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&DefaultPredatorInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);
	fread(&DefaultAlienInputPrimaryConfig,sizeof(PLAYER_INPUT_CONFIGURATION),1,file);

	/* default.cfg predates the reserved reload bind; migrate its unused byte. */
	DefaultMarineInputPrimaryConfig.ReloadWeapon = KEY_VOID;
	DefaultMarineInputPrimaryConfig.ReloadBindingProfileMarker = RELOAD_BINDING_PROFILE_MARKER;
	DefaultPredatorInputPrimaryConfig.ReloadWeapon = KEY_VOID;
	DefaultPredatorInputPrimaryConfig.ReloadBindingProfileMarker = RELOAD_BINDING_PROFILE_MARKER;

	fclose(file);
}
