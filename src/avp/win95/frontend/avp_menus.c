#include "screen_layout.h"
#include "event_latency.h"
#include "optimized_ammo.h"
#include "molecular_acid.h"
#include "campaign_state.h"
#include "tyrargo_instrumentation.h"
#include <math.h>
#include "menu_preview.h"
#include "crt_filter.h"
#include "menu_viewport.h"
#include "version.h"
#include "motion_params.h"
#include "shake_params.h"
#include <SDL3/SDL.h>
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "cheatmodes.h"
#include "net.h"
#include "opengl.h"
#include "pldnet.h"

#include "avp_menudata.h"
#include "avp_menus.h"
#include "camera_motion.h"
#include "dev_viewmodel.h"
#include "../text_encoding.h"
#include "avp_envinfo.h"

#include "hud_layout.h"
#include "d3d_hud.h"
#include "player_fov.h"
#include "avp_userprofile.h"
#include "huffman.hpp"

#include "hudgfx.h"
#include "usr_io.h"
#define UseLocalAssert Yes
#include "ourasert.h"
#include "iofocus.h"
#include <time.h>
#include "gammacontrol.h"
#include "avp_mp_config.h"
#include "psnd.h"
#include "savegame.h"
#include "game.h"
#include "avp_menugfx.hpp"
#include "avp_intro.h"
#include "fmv.h"
#include "water_effect.h"
#include "fmv_backend.h"

/* used to get file time */
#include <sys/types.h>
#include <sys/stat.h>

int SelectDirectDrawObject(void *pGUID);
                    
extern void StartMenuBackgroundBink(void);
extern int PlayMenuBackgroundBink(void);
extern void EndMenuBackgroundBink(void);


/* KJL 11:22:37 23/06/98 - Hopefully these will be the final menus! */

/* The custom FPS cap begins at 100; its first digit replaces that default. */
static int VideoFPSCapCustomEditStartsFresh;

extern int IDemandSelect(void);

extern char *GetVideoModeDescription(void);
extern char *GetVideoModeDescription2(void);
extern char *GetVideoModeDescription3(void);
extern char *GetVideoRendererDescription(void);
extern char *GetVideoAspectDescription(void);
extern char *GetVideoResolutionDescription(void);
extern char *GetVideoRefreshDescription(void);
extern char *GetVideoScreenModeDescription(void);
extern char *GetVideoVSyncDescription(void);
extern char *GetVideoFPSCapDescription(void);
extern void PreviousVideoMode(void);
extern void PreviousVideoMode2(void);
extern void NextVideoMode(void);
extern void NextVideoMode2(void);
extern void PreviousVideoAspect(void);
extern void NextVideoAspect(void);
extern void PreviousVideoRefresh(void);
extern void NextVideoRefresh(void);
extern void PreviousVideoScreenMode(void);
extern void NextVideoScreenMode(void);
extern void PreviousVideoVSync(void);
extern void NextVideoVSync(void);
extern void PreviousVideoFPSCap(void);
extern void NextVideoFPSCap(void);
extern int VideoFPSCapIsCustom(void);
extern void ResetVideoFPSCapToDefault(void);
extern int CurrentVideoFPSCapCustom;
extern void SaveVideoModeSettings(void);
extern void LoadDeviceAndVideoModePreferences(void);
extern void SaveDeviceAndVideoModePreferences(void);
extern int ScreenshotFormat;
extern int ApplySelectedVideoMode(void);
extern int RestartOpenGLVideoContextIfNeeded(void);

extern void MakeSelectSessionMenu(void);

extern void MakeInGameMenu(void);
extern void MakeMarineKeyConfigMenu(void);
extern void MakePredatorKeyConfigMenu(void);
extern void MakeAlienKeyConfigMenu(void);
extern void MakeUserProfileSelectMenu(void);

extern void SaveKeyConfiguration(void);
extern int MusicPlayMode;
extern int CRTFilterEnabled;


extern void D3D_DrawSliderBar(int x, int y, int alpha);
extern void D3D_DrawSlider(int x, int y, int alpha);
extern void D3D_FadeDownScreen(int brightness, int colour);
extern void PlayIntroSequence(void);
extern void FMVBackend_EnsureMenuMusic(void);
extern int MenuBackgroundType,MenuMusicMode,MenuMusic99Available;
extern void FMVBackend_RefreshMenuAssets(void);
extern int FMVBackend_DrawMenuBackground(void);

extern void MinimalNetCollectMessages(void);

extern int DirectPlay_HostGame(char *playerName, char *sessionName,int species,int gamestyle,int level);
extern int DirectPlay_JoinGame(void);
extern int DirectPlay_ConnectToSession(int sessionNumber, char *playerName);
extern int DirectPlay_Disconnect(void);

extern void ShowSplashScreens(void);
extern void Show_WinnerScreen(void);

extern void GetNextAllowedSpecies(int* species,BOOL search_forwards);
static void SetBriefingTextForMultiplayer();
int NumberOfAvailableLevels(I_PLAYER_TYPE playerID);
int LevelMostLikelyToPlay(I_PLAYER_TYPE playerID);
int MaxDifficultyLevelAllowed(I_PLAYER_TYPE playerID, int level);

int CloudTable[128][128];

extern char MP_Config_Name[];

void HandlePostGameFMVs(void);
void HandlePreGameFMVs(void);
extern void AvP_UpdateMenus(void);
static void SetupNewMenu(enum AVPMENU_ID menuID);
static void RenderMenu(void);
static void RenderBriefingScreenInfo(void);
/* Main-menu canvas units, scaled together with the existing menu. */
#define BRIEFING_HEADER_LIFT 54
static int AcidMenuTier=1;
extern void RenderAcidMenuText(char*,int,int,int);
extern void RenderCampaignModeText(char*,int,int,int,int);
static void RenderEpisodeSelectMenu(void);
static void RenderKeyConfigurationMenu(void);
static void RenderUserProfileSelectMenu(void);
static void RenderLoadGameMenu(void);
static int ScaleInGameMenuTextX(int x);
static int Hardware_RenderScaledInGameMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format);
static int Hardware_RenderScaledInGameMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue);
static int InGameMenuTextUsesNativeAnchors = 0;
static void RenderConfigurationDescriptionString();
static void ActUponUsersInput(void);
static void InteractWithMenuElement(enum AVPMENU_ELEMENT_INTERACTION_ID interactionID);
static void ResumeGameFromMenu(void);
/* ENTER restores the requested per-control defaults in both menu contexts. */
static int ResetRequestedTextSliderOnSelect(AVPMENU_ELEMENT *elementPtr)
{
	switch (elementPtr->a.TextDescription)
	{
		case TEXTSTRING_CROUCH_TOGGLE_NEW:
		case TEXTSTRING_UNLIMITED_SAVES_NEW:
		case TEXTSTRING_SMART_JETPACK_NEW:
		case TEXTSTRING_MARINE_CROSSHAIR_THICKNESS_NEW:
		case TEXTSTRING_VERTICAL_PITCH_NEW:
		case TEXTSTRING_HUD_TEXTURE_FILTER_NEW:
		case TEXTSTRING_NOISE_SCALE_NEW:
			*elementPtr->c.SliderValuePtr = 0;
			return 1;
		case TEXTSTRING_COLOR_BIT_DEPTH_NEW:
			*elementPtr->c.SliderValuePtr = OPENGL_COLOR_BIT_DEPTH_32;
			return 1;
		case TEXTSTRING_ANTIALIASING_NEW:
                *elementPtr->c.SliderValuePtr = 0;
                return 1;
		case TEXTSTRING_ANISOTROPY_NEW:
                *elementPtr->c.SliderValuePtr = 0;
                return 1;
		case TEXTSTRING_TEXTURE_FILTER_NEW:
                *elementPtr->c.SliderValuePtr = 1;
                return 1;
		case TEXTSTRING_WEAPON_FOV_STYLE_NEW:
			*elementPtr->c.SliderValuePtr = WEAPON_FOV_STYLE_STRETCHED;
			return 1;
		case TEXTSTRING_FMV_FILTER_NEW:
            *elementPtr->c.SliderValuePtr=1;
            SaveDeviceAndVideoModePreferences();
            return 1;
		case TEXTSTRING_FMV_SCALE_NEW:
			*elementPtr->c.SliderValuePtr = FMV_SCALE_ULTRAWIDE_X25;
			return 1;
		default:
			return 0;
	}
}

static int VanillaSliderUsesLiveCounter(int textDescription)
{
	switch (textDescription)
	{
		case TEXTSTRING_MOUSECONTROLS_YSENSITIVITY:
		case TEXTSTRING_MOUSECONTROLS_XSENSITIVITY:
		case TEXTSTRING_TRACKERBALL_VERTICAL_SENSITIVITY:
		case TEXTSTRING_TRACKERBALL_HORIZONTAL_SENSITIVITY:
		case TEXTSTRING_AVOPTIONS_EFFECTSVOLUME:
		case TEXTSTRING_AVOPTIONS_MUSICVOLUME:
		case TEXTSTRING_AVOPTIONS_CDVOLUME:
		case TEXTSTRING_AVOPTIONS_GAMMASETTING:
        case TEXTSTRING_IMAGE_BRIGHTNESS_NEW:
        case TEXTSTRING_IMAGE_CONTRAST_NEW:
        case TEXTSTRING_IMAGE_SATURATION_NEW:
			return 1;
		default:
			return 0;
	}
}

static void FormatVanillaSliderCounter(const AVPMENU_ELEMENT *elementPtr, char *buffer, size_t bufferSize)
{
	int value = *elementPtr->c.SliderValuePtr;
	int maximum = elementPtr->b.MaxSliderValue;
	switch (elementPtr->a.TextDescription)
	{
        case TEXTSTRING_IMAGE_BRIGHTNESS_NEW: snprintf(buffer,bufferSize,"%+d%%",value-50);break;
        case TEXTSTRING_IMAGE_CONTRAST_NEW:
        case TEXTSTRING_IMAGE_SATURATION_NEW: snprintf(buffer,bufferSize,"%d%%",value);break;
		case TEXTSTRING_AVOPTIONS_EFFECTSVOLUME:
		case TEXTSTRING_AVOPTIONS_MUSICVOLUME:
		case TEXTSTRING_AVOPTIONS_CDVOLUME:
			if (maximum > 0) snprintf(buffer, bufferSize, "%d%%", (value * 100 + maximum / 2) / maximum);
			else snprintf(buffer, bufferSize, "0%%");
			break;
		default:
			snprintf(buffer, bufferSize, "%d", value);
			break;
	}
}
extern void SetMenuAVContrast(int mode);
static void RenderMenuElement(AVPMENU_ELEMENT *elementPtr, int e, int y);
static int HeightOfMenuElement(AVPMENU_ELEMENT *elementPtr);
static int MenuElementIsVisible(AVPMENU_ELEMENT *elementPtr);
static int HUDPositionForAspect(int numerator, int denominator);
static void HUDPositionDescription(char *buffer, int bufferSize);
void DisplayVideoModeUnavailableScreen(void);
void CheckForCredits(void);
void DoCredits(void);
BOOL RollCreditsText(int position, unsigned char *textPtr);
extern void SelectMenuDisplayMode(void);
static void InitMainMenusBackdrop(void);
extern void DrawMainMenusBackdrop(void);
extern void RenderFrontendScreenModeNotice(void);
static void TestValidityOfCheatMenu(void);
void SetBriefingTextForEpisode(int episode, I_PLAYER_TYPE playerID);
void SetBriefingTextToBlank(void);
void CheckForKeysWithMultipleAssignments(void);
void HandleCheatModeFeatures(void);
extern int FrameRate;
int FPSDisplayPosition = FPS_DISPLAY_OFF;
void ShowMenuFrameRate(void);
static void KeyboardEntryQueue_Clear(void);
static void KeyboardEntryQueue_StartProcessing(void);

static void PasteFromClipboard(char* Text,int MaxTextLength);
/* KJL 11:23:03 23/06/98 - Requirements

	1.      'Floating' over backdrop, possibly using translucency effects
	2.      Quick to write!
	3.      Quick to change
	4.      Certain menus need to be used in-game - e.g. key configuration
*/

/* KJL 11:27:18 23/06/98 - Available menus

	1.      Main Menu
		
		Start Singleplayer
		Start Multiplayer 
		Gameplay Options
		Audio/Visual Options
		Exit
						  
	2.      Singleplayer
		
		Alien
		Marine
		Predator
		Exit

	3.      Multiplayer

		-> Complex

	4.      Gameplay Options

	5.      Audio/Visual Options
		
 */


/* KJL 11:57:21 23/06/98 - 


	Need mouse position code (?) etc. In-game menus using mouse? Ugh.
	No mouse then.

*/
SAVE_SLOT_HEADER SaveGameSlot[NUMBER_OF_SAVE_SLOTS];


extern int VideoModeNotAvailable;

extern int RealFrameTime;
extern unsigned char KeyboardInput[];
extern unsigned char DebouncedKeyboardInput[];
extern int DebouncedGotAnyKey;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int TimeScale;

static AVP_MENUS AvPMenus;

/* The software frontend has a legacy canvas or an expanded settings canvas. */
int AvP_IsSoftwareMenuActive(void)
{
	return (AvPMenus.MenusState == MENUSSTATE_MAINMENUS);
}
extern AVPMENU  AvPMenusData[];

extern int AlienEpisodeToPlay;
extern int MarineEpisodeToPlay;
extern int PredatorEpisodeToPlay;
static int RememberedEpisode(I_PLAYER_TYPE species){
 int max=NumberOfAvailableLevels(species);
 int selected=UserProfilePtr?((int)UserProfilePtr->LastSelectedEpisode[species]-1):-1;
 if(selected<0 || selected>max)selected=LevelMostLikelyToPlay(species);
 if(selected<0)selected=0;if(selected>max)selected=max;
 return selected;
}
static int EpisodeSelectionDirty;
void AvP_FlushEpisodeSelection(void){
 if(EpisodeSelectionDirty && UserProfilePtr){
  SaveUserProfile(UserProfilePtr);
  EpisodeSelectionDirty=0;
 }
}
static int IsEpisodeSelector(enum AVPMENU_ID menu){
 return menu==AVPMENU_MARINELEVELS || menu==AVPMENU_PREDATORLEVELS || menu==AVPMENU_ALIENLEVELS;
}
static void RememberEpisode(I_PLAYER_TYPE species,int episode){
 if(!UserProfilePtr || species<0 || species>2 || episode<0 || episode>254)return;
 unsigned char value=(unsigned char)(episode+1);
 if(UserProfilePtr->LastSelectedEpisode[species]!=value){
  UserProfilePtr->LastSelectedEpisode[species]=value;
  EpisodeSelectionDirty=1;
  /* Browsing must never synchronously rewrite the profile every arrow press.
   * Other callers (load/automatic advance) commit outside the carousel. */
  if(!IsEpisodeSelector(AvPMenus.CurrentMenu))AvP_FlushEpisodeSelection();
 }
}

extern int UserProfileNumber;
extern SDL_Window *window;
AVP_USER_PROFILE *UserProfilePtr;

SESSION_DESC SessionData[MAX_NO_OF_SESSIONS];
int NumberOfSessionsFound;
extern NETGAME_GAMEDATA netGameData;

static int InputIsDebounced = 0;
static int KeyDepressedCounter = 0;
extern int CloakingPhase;
static int EpisodeSelectScrollOffset;
static int MaximumSelectableLevel;
static int KeyConfigSelectionColumn;

static int Brightness[16];

#define MAX_KEYCONFIG_BINDINGS (NUMBER_OF_PREDATOR_INPUTS+2+NUMBER_OF_MARINE_WEAPON_BINDS+NUMBER_OF_GLOBAL_COMMAND_BINDINGS)
static unsigned char MultipleAssignments[2][MAX_KEYCONFIG_BINDINGS];

static char *GetDescriptionOfKey(unsigned char key);
static int KeyConfigVisibleBindingIndex(int menuElementIndex);
static int KeyConfigMenuElementIndexForBinding(int bindingIndex);
static void NormalizeMarineJetpackBindings(PLAYER_INPUT_CONFIGURATION *primaryInput, PLAYER_INPUT_CONFIGURATION *secondaryInput);
static void NormalizePredatorSmartZoomDisabledBindings(PLAYER_INPUT_CONFIGURATION *primaryInput, PLAYER_INPUT_CONFIGURATION *secondaryInput);
/* Smart Zoom is the only mode that authorises its zoom keys to share bindings
 * with these four Predator actions. Once disabled, retain the zoom keys,
 * clear every shared action, then make all remaining listed slots unique. */
static void NormalizePredatorSmartZoomDisabledBindings(PLAYER_INPUT_CONFIGURATION *primaryInput, PLAYER_INPUT_CONFIGURATION *secondaryInput)
{
	unsigned char *zoomBindings[4] = {
		&primaryInput->f.ZoomIn, &secondaryInput->f.ZoomIn,
		&primaryInput->g.ZoomOut, &secondaryInput->g.ZoomOut
	};
	unsigned char *actionBindings[8] = {
		&primaryInput->a.NextWeapon, &primaryInput->b.PreviousWeapon,
		&primaryInput->d.Cloak, &primaryInput->e.CycleVisionMode,
		&secondaryInput->a.NextWeapon, &secondaryInput->b.PreviousWeapon,
		&secondaryInput->d.Cloak, &secondaryInput->e.CycleVisionMode
	};
	int i;
	int j;

	/* Zoom holding has no meaning outside Smart Zoom and is no longer published. */
	primaryInput->ReloadWeapon=KEY_VOID;
	secondaryInput->ReloadWeapon=KEY_VOID;

	/* Zoom in/out revert to ordinary, non-repeating bindings. */
	for (i=0; i<4; i++)
	{
		if (*zoomBindings[i]==KEY_VOID) continue;
		for (j=0; j<i; j++)
		{
			if (*zoomBindings[i]==*zoomBindings[j])
			{
				*zoomBindings[i]=KEY_VOID;
				break;
			}
		}
	}

	/* An action that used a zoom key loses that shared assignment. Any other
	 * duplicate among the four actions is also cleared deterministically. */
	for (i=0; i<8; i++)
	{
		int sharedWithZoom=0;
		if (*actionBindings[i]==KEY_VOID) continue;
		for (j=0; j<4; j++)
		{
			if (*actionBindings[i]==*zoomBindings[j])
			{
				sharedWithZoom=1;
				break;
			}
		}
		if (sharedWithZoom)
		{
			*actionBindings[i]=KEY_VOID;
			continue;
		}
		for (j=0; j<i; j++)
		{
			if (*actionBindings[i]==*actionBindings[j])
			{
				*actionBindings[i]=KEY_VOID;
				break;
			}
		}
	}
}
extern int SmartZoomBindingRowPublished;

static int KeyConfigActionRowCount(void);
static int KeyConfigWeaponRowCount(void);
static int KeyConfigGlobalCommandRowCount(void);
static int KeyConfigWeaponSlotForRow(int weaponRow);
static int KeyConfigWeaponSlotForRow(int weaponRow)
{
	static const unsigned char MarineSlots[NUMBER_OF_MARINE_WEAPON_BINDS] = {7,8,0,1,5,2,4,3,6};
	static const unsigned char PredatorSlots[NUMBER_OF_PREDATOR_WEAPON_BINDS] = {0,1,2,5,4,3};

	switch (AvPMenus.CurrentMenu)
	{
		case AVPMENU_MARINEKEYCONFIG: return (weaponRow<NUMBER_OF_MARINE_WEAPON_BINDS) ? MarineSlots[weaponRow] : -1;
		case AVPMENU_PREDATORKEYCONFIG: return (weaponRow<NUMBER_OF_PREDATOR_WEAPON_BINDS) ? PredatorSlots[weaponRow] : -1;
		case AVPMENU_ALIENKEYCONFIG: return -1;
		default: return -1;
	}
}
static unsigned char *KeyConfigBindingPointer(int primary, int menuElementIndex);
static int OkayToPlayNextEpisode(void);
extern int LengthOfMenuText(char *textPtr);
/* Clip only the label, never the stored name, also on the delete prompt. */
static char *ProfileMenuLabel(const char *name)
{
    static char label[MAX_SIZE_OF_USERS_NAME+4];
    size_t n=strlen(name);
    if(n>MAX_SIZE_OF_USERS_NAME)n=MAX_SIZE_OF_USERS_NAME;
    memcpy(label,name,n);label[n]=0;
    while(n && LengthOfMenuText(label)>MENU_RIGHTXEDGE-MENU_LEFTXEDGE-20)
    {
        --n;
        /* Do not split a UTF-8 suffix in an imported profile name. */
        while(n && ((unsigned char)name[n]&0xc0)==0x80)--n;
        memcpy(label,name,n);label[n]=0;strcat(label,"...");
    }
    return label;
}
static unsigned char PlayerPreviousVisionBindings[2];
static PLAYER_INPUT_CONFIGURATION PlayerInputPrimaryConfig;
static PLAYER_INPUT_CONFIGURATION PlayerInputSecondaryConfig;
static WEAPON_INPUT_CONFIGURATION PlayerWeaponPrimaryConfig;
static WEAPON_INPUT_CONFIGURATION PlayerWeaponSecondaryConfig;
static GLOBAL_COMMAND_INPUT_CONFIGURATION PlayerGlobalCommandPrimaryConfig;
static GLOBAL_COMMAND_INPUT_CONFIGURATION PlayerGlobalCommandSecondaryConfig;
CONTROL_METHODS PlayerControlMethods;
JOYSTICK_CONTROL_METHODS PlayerJoystickControlMethods;

static void RenderScrollyMenu();
static void RenderHelpString();
static int MenuPreviewAmount,MenuPreviewTarget;
static double MenuPreviewProgress;
static double MenuColourBarsProgress=1.0;
static int MenuColourBarsAlpha=ONE_FIXED;
static int MenuAVScrollFirst;
float MenuPreviewBandLow=-1.0f,MenuPreviewBandHigh=-1.0f;
static Uint64 MenuPreviewLastTick;

static int MenuPreviewThicknessSelected(void)
{
 if(AvPMenus.MenusState!=MENUSSTATE_INGAMEMENUS || AvPMenus.CurrentMenu!=AVPMENU_INGAMEAVOPTIONS || AvP.PlayerType!=I_Marine)return 0;
 if(AvPMenus.CurrentlySelectedElement<0 || AvPMenus.CurrentlySelectedElement>=AvPMenus.NumberOfElementsInMenu)return 0;
 AVPMENU_ELEMENT *e=&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
 return e->ElementID==AVPMENU_ELEMENT_TEXTSLIDER && e->c.SliderValuePtr==&MarineCrosshairThicknessMode;
}
static int MenuPreviewSelected(void)
{
 AVPMENU_ELEMENT *e;
 if(AvP.Network!=I_No_Network && !netGameData.skirmishMode)return 0;
 if(MenuPreviewThicknessSelected())return 1;
 if(AvPMenus.MenusState!=MENUSSTATE_INGAMEMENUS || AvPMenus.CurrentMenu!=AVPMENU_INGAMEAVOPTIONS)return 0;
 if(AvPMenus.CurrentlySelectedElement<0||AvPMenus.CurrentlySelectedElement>=AvPMenus.NumberOfElementsInMenu)return 0;
 e=&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
 return e->ElementID==AVPMENU_ELEMENT_FOV || e->ElementID==AVPMENU_ELEMENT_WEAPON_FOV ||
  e->ElementID==AVPMENU_ELEMENT_HUD_POSITION || e->ElementID==AVPMENU_ELEMENT_HUD_SCALE ||
  e->ElementID==AVPMENU_ELEMENT_CROSSHAIR_SCALE || e->ElementID==AVPMENU_ELEMENT_HUD_ANCHORED ||
  (e->ElementID==AVPMENU_ELEMENT_TEXTSLIDER &&
   (e->c.SliderValuePtr==&WeaponFOVStyle || e->c.SliderValuePtr==&MonitorNoSignalEffectMode ||
    e->c.SliderValuePtr==&FPSDisplayPosition || e->c.SliderValuePtr==&CRTFilterEnabled || e->c.SliderValuePtr==&WaterEffectMode)) ||
  (e->ElementID==AVPMENU_ELEMENT_SLIDER && (e->c.SliderValuePtr==&RequestedGammaSetting ||
   e->c.SliderValuePtr==&ImageBrightness || e->c.SliderValuePtr==&ImageContrast || e->c.SliderValuePtr==&ImageSaturation));
}
int AvP_MenuIsActive(void)
{
 return AvPMenus.MenusState==MENUSSTATE_MAINMENUS || AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS;
}
int AvP_MenuPreviewHideCrosshair(void)
{
 /* Paused scene never supplies an aiming reticle. The selected scale control
  * (or Marine stroke editor) draws its own sample below the row. */
 return AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS;
}
void AvP_UpdateMenuPreview(void)
{
 Uint64 now=SDL_GetTicks(),dt=MenuPreviewLastTick?now-MenuPreviewLastTick:0;
 MenuPreviewBandLow=MenuPreviewBandHigh=-1.0f;MenuPreviewLastTick=now;MenuPreviewTarget=MenuPreviewSelected();
 if(AvPMenus.MenusState!=MENUSSTATE_INGAMEMENUS){MenuPreviewAmount=MenuPreviewTarget=0;MenuPreviewProgress=0;MenuColourBarsProgress=1;MenuColourBarsAlpha=ONE_FIXED;return;}
 if(dt>100)dt=100;
 {
  int hideBars=0;
  if(AvPMenus.CurrentMenu==AVPMENU_INGAMEAVOPTIONS && AvPMenus.CurrentlySelectedElement>=0 &&
     AvPMenus.CurrentlySelectedElement<AvPMenus.NumberOfElementsInMenu){
   int id=AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement].ElementID;
   /* Every scene preview except the four colour controls needs an unobstructed view. */
   hideBars=MenuPreviewSelected() && id!=AVPMENU_ELEMENT_SLIDER;
  }
  MenuColourBarsProgress+=(hideBars?-1.0:1.0)*(double)dt/1600.0;
  if(MenuColourBarsProgress<0)MenuColourBarsProgress=0;
  if(MenuColourBarsProgress>1)MenuColourBarsProgress=1;
  MenuColourBarsAlpha=(int)(ONE_FIXED*MenuColourBarsProgress*MenuColourBarsProgress*(3.0-2.0*MenuColourBarsProgress)+0.5);
 }
 MenuPreviewProgress+=(MenuPreviewTarget?1.0:-1.0)*(double)dt/1600.0;
 if(MenuPreviewProgress<0)MenuPreviewProgress=0;if(MenuPreviewProgress>1)MenuPreviewProgress=1;
 MenuPreviewAmount=(int)(ONE_FIXED*MenuPreviewProgress*MenuPreviewProgress*(3.0-2.0*MenuPreviewProgress)+0.5);
}
int AvP_MenuPreviewScene(void){return MenuPreviewTarget || MenuPreviewAmount>0;}
static Uint64 TyrargoMenuVersionStart;
static int TyrargoMenuVersionStarted;
/* SetupNewMenu selects one of four root variants from the profile/debug state. */
static int TyrargoMainMenu(void)
{
 if(AvPMenus.MenusState!=MENUSSTATE_MAINMENUS)return 0;
 switch(AvPMenus.CurrentMenu){
 case AVPMENU_MAIN:
 case AVPMENU_MAIN_WITHCHEATS:
 case AVPMENU_DEBUG_MAIN:
 case AVPMENU_DEBUG_MAIN_WITHCHEATS:return 1;
 default:return 0;
 }
}
static int TyrargoVersionMenu(void)
{
 return TyrargoMainMenu() ||
        (AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS && (AvPMenus.CurrentMenu==AVPMENU_INGAME || AvPMenus.CurrentMenu==AVPMENU_INNETGAME));
}
static int TyrargoVersionReserve(void)
{
 int scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
 return TyrargoVersionMenu()?2*MUL_FIXED(17,scale)+10:0;
}
static void RenderTyrargoVersion(void)
{
 int alpha,scale,line,y,ingame;
 if(!TyrargoVersionMenu())return;
 if(!TyrargoMenuVersionStarted){TyrargoMenuVersionStart=SDL_GetTicks();TyrargoMenuVersionStarted=1;}
 /* Match idle pause entries exactly; fade towards their existing grey. */
 alpha=MUL_FIXED(TyrargoVersionOpacity(SDL_GetTicks()-TyrargoMenuVersionStart),BRIGHTNESS_OF_DARKENED_ELEMENT);
 if(!alpha)return;
 ingame=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS;
 scale=ingame?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
 line=MUL_FIXED(17,scale);y=ScreenDescriptorBlock.SDB_Height-16-2*line;
 if(ingame){
  D3D_SetInGameMenuTextRenderActive(1);
  Hardware_RenderSmallMenuText(TYRARGO_PROJECT_NAME,ScreenDescriptorBlock.SDB_Width/2,y,alpha,AVPMENUFORMAT_CENTREJUSTIFIED);
  Hardware_RenderSmallMenuText(TYRARGO_VERSION_LABEL,ScreenDescriptorBlock.SDB_Width/2,y+line,alpha,AVPMENUFORMAT_CENTREJUSTIFIED);
  D3D_SetInGameMenuTextRenderActive(0);
 }else{
  RenderSmallMenuText(TYRARGO_PROJECT_NAME,MENU_CENTREX,y,alpha,AVPMENUFORMAT_CENTREJUSTIFIED);
  RenderSmallMenuText(TYRARGO_VERSION_LABEL,MENU_CENTREX,y+line,alpha,AVPMENUFORMAT_CENTREJUSTIFIED);
 }
}
static void CheckForLoadGame();

static void UpdateMultiplayerConfigurationMenu();

static char KeyboardEntryQueue_ProcessCharacter(void);

static BOOL LaunchingMplayer=FALSE;

static int MultiplayerConfigurationIndex; //just used for the configuration deletion stuff
static const char* MultiplayerConfigurationName=0; //ditto

extern int DebuggingCommandsActive;

/* Key-config menu rows include text sliders. Map only actual bind rows to the
   packed configuration structs so options never shift a binding. */
static int KeyConfigVisibleBindingIndex(int menuElementIndex)
{
	int i;
	int bindingIndex=0;
	for (i=0; i<menuElementIndex; i++)
	{
		if (AvPMenus.MenuElements[i].ElementID==AVPMENU_ELEMENT_KEYCONFIG) bindingIndex++;
	}
	return bindingIndex;
}

static int KeyConfigMenuElementIndexForBinding(int bindingIndex)
{
	int i;
	int current=0;
	for (i=0; i<AvPMenus.NumberOfElementsInMenu; i++)
	{
		if (AvPMenus.MenuElements[i].ElementID==AVPMENU_ELEMENT_KEYCONFIG)
		{
			if (current==bindingIndex) return i;
			current++;
		}
	}
	return -1;
}

static void NormalizeMarineJetpackBindings(PLAYER_INPUT_CONFIGURATION *primaryInput, PLAYER_INPUT_CONFIGURATION *secondaryInput)
{
	/* Smart Jetpack uses a dedicated toggle key; Jump must always stay distinct. */
	if (primaryInput->f.Jetpack!=KEY_VOID
		&& (primaryInput->f.Jetpack==primaryInput->Jump || primaryInput->f.Jetpack==secondaryInput->Jump))
		primaryInput->f.Jetpack=KEY_VOID;
	if (secondaryInput->f.Jetpack!=KEY_VOID
		&& (secondaryInput->f.Jetpack==primaryInput->Jump || secondaryInput->f.Jetpack==secondaryInput->Jump))
		secondaryInput->f.Jetpack=KEY_VOID;
}

extern int SmartZoomBindingRowPublished;

static int KeyConfigActionRowCount(void)
{
	switch (AvPMenus.CurrentMenu)
	{
		case AVPMENU_MARINEKEYCONFIG: return NUMBER_OF_MARINE_INPUTS;
		case AVPMENU_PREDATORKEYCONFIG: return NUMBER_OF_PREDATOR_INPUTS+SmartZoomBindingRowPublished+1;
		case AVPMENU_ALIENKEYCONFIG: return NUMBER_OF_ALIEN_INPUTS;
		default: return 0;
	}
}

static int KeyConfigWeaponRowCount(void)
{
	switch (AvPMenus.CurrentMenu)
	{
		case AVPMENU_MARINEKEYCONFIG: return NUMBER_OF_MARINE_WEAPON_BINDS;
		case AVPMENU_PREDATORKEYCONFIG: return NUMBER_OF_PREDATOR_WEAPON_BINDS;
		case AVPMENU_ALIENKEYCONFIG: return NUMBER_OF_ALIEN_WEAPON_BINDS;
		default: return 0;
	}
}

static int KeyConfigGlobalCommandRowCount(void)
{
	switch (AvPMenus.CurrentMenu)
	{
		case AVPMENU_MARINEKEYCONFIG:
		case AVPMENU_PREDATORKEYCONFIG:
		case AVPMENU_ALIENKEYCONFIG:
			return NUMBER_OF_GLOBAL_COMMAND_BINDINGS;
		default: return 0;
	}
}

#define PREDATOR_SMART_ZOOM_HOLDING_BIND_INDEX 23

static unsigned char *KeyConfigBindingPointer(int primary, int menuElementIndex)
{
	int visibleIndex = KeyConfigVisibleBindingIndex(menuElementIndex);
	PLAYER_INPUT_CONFIGURATION *inputConfig = primary ? &PlayerInputPrimaryConfig : &PlayerInputSecondaryConfig;
	WEAPON_INPUT_CONFIGURATION *weaponConfig = primary ? &PlayerWeaponPrimaryConfig : &PlayerWeaponSecondaryConfig;

    if (visibleIndex < 0) return 0;
    if (visibleIndex < KeyConfigActionRowCount())
    {
        /* Map Predator actions by identity, independently of their display order. */
        if (AvPMenus.CurrentMenu==AVPMENU_PREDATORKEYCONFIG)
        {
            int label=AvPMenus.MenuElements[menuElementIndex].a.TextDescription;
            if (label==TEXTSTRING_PREDATOR_PREVIOUS_VISION_NEW)
                return &PlayerPreviousVisionBindings[primary?0:1];
            if (label==TEXTSTRING_PREDATOR_ZOOM_HOLDING_NEW)
                return &inputConfig->ReloadWeapon;
            if (label>=TEXTSTRING_PREDATOR_KEY_FORWARD &&
                label<TEXTSTRING_PREDATOR_KEY_FORWARD+NUMBER_OF_PREDATOR_INPUTS)
                return ((unsigned char *)inputConfig)+(label-TEXTSTRING_PREDATOR_KEY_FORWARD);
            return 0;
        }
		if (AvPMenus.CurrentMenu==AVPMENU_MARINEKEYCONFIG
			&& visibleIndex>=19)
		{
			if (visibleIndex==19) return &inputConfig->ReloadWeapon;
			return ((unsigned char *)inputConfig)+(visibleIndex-1);
		}
		return ((unsigned char *)inputConfig)+visibleIndex;
	}

	visibleIndex -= KeyConfigActionRowCount();
	if (visibleIndex < KeyConfigWeaponRowCount())
	{
		int weaponSlot=KeyConfigWeaponSlotForRow(visibleIndex);
		if (weaponSlot>=0) return &weaponConfig->Weapon[weaponSlot];
	}
	visibleIndex -= KeyConfigWeaponRowCount();
	if (visibleIndex < KeyConfigGlobalCommandRowCount())
	{
		GLOBAL_COMMAND_INPUT_CONFIGURATION *globalConfig = primary
			? &PlayerGlobalCommandPrimaryConfig : &PlayerGlobalCommandSecondaryConfig;
		return ((unsigned char *)globalConfig)+visibleIndex;
	}
	return 0;
}
extern void SetFrontendModalFramesActive(int active);

int AvP_MainMenus(void)
{
 SetFrontendModalFramesActive(0);
 EventLatencyFrameBegin("[frontend setup]",-1);
	/* Tyrargo defaults are compiled from the calibrated profile. Legacy
	 * default.cfg must not silently replace them each time this menu opens.
	 * Saved user profiles continue to restore their own bindings. */
	
	SoundSys_ResetFadeLevel();
	SoundSys_Management();	
	
	TimeScale = ONE_FIXED;



 	if (!LobbiedGame)  // Edmond
		CheckForCredits();
	
	TimeStampedMessage("start of menus");
	// open a 640x480x16 screen     
	{ EVENT_TICK stamp=EventLatencyBegin(); SelectMenuDisplayMode(); EventLatencyEnd(stamp,"frontend display mode",0,0,0,0,0); }
	TimeStampedMessage("after SelectMenuDisplayMode");

	
	


	InitialiseMenuGfx();
	TimeStampedMessage("after InitialiseMenuGfx");

	// inform backdrop code
	InitMainMenusBackdrop();
	TimeStampedMessage("after InitMainMenusBackdrop");


	#if PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO
	if (AvP.LevelCompleted)
	{
		Show_WinnerScreen();
	}
	#endif

	{ EVENT_TICK stamp=EventLatencyBegin(); LoadAllAvPMenuGfx(); EventLatencyEnd(stamp,"frontend load graphics",0,0,0,0,0); }
	TimeStampedMessage("after LoadAllAvPMenuGfx");



	ResetFrameCounter();
 EventLatencyFrameEnd();

 	if (!LobbiedGame)	// Edmond
		PlayIntroSequence();
	
 EventLatencyFrameBegin("[frontend entry]",-1);
	if (VideoModeNotAvailable)
	{	
		LoadGameRequest = SAVELOAD_REQUEST_NONE;
		DisplayVideoModeUnavailableScreen();
	}
	VideoModeNotAvailable = 0;

    /* Play a completed campaign's ending on every run, including replay/debug.
       Keep the original restrictions on recording progression below. */
    if(AvP.LevelCompleted && AvP.Network==I_No_Network && !LobbiedGame)
        HandlePostGameFMVs();
	if(AvP.LevelCompleted && AvP.Network==I_No_Network && !LobbiedGame && CheatMode_Active == CHEATMODE_NONACTIVE)
	{
		AvPMenus.MenusState = MENUSSTATE_MAINMENUS;
		OkayToPlayNextEpisode();
		AvP.LevelCompleted = 0;
	}
	else if(!LobbiedGame)
	{
		if (UserProfileNumber==-1)
		{
			SetupNewMenu(AVPMENU_USERPROFILESELECT);
		}
		else
		{
			SetupNewMenu(AVPMENU_MAIN);
		}
		AvPMenus.MenusState = MENUSSTATE_MAINMENUS;
	}
	else
	{
 		SetupNewMenu(AVPMENU_USERPROFILESELECT);
	  //AvPMenus.MenusState = MENUSSTATE_STARTGAME;	
		
	}

	CheatMode_Active = CHEATMODE_NONACTIVE;
	

	TimeStampedMessage("starting general menus");

/* Gameplay returns can bypass the logo path; only reopen menu music when inactive. */
FMVBackend_RefreshMenuAssets();
FMVBackend_EnsureMenuMusic();

EventLatencyFrameEnd();
while(AvPMenus.MenusState == MENUSSTATE_MAINMENUS)
	{
 EventLatencyFrameBegin("[frontend]",-1);
 EventLatencyMenu(AvPMenus.CurrentMenu,AvPMenus.CurrentlySelectedElement);
		{ EVENT_TICK stamp=EventLatencyBegin();
          /* SetupNewMenu has completed. Scope permission to this event pump,
           * never the intro, display setup, credits or graphics teardown. */
          SetFrontendModalFramesActive(1);
          CheckForWindowsMessages();
          SetFrontendModalFramesActive(0);
          EventLatencyEnd(stamp,"frontend window events",0,0,0,0,0); }
		{ EVENT_TICK stamp=EventLatencyBegin(); DrawMainMenusBackdrop(); EventLatencyEnd(stamp,"frontend backdrop",0,0,0,0,0); }
		{ EVENT_TICK stamp=EventLatencyBegin(); ReadUserInput(); EventLatencyEnd(stamp,"frontend input",0,0,0,0,0); }
		AvP_UpdateMenus();
        RenderFrontendScreenModeNotice();
   //	BezierCurve();
		
		ShowMenuFrameRate();

		{ EVENT_TICK stamp=EventLatencyBegin(); FlipBuffers(); EventLatencyEnd(stamp,"frontend present / swap",0,0,0,0,0); }
		{ EVENT_TICK stamp=EventLatencyBegin(); FrameCounterHandler(); EventLatencyEnd(stamp,"frontend frame limiter",0,0,0,0,0); }
		{ EVENT_TICK stamp=EventLatencyBegin(); PlayMenuMusic(); EventLatencyEnd(stamp,"frontend music",0,0,0,0,0); }
		#if 0
		{
			extern int EffectsSoundVolume;
			SoundSys_ChangeVolume(EffectsSoundVolume);
		}
		#endif
		{ EVENT_TICK stamp=EventLatencyBegin(); SoundSys_Management(); EventLatencyEnd(stamp,"frontend audio",0,0,0,0,0); }
		{ EVENT_TICK stamp=EventLatencyBegin(); UpdateGammaSettings(); EventLatencyEnd(stamp,"frontend gamma",0,0,0,0,0); }

		{ EVENT_TICK stamp=EventLatencyBegin(); CheckForLoadGame(); EventLatencyEnd(stamp,"frontend load request",0,0,0,0,0); }
 EventLatencyFrameEnd();
	}
	
	if (AvPMenus.MenusState==MENUSSTATE_OUTSIDEMENUS)
	{
		//Don't bother showing credits if we are just exiting in order to start a game
		//using mplayer. The credits will get shown later after the player has actually
		//played the game
		if(!LaunchingMplayer)
		{
 			if (!LobbiedGame)	// Edmond
				DoCredits();
		}
	}
 EventLatencyFrameBegin("[frontend exit]",-1);
	AvP_FlushEpisodeSelection();
	TimeStampedMessage("ready to exit menus");
	
	EndMenuMusic();
	EndMenuBackgroundBink();
	TimeStampedMessage("after EndMenuMusic");

	#if PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO
	if ((AvPMenus.MenusState != MENUSSTATE_STARTGAME)) ShowSplashScreens();
	TimeStampedMessage("after ShowSplashScreens");
	#endif
	ReleaseAllAvPMenuGfx();

	TimeStampedMessage("after ReleaseAllAvPMenuGfx");

	SoundSys_StopAll();
	SoundSys_Management();

 EventLatencyFrameEnd();
 EventLatencyFlush();
	if (CheatMode_Active == CHEATMODE_NONACTIVE) HandlePreGameFMVs();

	HandleCheatModeFeatures();
	AvP.LevelCompleted = 0;

	return (AvPMenus.MenusState == MENUSSTATE_STARTGAME);
}
void HandlePostGameFMVs(void)
{
	switch(AvP.PlayerType)
	{
		case I_Marine:
		{
			if (MarineEpisodeToPlay==MAX_NO_OF_BASIC_MARINE_EPISODES-1)
			{
				ClearScreenToBlack();
				FlipBuffers();
				ClearScreenToBlack();
				PlayBinkedFMV("FMVs/marineoutro.bik");
			}
			break;
		}
		case I_Alien:
		{
			if (AlienEpisodeToPlay==MAX_NO_OF_BASIC_ALIEN_EPISODES-1)
			{
				ClearScreenToBlack();
				FlipBuffers();
				ClearScreenToBlack();
				PlayBinkedFMV("FMVs/alienoutro.bik");
			}
			break;
		}
		case I_Predator:
		{
			if (PredatorEpisodeToPlay==MAX_NO_OF_BASIC_PREDATOR_EPISODES-1)
			{
				ClearScreenToBlack();
				FlipBuffers();
				ClearScreenToBlack();
				PlayBinkedFMV("FMVs/predatoroutro.bik");
			}
			break;
		}
	}
}
void HandlePreGameFMVs(void)
{
	if (AvPMenus.MenusState == MENUSSTATE_STARTGAME && LoadGameRequest == SAVELOAD_REQUEST_NONE)
	{
		extern char LevelName[];
		if (!stricmp("derelict", LevelName))
		{
			ClearScreenToBlack();
			FlipBuffers();
			ClearScreenToBlack();
			PlayBinkedFMV("FMVs/marineintro.bik");
		}
		else if (!stricmp("temple", LevelName))
		{
			ClearScreenToBlack();
			FlipBuffers();
			ClearScreenToBlack();
			PlayBinkedFMV("FMVs/alienintro.bik");
		}
		else if (!stricmp("fall", LevelName))
		{
			ClearScreenToBlack();
			FlipBuffers();
			ClearScreenToBlack();
			PlayBinkedFMV("FMVs/predatorintro.bik");
		}
	}
}

extern void QuickSplashScreens(void)
{
	SelectMenuDisplayMode();
	if (AvP.LevelCompleted)
	{
		Show_WinnerScreen();
	}
	ShowSplashScreens();
}

extern void AvP_TriggerInGameMenus(void)
{
	AvPMenus.MenusState = MENUSSTATE_INGAMEMENUS;
	SetupNewMenu(AVPMENU_INGAME);
	/* Gameplay and the pause menu read the same input snapshot this frame.
	   Consume the opening ESC edge; keep its held state so key repeat
	   cannot generate another edge until it has been released. */
	DebouncedKeyboardInput[KEY_ESCAPE] = 0;

	/* KJL 16:32:55 21/07/98 - tell the console to go away */
	if (IOFOCUS_AcceptTyping()) IOFOCUS_Toggle();
//	SoundSys_PauseOn();
}

static void AvP_OpenInGameSaveLoadMenu(int menuID)
{
	if (AvP.Network != I_No_Network) return;
	AvPMenus.MenusState = MENUSSTATE_INGAMEMENUS;
	SetupNewMenu(menuID);
	if (IOFOCUS_AcceptTyping()) IOFOCUS_Toggle();
}

extern void AvP_OpenInGameSaveMenu(void)
{
	AvP_OpenInGameSaveLoadMenu(AVPMENU_SAVEGAME);
}

extern void AvP_OpenInGameLoadMenu(void)
{
	AvP_OpenInGameSaveLoadMenu(AVPMENU_LOADGAME);
}

int AvP_InGameMenus(void)
{
	if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
	{
		D3D_FadeDownScreen(AvP_MenuPreviewScene()?MenuPreviewAmount:ONE_FIXED/8,0);
		AvP_UpdateMenus();
		UpdateGammaSettings();

		return 1;
	}
	else return 0;
}

int InGameMenusAreRunning(void)
{
	return (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS);
}
extern void AvP_UpdateMenusLatencyBody(void)
{
//      DrawAvPMenuGfx(AVPMENUGFX_BIG_AVP_LOGO,50,50,16384);
//      DrawAvPMenuGfx(AVPMENUGFX_ALIENS_LOGO,10,75,16384,AVPMENUFORMAT_LEFTJUSTIFIED);
//      DrawAvPMenuGfx(AVPMENUGFX_PREDATOR_LOGO,380,77,16384,AVPMENUFORMAT_LEFTJUSTIFIED);
//      RenderMenuText("VS",330,95,16384,AVPMENUFORMAT_CENTREJUSTIFIED);

	#if 0
	if (AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYERJOINGAME2)
	{
		MinimalNetCollectMessages();
		/* KJL 19:49:04 05/07/98 - ugh, there goes the interface */
		{
			extern int MP_GameStyle;
			extern int MP_LevelNumber;
			extern int MP_Species;
			MP_GameStyle = netGameData.gameType;
			MP_LevelNumber = netGameData.levelNumber;
			switch (MP_Species)
			{
				default:
				case 0:
					netGameData.myCharacterType = NGCT_Marine;
					netGameData.myNextCharacterType = NGCT_Marine;
					AvP.PlayerType = I_Marine;
					break;
				case 1:
					netGameData.myCharacterType = NGCT_Predator;
					netGameData.myNextCharacterType = NGCT_Predator;
					AvP.PlayerType = I_Predator;
					break;
				case 2:
					netGameData.myCharacterType = NGCT_Alien;
					netGameData.myNextCharacterType = NGCT_Alien;
					AvP.PlayerType = I_Alien;
					break;
			}
		}
		AddNetMsg_PlayerDescription();

		NetSendMessages();
	}
	#endif
	
	if (AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYER_SPECIES_JOIN ||
	    AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYER_CONFIG_JOIN)
	{
		MinimalNetCollectMessages();
		if(AvP.Network==I_Host)
		{	/*
			We have become host while in the process of joining.
			This is bad.
			Best leave the game and return to the main menus.
			*/
			DirectPlay_Disconnect();
			SetupNewMenu(AVPMENU_MAIN);
			return;
		}

		if (AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYER_SPECIES_JOIN)
		{
			/* KJL 19:49:04 05/07/98 - ugh, there goes the interface */
			{
//				extern int MP_GameStyle;
//				extern int MP_LevelNumber;
				extern int MP_Species;
//				MP_GameStyle = netGameData.gameType;
//				MP_LevelNumber = netGameData.levelNumber;
				
				GetNextAllowedSpecies(&MP_Species,TRUE);
				netGameData.myCharacterSubType=NGSCT_General;
				switch (MP_Species)
				{
					default:
					case 0:
						netGameData.myCharacterType = NGCT_Marine;
						netGameData.myNextCharacterType = NGCT_Marine;
						AvP.PlayerType = I_Marine;
						break;
					case 1:
						netGameData.myCharacterType = NGCT_Predator;
						netGameData.myNextCharacterType = NGCT_Predator;
						AvP.PlayerType = I_Predator;
						break;
					case 2:
						netGameData.myCharacterType = NGCT_Alien;
						netGameData.myNextCharacterType = NGCT_Alien;
						AvP.PlayerType = I_Alien;
						break;

					case 3:	//various marine subtypes
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
					case 10:
						netGameData.myCharacterType = NGCT_Marine;
						netGameData.myNextCharacterType = NGCT_Marine;
						AvP.PlayerType = I_Marine;
						netGameData.myCharacterSubType =(NETGAME_SPECIALISTCHARACTERTYPE) (MP_Species-2);
						break;
				}
			}
			AddNetMsg_PlayerDescription();

			NetSendMessages();
		}
	}
	else if (AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYER_SPECIES_HOST)
	{
		extern int MP_Species;
		GetNextAllowedSpecies(&MP_Species,TRUE);
	}
	else if (AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYER_JOINING)
	{
		//check status of joining game
		int retval=0;
		if(LobbiedGame)
		{
			extern char MP_PlayerName[];
			retval=DirectPlay_ConnectingToLobbiedGame(MP_PlayerName);
			if(!retval)
			{
				//player has aborted , go back a menu
				SetupNewMenu(AVPMENU_MULTIPLAYER_LOBBIEDCLIENT);
				return ;
			}	
		}
		else
		{
			retval=DirectPlay_ConnectingToSession();
			if(!retval)
			{
				//player has aborted , go back a menu
				SetupNewMenu(AVPMENU_MULTIPLAYER);
				return ;
			}	
		}

		if(retval==AVPMENU_MULTIPLAYER_CONFIG_JOIN)
		{
			//successfully joined
			SetupNewMenu(AVPMENU_MULTIPLAYER_CONFIG_JOIN);
		}
	}
	else if (AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYERSELECTSESSION)
	{
		extern BOOL DirectPlay_UpdateSessionList(int * SelectedItem);
		int selection=AvPMenus.CurrentlySelectedElement;

		if(DirectPlay_UpdateSessionList(&selection))
		{
			//session list has changed , so we need to set the menu again
			SetupNewMenu(AVPMENU_MULTIPLAYERSELECTSESSION);
			//adjust the selected menu item
			AvPMenus.CurrentlySelectedElement=selection;
		}
	}



	/* render menu; episode select goes through a separate system */
	switch (AvPMenus.CurrentMenu)
	{
		case AVPMENU_LOADGAME:
		case AVPMENU_SAVEGAME:
		{
			RenderLoadGameMenu();
			RenderHelpString();
			break;
		}
		
		case AVPMENU_MARINELEVELS:
		case AVPMENU_PREDATORLEVELS:
		case AVPMENU_ALIENLEVELS:
		{
			RenderEpisodeSelectMenu();
			break;
		}
		case AVPMENU_USERPROFILESELECT:
		{
			RenderUserProfileSelectMenu();
			RenderHelpString();
			break;
		}
		case AVPMENU_USERPROFILEDELETE:
		{
			int i;
			AVP_USER_PROFILE *profilePtr = GetFirstUserProfile();
			time_t FileTime = profilePtr->FileTime;
			for (i=0; i<UserProfileNumber; i++)
				profilePtr = GetNextUserProfile();
			
			RenderMenuText(ProfileMenuLabel(profilePtr->FullName),MENU_CENTREX,MENU_CENTREY-100,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
			RenderSmallMenuText(ctime(&FileTime),MENU_CENTREX,MENU_CENTREY-70,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
			
			RenderMenu();
			RenderHelpString();
			break;

		}
		case AVPMENU_MULTIPLAYER_DELETECONFIG :
		{
			//show the name of the configuration we're trying to delete
			RenderMenuText(MultiplayerConfigurationName,MENU_CENTREX,MENU_CENTREY-100,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
			
			RenderMenu();
			RenderHelpString();
			break;
		}

		case AVPMENU_ALIENKEYCONFIG:
		case AVPMENU_MARINEKEYCONFIG:
		case AVPMENU_PREDATORKEYCONFIG:
		{
			CheckForKeysWithMultipleAssignments();
			RenderKeyConfigurationMenu();

			RenderHelpString();
		
			break;
		}
		case AVPMENU_SKIRMISH_CONFIG :
		case AVPMENU_MULTIPLAYER_CONFIG :
		case AVPMENU_MULTIPLAYER_CONFIG_JOIN :
		{
			UpdateMultiplayerConfigurationMenu();
			RenderScrollyMenu();
			RenderHelpString();
 			break;
		}
		case AVPMENU_MULTIPLAYEROPENADDRESS :
		{
			RenderMenu();
			RenderHelpString();
			break;
		}

		case AVPMENU_MULTIPLAYER_LOADIPADDRESS :
		{
			RenderScrollyMenu();
			break;
		}

		case AVPMENU_MULTIPLAYER_LOADCONFIG :
		{
			RenderScrollyMenu();
			RenderConfigurationDescriptionString();
			RenderHelpString();
 			break;
 		}
		case AVPMENU_INGAMEAVOPTIONS:
		{
            if(MenuColourBarsAlpha>0){
			int scale = DIV_FIXED(ScreenDescriptorBlock.SDB_Height,480);
			int h = scale/2048;
			int offset = scale/4096;
			D3D_DrawColourBarAlpha(offset, offset+h,  ONE_FIXED, 0, 0,MenuColourBarsAlpha);
			D3D_DrawColourBarAlpha(offset*2+h, offset*2+h*2,  0, ONE_FIXED, 0,MenuColourBarsAlpha);
			D3D_DrawColourBarAlpha(ScreenDescriptorBlock.SDB_Height-offset*2-h*2, ScreenDescriptorBlock.SDB_Height-offset*2-h,  0, 0, ONE_FIXED,MenuColourBarsAlpha);
			D3D_DrawColourBarAlpha(ScreenDescriptorBlock.SDB_Height-offset-h, ScreenDescriptorBlock.SDB_Height-offset, ONE_FIXED, ONE_FIXED, ONE_FIXED,MenuColourBarsAlpha);
            }
			RenderMenu();
			RenderHelpString();
			break;
		}

		case AVPMENU_CHEATOPTIONS:
		{
			TestValidityOfCheatMenu();
			RenderMenu();
			RenderHelpString();
			break;
		}
		default:
		{
			RenderMenu();
			RenderHelpString();
			break;
		}
	}
	RenderTyrargoVersion();
	ActUponUsersInput();

}
extern void AvP_UpdateMenus(void)
{
 EventLatencyMenu(AvPMenus.CurrentMenu,AvPMenus.CurrentlySelectedElement);
 EVENT_TICK stamp=EventLatencyBegin();
 AvP_UpdateMenusLatencyBody();
 EventLatencyEnd(stamp,"menu update/draw",AvPMenus.CurrentMenu,AvPMenus.CurrentlySelectedElement,0,0,0);
}

/* Separate storage keeps all ordinary Controls/pause data unchanged. */
static AVPMENU_ELEMENT DevControlsElements[64], DevPauseElements[32];
static AVPMENU_ELEMENT *DevControlsMenu(AVPMENU_ELEMENT *base)
{
    int n=0,i;
    while(base[n].ElementID!=AVPMENU_ELEMENT_ENDOFMENU && n<40){DevControlsElements[n]=base[n];n++;}
    for(i=0;i<DEV_FLAME_COUNT;i++){
        AVPMENU_ELEMENT *e=&DevControlsElements[n++];memset(e,0,sizeof(*e));
        e->ElementID=AVPMENU_ELEMENT_DEV_WEAPON_Z;e->a.TextDescription=TEXTSTRING_BLANK;e->c.Value=DEV_VIEWMODEL_COUNT+i;e->HelpString=TEXTSTRING_BLANK;
    }
    for(i=0;i<DEV_VIEWMODEL_COUNT;i++){
        AVPMENU_ELEMENT *e=&DevControlsElements[n++];memset(e,0,sizeof(*e));
        e->ElementID=AVPMENU_ELEMENT_DEV_WEAPON_Z;e->a.TextDescription=TEXTSTRING_BLANK;e->c.Value=DevViewmodelMenuIndex(i);e->HelpString=TEXTSTRING_BLANK;
    }
    memset(&DevControlsElements[n],0,2*sizeof(AVPMENU_ELEMENT));
    DevControlsElements[n].ElementID=AVPMENU_ELEMENT_DEV_EXPORT;
    DevControlsElements[n].a.TextDescription=DevControlsElements[n].HelpString=TEXTSTRING_BLANK;
    DevControlsElements[n+1].ElementID=AVPMENU_ELEMENT_ENDOFMENU;
    return DevControlsElements;
}
static AVPMENU_ELEMENT *DevPauseMenu(AVPMENU_ELEMENT *base)
{
    int n=0,i=0;
    while(base[i].ElementID!=AVPMENU_ELEMENT_ENDOFMENU && n<30){
        if(base[i].ElementID==AVPMENU_ELEMENT_QUITGAME){
            AVPMENU_ELEMENT *e=&DevPauseElements[n++];memset(e,0,sizeof(*e));
            e->ElementID=AVPMENU_ELEMENT_GOTOMENU;e->a.TextDescription=TEXTSTRING_MAINMENU_GAMEPLAYOPTIONS;e->b.MenuToGoTo=AVPMENU_OPTIONS;
        }
        DevPauseElements[n++]=base[i++];
    }
    memset(&DevPauseElements[n],0,sizeof(AVPMENU_ELEMENT));DevPauseElements[n].ElementID=AVPMENU_ELEMENT_ENDOFMENU;
    return DevPauseElements;
}

static AVPMENU_ELEMENT MotionElements[MP_COUNT+2];
static AVPMENU_ELEMENT *MotionCalibrationMenu(void){
 int i;memset(MotionElements,0,sizeof MotionElements);
 for(i=0;i<MP_COUNT;i++){
  MotionElements[i].ElementID=AVPMENU_ELEMENT_MOTION_PARAM;
  MotionElements[i].a.TextDescription=MotionElements[i].HelpString=TEXTSTRING_BLANK;
  MotionElements[i].c.Value=MotionParamMenuIndex(i);
 }
 MotionElements[MP_COUNT].ElementID=AVPMENU_ELEMENT_MOTION_EXPORT;
 MotionElements[MP_COUNT].a.TextDescription=MotionElements[MP_COUNT].HelpString=TEXTSTRING_BLANK;
 MotionElements[MP_COUNT+1].ElementID=AVPMENU_ELEMENT_ENDOFMENU;
 return MotionElements;
}
static AVPMENU_ELEMENT ShakeElements[HP_COUNT+2];
static AVPMENU_ELEMENT *ShakeCalibrationMenu(void){
 int i;memset(ShakeElements,0,sizeof ShakeElements);
 for(i=0;i<HP_COUNT;i++){
  ShakeElements[i].ElementID=AVPMENU_ELEMENT_SHAKE_PARAM;
  ShakeElements[i].a.TextDescription=ShakeElements[i].HelpString=TEXTSTRING_BLANK;
  ShakeElements[i].c.Value=ShakeParamMenuIndex(i);
 }
 ShakeElements[HP_COUNT].ElementID=AVPMENU_ELEMENT_SHAKE_EXPORT;
 ShakeElements[HP_COUNT].a.TextDescription=ShakeElements[HP_COUNT].HelpString=TEXTSTRING_BLANK;
 ShakeElements[HP_COUNT+1].ElementID=AVPMENU_ELEMENT_ENDOFMENU;
 return ShakeElements;
}
/* Publish into separate storage only with -dev2. No phantom rows, changed
 * stock indices, or mutation of the ordinary main/pause menu arrays. */
static AVPMENU_ELEMENT ShakeRootElements[64];
static AVPMENU_ELEMENT *ShakeRootMenu(AVPMENU_ELEMENT *base,int ingame){
 int n=0;
 while(base[n].ElementID!=AVPMENU_ELEMENT_ENDOFMENU && n<61){ShakeRootElements[n]=base[n];n++;}
 if(base[n].ElementID!=AVPMENU_ELEMENT_ENDOFMENU)return base;
 int position=n;
 if(n&&ShakeRootElements[n-1].ElementID==AVPMENU_ELEMENT_QUITGAME)position=n-1;
 memmove(&ShakeRootElements[position+1],&ShakeRootElements[position],(n-position)*sizeof(*base));
 AVPMENU_ELEMENT *entry=&ShakeRootElements[position];memset(entry,0,sizeof(*entry));
 entry->ElementID=AVPMENU_ELEMENT_GOTOMENU;entry->a.TextDescription=TEXTSTRING_EXPERIMENTAL2_NEW;
 entry->b.MenuToGoTo=ingame?AVPMENU_INGAMEEXPERIMENTAL2:AVPMENU_EXPERIMENTAL2;
 entry->HelpString=TEXTSTRING_EXPERIMENTAL2_HELP_NEW;
 memset(&ShakeRootElements[n+1],0,sizeof(*base));ShakeRootElements[n+1].ElementID=AVPMENU_ELEMENT_ENDOFMENU;
 return ShakeRootElements;
}


static void SetupNewMenuLatencyBody(enum AVPMENU_ID menuID)
{
 if(!TYRARGO_MOTION_CALIBRATION&&(menuID==AVPMENU_EXPERIMENTAL||menuID==AVPMENU_INGAMEEXPERIMENTAL))return;
 if(!ShakeEnabled&&(menuID==AVPMENU_EXPERIMENTAL2||menuID==AVPMENU_INGAMEEXPERIMENTAL2))return;
 enum AVPMENU_ID previousMenuID = AvPMenus.CurrentMenu;
 if(previousMenuID!=menuID && IsEpisodeSelector(previousMenuID))AvP_FlushEpisodeSelection();
	TyrargoMenuVersionStarted = 0;
	AvPMenus.CurrentMenu = menuID;

	/* set pointer to the start of the menu's element data */
	AvPMenus.MenuElements = AvPMenusData[menuID].MenuElements; // could use default
 if(menuID==AVPMENU_EXPERIMENTAL||menuID==AVPMENU_INGAMEEXPERIMENTAL)AvPMenus.MenuElements=MotionCalibrationMenu();
 if(ShakeEnabled&&(menuID==AVPMENU_EXPERIMENTAL2||menuID==AVPMENU_INGAMEEXPERIMENTAL2))AvPMenus.MenuElements=ShakeCalibrationMenu();

	AvPMenus.FontToUse = AvPMenusData[menuID].FontToUse;
    if(DevViewmodelEnabled && menuID==AVPMENU_OPTIONS){
        AvPMenus.MenuElements=DevControlsMenu(AvPMenus.MenuElements);
        AvPMenus.FontToUse=AVPMENU_FONT_SMALL;
    }

	AvPMenus.CurrentlySelectedElement = 0;
	AvPMenus.UserEnteringText = 0;
	AvPMenus.UserEnteringNumber = 0;
	AvPMenus.UserChangingKeyConfig = 0;
	AvPMenus.PositionInTextField = 0;
	AvPMenus.WidthLeftForText = 0;


	/* menu specific stuff */
	switch (menuID)
	{
		case AVPMENU_MAIN:
		{
			GetSettingsFromUserProfile();
			SaveUserProfile(UserProfilePtr);
			
			if (DebuggingCommandsActive)
			{
				if (AnyCheatModesAllowed())
				{
					// turn on cheats
					SetupNewMenu(AVPMENU_DEBUG_MAIN_WITHCHEATS);
				}							   
				else
				{
					SetupNewMenu(AVPMENU_DEBUG_MAIN);
				}
			}
			else
			{
				if (AnyCheatModesAllowed())
				{
					// turn on cheats
					SetupNewMenu(AVPMENU_MAIN_WITHCHEATS);
				}
			}
			break;
		}
		
		case AVPMENU_VIDEOMODE:
		case AVPMENU_INGAMEVIDEOMODE:
		{	
			LoadDeviceAndVideoModePreferences();
			break;
		}

		case AVPMENU_USERPROFILESELECT:
		{
			InitialiseGammaSettings(DEFAULT_GAMMA_SETTING);
			EpisodeSelectScrollOffset=0;
			ExamineSavedUserProfiles();
			MakeUserProfileSelectMenu();
			break;
		}
		case AVPMENU_USERPROFILEENTERNAME:
		{
			AvPMenus.UserEnteringText = 1;
			SDL_StartTextInput(window);
			KeyboardEntryQueue_Clear();
			AvPMenus.MenuElements->c.TextPtr = UserProfilePtr->FullName;
			UserProfilePtr->FullName[0] = 0;
			AvPMenus.PositionInTextField = 0;
			AvPMenus.WidthLeftForText = 0; //will be calculated properly when menus are drawn
			break;
		}
		case AVPMENU_MULTIPLAYER_SAVECONFIG:
		{
			AvPMenus.UserEnteringText = 1;
			SDL_StartTextInput(NULL);
			KeyboardEntryQueue_Clear();
			AvPMenus.WidthLeftForText = 0; //will be calculated properly when menus are drawn
			break;
		}

		case AVPMENU_SINGLEPLAYER:
		{
			break;
		}
	
		case AVPMENU_MARINELEVELS:
		{
			EpisodeSelectScrollOffset=0;
			AvPMenus.MenuElements->b.MaxSliderValue = NumberOfAvailableLevels(I_Marine);
			*AvPMenus.MenuElements->c.SliderValuePtr = RememberedEpisode(I_Marine);
			break;
		}
		case AVPMENU_PREDATORLEVELS:
		{
			EpisodeSelectScrollOffset=0;
			AvPMenus.MenuElements->b.MaxSliderValue = NumberOfAvailableLevels(I_Predator);
			*AvPMenus.MenuElements->c.SliderValuePtr = RememberedEpisode(I_Predator);
			break;
		}
		case AVPMENU_ALIENLEVELS:
		{
			EpisodeSelectScrollOffset=0;
			AvPMenus.MenuElements->b.MaxSliderValue = NumberOfAvailableLevels(I_Alien);
			*AvPMenus.MenuElements->c.SliderValuePtr = RememberedEpisode(I_Alien);
			break;
		}
		case AVPMENU_MULTIPLAYERSELECTSESSION:
		{
			if(previousMenuID!=AVPMENU_MULTIPLAYERSELECTSESSION)
			{
				//save ip address (if it has been set)
				extern char IPAddressString[]; 
				SaveIPAddress(IP_Address_Name,IPAddressString);
				DirectPlay_JoinGame();
			}
			MakeSelectSessionMenu();
			break;
		}
		
		case AVPMENU_MULTIPLAYER_SKIRMISH :
		{
			extern char MP_Config_Description[];
			netGameData.skirmishMode=TRUE;
			LoadMultiplayerConfiguration(GetTextString(TEXTSTRING_PREVIOUSGAME_FILENAME));
			MP_Config_Description[0]=0;
			break;
		}
		
		case AVPMENU_MULTIPLAYER_CONNECTION:
		{
			extern char MP_Config_Description[];
			//skirmishMode must be false
			netGameData.skirmishMode=FALSE;
			
			LoadMultiplayerConfiguration(GetTextString(TEXTSTRING_PREVIOUSGAME_FILENAME));
			MP_Config_Description[0]=0;
			

			if(LobbiedGame)
			{
				//use alternative multiplayer menus for lobbied games
				if(LobbiedGame==LobbiedGame_Server)
				{
					SetupNewMenu(AVPMENU_MULTIPLAYER_LOBBIEDSERVER);
				}
				else
				{
					SetupNewMenu(AVPMENU_MULTIPLAYER_LOBBIEDCLIENT);
				}
				return;
			}
			
			DirectPlay_EnumConnections();
			MakeConnectionSelectMenu();
			break;
		}

		case AVPMENU_INGAME:
		case AVPMENU_INNETGAME:
		{
			if(AvP.Network != I_No_Network)
			{
				//in a multiplayer game set up a menu without the restart mission option
				menuID=AVPMENU_INNETGAME;
				AvPMenus.CurrentMenu = menuID;
				AvPMenus.MenuElements = AvPMenusData[menuID].MenuElements; // could use default
 if(menuID==AVPMENU_EXPERIMENTAL||menuID==AVPMENU_INGAMEEXPERIMENTAL)AvPMenus.MenuElements=MotionCalibrationMenu();
 if(ShakeEnabled&&(menuID==AVPMENU_EXPERIMENTAL2||menuID==AVPMENU_INGAMEEXPERIMENTAL2))AvPMenus.MenuElements=ShakeCalibrationMenu();
				AvPMenus.FontToUse = AvPMenusData[menuID].FontToUse;
			}
			MakeInGameMenu();
            if(DevViewmodelEnabled)AvPMenus.MenuElements=DevPauseMenu(AvPMenus.MenuElements);
			break;
		}
		case AVPMENU_MARINEKEYCONFIG:
		{
			MakeMarineKeyConfigMenu();
			PlayerInputPrimaryConfig = MarineInputPrimaryConfig;
			PlayerInputSecondaryConfig = MarineInputSecondaryConfig;
			PlayerWeaponPrimaryConfig = MarineWeaponPrimaryConfig;
			PlayerWeaponSecondaryConfig = MarineWeaponSecondaryConfig;
			PlayerGlobalCommandPrimaryConfig = GlobalCommandPrimaryConfig;
			PlayerGlobalCommandSecondaryConfig = GlobalCommandSecondaryConfig;
			EpisodeSelectScrollOffset=0;
			KeyConfigSelectionColumn = 0;
			break;
		}
		case AVPMENU_PREDATORKEYCONFIG:
		{
			MakePredatorKeyConfigMenu();
            memcpy(PlayerPreviousVisionBindings,PredatorPreviousVisionBindings,2);
			PlayerInputPrimaryConfig = PredatorInputPrimaryConfig;
			PlayerInputSecondaryConfig = PredatorInputSecondaryConfig;
			PlayerWeaponPrimaryConfig = PredatorWeaponPrimaryConfig;
			PlayerWeaponSecondaryConfig = PredatorWeaponSecondaryConfig;
			PlayerGlobalCommandPrimaryConfig = GlobalCommandPrimaryConfig;
			PlayerGlobalCommandSecondaryConfig = GlobalCommandSecondaryConfig;
			EpisodeSelectScrollOffset=0;
			KeyConfigSelectionColumn = 0;
			break;
		}
		case AVPMENU_ALIENKEYCONFIG:
		{
			MakeAlienKeyConfigMenu();
			PlayerInputPrimaryConfig = AlienInputPrimaryConfig;
			PlayerInputSecondaryConfig = AlienInputSecondaryConfig;
			PlayerWeaponPrimaryConfig = AlienWeaponPrimaryConfig;
			PlayerWeaponSecondaryConfig = AlienWeaponSecondaryConfig;
			PlayerGlobalCommandPrimaryConfig = GlobalCommandPrimaryConfig;
			PlayerGlobalCommandSecondaryConfig = GlobalCommandSecondaryConfig;
			EpisodeSelectScrollOffset=0;
			KeyConfigSelectionColumn = 0;
			break;
		}
		case AVPMENU_CONTROLS:
		{
			PlayerControlMethods = ControlMethods;
			break;
		}
		case AVPMENU_JOYSTICKCONTROLS:
		{
			PlayerJoystickControlMethods = JoystickControlMethods;
			break;
		}

		case AVPMENU_MULTIPLAYERJOINGAME:
		{
			extern char IPAddressString[]; 
			extern char CommandLineIPAddressString[]; 
			strcpy(IPAddressString,CommandLineIPAddressString);
			IP_Address_Name[0] = 0;

			if(netGameData.connectionType!=CONN_TCPIP)  
			{
				////for non tcpip games skip to the select session menu
				SetupNewMenu(AVPMENU_MULTIPLAYERSELECTSESSION);
				return;
	
			}
			

			break;
		}

		case AVPMENU_MULTIPLAYER :
		{
			break;
		}

		case AVPMENU_MULTIPLAYER_LOADCONFIG :
		{
			extern AVPMENU_ELEMENT* AvPMenu_Multiplayer_LoadConfig;
			if(!BuildLoadMPConfigMenu())
			{
				SetupNewMenu(AVPMENU_MULTIPLAYER_CONFIG);
				return;
			}
			AvPMenus.MenuElements=AvPMenu_Multiplayer_LoadConfig;
			break;
 		}

		case AVPMENU_MULTIPLAYER_CONFIG :
		{
			//need to set the menu we return to , according to whether this is
			//a lobbied game or not.
			if(LobbiedGame)
			{
				AvPMenusData[AvPMenus.CurrentMenu].ParentMenu=AVPMENU_MULTIPLAYER_LOBBIEDSERVER;
			}
			else if(netGameData.skirmishMode)
			{
				//for skirmish games , use skirmish config menu instead
				SetupNewMenu(AVPMENU_SKIRMISH_CONFIG);
				return;
			}
			else
			{
				AvPMenusData[AvPMenus.CurrentMenu].ParentMenu=AVPMENU_MULTIPLAYER;
			}
			break;
		}

		case AVPMENU_MULTIPLAYER_CONFIG_JOIN :
		{
			//need to set the menu we return to , according to whether this is
			//a lobbied game or not.
			if(LobbiedGame)
			{
				AvPMenusData[AvPMenus.CurrentMenu].ParentMenu=AVPMENU_MULTIPLAYER_LOBBIEDCLIENT;
			}
			else
			{
				AvPMenusData[AvPMenus.CurrentMenu].ParentMenu=AVPMENU_MULTIPLAYER;
			}
			break;
		}

		case AVPMENU_MULTIPLAYER_LOADIPADDRESS :
		{
			extern AVPMENU_ELEMENT* AvPMenu_Multiplayer_LoadIPAddress;
			AvPMenus.MenuElements=AvPMenu_Multiplayer_LoadIPAddress;
			break;
		}

		case AVPMENU_MULTIPLAYEROPENADDRESS :
		{
			extern void MakeOpenIPAddressMenu();
			MakeOpenIPAddressMenu();
			break;
		}
		
		case AVPMENU_SAVEGAME:
		case AVPMENU_LOADGAME:
		{
			ScanSaveSlots();
			break;
		}

		default:
			break;
	}

 if(ShakeEnabled&&(TyrargoMainMenu()||menuID==AVPMENU_INGAME||menuID==AVPMENU_INNETGAME))
  AvPMenus.MenuElements=ShakeRootMenu(AvPMenus.MenuElements,menuID==AVPMENU_INGAME||menuID==AVPMENU_INNETGAME);
 /* count the number of elements in the menu */
	{
		AVPMENU_ELEMENT *elementPtr = AvPMenus.MenuElements;
		AvPMenus.NumberOfElementsInMenu=0;
		AvPMenus.MenuHeight=0;
		do
		{
			elementPtr->Brightness = BRIGHTNESS_OF_DARKENED_ELEMENT;

			AvPMenus.MenuHeight += HeightOfMenuElement(elementPtr);

			AvPMenus.NumberOfElementsInMenu++;
			elementPtr++;
		}
		while(elementPtr->ElementID != AVPMENU_ELEMENT_ENDOFMENU);
	}       

	if (menuID == AVPMENU_SAVEGAME || menuID == AVPMENU_LOADGAME)
	{
		/* Generic layout must never use the full height of 50 data rows. */
		AvPMenus.MenuHeight = MENU_HEIGHT;
	}

	switch(menuID)
	{
		case AVPMENU_CHEATOPTIONS:
		{
			CheatMode_Active = 0;
			break;
		}
		
		case AVPMENU_MARINEKEYCONFIG:
		case AVPMENU_ALIENKEYCONFIG:
		case AVPMENU_PREDATORKEYCONFIG:
			AvPMenus.MenuHeight += 50;
			break;
		case AVPMENU_LEVELBRIEFING_BASIC:
		{
			int episodeToPlay;
			switch (AvP.PlayerType)
			{
				case I_Marine:
				{
					episodeToPlay = MarineEpisodeToPlay;
					break;
				}
				case I_Predator:
				{
					episodeToPlay = PredatorEpisodeToPlay;
					break;
				}
				case I_Alien:
				{
					episodeToPlay = AlienEpisodeToPlay;
					break;
				}
			}
			
			/* find available difficulty levels */
			AvPMenus.NumberOfElementsInMenu=MaxDifficultyLevelAllowed(AvP.PlayerType, episodeToPlay);
            AvPMenus.MenuHeight=0;
            for(int row=0;row<AvPMenus.NumberOfElementsInMenu;row++)AvPMenus.MenuHeight+=HeightOfMenuElement(&AvPMenus.MenuElements[row]);
			/* highlight a suitable level of difficulty */
			if (episodeToPlay)
			{
				AvPMenus.CurrentlySelectedElement = AvPMenus.NumberOfElementsInMenu-1;
			}
			else // default to medium difficulty for first level
			{
				AvPMenus.CurrentlySelectedElement = 1;
			}
			
			SetBriefingTextForEpisode(episodeToPlay, AvP.PlayerType);

			break;
		}
		case AVPMENU_LEVELBRIEFING_BONUS:
		{
			int episodeToPlay;
			switch (AvP.PlayerType)
			{
				case I_Marine:
				{
					episodeToPlay = MarineEpisodeToPlay;
					break;
				}
				case I_Predator:
				{
					episodeToPlay = PredatorEpisodeToPlay;
					break;
				}
				case I_Alien:
				{
					episodeToPlay = AlienEpisodeToPlay;
					break;
				}
			}
			
            AvPMenus.NumberOfElementsInMenu=MaxDifficultyLevelAllowed(AvP.PlayerType, episodeToPlay);
            AvPMenus.MenuHeight=0;
            for(int row=0;row<AvPMenus.NumberOfElementsInMenu;row++)AvPMenus.MenuHeight+=HeightOfMenuElement(&AvPMenus.MenuElements[row]);
			AvPMenus.CurrentlySelectedElement = 1; /* Same default as basic episode one. */
			SetBriefingTextForEpisode(episodeToPlay, AvP.PlayerType);
			break;
		}
		default:
			SetBriefingTextToBlank();
			break;
	}
}
static void SetupNewMenu(enum AVPMENU_ID menuID)
{
 EVENT_TICK stamp=EventLatencyBegin();
 SetupNewMenuLatencyBody(menuID);
 EventLatencyEnd(stamp,"menu setup",menuID,AvPMenus.MenusState,0,0,2);
}


int LengthOfSmallMenuText(char *textPtr);
static int MotionMenuLanguage(void){
 const char*label=GetTextString(TEXTSTRING_MOUSECONTROLS_AUTOCENTRE);
 if(label&&(!strcmp(label,"Auto-centrar")||!strcmp(label,"Auto Centrar")))return I_Spanish;
 return AvP.Language>=0&&AvP.Language<6?AvP.Language:I_English;
}
static int WrapMenuHelp(const char *text,char lines[64][512],int width,int scale){
 int row=0,n=0,lastSpace=-1;lines[0][0]=0;if(!text)return 0;
 while(*text&&row<64){
  unsigned char c=(unsigned char)*text++;
  if(c=='\r')continue;
  if(c=='\n'){lines[row][n]=0;if(++row==64)return 64;n=0;lastSpace=-1;lines[row][0]=0;continue;}
  if(!n&&c==' ')continue;
  if(n>=510){lines[row][n]=0;if(++row==64)return 64;n=0;lastSpace=-1;}
  lines[row][n++]=(char)c;lines[row][n]=0;if(c==' ')lastSpace=n-1;
  if(MUL_FIXED(LengthOfSmallMenuText(lines[row]),scale)>width&&n>1){
   int cut=lastSpace>0?lastSpace:n-1,skip=lastSpace>0?1:0,rest=n-cut-skip;
   char carry[512];memcpy(carry,lines[row]+cut+skip,rest);carry[rest]=0;lines[row][cut]=0;
   if(++row==64)return 64;memcpy(lines[row],carry,rest+1);n=rest;lastSpace=-1;
  }
 }
 return row+(n>0);
}
static const char *MenuHelpText(void){
 AVPMENU_ELEMENT*e=&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
 static char help[1536];int lang=MotionMenuLanguage();
 if(e->ElementID==AVPMENU_ELEMENT_SHAKE_PARAM)return ShakeParamHelp(e->c.Value,lang);
 if(e->ElementID==AVPMENU_ELEMENT_SHAKE_EXPORT){
  snprintf(help,sizeof help,"%s. %s",ShakeParamsStatus(lang),ShakeParamsHelp(lang,1));return help;
 }
 if(e->ElementID==AVPMENU_ELEMENT_MOTION_PARAM)return MotionParamHelp(e->c.Value,lang);
 if(e->ElementID==AVPMENU_ELEMENT_MOTION_EXPORT){
  snprintf(help,sizeof help,"%s. %s",MotionParamsStatus(lang),MotionParamsHelp(lang,e->ElementID==AVPMENU_ELEMENT_MOTION_EXPORT));return help;
 }
 if(e->HelpString!=TEXTSTRING_BLANK)return GetTextString(e->HelpString);
 return "";
}
static int MenuHelpVisibleLines(int count,int scale){
 int lineHeight=MUL_FIXED(17,scale),maxLines=(ScreenDescriptorBlock.SDB_Height/3)/lineHeight;
 if(AvPMenus.CurrentMenu==AVPMENU_LEVELBRIEFING_BASIC || AvPMenus.CurrentMenu==AVPMENU_LEVELBRIEFING_BONUS)maxLines=4;
 if(AvPMenus.CurrentMenu==AVPMENU_SINGLEPLAYER)maxLines=3; /* Keep long campaign help below the fixed option row. */
 int rowHeight=MUL_FIXED(HeightOfMenuElement(&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement]),scale);
 int room=(ScreenDescriptorBlock.SDB_Height-rowHeight-MUL_FIXED(24,scale)-64)/lineHeight;
 if(room<maxLines)maxLines=room;
 if(maxLines<1)maxLines=1;return count<maxLines?count:maxLines;
}
static int MenuHelpReserve(void){
 char lines[64][512];int scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
 int count=WrapMenuHelp(MenuHelpText(),lines,ScreenDescriptorBlock.SDB_Width-48,scale);
 return MenuHelpVisibleLines(count,scale)*MUL_FIXED(17,scale)+32+TyrargoVersionReserve();
}
static int MotionParamLabel(int id,char lines[64][512]){
 char label[256];int scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
 snprintf(label,sizeof label,"%s = %.4g %s",MotionParams[id].label[MotionMenuLanguage()],MotionValues[id],MotionParams[id].unit);
 {
  int count=WrapMenuHelp(label,lines,ScreenDescriptorBlock.SDB_Width-48,scale);
  int maxLines=(ScreenDescriptorBlock.SDB_Height-80-MUL_FIXED(63,scale))/MUL_FIXED(17,scale);
  static int previousMenu=-1,previousElement=-1;static Uint64 start=0;
  if(previousMenu!=AvPMenus.CurrentMenu||previousElement!=AvPMenus.CurrentlySelectedElement){start=SDL_GetTicks();previousMenu=AvPMenus.CurrentMenu;previousElement=AvPMenus.CurrentlySelectedElement;}
  if(maxLines<1)maxLines=1;
  if(count>maxLines){
   int first=(int)(((SDL_GetTicks()-start)/6500)%((count+maxLines-1)/maxLines))*maxLines,i;
   for(i=0;i<maxLines;i++){if(first+i<count)memmove(lines[i],lines[first+i],512);else lines[i][0]=0;}
   return maxLines;
  }
  return count;
 }
}
static int ShakeParamLabel(int id,char lines[64][512]){
 char label[256];int scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
 snprintf(label,sizeof label,"%s = %.4g %s",ShakeParams[id].label[MotionMenuLanguage()],ShakeValues[id],ShakeParams[id].unit);
 {
  int count=WrapMenuHelp(label,lines,ScreenDescriptorBlock.SDB_Width-48,scale);
  int maxLines=(ScreenDescriptorBlock.SDB_Height-80-MUL_FIXED(63,scale))/MUL_FIXED(17,scale);
  static int previousMenu=-1,previousElement=-1;static Uint64 start=0;
  if(previousMenu!=AvPMenus.CurrentMenu||previousElement!=AvPMenus.CurrentlySelectedElement){start=SDL_GetTicks();previousMenu=AvPMenus.CurrentMenu;previousElement=AvPMenus.CurrentlySelectedElement;}
  if(maxLines<1)maxLines=1;
  if(maxLines>4)maxLines=4; /* Page unusually long labels before crowding help. */
  if(count>maxLines){
   int first=(int)(((SDL_GetTicks()-start)/6500)%((count+maxLines-1)/maxLines))*maxLines,i;
   for(i=0;i<maxLines;i++){if(first+i<count)memmove(lines[i],lines[first+i],512);else lines[i][0]=0;}
   return maxLines;
  }
  return count;
 }
}

static void RenderPreviewRow(AVPMENU_ELEMENT *element,int index,int y,int scale)
{
 int pad=MUL_FIXED(3,scale),bottom=y+MUL_FIXED(HeightOfMenuElement(element),scale);
 MenuPreviewBandLow=1.0f-(float)bottom/ScreenDescriptorBlock.SDB_Height;
 MenuPreviewBandHigh=1.0f-(float)(y-pad)/ScreenDescriptorBlock.SDB_Height;
 D3D_DrawSolidRect(0,y-pad,ScreenDescriptorBlock.SDB_Width,bottom,0xff000000u,0);
 RenderMenuElement(element,index,y);
 if(element->ElementID==AVPMENU_ELEMENT_CROSSHAIR_SCALE || MenuPreviewThicknessSelected()){
  int half=MUL_FIXED(HUD_GetCrosshairScaleFixed(),9)+4;
  int sampleY=bottom+MUL_FIXED(12,scale)+half;
  int limit=ScreenDescriptorBlock.SDB_Height-MenuHelpReserve()-half;
  if(sampleY>limit)sampleY=limit;
  RenderMenuCrosshairPreview(ScreenDescriptorBlock.SDB_Width/2,sampleY,
    MarineCrosshairThicknessMode);
 }
}

static int IsMissionBriefingMenu(void)
{
    return AvPMenus.CurrentMenu==AVPMENU_LEVELBRIEFING_BASIC ||
           AvPMenus.CurrentMenu==AVPMENU_LEVELBRIEFING_BONUS;
}

static void RenderMenu(void)
{
	AVPMENU_ELEMENT *elementPtr;
	int e;
	int y;
	int firstElement = 0;
	int lastElement = AvPMenus.NumberOfElementsInMenu - 1;
	const int inGameMenuText = (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS);
	const int textScaleFixed = inGameMenuText ? D3D_GetConsoleTextScaleFixed() : ONE_FIXED;
	int renderedMenuHeight = AvPMenus.MenuHeight;
    if(AvPMenus.NumberOfElementsInMenu>0 && AvPMenus.CurrentlySelectedElement>=0 &&
       AvPMenus.CurrentlySelectedElement<AvPMenus.NumberOfElementsInMenu &&
       !MenuElementIsVisible(&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement])){
        int count=AvPMenus.NumberOfElementsInMenu;
        for(int q=1;q<count;++q){
            int next=(AvPMenus.CurrentlySelectedElement+q)%count;
            if(MenuElementIsVisible(&AvPMenus.MenuElements[next])){AvPMenus.CurrentlySelectedElement=next;break;}
        }
    }
	const int devControls = DevViewmodelEnabled && AvPMenus.CurrentMenu==AVPMENU_OPTIONS;
 const int experimental = AvPMenus.CurrentMenu==AVPMENU_EXPERIMENTAL || AvPMenus.CurrentMenu==AVPMENU_INGAMEEXPERIMENTAL || AvPMenus.CurrentMenu==AVPMENU_EXPERIMENTAL2 || AvPMenus.CurrentMenu==AVPMENU_INGAMEEXPERIMENTAL2;
 const int useDevControlsCarousel = devControls || inGameMenuText || experimental;
	const int useAVOptionsCarousel = (AvPMenus.MenusState == MENUSSTATE_MAINMENUS && AvPMenus.CurrentMenu == AVPMENU_MAINMENUAVOPTIONS);

    const int dynamicList = TyrargoMainMenu() || inGameMenuText || devControls || experimental || useAVOptionsCarousel ||
        AvPMenus.CurrentMenu==AVPMENU_JOYSTICKCONTROLS || AvPMenus.CurrentMenu==AVPMENU_DETAILLEVELS || (AvPMenus.MenuHeight>MENU_HEIGHT && AvPMenus.FontToUse==AVPMENU_FONT_SMALL);
    if(dynamicList){
        int top,bottom,count=AvPMenus.NumberOfElementsInMenu;
        int heights[count>0?count:1];
        MenuListBounds(ScreenDescriptorBlock.SDB_Height,textScaleFixed,MenuHelpReserve(),!inGameMenuText,&top,&bottom);
        /* Keep the root list between its raised title and the reserved help/footer. */
        if(TyrargoMainMenu()){top=132;bottom+=12;} /* Lower only the options, preserving list height. */
        if(inGameMenuText && AvPMenus.CurrentMenu==AVPMENU_INGAMEAVOPTIONS){
            int bandTop,bandBottom;MenuAVBandBounds(ScreenDescriptorBlock.SDB_Height,textScaleFixed,&bandTop,&bandBottom);
            if(top<bandTop)top=bandTop;if(bottom>bandBottom)bottom=bandBottom;
        }
        if(devControls)bottom-=MUL_FIXED(54,textScaleFixed);
        if(AvPMenus.CurrentMenu==AVPMENU_EXPERIMENTAL2||AvPMenus.CurrentMenu==AVPMENU_INGAMEEXPERIMENTAL2){
            /* Reserve the counter explicitly; use the actual slider height.
             * Large text still fits at 640x480 without touching the help. */
            if(inGameMenuText)top=MUL_FIXED(4,textScaleFixed);
            bottom=ScreenDescriptorBlock.SDB_Height-MenuHelpReserve()-8-MUL_FIXED(21,textScaleFixed);
        }
        for(int q=0;q<count;q++)heights[q]=MUL_FIXED(HeightOfMenuElement(&AvPMenus.MenuElements[q]),textScaleFixed);
        if(inGameMenuText && AvPMenus.CurrentMenu==AVPMENU_INGAMEAVOPTIONS){
            MenuStableVisibleRange(heights,count,AvPMenus.CurrentlySelectedElement,bottom-top,&MenuAVScrollFirst,&lastElement,&renderedMenuHeight);
            firstElement=MenuAVScrollFirst;
        }else {
            MenuAVScrollFirst=0;
            MenuVisibleRange(heights,count,AvPMenus.CurrentlySelectedElement,bottom-top,&firstElement,&lastElement,&renderedMenuHeight);
        }
        y=(inGameMenuText && AvPMenus.CurrentMenu==AVPMENU_INGAMEAVOPTIONS)?top:top+(bottom-top-renderedMenuHeight)/2;
        if(y<top)y=top;
    } else if(AvPMenus.CurrentMenu==AVPMENU_LEVELBRIEFING_BASIC || AvPMenus.CurrentMenu==AVPMENU_LEVELBRIEFING_BONUS){
        y=MENU_BOTTOMYEDGE+3-AvPMenus.MenuHeight;RenderBriefingScreenInfo();
    } else y=MENU_CENTREY-AvPMenus.MenuHeight/2-(AvPMenus.CurrentMenu==AVPMENU_SINGLEPLAYER?24+(40-(HUD_FONT_HEIGHT+5))/2:0);

	/* Preserve brightness animation for carousel entries outside the viewport. */
	elementPtr = AvPMenus.MenuElements;
	for (e = 0; e < AvPMenus.NumberOfElementsInMenu; e++, elementPtr++)
	{
		int targetBrightness = (e == AvPMenus.CurrentlySelectedElement) ? BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT : BRIGHTNESS_OF_DARKENED_ELEMENT;
		if (targetBrightness > elementPtr->Brightness)
		{
			elementPtr->Brightness += BRIGHTNESS_CHANGE_SPEED;
			if (elementPtr->Brightness > targetBrightness) elementPtr->Brightness = targetBrightness;
		}
		else
		{
			elementPtr->Brightness -= BRIGHTNESS_CHANGE_SPEED;
			if (elementPtr->Brightness < targetBrightness) elementPtr->Brightness = targetBrightness;
		}
	}

	elementPtr = &AvPMenus.MenuElements[firstElement];
	if (inGameMenuText) D3D_SetInGameMenuTextRenderActive(1);
	for (e = firstElement; e <= lastElement; e++, elementPtr++)
	{
        SetMenuAVContrast(AvPMenus.MenusState==MENUSSTATE_MAINMENUS&&AvPMenus.CurrentMenu==AVPMENU_MAINMENUAVOPTIONS ? (e==AvPMenus.CurrentlySelectedElement?2:1) : 0);
        if(inGameMenuText && MenuPreviewTarget && e==AvPMenus.CurrentlySelectedElement)
            RenderPreviewRow(elementPtr,e,y,textScaleFixed);
        else if(!inGameMenuText || !MenuPreviewTarget)
            RenderMenuElement(elementPtr,e,y);
        SetMenuAVContrast(0);
		y += MUL_FIXED(HeightOfMenuElement(elementPtr), textScaleFixed);
	}
    if(useDevControlsCarousel){
        int (*text)(char*,int,int,int,enum AVPMENUFORMAT_ID)=inGameMenuText?Hardware_RenderSmallMenuText:RenderSmallMenuText;
        if(experimental || devControls){
            char position[40];
            snprintf(position,sizeof position,"%d / %d",AvPMenus.CurrentlySelectedElement+1,AvPMenus.NumberOfElementsInMenu);
            text(position,ScreenDescriptorBlock.SDB_Width/2,y+MUL_FIXED(4,textScaleFixed),ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
        }
        if(devControls){
            text((char*)DevViewmodelHelp(),MENU_CENTREX,y+MUL_FIXED(22,textScaleFixed),ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
            text((char*)DevViewmodelStatus(),MENU_CENTREX,y+MUL_FIXED(40,textScaleFixed),ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
        }
    }
	if (inGameMenuText) D3D_SetInGameMenuTextRenderActive(0);

	if (AvPMenus.MenusState == MENUSSTATE_MAINMENUS)
	{
        if(TyrargoMainMenu()&&!TyrargoMenuVersionStarted){TyrargoMenuVersionStart=SDL_GetTicks();TyrargoMenuVersionStarted=1;}
		char *textPtr = GetTextString(AvPMenusData[AvPMenus.CurrentMenu].MenuTitle);
		RenderMenuText(textPtr,MENU_CENTREX,AvPMenus.CurrentMenu==AVPMENU_SINGLEPLAYER?15:(TyrargoMainMenu()?21:(IsMissionBriefingMenu()?70-BRIEFING_HEADER_LIFT:70)),ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
#if 1
		if (AvPMenusData[AvPMenus.CurrentMenu].MenuTitle==TEXTSTRING_MAINMENU_TITLE)
        {
            extern void RenderTyrargoTitle(int centreX,int y,int alpha,double pulse);
            Uint64 elapsed=SDL_GetTicks()-TyrargoMenuVersionStart;
            RenderTyrargoTitle(MENU_CENTREX,60,TyrargoVersionOpacity(elapsed),0.88+0.12*sin((double)elapsed*6.283185307179586/4400.0));
        }
#endif
	}
}

void ToggleFPSCounterFromConsole(void)
{
	if (FPSDisplayPosition == FPS_DISPLAY_OFF) FPSDisplayPosition = FPS_DISPLAY_TOP_RIGHT;
	else FPSDisplayPosition = FPS_DISPLAY_OFF;
}

int LengthOfSmallMenuText(char *textPtr);

void RenderFPSCounter(void)
{
 char text[24];int x,y,textScaleFixed,width,digitWidth=0,digits=3;
 int right=(FPSDisplayPosition==FPS_DISPLAY_TOP_RIGHT || FPSDisplayPosition==FPS_DISPLAY_BOTTOM_RIGHT);
 if(FPSDisplayPosition<=FPS_DISPLAY_OFF || FPSDisplayPosition>FPS_DISPLAY_BOTTOM_LEFT)return;
 textScaleFixed=D3D_GetConsoleTextScaleFixed();
 sprintf(text,"FPS %d",FrameRate);
 /* Keep a compact 3-digit field stable across 99 -> 100.
    Grow only for genuinely larger counters, never reserve six empty places. */
 for(int n=FrameRate;n>=1000;n/=10)digits++;
 for(char digit='0';digit<='9';digit++){
  char sample[2]={digit,0};int w=LengthOfSmallMenuText(sample);if(w>digitWidth)digitWidth=w;
 }
 width=MUL_FIXED(LengthOfSmallMenuText("FPS ")+digits*digitWidth,textScaleFixed);
 y=(FPSDisplayPosition==FPS_DISPLAY_BOTTOM_RIGHT || FPSDisplayPosition==FPS_DISPLAY_BOTTOM_LEFT)?ScreenDescriptorBlock.SDB_Height-MUL_FIXED(HUD_FONT_HEIGHT,textScaleFixed)-16:16;
 x=right?ScreenDescriptorBlock.SDB_Width-16:16;
 HUD_ResolveFPSCounterPlacement(FPSDisplayPosition,width,MUL_FIXED(HUD_FONT_HEIGHT,textScaleFixed),&x,&y);
 D3D_SetFPSCounterTextRenderActive(1);
 if(right){
  /* Keep the label fixed and the numeric field flush with the right anchor.
     Unused digits belong between label and value, not in the screen margin. */
  Hardware_RenderSmallMenuText("FPS",x-width,y,ONE_FIXED,AVPMENUFORMAT_LEFTJUSTIFIED);
  Hardware_RenderSmallMenuText(text+4,x,y,ONE_FIXED,AVPMENUFORMAT_RIGHTJUSTIFIED);
 }else{
  Hardware_RenderSmallMenuText(text,x,y,ONE_FIXED,AVPMENUFORMAT_LEFTJUSTIFIED);
 }
 D3D_SetFPSCounterTextRenderActive(0);
}
static void RenderBriefingScreenInfo(void)
{
	enum TEXTSTRING_ID textID;
	switch (AvP.PlayerType)
	{
		case I_Marine:
		{
			textID = MarineEpisodeToPlay+TEXTSTRING_MARINELEVELS_1;
			break;
		}
		case I_Predator:
		{
			textID = PredatorEpisodeToPlay+TEXTSTRING_PREDATORLEVELS_1;
			break;
		}
		case I_Alien:
		{
			textID = AlienEpisodeToPlay+TEXTSTRING_ALIENLEVELS_1;
			break;
		}
	}
	RenderMenuText(GetTextString(textID),MENU_LEFTXEDGE,135-BRIEFING_HEADER_LIFT,ONE_FIXED,AVPMENUFORMAT_LEFTJUSTIFIED);
#if 0
	switch (AvP.PlayerType)
	{
		case I_Marine:
		{
			textID = MarineEpisodeToPlay+TEXTSTRING_LEVELBRIEFING_MARINELEVELS_1;
			break;
		}
		case I_Predator:
		{
			textID = PredatorEpisodeToPlay+TEXTSTRING_LEVELBRIEFING_PREDATORLEVELS_1;
			break;
		}
		case I_Alien:
		{
			textID = AlienEpisodeToPlay+TEXTSTRING_LEVELBRIEFING_ALIENLEVELS_1;
			break;
		}
	}
	RenderMenuText(GetTextString(textID),MENU_LEFTXEDGE,180,ONE_FIXED/2,AVPMENUFORMAT_LEFTJUSTIFIED);
#endif	
	/* Move this selection-screen briefing only; loading/in-game briefings
     * retain their own placement. The difficulty list stays anchored below. */
    RenderBriefingText(ScreenDescriptorBlock.SDB_Height/2-65,ONE_FIXED);
}
/* KJL 12:11:18 24/09/98 - specialised code to handle episode selection screen, which
has features which make it too awkward to add to the general system */
static void RenderEpisodeSelectMenu(void)
{
	AVPMENU_ELEMENT *elementPtr = &AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
	int currentEpisode = *(elementPtr->c.SliderValuePtr);
	int centrePosition = (currentEpisode)*65536+EpisodeSelectScrollOffset;
	enum AVPMENUGFX_ID graphicID;
	I_PLAYER_TYPE playerID;
	int i;
	int numberOfBasicLevels;
	
	switch (AvPMenus.CurrentMenu)
	{
		default:
		{
			LOCALASSERT(0);/* Panic */
		}
		case AVPMENU_MARINELEVELS:
		{
			graphicID = AVPMENUGFX_MARINE_EPISODE1;
			numberOfBasicLevels = MAX_NO_OF_BASIC_MARINE_EPISODES;
			playerID = I_Marine;
			break;
		}
		case AVPMENU_PREDATORLEVELS:
		{
			graphicID = AVPMENUGFX_PREDATOR_EPISODE1;
			numberOfBasicLevels = MAX_NO_OF_BASIC_PREDATOR_EPISODES;
			playerID = I_Predator;
			break;
		}
		case AVPMENU_ALIENLEVELS:
		{
			graphicID = AVPMENUGFX_ALIEN_EPISODE1;
			numberOfBasicLevels = MAX_NO_OF_BASIC_ALIEN_EPISODES;
			playerID = I_Alien;
			break;
		}
	}
	{
		char *textPtr = GetTextString(AvPMenusData[AvPMenus.CurrentMenu].MenuTitle);
		RenderMenuText(textPtr,MENU_CENTREX,70,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
	}
	for (i=0; i<=elementPtr->b.MaxSliderValue; i++)
	{
		int y;

		y = MUL_FIXED(i*65536-centrePosition,100);

		if (y>=-150 && y<=150)
		{
			char *textPtr = GetTextString(elementPtr->a.TextDescription+i);
			int b;
			int targetBrightness;
			
			if (i==currentEpisode)
			{
				targetBrightness = BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT;
			}
			else
			{ 
				targetBrightness = BRIGHTNESS_OF_DARKENED_ELEMENT;
			}

			if (targetBrightness > Brightness[i])
			{
				Brightness[i]+=BRIGHTNESS_CHANGE_SPEED;
				if(Brightness[i]>targetBrightness)
				{
					Brightness[i] = targetBrightness;
				}
			}
			else
			{
				Brightness[i]-=BRIGHTNESS_CHANGE_SPEED;
				if(Brightness[i]<targetBrightness)
				{
					Brightness[i] = targetBrightness;
				}
			}

			b=Brightness[i];
			{
				int yCoord = MENU_CENTREY+y-60;

				RenderMenuText_Clipped(textPtr,MENU_LEFTXEDGE+150, yCoord, b,AVPMENUFORMAT_LEFTJUSTIFIED,MENU_CENTREY-60-100,MENU_CENTREY-60+180);
				DrawAvPMenuGfx_Clipped(graphicID+i, MENU_LEFTXEDGE, yCoord, b,AVPMENUFORMAT_LEFTJUSTIFIED, MENU_CENTREY-60-100, MENU_CENTREY-60+180);

				if (MaximumSelectableLevel>=i)
				{
					char *completedTextPtr;

					if (i<numberOfBasicLevels)
					{
						completedTextPtr = GetTextString(UserProfilePtr->LevelCompleted[playerID][i]>=5 ? TEXTSTRING_COMPLETED_ACID_NEW : UserProfilePtr->LevelCompleted[playerID][i]>=4 ? TEXTSTRING_COMPLETED_IMPOSSIBLE_NEW : TEXTSTRING_NOTYETCOMPLETED+UserProfilePtr->LevelCompleted[playerID][i]);
					}
					else
					{
						if (UserProfilePtr->LevelCompleted[playerID][i])
						{
							completedTextPtr = GetTextString(TEXTSTRING_COMPLETED);
						}	
						else
						{
							completedTextPtr = GetTextString(TEXTSTRING_NOTYETCOMPLETED);
						}
					}
					
					RenderSmallMenuText
					(
						completedTextPtr,
						MENU_LEFTXEDGE+150,
						yCoord+30,
						b,
						AVPMENUFORMAT_LEFTJUSTIFIED
					);
				}
				else
				{
					if (i == elementPtr->b.MaxSliderValue)
					{
						RenderSmallMenuText
						(
							GetTextString(TEXTSTRING_NOTYETAVAILABLE_2),
							MENU_LEFTXEDGE+150,
							yCoord+30,
							b,
							AVPMENUFORMAT_LEFTJUSTIFIED
						);
					}
					else
					{
						RenderSmallMenuText
						(
							GetTextString(TEXTSTRING_NOTYETAVAILABLE_1),
							MENU_LEFTXEDGE+150,
							yCoord+30,
							b,
							AVPMENUFORMAT_LEFTJUSTIFIED
						);
					}
				}
				/*
					,MENU_CENTREY-60-100,
					MENU_CENTREY-60+180
				);*/
			}
		}
	}

	if (EpisodeSelectScrollOffset>0)
	{
		EpisodeSelectScrollOffset -= MUL_FIXED(EpisodeSelectScrollOffset*2+8192,RealFrameTime<<1);
		if (EpisodeSelectScrollOffset<0)
		{
			EpisodeSelectScrollOffset=0;
		}
	}
	else if (EpisodeSelectScrollOffset<0)
	{
		EpisodeSelectScrollOffset += MUL_FIXED(-EpisodeSelectScrollOffset*2+8192,RealFrameTime<<1);
		if (EpisodeSelectScrollOffset>0)
		{
			EpisodeSelectScrollOffset=0;
		}
	}

	
}
static void RenderInGameKeyConfiguration(void)
{
 int scale=D3D_GetConsoleTextScaleFixed(),count=AvPMenus.NumberOfElementsInMenu;
 int row=MUL_FIXED(20,scale),pad=MUL_FIXED(10,scale),help=0,i,top,bottom,first,last,used;
 char lines[64][512];
 for(i=0;i<count;i++){
  AVPMENU_ELEMENT *e=&AvPMenus.MenuElements[i];
  int n=WrapMenuHelp(e->HelpString==TEXTSTRING_BLANK?"":GetTextString(e->HelpString),lines,ScreenDescriptorBlock.SDB_Width-48,scale);
  int reserve=MenuHelpVisibleLines(n,scale)*MUL_FIXED(17,scale)+32;
  if(reserve>help)help=reserve;
 }
 D3D_SetInGameMenuTextRenderActive(1);InGameMenuTextUsesNativeAnchors=1;
 if(MenuPreviewTarget && MenuPreviewThicknessSelected()){
  RenderPreviewRow(&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement],AvPMenus.CurrentlySelectedElement,
                   ScreenDescriptorBlock.SDB_Height/2-row/2,scale);
 }else{
  int headerY;MenuKeyConfigBounds(ScreenDescriptorBlock.SDB_Height,scale,help,&headerY,&top,&bottom);
  for(i=0;i<2;i++){
   AvPMenus.MenuElements[i].Brightness=i==AvPMenus.CurrentlySelectedElement?BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT:BRIGHTNESS_OF_DARKENED_ELEMENT;
   RenderMenuElement(&AvPMenus.MenuElements[i],i,headerY+i*row);
  }
  Hardware_RenderKeyConfigRectangle(AvPMenus.CurrentlySelectedElement>=2?ONE_FIXED:ONE_FIXED/4,top,bottom);
  if(count>2){
   int heights[count-2],selected=AvPMenus.CurrentlySelectedElement-2;
   for(i=0;i<count-2;i++)heights[i]=MUL_FIXED(HeightOfMenuElement(&AvPMenus.MenuElements[i+2]),scale);
   MenuVisibleRange(heights,count-2,selected,bottom-top-2*pad,&first,&last,&used);
   int y=top+pad;
   for(i=first;i<=last;i++){
    AVPMENU_ELEMENT *e=&AvPMenus.MenuElements[i+2];
    e->Brightness=i+2==AvPMenus.CurrentlySelectedElement?BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT:BRIGHTNESS_OF_DARKENED_ELEMENT;
    RenderMenuElement(e,i+2,y);y+=heights[i];
   }
  }
 }
 InGameMenuTextUsesNativeAnchors=0;D3D_SetInGameMenuTextRenderActive(0);
}
static void RenderKeyConfigurationMenu(void)
{
 if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS){RenderInGameKeyConfiguration();return;}
	AVPMENU_ELEMENT *elementPtr = AvPMenus.MenuElements;//AvPMenus.CurrentlySelectedElement];
	int centrePosition;
	int i;
	int centreY = ScreenDescriptorBlock.SDB_Height/2+25;
	int y;
	int keyConfigFrameAlpha = 0;
	int keyConfigListTop = -100;
	int keyConfigListBottom = 100;
	const int inGameMenuText = (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS);
	const int textScaleFixed = inGameMenuText ? D3D_GetConsoleTextScaleFixed() : ONE_FIXED;
	
	if (AvPMenus.MenusState == MENUSSTATE_MAINMENUS)
	{
		char *textPtr = GetTextString(AvPMenusData[AvPMenus.CurrentMenu].MenuTitle);
		int b;
		if (AvPMenus.CurrentlySelectedElement>=2)
		{
			b = ONE_FIXED;
		}
		else
		{
			b = ONE_FIXED/4;
		}
		
		RenderKeyConfigRectangle(b);
		RenderMenuText(textPtr,MENU_CENTREX,70,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
	}
	else
	{
		if (AvPMenus.CurrentlySelectedElement>=2)
		{
			keyConfigFrameAlpha = ONE_FIXED;
		}
		else
		{
			keyConfigFrameAlpha = ONE_FIXED/4;
		}
	}
	if (inGameMenuText)
	{
		D3D_SetInGameMenuTextRenderActive(1);
		InGameMenuTextUsesNativeAnchors = 1;
	}		       
	y = centreY-MUL_FIXED(160, textScaleFixed);
	for (i = 0; i<2; i++, elementPtr++)
	{
		int targetBrightness;

		if (i==AvPMenus.CurrentlySelectedElement)
		{
			targetBrightness = BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT;
		}
		else
		{ 
			targetBrightness = BRIGHTNESS_OF_DARKENED_ELEMENT;
		}

		if (targetBrightness > elementPtr->Brightness)
		{
			elementPtr->Brightness+=BRIGHTNESS_CHANGE_SPEED;
			if(elementPtr->Brightness>targetBrightness)
			{
				elementPtr->Brightness = targetBrightness;
			}
		}
		else
		{
			elementPtr->Brightness-=BRIGHTNESS_CHANGE_SPEED;
			if(elementPtr->Brightness<targetBrightness)
			{
				elementPtr->Brightness = targetBrightness;
			}
			
		}
		
		RenderMenuElement(elementPtr,i,y);
		y += MUL_FIXED(HeightOfMenuElement(elementPtr), textScaleFixed);
	}
	if (inGameMenuText)
	{
		int keyConfigFrameTop = y + MUL_FIXED(10, textScaleFixed);
		int keyConfigFrameBottom = ScreenDescriptorBlock.SDB_Height - 80;
		if (keyConfigFrameTop < 150) keyConfigFrameTop = 150;
		Hardware_RenderKeyConfigRectangle(keyConfigFrameAlpha, keyConfigFrameTop, keyConfigFrameBottom);
		keyConfigListTop = keyConfigFrameTop + MUL_FIXED(15, textScaleFixed) - centreY;
		keyConfigListBottom = keyConfigFrameBottom - MUL_FIXED(15, textScaleFixed) - centreY;
	}
	centrePosition = (AvPMenus.CurrentlySelectedElement) * ONE_FIXED;
	if (centrePosition<2*ONE_FIXED) centrePosition = 2*ONE_FIXED;
	for (i=2; i<AvPMenus.NumberOfElementsInMenu; i++,elementPtr++)
	{

		y = MUL_FIXED(MUL_FIXED(i*65536-centrePosition,20), textScaleFixed);

		if (y>=keyConfigListTop && y<=keyConfigListBottom)
		{
//			char *textPtr = GetTextString(elementPtr->TextDescription);
			int targetBrightness;

			if (i==AvPMenus.CurrentlySelectedElement)
			{
				targetBrightness = BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT;
			}
			else
			{ 
				targetBrightness = BRIGHTNESS_OF_DARKENED_ELEMENT;
			}

			#if 0
			if (targetBrightness > elementPtr->Brightness)
			{
				elementPtr->Brightness+=BRIGHTNESS_CHANGE_SPEED;
				if(elementPtr->Brightness>targetBrightness)
				{
					elementPtr->Brightness = targetBrightness;
				}
			}
			else
			{
				elementPtr->Brightness-=BRIGHTNESS_CHANGE_SPEED;
				if(elementPtr->Brightness<targetBrightness)
				{
					elementPtr->Brightness = targetBrightness;
				}
				
			}
			#else
			elementPtr->Brightness = targetBrightness;
			#endif
			RenderMenuElement(elementPtr, i, centreY+y);
			#if 0
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
			{
				Hardware_RenderSmallMenuText(textPtr,MENU_LEFTXEDGE+150,centreY+y,b,AVPMENUFORMAT_LEFTJUSTIFIED/*,MENU_CENTREY-60-100,MENU_CENTREY-60+180*/);
			}
			else
			{
				RenderSmallMenuText(textPtr,MENU_LEFTXEDGE+150,centreY+y,b,AVPMENUFORMAT_LEFTJUSTIFIED/*,MENU_CENTREY-60-100,MENU_CENTREY-60+180*/);
			}
			#endif

		}
	}

	if (EpisodeSelectScrollOffset>0)
	{
		EpisodeSelectScrollOffset -= MUL_FIXED(EpisodeSelectScrollOffset*2+8192,RealFrameTime<<1);
		if (EpisodeSelectScrollOffset<0)
		{
			EpisodeSelectScrollOffset=0;
		}
	}
	else if (EpisodeSelectScrollOffset<0)
	{
		EpisodeSelectScrollOffset += MUL_FIXED(-EpisodeSelectScrollOffset*2+8192,RealFrameTime<<1);
		if (EpisodeSelectScrollOffset>0)
		{
			EpisodeSelectScrollOffset=0;
		}
	}
	if (inGameMenuText)
	{
		InGameMenuTextUsesNativeAnchors = 0;
		D3D_SetInGameMenuTextRenderActive(0);
	}
}

static void RenderScrollyMenu()
{
	AVPMENU_ELEMENT *elementPtr = AvPMenus.MenuElements;
	{
		//draw the title
		char *textPtr = GetTextString(AvPMenusData[AvPMenus.CurrentMenu].MenuTitle);
		RenderMenuText(textPtr,MENU_CENTREX,70,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
	}

	{
		int first=AvPMenus.CurrentlySelectedElement;
		int last=AvPMenus.CurrentlySelectedElement;
		int i;
		int y;
		BOOL done=FALSE;

		int available_above=(MENU_HEIGHT-HeightOfMenuElement(&elementPtr[AvPMenus.CurrentlySelectedElement]))/2;
		int available_below=available_above;

		//work out the first and last element to be drawn
		do
		{
			done=TRUE;
			if(first-1>=0)
			{
				int h=HeightOfMenuElement(&elementPtr[first-1]);
				if(h<=available_above)
				{
					available_above-=h;
					first--;
					done=FALSE;
				}
				else
				{
					available_below+=available_above;
					available_above=0;
				}
			}
			if(first==0)
			{
				//no more elements above selected element
				available_below+=available_above;
				available_above=0;
			}

			if(last+1<AvPMenus.NumberOfElementsInMenu)
			{
				int h=HeightOfMenuElement(&elementPtr[last+1]);
				if(h<=available_below)
				{
					available_below-=h;
					last++;
					done=FALSE;
				}
			}
			if(last==(AvPMenus.NumberOfElementsInMenu-1))
			{
				//no more elements below selected element
				available_above+=available_below;
				available_below=0;
			}
		}
		while(!done);
		
		
		//draw the appropriate elements
		elementPtr=&elementPtr[first];
		y=MENU_TOPY;
		for(i=first;i<=last;i++,elementPtr++)
		{
//			char *textPtr = GetTextString(elementPtr->TextDescription);
			int targetBrightness;

			if (i==AvPMenus.CurrentlySelectedElement)
			{
				targetBrightness = BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT;
			}
			else
			{ 
				targetBrightness = BRIGHTNESS_OF_DARKENED_ELEMENT;
			}

			elementPtr->Brightness = targetBrightness;
			
			RenderMenuElement(elementPtr, i, y);
			y+=HeightOfMenuElement(elementPtr);
		}
		
	}
	
}


static void RenderUserProfileSelectMenu(void)
{
	AVPMENU_ELEMENT *elementPtr = &AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
	int currentEpisode = *(elementPtr->c.SliderValuePtr);
	int centrePosition = (currentEpisode)*65536+EpisodeSelectScrollOffset;
	int i;
	AVP_USER_PROFILE *profilePtr = GetFirstUserProfile();
	{
		char *textPtr = GetTextString(AvPMenusData[AvPMenus.CurrentMenu].MenuTitle);
		RenderMenuText(textPtr,MENU_CENTREX,70,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
	}

	for (i=0; i<=elementPtr->b.MaxSliderValue; i++, profilePtr = GetNextUserProfile())
	{
		int y;

		y = MUL_FIXED(i*65536-centrePosition,80);

		if (y>=-150 && y<=150)
		{
            char *textPtr = ProfileMenuLabel(profilePtr->FullName);
			time_t FileTime = profilePtr->FileTime;
			int b;
			int targetBrightness;

			if (i==currentEpisode)
			{
				targetBrightness = BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT;
			}
			else
			{ 
				targetBrightness = BRIGHTNESS_OF_DARKENED_ELEMENT;
			}

			if (targetBrightness > Brightness[i])
			{
				Brightness[i]+=BRIGHTNESS_CHANGE_SPEED;
				if(Brightness[i]>targetBrightness)
				{
					Brightness[i] = targetBrightness;
				}
			}
			else
			{
				Brightness[i]-=BRIGHTNESS_CHANGE_SPEED;
				if(Brightness[i]<targetBrightness)
				{
					Brightness[i] = targetBrightness;
				}
			}
			b=Brightness[i];
			RenderMenuText_Clipped(textPtr,MENU_CENTREX,MENU_CENTREY+y-60,b,AVPMENUFORMAT_CENTREJUSTIFIED,MENU_CENTREY-60-100,MENU_CENTREY-30+150);
			if (i > 0)
				RenderSmallMenuText(ctime(&FileTime),MENU_CENTREX,MENU_CENTREY+y-30,b,AVPMENUFORMAT_CENTREJUSTIFIED);
		}
	}

	if (EpisodeSelectScrollOffset>0)
	{
		EpisodeSelectScrollOffset -= MUL_FIXED(EpisodeSelectScrollOffset*2+8192,RealFrameTime<<1);
		if (EpisodeSelectScrollOffset<0)
		{
			EpisodeSelectScrollOffset=0;
		}
	}
	else if (EpisodeSelectScrollOffset<0)
	{
		EpisodeSelectScrollOffset += MUL_FIXED(-EpisodeSelectScrollOffset*2+8192,RealFrameTime<<1);
		if (EpisodeSelectScrollOffset>0)
		{
			EpisodeSelectScrollOffset=0;
		}
	}

	
}

extern int RenderSmallMenuText_QuickLoad(char*,int,int,int,enum AVPMENUFORMAT_ID);
extern int Hardware_RenderSmallMenuText_QuickLoad(char*,int,int,int,enum AVPMENUFORMAT_ID);
static int QuickLoadText(char *text,int x,int y,int alpha,enum AVPMENUFORMAT_ID format)
{
 if(AvPMenus.CurrentlySelectedElement==0)return RenderSmallMenuText_QuickLoad(text,x,y,alpha,format);
 return RenderSmallMenuText_Coloured(text,x,y,alpha,format,178*ONE_FIXED/255,24*ONE_FIXED/255,240*ONE_FIXED/255);
}
static int HardwareQuickLoadText(char *text,int x,int y,int alpha,enum AVPMENUFORMAT_ID format)
{
 if(AvPMenus.CurrentlySelectedElement==0)return Hardware_RenderSmallMenuText_QuickLoad(text,x,y,alpha,format);
 return Hardware_RenderSmallMenuText_Coloured(text,x,y,alpha,format,178*ONE_FIXED/255,24*ONE_FIXED/255,240*ONE_FIXED/255);
}

static void RenderLoadGameMenu(void)
{
	AVPMENU_ELEMENT *elementPtr;
	int e;
	int y;
	int first;
	int last;
	int visibleRows;
	int visibleHeight = 0;
	int (*RenderText)(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format);
	const int inGameMenuText = (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS);
	const int textScaleFixed = inGameMenuText ? D3D_GetConsoleTextScaleFixed() : ONE_FIXED;

	/* 50 manual rows plus one read-only Quick Load row; render a stable nine-row window. */
	if (AvPMenus.NumberOfElementsInMenu <= 0)
		return;
	if (inGameMenuText) D3D_SetInGameMenuTextRenderActive(1);
	if (AvPMenus.CurrentlySelectedElement < 0)
		AvPMenus.CurrentlySelectedElement = 0;
	if (AvPMenus.CurrentlySelectedElement >= AvPMenus.NumberOfElementsInMenu)
		AvPMenus.CurrentlySelectedElement = AvPMenus.NumberOfElementsInMenu - 1;

    if(!inGameMenuText){
        int count=AvPMenus.NumberOfElementsInMenu,heights[count],top=MENU_TOPY;
        int bottom=ScreenDescriptorBlock.SDB_Height-MenuHelpReserve()-12;
        for(e=0;e<count;e++)heights[e]=HeightOfMenuElement(&AvPMenus.MenuElements[e]);
        MenuVisibleRange(heights,count,AvPMenus.CurrentlySelectedElement,bottom-top,&first,&last,&visibleHeight);
        y=top;RenderText=RenderSmallMenuText;
    }else{
	visibleRows = AvPMenus.NumberOfElementsInMenu;
	if (visibleRows > 9)
		visibleRows = 9;
	first = AvPMenus.CurrentlySelectedElement - visibleRows / 2;
	if (first < 0)
		first = 0;
	if (first + visibleRows > AvPMenus.NumberOfElementsInMenu)
		first = AvPMenus.NumberOfElementsInMenu - visibleRows;
	last = first + visibleRows - 1;

	for (e = first; e <= last; e++)
		visibleHeight += MUL_FIXED(HeightOfMenuElement(&AvPMenus.MenuElements[e]), textScaleFixed);

	if (AvPMenus.MenusState == MENUSSTATE_MAINMENUS)
	{
		y = MENU_TOPY;
		RenderText = RenderSmallMenuText;
	}
	else
	{
		y = (ScreenDescriptorBlock.SDB_Height - visibleHeight) / 2;
		RenderText = Hardware_RenderSmallMenuText;
	}

    }
	elementPtr = &AvPMenus.MenuElements[first];
	for (e = first; e <= last; e++, elementPtr++)
	{
		char buffer[100];
		int isQuickLoadRow = (AvPMenus.CurrentMenu == AVPMENU_LOADGAME && e == 0);
		int slot = isQuickLoadRow ? QUICK_SAVE_SLOT : ((AvPMenus.CurrentMenu == AVPMENU_LOADGAME) ? e - 1 : e);
		SAVE_SLOT_HEADER *slotPtr = &SaveGameSlot[slot];
		int targetBrightness;
        int (*RenderRowText)(char *,int,int,int,enum AVPMENUFORMAT_ID) =
            isQuickLoadRow ? (inGameMenuText ? HardwareQuickLoadText : QuickLoadText) : RenderText;

		if (e == AvPMenus.CurrentlySelectedElement)
		{
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				Hardware_RenderHighlightRectangle(MENU_LEFTXEDGE, y - MUL_FIXED(2, textScaleFixed), MENU_RIGHTXEDGE, y + MUL_FIXED(4 + HUD_FONT_HEIGHT * 2, textScaleFixed), 0, 128, isQuickLoadRow ? 192 : 0);
			else
				RenderHighlightRectangle(MENU_LEFTXEDGE, y - 2, MENU_RIGHTXEDGE, y + 4 + HUD_FONT_HEIGHT * 2, 0, 128, isQuickLoadRow ? 192 : 0);
			targetBrightness = BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT;
		}
		else
		{
			targetBrightness = BRIGHTNESS_OF_DARKENED_ELEMENT;
		}

		if (targetBrightness > elementPtr->Brightness)
		{
			elementPtr->Brightness += BRIGHTNESS_CHANGE_SPEED;
			if (elementPtr->Brightness > targetBrightness) elementPtr->Brightness = targetBrightness;
		}
		else
		{
			elementPtr->Brightness -= BRIGHTNESS_CHANGE_SPEED;
			if (elementPtr->Brightness < targetBrightness) elementPtr->Brightness = targetBrightness;
		}

		if (isQuickLoadRow)
		{
			/* Right-aligned label replaces the quick-slot difficulty field. */
			RenderRowText(GetTextString(TEXTSTRING_GLOBAL_QUICKLOAD_NEW), MENU_RIGHTXEDGE - 30, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
		}
		else
		{
			sprintf(buffer, "%d.", (AvPMenus.CurrentMenu == AVPMENU_LOADGAME) ? e : e + 1);
			RenderRowText(buffer, MENU_LEFTXEDGE + 20, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
		}
		if (slotPtr->SlotUsed)
		{
			int textID;
			int numberOfBasicEpisodes;
			switch (slotPtr->Species)
			{
				case I_Marine: textID = TEXTSTRING_MARINELEVELS_1; numberOfBasicEpisodes = MAX_NO_OF_BASIC_MARINE_EPISODES; break;
				case I_Alien: textID = TEXTSTRING_ALIENLEVELS_1; numberOfBasicEpisodes = MAX_NO_OF_BASIC_ALIEN_EPISODES; break;
				case I_Predator:
				default: textID = TEXTSTRING_PREDATORLEVELS_1; numberOfBasicEpisodes = MAX_NO_OF_BASIC_PREDATOR_EPISODES; break;
			}
				sprintf(buffer, "%s", GetTextString(TEXTSTRING_MULTIPLAYER_MARINE + slotPtr->Species));
				RenderRowText(buffer, MENU_LEFTXEDGE + 30, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			sprintf(buffer, "%s", GetTextString(textID + slotPtr->Episode));
			RenderRowText(buffer, MENU_CENTREX, y, elementPtr->Brightness, AVPMENUFORMAT_CENTREJUSTIFIED);
			if (numberOfBasicEpisodes > slotPtr->Episode && !isQuickLoadRow)
			{
				if(slotPtr->AcidTier)snprintf(buffer,sizeof buffer,"%s %s",GetTextString(TEXTSTRING_MOLECULAR_ACID_NEW),AcidMultiplierLabel(slotPtr->AcidTier));
                else sprintf(buffer, "%s", GetTextString(slotPtr->Difficulty==I_Impossible ? TEXTSTRING_DIFFICULTY_IMPOSSIBLE_NEW : TEXTSTRING_DIFFICULTY_EASY + slotPtr->Difficulty));
				RenderRowText(buffer, MENU_RIGHTXEDGE - 30, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			}
			sprintf(buffer, "%s %02d:%02d:%02d", GetTextString(TEXTSTRING_GAMESTATS_TIMEELAPSED), slotPtr->ElapsedTime_Hours, slotPtr->ElapsedTime_Minutes, slotPtr->ElapsedTime_Seconds);
			RenderRowText(buffer, MENU_LEFTXEDGE + 30, y + MUL_FIXED(HUD_FONT_HEIGHT + 1, textScaleFixed), elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			sprintf(buffer, "%s: %d", GetTextString(TEXTSTRING_SAVEGAME_SAVESLEFT), slotPtr->SavesLeft);
			RenderRowText(buffer, MENU_CENTREX, y + MUL_FIXED(HUD_FONT_HEIGHT + 1, textScaleFixed), elementPtr->Brightness, AVPMENUFORMAT_CENTREJUSTIFIED);
			RenderRowText(ctime(&slotPtr->TimeStamp), MENU_RIGHTXEDGE - 30, y + MUL_FIXED(HUD_FONT_HEIGHT + 1, textScaleFixed), elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
		}
		else
		{
			RenderRowText(GetTextString(TEXTSTRING_SAVEGAME_EMPTYSLOT), MENU_CENTREX, y, elementPtr->Brightness, AVPMENUFORMAT_CENTREJUSTIFIED);
		}
		y += MUL_FIXED(HeightOfMenuElement(elementPtr), textScaleFixed);
	}

	if (AvPMenus.MenusState == MENUSSTATE_MAINMENUS)
	{
		RenderMenuText(GetTextString(AvPMenusData[AvPMenus.CurrentMenu].MenuTitle), MENU_CENTREX, 70, ONE_FIXED, AVPMENUFORMAT_CENTREJUSTIFIED);
	}
	else
	{
		int listTop = (ScreenDescriptorBlock.SDB_Height - visibleHeight) / 2;
		AVPMENU_ELEMENT *selectedElement = &AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
		RenderText(GetTextString(AvPMenusData[AvPMenus.CurrentMenu].MenuTitle), MENU_CENTREX, listTop - MUL_FIXED(30, textScaleFixed), ONE_FIXED, AVPMENUFORMAT_CENTREJUSTIFIED);
		RenderText(GetTextString(selectedElement->HelpString), MENU_CENTREX, listTop + visibleHeight + MUL_FIXED(20, textScaleFixed), ONE_FIXED, AVPMENUFORMAT_CENTREJUSTIFIED);
	}
	if (inGameMenuText) D3D_SetInGameMenuTextRenderActive(0);
}

extern void D3D_RenderHUDStringOutlined(char *text,int x,int y,int colour,int radius);
static int MenuHelpNeedsOutline(int y,int lineHeight)
{
 if(AvPMenus.MenusState!=MENUSSTATE_INGAMEMENUS || AvPMenus.CurrentMenu!=AVPMENU_INGAMEAVOPTIONS)return 0;
 if(AvPMenus.CurrentlySelectedElement<0 || AvPMenus.CurrentlySelectedElement>=AvPMenus.NumberOfElementsInMenu)return 0;
 if(MenuColourBarsAlpha>0){
  int scale=DIV_FIXED(ScreenDescriptorBlock.SDB_Height,480),h=scale/2048,offset=scale/4096;
  int top=ScreenDescriptorBlock.SDB_Height-offset-h,bottom=ScreenDescriptorBlock.SDB_Height-offset;
  if(y<bottom && y+lineHeight>top)return 1;
 }
 AVPMENU_ELEMENT *e=&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
 if(e->HelpString==TEXTSTRING_FMV_SCALE_HELP_NEW || e->HelpString==TEXTSTRING_MUSIC_PLAY_MODE_HELP_NEW)return 1;
 return AvP.PlayerType==I_Alien && MenuPreviewSelected();
}

static void RenderHelpString(void)
{
 if(IsMissionBriefingMenu()||AvPMenus.CurrentMenu==AVPMENU_SINGLEPLAYER||TyrargoVersionMenu()||AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS||AvPMenus.CurrentMenu==AVPMENU_EXPERIMENTAL||AvPMenus.CurrentMenu==AVPMENU_EXPERIMENTAL2||AvPMenus.CurrentMenu==AVPMENU_MAINMENUAVOPTIONS||AvPMenus.CurrentMenu==AVPMENU_JOYSTICKCONTROLS||AvPMenus.CurrentMenu==AVPMENU_DETAILLEVELS||AvPMenus.MenuHeight>MENU_HEIGHT){
  char lines[64][512];int scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
  int count=WrapMenuHelp(MenuHelpText(),lines,ScreenDescriptorBlock.SDB_Width-48,scale);
  int lineHeight=MUL_FIXED(17,scale),visible=MenuHelpVisibleLines(count,scale),first=0,i,y;
  static int previousMenu=-1,previousElement=-1,previousScale=-1;static Uint64 start=0;
  if(previousMenu!=AvPMenus.CurrentMenu||previousElement!=AvPMenus.CurrentlySelectedElement||previousScale!=scale){start=SDL_GetTicks();previousMenu=AvPMenus.CurrentMenu;previousElement=AvPMenus.CurrentlySelectedElement;previousScale=scale;}
  if(!count)return;
  /* Preserve the selected text size. Long descriptions page, never shrink. */
  if(count>visible)first=(int)(((SDL_GetTicks()-start)/6500)%((count+visible-1)/visible))*visible;
  y=ScreenDescriptorBlock.SDB_Height-16-visible*lineHeight-TyrargoVersionReserve();
  if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS)D3D_SetInGameMenuTextRenderActive(1);
  SetMenuAVContrast(AvPMenus.MenusState==MENUSSTATE_MAINMENUS&&AvPMenus.CurrentMenu==AVPMENU_MAINMENUAVOPTIONS?3:0);
  for(i=0;i<visible&&first+i<count;i++){
   if(MenuHelpNeedsOutline(y+i*lineHeight,lineHeight)){
    int width=MUL_FIXED(AvPTextStringWidth(lines[first+i],AAFontWidths),scale);
    int radius=(scale+ONE_FIXED-1)/ONE_FIXED;
    D3D_RenderHUDStringOutlined(lines[first+i],(ScreenDescriptorBlock.SDB_Width-width)/2,
                               y+i*lineHeight,0xffffffff,radius);
   }else if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS)Hardware_RenderSmallMenuText(lines[first+i],ScreenDescriptorBlock.SDB_Width/2,y+i*lineHeight,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
   else RenderSmallMenuText(lines[first+i],MENU_CENTREX,y+i*lineHeight,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
  }
  SetMenuAVContrast(0);
  if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS)D3D_SetInGameMenuTextRenderActive(0);
  return;
 }
 {AVPMENU_ELEMENT*e=&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
  if(e->HelpString!=TEXTSTRING_BLANK){
   RECT area;area.left=MENU_LEFTXEDGE;area.right=MENU_RIGHTXEDGE;area.top=420;area.bottom=ScreenDescriptorBlock.SDB_Height;
   RenderSmallFontString_Wrapped(GetTextString(e->HelpString),&area,BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT,0,0);
  }
 }
}

static void RenderConfigurationDescriptionString()
{
	const char* text=GetMultiplayerConfigDescription(AvPMenus.CurrentlySelectedElement);
	if(text)
	{
		RECT area;
		//draw the text at the bottom of the screen
		//now at the top.

		area.left=MENU_LEFTXEDGE;
		area.right=MENU_RIGHTXEDGE;
		area.top=0;
		area.bottom=60;

		RenderSmallFontString_Wrapped(text,&area,BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT,0,0);
	}
}

/*ATSB: since this darn input issue is still here in SDL3, let's have some alien-inspired random names instead*/
static void GenerateRandomName(char* dest, size_t maxLength)
{
	const char* names[] = {
		"Ripley", "Hicks", "Hudson", "Vasquez", "Bishop", "Ash", "Dallas", "Kane",
		"Lambert", "Parker", "Brett", "Burke", "Newt", "Clemens", "Dillon", "David",
		"Shaw", "Daniels", "Jones", "Gorman", "Apone", "Drake", "Frost", "Ferro",
		"Spunkmeyer", "Wierzbowski", "Dietrich", "Crowe", "Walter", "Tennessee"
	};
	const char* locations_creatures_etc[] = {
		"Nostromo", "Sulaco", "Hadley", "Acheron", "Fiorina", "Xenomorph", "Neomorph",
		"Facehugger", "Wey_Yu", "USCM", "Sevastopol", "Prometheus", "Covenant",
		"Jonesy", "LV-426", "Fury161", "Gateway", "Auriga", "Origae6", "TheCompany",
		"Chestburster", "Queen", "Drone", "Warrior"
	};\
	int numNames = sizeof(names) / sizeof(names[0]);
	int numLocationsEtc = sizeof(locations_creatures_etc) / sizeof(locations_creatures_etc[0]);
	int choice = rand() % 10;

	if (choice < 7) {
		const char* name = names[rand() % numNames];
		strncpy(dest, name, maxLength - 1);
		dest[maxLength - 1] = '\0';
	}
	else {
		const char* loc = locations_creatures_etc[rand() % numLocationsEtc];
		strncpy(dest, loc, maxLength - 1);
		dest[maxLength - 1] = '\0';
	}
	dest[maxLength - 1] = '\0';
}

static void ActUponUsersInput(void)
{
	static int BackspaceTimer=0;
	//Set up a keyboard repeat rate thingy for deleting long strings
	if(KeyboardInput[KEY_BACKSPACE])
	{
		BackspaceTimer+=RealFrameTime;
	}
	else
	{
		BackspaceTimer=0;
	}
	
	if (AvPMenus.UserEnteringText)
	{
		AVPMENU_ELEMENT *elementPtr = &AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];

		if (DebouncedKeyboardInput[KEY_ESCAPE] || DebouncedKeyboardInput[KEY_CR])
		{
			elementPtr->c.TextPtr[AvPMenus.PositionInTextField] = 0;
			AvPMenus.UserEnteringText = 0;

			/* A profile must never silently become a random filename. Escape cancels
			   creation; Return on an empty field keeps the cursor in the field. */
			if (AvPMenus.CurrentMenu == AVPMENU_USERPROFILEENTERNAME)
			{
				if (DebouncedKeyboardInput[KEY_ESCAPE])
				{
					SetupNewMenu(AVPMENU_USERPROFILESELECT);
					return;
				}
				if (AvPMenus.PositionInTextField == 0)
				{
					AvPMenus.UserEnteringText = 1;
					SDL_StartTextInput(window);
					return;
				}
				AvPMenus.CurrentlySelectedElement++;
			}
			else if((AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYER_SKIRMISH)
 			 ||(AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYER))
			{
				if(AvPMenus.PositionInTextField==0)
				{
					GenerateRandomName(elementPtr->c.TextPtr, elementPtr->b.MaxTextLength);
					AvPMenus.PositionInTextField = strlen(elementPtr->c.TextPtr);
				}
				AvPMenus.CurrentlySelectedElement++;
			}
		}
		else if (DebouncedKeyboardInput[KEY_BACKSPACE] || DebouncedKeyboardInput[KEY_LEFT])
		{
			if (AvPMenus.PositionInTextField>0)
			{
				elementPtr->c.TextPtr[--AvPMenus.PositionInTextField] = 0;
			}
		}
		else if(BackspaceTimer>ONE_FIXED/2)
		{
			//check for backspace being held down for a long time
			while(BackspaceTimer>ONE_FIXED/2)
			{
				BackspaceTimer-=ONE_FIXED/20;
				if (AvPMenus.PositionInTextField>0)
				{
					elementPtr->c.TextPtr[--AvPMenus.PositionInTextField] = 0;
				}
			}
		}
		else
		{
			//allow Ctrl+V to paste from the clipboard (really just for pasting in ip addresses)
			if((KeyboardInput[KEY_LEFTCTRL] || KeyboardInput[KEY_RIGHTCTRL]) && KeyboardInput[KEY_V])
			{
				PasteFromClipboard(elementPtr->c.TextPtr,elementPtr->b.MaxTextLength);
				AvPMenus.PositionInTextField = strlen(elementPtr->c.TextPtr);
			}
			else if (AvPMenus.PositionInTextField<elementPtr->b.MaxTextLength)
			{
				char c=0;
				KeyboardEntryQueue_StartProcessing();
				
				while((c=KeyboardEntryQueue_ProcessCharacter()))
				{
					if (AvPMenus.PositionInTextField<elementPtr->b.MaxTextLength)
					{
						//see if there is room for this character
						if(AvPMenus.FontToUse==AVPMENU_FONT_BIG && elementPtr->ElementID !=AVPMENU_ELEMENT_TEXTFIELD_SMALLWRAPPED)
						{
							//using large font
							//allocate 32 pixels for each new character for the moment
							//the true amount used will be worked out when the font is drawn
							//Might cause a slight glitch for fast typists , but I forgot about 
							//the damned kerned fonts until after I'd written most of this
							if(AvPMenus.WidthLeftForText<32) break;
							AvPMenus.WidthLeftForText-=32;

						}
						else
						{
							extern char AAFontWidths[256];
							//using small font
							if(AvPMenus.WidthLeftForText<AAFontWidths[(unsigned char)c]) break;
							AvPMenus.WidthLeftForText-=AAFontWidths[(unsigned char)c];
						}

						elementPtr->c.TextPtr[AvPMenus.PositionInTextField++] = c;
						elementPtr->c.TextPtr[AvPMenus.PositionInTextField] = 0;
					}
				}

			}
			KeyboardEntryQueue_Clear();
		}
	}
	else if (AvPMenus.UserEnteringNumber)
	{
		AVPMENU_ELEMENT *elementPtr = &AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];
		if (DebouncedKeyboardInput[KEY_ESCAPE] || DebouncedKeyboardInput[KEY_CR] || DebouncedKeyboardInput[KEY_NUMPADENTER])
		{
			if (elementPtr->ElementID == AVPMENU_ELEMENT_VIDEO_FPSCAP && (*elementPtr->c.NumberPtr < 1 || *elementPtr->c.NumberPtr > 1000))
				*elementPtr->c.NumberPtr = 60;
			VideoFPSCapCustomEditStartsFresh = 0;
			AvPMenus.UserEnteringNumber = 0;
		}
		else if (DebouncedKeyboardInput[KEY_BACKSPACE] || DebouncedKeyboardInput[KEY_LEFT])
		{
			if (elementPtr->ElementID == AVPMENU_ELEMENT_VIDEO_FPSCAP && VideoFPSCapCustomEditStartsFresh)
				*elementPtr->c.NumberPtr = 0;
			else
				(*elementPtr->c.NumberPtr) /= 10;
			VideoFPSCapCustomEditStartsFresh = 0;
		}
		else
		{
			int digit = -1;
			int key;

			/* SDL text-input events are locale/IME dependent. Numeric menu fields
			 * deliberately use NakedAVP's debounced physical-key path instead. */
			for (key = KEY_0; key <= KEY_9; ++key)
			{
				if (DebouncedKeyboardInput[key]) { digit = key - KEY_0; break; }
			}
			if (digit < 0)
			{
				for (key = KEY_NUMPAD0; key <= KEY_NUMPAD9; ++key)
				{
					if (DebouncedKeyboardInput[key]) { digit = key - KEY_NUMPAD0; break; }
				}
			}
			if (digit >= 0)
			{
				if (elementPtr->ElementID == AVPMENU_ELEMENT_VIDEO_FPSCAP && VideoFPSCapCustomEditStartsFresh)
				{
					*elementPtr->c.NumberPtr = 0;
					VideoFPSCapCustomEditStartsFresh = 0;
				}
				(*elementPtr->c.NumberPtr) = (*elementPtr->c.NumberPtr) * 10 + digit;
				if (*elementPtr->c.NumberPtr > elementPtr->b.MaxValue)
					*elementPtr->c.NumberPtr = elementPtr->b.MaxValue;
			}
		}
		KeyboardEntryQueue_Clear();
	}
	else if (AvPMenus.UserChangingKeyConfig)
	{
		if (DebouncedKeyboardInput[KEY_ESCAPE])
		{
			AvPMenus.UserChangingKeyConfig = 0;
		}
		else
		{
			signed int key,selectedKey=-1;

			// see if a valid key has been pressed
			for (key = 0 ; key < MAX_NUMBER_OF_INPUT_KEYS ; key++)
			{
				if ((key != KEY_ESCAPE) && DebouncedKeyboardInput[key])
				{
					selectedKey = key;
					break;
				}
			}
			
			if (AvPMenus.ChangingPrimaryConfig)
			{
				if (selectedKey!=-1)
				{
					if (AvPMenus.CurrentMenu==AVPMENU_MARINEKEYCONFIG)
{
	extern int SmartJetpackEnabled_Temp;
	int bindingIndex=KeyConfigVisibleBindingIndex(AvPMenus.CurrentlySelectedElement);
	if (bindingIndex==12
		&& (selectedKey==PlayerInputPrimaryConfig.f.Jetpack || selectedKey==PlayerInputSecondaryConfig.f.Jetpack)) selectedKey=-1;
	if (bindingIndex==22
		&& (selectedKey==PlayerInputPrimaryConfig.Jump || selectedKey==PlayerInputSecondaryConfig.Jump)) selectedKey=-1;
}
if (selectedKey!=-1) *KeyConfigBindingPointer(1,AvPMenus.CurrentlySelectedElement) = selectedKey;
					AvPMenus.UserChangingKeyConfig=0;
				}
			}
			else // changing Secondary
			{
				if (selectedKey!=-1)
				{
					if (AvPMenus.CurrentMenu==AVPMENU_MARINEKEYCONFIG)
{
	extern int SmartJetpackEnabled_Temp;
	int bindingIndex=KeyConfigVisibleBindingIndex(AvPMenus.CurrentlySelectedElement);
	if (bindingIndex==12
		&& (selectedKey==PlayerInputPrimaryConfig.f.Jetpack || selectedKey==PlayerInputSecondaryConfig.f.Jetpack)) selectedKey=-1;
	if (bindingIndex==22
		&& (selectedKey==PlayerInputPrimaryConfig.Jump || selectedKey==PlayerInputSecondaryConfig.Jump)) selectedKey=-1;
}
if (selectedKey!=-1) *KeyConfigBindingPointer(0,AvPMenus.CurrentlySelectedElement) = selectedKey;
					AvPMenus.UserChangingKeyConfig=0;
				}
			}
		}
	}
	else
	{	
        if (DebouncedKeyboardInput[KEY_ESCAPE] && AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS && AvPMenus.CurrentMenu==AVPMENU_INGAME)
        {
            Sound_Play(SID_MENUS_SELECT_ITEM,"r");
            ResumeGameFromMenu();
        }
        else if (DebouncedKeyboardInput[KEY_ESCAPE]
            && AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS
			&& (AvPMenus.CurrentMenu == AVPMENU_SAVEGAME || AvPMenus.CurrentMenu == AVPMENU_LOADGAME))
		{
			/* Direct save/load binds never make the player walk back through pause. */
			AvPMenus.MenusState = MENUSSTATE_STARTGAME;
		}
		else if (DebouncedKeyboardInput[KEY_ESCAPE] && (AvPMenus.CurrentMenu != AVPMENU_MAIN && AvPMenus.CurrentMenu != AVPMENU_INGAME))
		{
            /* Escape applies the exact same pending controls as the OK row.
             * Key-capture cancellation is handled above and stays separate. */
            if (AvPMenus.CurrentMenu==AVPMENU_MARINEKEYCONFIG ||
                AvPMenus.CurrentMenu==AVPMENU_PREDATORKEYCONFIG ||
                AvPMenus.CurrentMenu==AVPMENU_ALIENKEYCONFIG ||
                AvPMenus.CurrentMenu==AVPMENU_CONTROLS ||
                AvPMenus.CurrentMenu==AVPMENU_JOYSTICKCONTROLS)
            {
                for(int row=0;row<AvPMenus.NumberOfElementsInMenu;row++)
                    if(AvPMenus.MenuElements[row].ElementID==AVPMENU_ELEMENT_KEYCONFIGOK){
                        AvPMenus.CurrentlySelectedElement=row;
                        InteractWithMenuElement(AVPMENU_ELEMENT_INTERACTION_SELECT);
                        return;
                    }
            }
			//if (AvPMenus.CurrentMenu == AVPMENU_MULTIPLAYERJOINGAME2)
			switch(AvPMenus.CurrentMenu)
			{
				case AVPMENU_MULTIPLAYER_CONFIG_JOIN:
				{
					AddNetMsg_PlayerLeaving();
					NetSendMessages();
					if(!LobbiedGame)
					{
						DirectPlay_Disconnect();
					}
					break;
				}
				case AVPMENU_DETAILLEVELS:
				{
					SetDetailLevelsFromMenu();
					SaveUserProfile(UserProfilePtr);
					SaveDeviceAndVideoModePreferences();
					if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
					{
						SetupNewMenu(AVPMENU_INGAMEAVOPTIONS);
					}
					break;
				}
				case AVPMENU_INGAMEAVOPTIONS:
				case AVPMENU_MAINMENUAVOPTIONS:
				{
					SaveUserProfile(UserProfilePtr);
                    SaveDeviceAndVideoModePreferences();
					break;
				}
				case AVPMENU_CHEATOPTIONS:
				{
					CheatMode_Active = CHEATMODE_NONACTIVE;
					break;	
				}

				case AVPMENU_MULTIPLAYER_CONFIG :
				case AVPMENU_SKIRMISH_CONFIG :
				{
					//reload the previous multiplayer configuration
					extern char MP_Config_Description[];
					LoadMultiplayerConfiguration(GetTextString(TEXTSTRING_PREVIOUSGAME_FILENAME));
					MP_Config_Description[0]=0;
					break;
				}
				
				default:
					break;
			}


			if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				{
					/* Escape from the in-game video picker returns to InGame AV Options. */
					SetupNewMenu((AvPMenus.CurrentMenu == AVPMENU_INGAMEVIDEOMODE) ? AVPMENU_INGAMEAVOPTIONS : AVPMENU_INGAME);
				}
				else
			{
				SetupNewMenu(AvPMenusData[AvPMenus.CurrentMenu].ParentMenu);
			}
		}
		else if (IDemandSelect()) // select element
		{
			if (InputIsDebounced)
			{
				InteractWithMenuElement(AVPMENU_ELEMENT_INTERACTION_SELECT);
				InputIsDebounced = 0;
			}
		}
		else if (IDemandGoForward()) // previous element
		{
			if (InputIsDebounced)
			{
				switch (AvPMenus.CurrentMenu)
				{
					case AVPMENU_USERPROFILESELECT:
					case AVPMENU_MARINELEVELS:
					case AVPMENU_PREDATORLEVELS:
					case AVPMENU_ALIENLEVELS:
					{
						InteractWithMenuElement(AVPMENU_ELEMENT_INTERACTION_DECREASE);
						break;
					}
					default:
					{
						AvPMenus.CurrentlySelectedElement--;
						if (AvPMenus.CurrentlySelectedElement<0)
						{
							AvPMenus.CurrentlySelectedElement= AvPMenus.NumberOfElementsInMenu-1;
						}
						/* Renderer is informational and is skipped by video-menu navigation. */
						if ((AvPMenus.CurrentMenu==AVPMENU_VIDEOMODE || AvPMenus.CurrentMenu==AVPMENU_INGAMEVIDEOMODE) && AvPMenus.CurrentlySelectedElement==0)
							AvPMenus.CurrentlySelectedElement=AvPMenus.NumberOfElementsInMenu-1;						while (!MenuElementIsVisible(&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement]))
						{
							AvPMenus.CurrentlySelectedElement--;
							if (AvPMenus.CurrentlySelectedElement<0) AvPMenus.CurrentlySelectedElement=AvPMenus.NumberOfElementsInMenu-1;
						}
						Sound_Play(SID_MENUS_CHANGE_ITEM,"r");
						break;
					}
				}
				InputIsDebounced = 0;
			}
			else
			{
				KeyDepressedCounter += RealFrameTime;
			}

		}
		else if (IDemandGoBackward()) // next element
		{
			if (InputIsDebounced)
			{
				switch (AvPMenus.CurrentMenu)
				{
					case AVPMENU_USERPROFILESELECT:
			 	       case AVPMENU_MARINELEVELS:
					case AVPMENU_PREDATORLEVELS:
					case AVPMENU_ALIENLEVELS:
					{
						InteractWithMenuElement(AVPMENU_ELEMENT_INTERACTION_INCREASE);
						break;
					}
					default:
					{
						AvPMenus.CurrentlySelectedElement++;
						if (AvPMenus.CurrentlySelectedElement>=AvPMenus.NumberOfElementsInMenu)
						{
							AvPMenus.CurrentlySelectedElement= 0;
						}
						/* Renderer is informational and is skipped by video-menu navigation. */
						if ((AvPMenus.CurrentMenu==AVPMENU_VIDEOMODE || AvPMenus.CurrentMenu==AVPMENU_INGAMEVIDEOMODE) && AvPMenus.CurrentlySelectedElement==0)
							AvPMenus.CurrentlySelectedElement=1;						while (!MenuElementIsVisible(&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement]))
						{
							AvPMenus.CurrentlySelectedElement++;
							if (AvPMenus.CurrentlySelectedElement>=AvPMenus.NumberOfElementsInMenu) AvPMenus.CurrentlySelectedElement=0;
						}
						Sound_Play(SID_MENUS_CHANGE_ITEM,"r");
						break;
					}
				}

				InputIsDebounced = 0;
			}
			else
			{
				KeyDepressedCounter += RealFrameTime;
			}
		}
		else if (IDemandTurnLeft()) 
		{
			if (InputIsDebounced)
			{
				InteractWithMenuElement(AVPMENU_ELEMENT_INTERACTION_DECREASE);
				InputIsDebounced = 0;
			}
			else
			{
				KeyDepressedCounter += RealFrameTime;
			}
		}
		else if (IDemandTurnRight()) 
		{
			if (InputIsDebounced)
			{
				InteractWithMenuElement(AVPMENU_ELEMENT_INTERACTION_INCREASE);
				InputIsDebounced = 0;
			}
			else
			{
				KeyDepressedCounter += RealFrameTime;
			}
		}
		else if (DebouncedKeyboardInput[KEY_BACKSPACE]) 
		{
			switch (AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement].ElementID)
			{
				case AVPMENU_ELEMENT_KEYCONFIG:
				{
					if (!KeyConfigSelectionColumn)
					{
						*KeyConfigBindingPointer(1,AvPMenus.CurrentlySelectedElement) = KEY_VOID;
					}
					else // changing Secondary
					{
						*KeyConfigBindingPointer(0,AvPMenus.CurrentlySelectedElement) = KEY_VOID;
					}
					break;
				}
				case AVPMENU_ELEMENT_USERPROFILE:
				{
					if (UserProfileNumber)
					{
						SetupNewMenu(AVPMENU_USERPROFILEDELETE);
					}
					break;
				}
				case AVPMENU_ELEMENT_LOADMPCONFIG :
				{
					//take note of the current configuration
					MultiplayerConfigurationIndex=AvPMenus.CurrentlySelectedElement;
					MultiplayerConfigurationName=AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement].c.TextPtr;
					//setup the delete configuration menu
					SetupNewMenu(AVPMENU_MULTIPLAYER_DELETECONFIG);
					break;
				}
				default:
					break;
			}
		}
		else  
		{
			InputIsDebounced=1;
			KeyDepressedCounter = 0;
		}

		if (KeyDepressedCounter>ONE_FIXED)
			InputIsDebounced = 1;
	}

}
static int MenuElementIsVisible(AVPMENU_ELEMENT *elementPtr)
{
    if(elementPtr->ElementID==AVPMENU_ELEMENT_TEXTSLIDER &&
       elementPtr->c.SliderValuePtr==&MenuMusicMode && !MenuMusic99Available)return 0;
    if(elementPtr->ElementID==AVPMENU_ELEMENT_HUD_ANIMATION_INTENSITY){
        int flags=CameraMotionFlags(MarineCameraMode);
        if(AvP.PlayerType!=I_Marine)return 0;
        if(elementPtr->c.SliderValuePtr==&HUDShakeIntensity)return (flags&4)!=0;
        if(elementPtr->c.SliderValuePtr==&HUDSwayIntensity)return (flags&1)!=0;
        if(elementPtr->c.SliderValuePtr==&HUDBobbingIntensity)return (flags&2)!=0;
        return 0;
    }
    if(elementPtr->ElementID==AVPMENU_ELEMENT_TEXTSLIDER &&
       elementPtr->c.SliderValuePtr==&MarineCrosshairThicknessMode && AvP.PlayerType!=I_Marine)return 0;
    if(!VideoHUDUsesWideLayout() &&
       (elementPtr->ElementID==AVPMENU_ELEMENT_HUD_ANCHORED ||
        elementPtr->ElementID==AVPMENU_ELEMENT_HUD_POSITION))return 0;
	if(AvP.PlayerType==I_Alien &&
       (elementPtr->ElementID==AVPMENU_ELEMENT_CROSSHAIR_SCALE ||
        elementPtr->ElementID==AVPMENU_ELEMENT_HUD_ANCHORED ||
        elementPtr->ElementID==AVPMENU_ELEMENT_HUD_POSITION))return 0;
	return (elementPtr->ElementID != AVPMENU_ELEMENT_HUD_POSITION
		|| HUDAnchorMode == HUD_ANCHOR_UNLOCKED);
}

static int HUDPositionForAspect(int numerator, int denominator)
{
	int width = ScreenDescriptorBlock.SDB_Width;
	int height = ScreenDescriptorBlock.SDB_Height;
	int minimumWidth = (height * 4) / 3;
	int desiredWidth = (height * numerator) / denominator;
	int position;

	if (minimumWidth > width) minimumWidth = width;
	if (desiredWidth < minimumWidth) desiredWidth = minimumWidth;
	if (desiredWidth > width) desiredWidth = width;
	if (width <= minimumWidth) return 0;
	position = ((desiredWidth - minimumWidth) * 100 + (width - minimumWidth) / 2) / (width - minimumWidth);
	if (position < 0) return 0;
	if (position > 100) return 100;
	return position;
}

static void HUDPositionDescription(char *buffer, int bufferSize)
{
	int layoutWidth = HUD_GetLayoutWidth();
	int layoutHeight = HUD_GetLayoutHeight();
	if (layoutHeight <= 0) layoutHeight = 1;
	if (HUDAnchorPosition == 0) {
		snprintf(buffer, bufferSize, "4:3");
	} else if (HUDAnchorPosition == HUDPositionForAspect(16,9)) {
		snprintf(buffer, bufferSize, "16:9");
	} else {
		snprintf(buffer, bufferSize, "%.2f:1", (double)layoutWidth/(double)layoutHeight);
	}
}
static void ResumeGameFromMenu(void) { AvPMenus.MenusState=MENUSSTATE_STARTGAME; }

static void InteractWithMenuElement(enum AVPMENU_ELEMENT_INTERACTION_ID interactionID)
{
	AVPMENU_ELEMENT *elementPtr = &AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];

	if (interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)
	{
		Sound_Play(SID_MENUS_SELECT_ITEM,"r");
	}
	else
	{
		Sound_Play(SID_MENUS_CHANGE_ITEM,"r");
	}
	switch(elementPtr->ElementID)
    {
        case AVPMENU_ELEMENT_SHAKE_PARAM:
            if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)ShakeParamDefault(elementPtr->c.Value);
            else ShakeParamAdjust(elementPtr->c.Value,interactionID==AVPMENU_ELEMENT_INTERACTION_INCREASE?1:-1);
            break;
        case AVPMENU_ELEMENT_SHAKE_EXPORT:
            if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)ShakeParamsExport();
            break;
        case AVPMENU_ELEMENT_MOTION_PARAM:
            if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)MotionParamDefault(elementPtr->c.Value);
            else MotionParamAdjust(elementPtr->c.Value,interactionID==AVPMENU_ELEMENT_INTERACTION_INCREASE?1:-1);
            break;
        case AVPMENU_ELEMENT_MOTION_EXPORT:
            if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)MotionParamsExport();
            break;
        case AVPMENU_ELEMENT_DEV_WEAPON_Z:
            if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)DevViewmodelToggle(elementPtr->c.Value);
            else DevViewmodelAdjust(elementPtr->c.Value,interactionID==AVPMENU_ELEMENT_INTERACTION_INCREASE?1:-1);
            break;
        case AVPMENU_ELEMENT_DEV_EXPORT:
            if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)DevViewmodelExport();
            break;

		default:
		GLOBALASSERT("UNKNOWN MENU ELEMENT"==0);
		case AVPMENU_ELEMENT_GOTOMENU:
		case AVPMENU_ELEMENT_GOTOMENU_GFX:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				//Check to see if we are on one of the menus for entering multiplayer name
				if(AvPMenus.CurrentMenu==AVPMENU_MULTIPLAYER ||
				   AvPMenus.CurrentMenu==AVPMENU_MULTIPLAYER_LOBBIEDSERVER)
				{
					//save profile , so that multiplayer name is remembered
					SaveUserProfile(UserProfilePtr);
				}
			
				SetupNewMenu(elementPtr->b.MenuToGoTo);
			}
			break;
		}
		case AVPMENU_ELEMENT_SAVEMPCONFIG :
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				if(MP_Config_Name[0])
				{
					SaveMultiplayerConfiguration(MP_Config_Name);
				}
				SetupNewMenu(elementPtr->b.MenuToGoTo);
			}
			break;
		}
		case AVPMENU_ELEMENT_TEXTFIELD:
		{
			AvPMenus.UserEnteringText = 1;
			SDL_StartTextInput(NULL);
			KeyboardEntryQueue_Clear();
			AvPMenus.PositionInTextField = strlen(elementPtr->c.TextPtr);
			elementPtr->c.TextPtr[AvPMenus.PositionInTextField] = 0;
			AvPMenus.WidthLeftForText = 0; //will be calculated properly when menus are drawn
			break;
		}
		case AVPMENU_ELEMENT_TEXTFIELD_SMALLWRAPPED:
		{
			AvPMenus.UserEnteringText = 1;
			SDL_StartTextInput(NULL);
			KeyboardEntryQueue_Clear();
			AvPMenus.PositionInTextField = strlen(elementPtr->c.TextPtr);
			AvPMenus.WidthLeftForText = 0; //will be calculated properly when menus are drawn
			break;
		}

		case AVPMENU_ELEMENT_NUMBERFIELD:
		{
			if(interactionID == AVPMENU_ELEMENT_INTERACTION_DECREASE)
			{
				(*elementPtr->c.NumberPtr)--;
				if(*elementPtr->c.NumberPtr<0)
				{
					*elementPtr->c.NumberPtr=0;
				}
			}
			else if(interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				(*elementPtr->c.NumberPtr)++;
				if(*elementPtr->c.NumberPtr>elementPtr->b.MaxValue)
				{
					*elementPtr->c.NumberPtr=elementPtr->b.MaxValue;
				}
				
			}
			else
			{
				*elementPtr->c.NumberPtr=0;
				AvPMenus.UserEnteringNumber = 1;
				KeyboardEntryQueue_Clear();
			}
			break;
		}
		case AVPMENU_ELEMENT_DUMMYTEXTFIELD:
		case AVPMENU_ELEMENT_DUMMYTEXTSLIDER:
		case AVPMENU_ELEMENT_DUMMYTEXTSLIDER_POINTER:
		case AVPMENU_ELEMENT_DUMMYNUMBERFIELD:
		case AVPMENU_ELEMENT_TOGGLE:
		{
			break;
		}
						case AVPMENU_ELEMENT_WEAPON_FOV:
		{
			SynchroniseWeaponFOVForCurrentSpecies();
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				WeaponFOV = WeaponFOVDefaultForCurrentSpecies();
			}
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				if (WeaponFOV < WeaponFOVMaximumForCurrentSpecies()) WeaponFOV++;
			}
			else
			{
				if (WeaponFOV > WEAPON_FOV_MINIMUM) WeaponFOV--;
			}
			SynchroniseWeaponFOVForCurrentSpecies();
			break;
		}
		case AVPMENU_ELEMENT_FOV:
		{
			ClampPlayerFOVForCurrentSpecies();

if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				/* ENTER restores the active species native FOV. */
				PlayerFOV = PlayerFOVDefaultForCurrentSpecies();
			}
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				if (PlayerFOV < PlayerFOVMaximumForCurrentSpecies()) PlayerFOV++;
			}
			else
			{
				if (PlayerFOV > PlayerFOVMinimumForCurrentSpecies()) PlayerFOV--;
			}
			RefreshPlayerFOV();
			break;
		}
		case AVPMENU_ELEMENT_HUD_ANIMATION_INTENSITY:
        if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT)(*elementPtr->c.SliderValuePtr)=50;
        else if(interactionID==AVPMENU_ELEMENT_INTERACTION_INCREASE){if((*elementPtr->c.SliderValuePtr)<100)(*elementPtr->c.SliderValuePtr)++;}
        else if((*elementPtr->c.SliderValuePtr)>1)(*elementPtr->c.SliderValuePtr)--;
        break;
        case AVPMENU_ELEMENT_HUD_SCALE:
		{
			HUD_SynchronisePerSpeciesOptions();
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT) HUDUserScaleTenths = HUD_USER_SCALE_TENTHS_DEFAULT;
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE) { if (HUDUserScaleTenths < HUD_USER_SCALE_TENTHS_MAXIMUM) HUDUserScaleTenths++; }
			else { if (HUDUserScaleTenths > HUD_USER_SCALE_TENTHS_MINIMUM) HUDUserScaleTenths--; }
			HUD_GetUserScaleFixed();
			break;
		}
				case AVPMENU_ELEMENT_CROSSHAIR_SCALE:
		{
			HUD_ClampCrosshairScaleForCurrentSpecies();
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT) CrosshairScaleTenths = AvP.PlayerType==I_Predator?20:CROSSHAIR_SCALE_TENTHS_DEFAULT;
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE) { if (CrosshairScaleTenths < CROSSHAIR_SCALE_TENTHS_MAXIMUM) CrosshairScaleTenths++; }
			else { if (CrosshairScaleTenths > CROSSHAIR_SCALE_TENTHS_MINIMUM) CrosshairScaleTenths--; }
			HUD_GetCrosshairScaleFixed();
			break;
		}case AVPMENU_ELEMENT_HUD_ANCHORED:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				HUDAnchorMode = HUD_ANCHOR_16_9;
				HUDAnchorPosition = HUDPositionForAspect(16,9);
			}
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				HUDAnchorMode++;
				if (HUDAnchorMode > HUD_ANCHOR_UNLOCKED) HUDAnchorMode = HUD_ANCHOR_4_3;
			}
			else
			{
				HUDAnchorMode--;
				if (HUDAnchorMode < HUD_ANCHOR_4_3) HUDAnchorMode = HUD_ANCHOR_UNLOCKED;
			}
			if (HUDAnchorMode == HUD_ANCHOR_UNLOCKED)
			{
				HUDAnchorPosition = HUDAnchorUnlockedPosition;
			}
			HUD_RefreshLayout();
			break;
		}
		case AVPMENU_ELEMENT_HUD_POSITION:
		{
			int aspect16By9;
			HUD_SynchronisePerSpeciesOptions();
			aspect16By9 = HUDPositionForAspect(16,9);
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				if (HUDAnchorPosition == 0)
					HUDAnchorPosition = aspect16By9;
				else if (HUDAnchorPosition == aspect16By9)
					HUDAnchorPosition = HUDAnchorUnlockedPosition;
				else
					HUDAnchorPosition = 0;
			}
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				if (HUDAnchorPosition < 100) HUDAnchorPosition++;
				HUDAnchorUnlockedPosition = HUDAnchorPosition;
			}
			else
			{
				if (HUDAnchorPosition > 0) HUDAnchorPosition--;
				HUDAnchorUnlockedPosition = HUDAnchorPosition;
			}
			HUD_SynchronisePerSpeciesOptions();
			HUD_RefreshLayout();
			break;
		}case AVPMENU_ELEMENT_ZOOM_SENSITIVITY:
		{
			/* ENTER resets this one slider; left/right retain the usual fine steps. */
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				*elementPtr->c.SliderValuePtr = 13;
			}
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				if (*elementPtr->c.SliderValuePtr < elementPtr->b.MaxSliderValue)
					*elementPtr->c.SliderValuePtr += 1;
			}
			else if (*elementPtr->c.SliderValuePtr > 0)
			{
				*elementPtr->c.SliderValuePtr -= 1;
			}
			break;
		}		case AVPMENU_ELEMENT_SLIDER:
		{
            if(interactionID==AVPMENU_ELEMENT_INTERACTION_SELECT &&
                (elementPtr->a.TextDescription==TEXTSTRING_IMAGE_BRIGHTNESS_NEW || elementPtr->a.TextDescription==TEXTSTRING_IMAGE_CONTRAST_NEW || elementPtr->a.TextDescription==TEXTSTRING_IMAGE_SATURATION_NEW)){
                *elementPtr->c.SliderValuePtr=elementPtr->a.TextDescription==TEXTSTRING_IMAGE_BRIGHTNESS_NEW?50:100;break;
            }

			/* ENTER restores Gamma to its original vanilla midpoint (128).
			   Left/right retain their regular one-step behaviour. */
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
				&& (elementPtr->a.TextDescription == TEXTSTRING_AVOPTIONS_GAMMASETTING))
			{
				InitialiseGammaSettings(DEFAULT_GAMMA_SETTING);
				break;
			}
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			  ||(interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE))
			{
				if (*elementPtr->c.SliderValuePtr<elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr+=1;
				}
			}
			else
			{
				if (*elementPtr->c.SliderValuePtr>0)
				{
					*elementPtr->c.SliderValuePtr-=1;
				}
			}
			break;
		}
		case AVPMENU_ELEMENT_TEXTSLIDER:
		case AVPMENU_ELEMENT_TEXTSLIDER_POINTER:
		{
            if(elementPtr->c.SliderValuePtr==&ConsoleTextScaleMode) {
                ConsoleTextScaleMode=ConsoleScaleStep(ConsoleTextScaleMode,interactionID!=AVPMENU_ELEMENT_INTERACTION_DECREASE);
                break;
            }
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			 && ResetRequestedTextSliderOnSelect(elementPtr))
			{
				break;
			}
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			  ||(interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE))
			{
				*elementPtr->c.SliderValuePtr+=1;
				if (*elementPtr->c.SliderValuePtr>elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr=0;
				}
			}
			else
			{
				*elementPtr->c.SliderValuePtr-=1;
				if (*elementPtr->c.SliderValuePtr<0)
				{
					*elementPtr->c.SliderValuePtr=elementPtr->b.MaxSliderValue;
				}
			}
			if(elementPtr->c.SliderValuePtr==&FMVFilterMode || elementPtr->c.SliderValuePtr==&RetainPlayerState || elementPtr->c.SliderValuePtr==&OptimizedAmmo)SaveDeviceAndVideoModePreferences();
            if(elementPtr->c.SliderValuePtr==&MarineCrosshairThicknessMode && UserProfilePtr)
                SaveUserProfile(UserProfilePtr);
            if (elementPtr->c.SliderValuePtr == &ScreenshotFormat) {
                SaveDeviceAndVideoModePreferences();
                if(UserProfilePtr)SaveUserProfile(UserProfilePtr);
            }
			/* Esta preferencia es global: persiste aunque se salga sin aplicar el men?. */
			if ((elementPtr->c.SliderValuePtr == &MusicPlayMode || elementPtr->c.SliderValuePtr == &CRTFilterEnabled) && UserProfilePtr)
			{
				if (MusicPlayMode < 0 || MusicPlayMode > 2) MusicPlayMode = 1;
				if (CRTFilterEnabled != 0) CRTFilterEnabled = 1;
				SaveUserProfile(UserProfilePtr);
			}

			break;
		}
		case AVPMENU_ELEMENT_SPECIES_TEXTSLIDER:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			  ||(interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE))
			{
				*elementPtr->c.SliderValuePtr+=1;
				if (*elementPtr->c.SliderValuePtr>elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr=0;
				}
				GetNextAllowedSpecies(elementPtr->c.SliderValuePtr,TRUE);
			}
			else
			{
				*elementPtr->c.SliderValuePtr-=1;
				if (*elementPtr->c.SliderValuePtr<0)
				{
					*elementPtr->c.SliderValuePtr=elementPtr->b.MaxSliderValue;
				}
				GetNextAllowedSpecies(elementPtr->c.SliderValuePtr,FALSE);
			}
			break;
		}
		case AVPMENU_ELEMENT_CHEATMODE_SPECIES_TEXTSLIDER:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			  ||(interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE))
			{
				*elementPtr->c.SliderValuePtr+=1;
				if (*elementPtr->c.SliderValuePtr>elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr=0;
				}
				CheatMode_GetNextAllowedSpecies(elementPtr->c.SliderValuePtr,TRUE);
			}
			else
			{
				*elementPtr->c.SliderValuePtr-=1;
				if (*elementPtr->c.SliderValuePtr<0)
				{
					*elementPtr->c.SliderValuePtr=elementPtr->b.MaxSliderValue;
				}
				CheatMode_GetNextAllowedSpecies(elementPtr->c.SliderValuePtr,FALSE);
			}
			break;
		}
		case AVPMENU_ELEMENT_CHEATMODE_TEXTSLIDER:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			  ||(interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE))
			{
				*elementPtr->c.SliderValuePtr+=1;
				if (*elementPtr->c.SliderValuePtr>elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr=0;
				}
				CheatMode_GetNextAllowedMode(elementPtr->c.SliderValuePtr,TRUE);
			}
			else
			{
				*elementPtr->c.SliderValuePtr-=1;
				if (*elementPtr->c.SliderValuePtr<0)
				{
					*elementPtr->c.SliderValuePtr=elementPtr->b.MaxSliderValue;
				}
				CheatMode_GetNextAllowedMode(elementPtr->c.SliderValuePtr,FALSE);
			}
			break;
		}
		case AVPMENU_ELEMENT_CHEATMODE_ENVIRONMENT_TEXTSLIDER:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			  ||(interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE))
			{
				*elementPtr->c.SliderValuePtr+=1;
				if (*elementPtr->c.SliderValuePtr>elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr=0;
				}
				CheatMode_GetNextAllowedEnvironment(elementPtr->c.SliderValuePtr,TRUE);
			}
			else
			{
				*elementPtr->c.SliderValuePtr-=1;
				if (*elementPtr->c.SliderValuePtr<0)
				{
					*elementPtr->c.SliderValuePtr=elementPtr->b.MaxSliderValue;
				}
				CheatMode_GetNextAllowedEnvironment(elementPtr->c.SliderValuePtr,FALSE);
			}
			break;
		}

		case AVPMENU_ELEMENT_QUITGAME:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				{
					AvP.MainLoopRunning = 0;
				}
				AvPMenus.MenusState = MENUSSTATE_OUTSIDEMENUS;
			}
			break;
		}
		
		case AVPMENU_ELEMENT_USERPROFILE:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				/* a profile has been selected */
				int i;
				UserProfilePtr = GetFirstUserProfile();
				for (i=0; i<UserProfileNumber; i++)
					UserProfilePtr = GetNextUserProfile();

  				if (UserProfileNumber)
  				{
 					// Edmond too
 				 	GetSettingsFromUserProfile();
 					// Edmond
 					if (LobbiedGame == LobbiedGame_Server)
 					{
 						SetupNewMenu(AVPMENU_MULTIPLAYER_CONFIG);			// Edmond
 					}
 					else if (LobbiedGame == LobbiedGame_Client)
 					{
 						SetupNewMenu(AVPMENU_MULTIPLAYER_LOBBIEDCLIENT);
 					}
 					else
 						SetupNewMenu(AVPMENU_MAIN);
  				}
				else
				{
					SetupNewMenu(AVPMENU_USERPROFILEENTERNAME);
				}
			}
			else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				if (*elementPtr->c.SliderValuePtr<elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr+=1;
					EpisodeSelectScrollOffset-=ONE_FIXED;
				}
			}
			else
			{
				if (*elementPtr->c.SliderValuePtr>0)
				{
					*elementPtr->c.SliderValuePtr-=1;
					EpisodeSelectScrollOffset+=ONE_FIXED;
				}
			}
			break;
		}
		case AVPMENU_ELEMENT_USERPROFILE_DELETE:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				DeleteUserProfile(UserProfileNumber);
				SetupNewMenu(AVPMENU_USERPROFILESELECT);
			}
			break;  
		}
		case AVPMENU_ELEMENT_DELETEMPCONFIG:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				DeleteMultiplayerConfigurationByIndex(MultiplayerConfigurationIndex);
				//go back to the load config menu
				SetupNewMenu(AVPMENU_MULTIPLAYER_LOADCONFIG);
				
			}
			break;  
		}
		case AVPMENU_ELEMENT_DIFFICULTYLEVEL:
		{
            if(AvPMenus.CurrentlySelectedElement==4 && interactionID!=AVPMENU_ELEMENT_INTERACTION_SELECT){
                AcidMenuTier+=(interactionID==AVPMENU_ELEMENT_INTERACTION_INCREASE)?1:-1;
                if(AcidMenuTier>3)AcidMenuTier=1;if(AcidMenuTier<1)AcidMenuTier=3;
            }
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				if(AvPMenus.CurrentlySelectedElement==4)AcidSetTier(AcidMenuTier);
                else{AcidSetTier(0);AvP.Difficulty=AvPMenus.CurrentlySelectedElement;}
				AvPMenus.MenusState = MENUSSTATE_STARTGAME;
			}
			break;
		}

		case AVPMENU_ELEMENT_ALIENEPISODE:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT
			  &&MaximumSelectableLevel>=*elementPtr->c.SliderValuePtr)
			{
				AvP.PlayerType = I_Alien;
				RememberEpisode(I_Alien,AlienEpisodeToPlay);
                SetLevelToLoadForAlien(AlienEpisodeToPlay);

				if (AlienEpisodeToPlay<MAX_NO_OF_BASIC_ALIEN_EPISODES)
				{
					SetupNewMenu(AVPMENU_LEVELBRIEFING_BASIC);
				}
				else
				{
					SetupNewMenu(AVPMENU_LEVELBRIEFING_BONUS);
				}
				AvPMenusData[AvPMenus.CurrentMenu].ParentMenu = AVPMENU_ALIENLEVELS;
				break;
			}
			/* else let it fall through */

		}
		case AVPMENU_ELEMENT_MARINEEPISODE:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT
				&&MaximumSelectableLevel>=*elementPtr->c.SliderValuePtr)
			{
				AvP.PlayerType = I_Marine;
				RememberEpisode(I_Marine,MarineEpisodeToPlay);
                SetLevelToLoadForMarine(MarineEpisodeToPlay);

				if (MarineEpisodeToPlay<MAX_NO_OF_BASIC_MARINE_EPISODES)
				{
					SetupNewMenu(AVPMENU_LEVELBRIEFING_BASIC);
				}
				else
				{
					SetupNewMenu(AVPMENU_LEVELBRIEFING_BONUS);
				}
				AvPMenusData[AvPMenus.CurrentMenu].ParentMenu = AVPMENU_MARINELEVELS;
				break;
			}
			/* else let it fall through */
		}
		case AVPMENU_ELEMENT_PREDATOREPISODE:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT
				&&MaximumSelectableLevel>=*elementPtr->c.SliderValuePtr)
			{
				AvP.PlayerType = I_Predator;
				RememberEpisode(I_Predator,PredatorEpisodeToPlay);
                SetLevelToLoadForPredator(PredatorEpisodeToPlay);

				if (PredatorEpisodeToPlay<MAX_NO_OF_BASIC_PREDATOR_EPISODES)
				{
					SetupNewMenu(AVPMENU_LEVELBRIEFING_BASIC);
				}
				else
				{
					SetupNewMenu(AVPMENU_LEVELBRIEFING_BONUS);
				}
				AvPMenusData[AvPMenus.CurrentMenu].ParentMenu = AVPMENU_PREDATORLEVELS;
				break;
			}
			/* else let it fall through */
		}
		{
			/* This code is reached when an EPISODE element has been
			activated but not "selected" i.e. the user wants to change
			the current episode, not start playing the current episode */
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				if (*elementPtr->c.SliderValuePtr<elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr+=1;
					EpisodeSelectScrollOffset-=ONE_FIXED;
				}
			}
			else
			{
				if (*elementPtr->c.SliderValuePtr>0)
				{
					*elementPtr->c.SliderValuePtr-=1;
					EpisodeSelectScrollOffset+=ONE_FIXED;
				}
			}
            if(elementPtr->ElementID==AVPMENU_ELEMENT_ALIENEPISODE)RememberEpisode(I_Alien,AlienEpisodeToPlay);
            else if(elementPtr->ElementID==AVPMENU_ELEMENT_MARINEEPISODE)RememberEpisode(I_Marine,MarineEpisodeToPlay);
            else if(elementPtr->ElementID==AVPMENU_ELEMENT_PREDATOREPISODE)RememberEpisode(I_Predator,PredatorEpisodeToPlay);
			break;
		}
		case AVPMENU_ELEMENT_BUTTONSETTING:
		{
			
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				break;
			}
			/* else let it fall through */

			if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)
			{
				if (*elementPtr->c.SliderValuePtr<elementPtr->b.MaxSliderValue)
				{
					*elementPtr->c.SliderValuePtr+=1;
					EpisodeSelectScrollOffset-=ONE_FIXED;
				}
			}
			else
			{
				if (*elementPtr->c.SliderValuePtr>0)
				{
					*elementPtr->c.SliderValuePtr-=1;
					EpisodeSelectScrollOffset+=ONE_FIXED;
				}
			}
			break;
		}

		case AVPMENU_ELEMENT_STARTMARINEDEMO:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				AvPMenus.MenusState = MENUSSTATE_STARTGAME;
				AvP.PlayerType = I_Marine;
				AcidSetTier(0);AvP.Difficulty = 1;
				SetLevelToLoad(AVP_ENVIRONMENT_INVASION);
			}
			break;
		}
		case AVPMENU_ELEMENT_STARTPREDATORDEMO:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				AvPMenus.MenusState = MENUSSTATE_STARTGAME;
				AvP.PlayerType = I_Predator;
				AcidSetTier(0);AvP.Difficulty = 1;
				SetLevelToLoad(AVP_ENVIRONMENT_INVASION_P);
			}
			break;
		}
		case AVPMENU_ELEMENT_STARTALIENDEMO:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				AvPMenus.MenusState = MENUSSTATE_STARTGAME;
				AvP.PlayerType = I_Alien;
				AcidSetTier(0);AvP.Difficulty = 1;
				SetLevelToLoad(AVP_ENVIRONMENT_INVASION_A);
			}
			break;
		}
		case AVPMENU_ELEMENT_STARTLEVELWITHCHEAT:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				AvPMenus.MenusState = MENUSSTATE_STARTGAME;
				AvP.PlayerType = CheatMode_Species;
				AcidSetTier(0);AvP.Difficulty = 1;
				SetLevelToLoadForCheatMode(CheatMode_Environment);
			}
			break;
		}
		case AVPMENU_ELEMENT_STARTMPGAME:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				extern int DirectPlay_HostGame(char *playerName, char *sessionName,int species,int gamestyle,int level);
				extern char MP_PlayerName[];
				extern char MP_SessionName[];
				extern int MP_Species;
				extern char MP_Config_Description[];
//				extern int MP_GameStyle;
//				extern int MP_LevelNumber;

				//save the 'Previous Game' multiplayer configuration
				strcpy(MP_Config_Description,GetTextString(TEXTSTRING_PREVIOUSGAME_FILENAME));
				SaveMultiplayerConfiguration(GetTextString(TEXTSTRING_PREVIOUSGAME_FILENAME));

				
				AcidSetTier(0);AvP.Difficulty = 1;

				if (DirectPlay_HostGame(MP_PlayerName,MP_SessionName,MP_Species,netGameData.gameType,netGameData.levelNumber))
				{
					AvPMenus.MenusState = MENUSSTATE_STARTGAME;
					if(netGameData.gameType==NGT_Coop)
						SetLevelToLoadForCooperative(netGameData.levelNumber);
					else
						SetLevelToLoadForMultiplayer(netGameData.levelNumber);
	
					
					SetBriefingTextForMultiplayer();
					
				}
			}
			break;
		}
		case AVPMENU_ELEMENT_JOINMPGAME:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				AvPMenus.MenusState = MENUSSTATE_STARTGAME;
				netGameData.myStartFlag = 1;	    
//				netGameData.myGameState = NGS_Playing;
				if(netGameData.gameType==NGT_Coop)
					SetLevelToLoadForCooperative(netGameData.levelNumber);
				else
					SetLevelToLoadForMultiplayer(netGameData.levelNumber);
				SetBriefingTextForMultiplayer();
			}
			break;
		}
		case AVPMENU_ELEMENT_LISTCHOICE:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				extern char MP_PlayerName[];
				extern char MP_SessionName[];
				int s = AvPMenus.CurrentlySelectedElement;

				if(SessionData[s].AllowedToJoin)
				{
					//copy the session name , leaving of the player count information
					char * braket_pos;
					strcpy(MP_SessionName,SessionData[s].Name);
										
					braket_pos=strrchr(MP_SessionName,'(');
					if(braket_pos) *braket_pos=0;

					if(DirectPlay_ConnectToSession(s,MP_PlayerName))
						SetupNewMenu(elementPtr->b.MenuToGoTo);
				}
			}
			break;
		}
		case AVPMENU_ELEMENT_JOINLOBBIED :
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				//save profile , so that multiplayer name is remembered
				SaveUserProfile(UserProfilePtr);

				InitAVPNetGameForJoin();
				SetupNewMenu(elementPtr->b.MenuToGoTo);
			}
			break;
		}

		case AVPMENU_ELEMENT_LOADMPCONFIG:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				LoadMultiplayerConfigurationByIndex(AvPMenus.CurrentlySelectedElement);
				SetupNewMenu(elementPtr->b.MenuToGoTo);
			}
			break;
		}

		case AVPMENU_ELEMENT_LOADIPADDRESS:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				LoadIPAddress(elementPtr->c.TextPtr);
				SetupNewMenu(elementPtr->b.MenuToGoTo);
			}
			break;
		}

		case AVPMENU_ELEMENT_CONNECTIONCHOICE :
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
 				netGameData.connectionType=elementPtr->c.Value;
				if(netGameData.connectionType == CONN_Mplayer)
				{
					//exit the game and launch the mplayer stuff
				fprintf(stderr, "STUB: InteractWithMenuElement (launching mplayer...)\n");
				#if 0
					LaunchingMplayer=TRUE;
					LaunchMplayer();
				#endif					
					AvP.MainLoopRunning = 0;
					AvPMenus.MenusState = MENUSSTATE_OUTSIDEMENUS; 
					break;
				}											   
				else
				{
 					SetupNewMenu(elementPtr->b.MenuToGoTo);
				}
						
			}
			break;
		}
		
		case AVPMENU_ELEMENT_VIDEOMODE:
		case AVPMENU_ELEMENT_VIDEO_RESOLUTION:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT) || (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)) NextVideoMode2();
			else PreviousVideoMode2();
			break;
		}
		case AVPMENU_ELEMENT_VIDEO_ASPECT:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT) || (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)) NextVideoAspect();
			else PreviousVideoAspect();
			break;
		}
		case AVPMENU_ELEMENT_VIDEO_REFRESH:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT) || (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)) NextVideoRefresh();
			else PreviousVideoRefresh();
			break;
		}
		case AVPMENU_ELEMENT_VIDEO_SCREENMODE:
		{
			if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT) || (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)) NextVideoScreenMode();
			else PreviousVideoScreenMode();
			break;
		}
		case AVPMENU_ELEMENT_VIDEO_VSYNC:
{
    if ((interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT) || (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE)) NextVideoVSync();
    else PreviousVideoVSync();
    break;
}
case AVPMENU_ELEMENT_VIDEO_FPSCAP:
{
    if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
    {
        if (VideoFPSCapIsCustom())
        {
            CurrentVideoFPSCapCustom = 100;
            VideoFPSCapCustomEditStartsFresh = 1;
            AvPMenus.PositionInTextField = 0;
            AvPMenus.UserEnteringNumber = 1;
            SDL_StartTextInput(NULL);
            KeyboardEntryQueue_Clear();
        }
        else ResetVideoFPSCapToDefault();
    }
    else if (interactionID == AVPMENU_ELEMENT_INTERACTION_INCREASE) NextVideoFPSCap();
    else PreviousVideoFPSCap();
    break;
}
case AVPMENU_ELEMENT_VIDEO_RENDERER:
		{
			break;
		}
		case AVPMENU_ELEMENT_VIDEOMODEOK:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				ApplySelectedVideoMode();
				SaveDeviceAndVideoModePreferences();
				/* The table records the correct parent for frontend and in-game callers. */
				SetupNewMenu(elementPtr->b.MenuToGoTo);
			}
			break;
		}
		case AVPMENU_ELEMENT_RESUMEGAME:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				ResumeGameFromMenu();
			}
			break;
		}
		case AVPMENU_ELEMENT_RESTARTGAME:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				AvP.RestartLevel=1;
				AvPMenus.MenusState = MENUSSTATE_STARTGAME;
			}
			break;
		}

		case AVPMENU_ELEMENT_KEYCONFIG:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				AvPMenus.UserChangingKeyConfig = 1;
				if (KeyConfigSelectionColumn)
				{
					AvPMenus.ChangingPrimaryConfig = 0;
				}
				else
				{
					AvPMenus.ChangingPrimaryConfig = 1;
				}
			}
			else
			{
				KeyConfigSelectionColumn=!KeyConfigSelectionColumn;     
			}
			break;
		}
		case AVPMENU_ELEMENT_KEYCONFIGOK:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				switch (AvPMenus.CurrentMenu)
				{
					case AVPMENU_MARINEKEYCONFIG:
					{
						MarineInputPrimaryConfig   = PlayerInputPrimaryConfig;
						MarineInputSecondaryConfig = PlayerInputSecondaryConfig;
						MarineWeaponPrimaryConfig = PlayerWeaponPrimaryConfig;
						MarineWeaponSecondaryConfig = PlayerWeaponSecondaryConfig;
						{
							extern int AutoWeaponChangeOn_Temp;
							extern int AutoWeaponChangeOn;
							AutoWeaponChangeOn = AutoWeaponChangeOn_Temp;
						}
					{
										NormalizeMarineJetpackBindings(&MarineInputPrimaryConfig,&MarineInputSecondaryConfig);
					}
					break;
					}
					case AVPMENU_PREDATORKEYCONFIG:
					{
						memcpy(PredatorPreviousVisionBindings,PlayerPreviousVisionBindings,2);
                        PredatorInputPrimaryConfig       = PlayerInputPrimaryConfig;
						PredatorInputSecondaryConfig = PlayerInputSecondaryConfig;
						PredatorWeaponPrimaryConfig = PlayerWeaponPrimaryConfig;
						PredatorWeaponSecondaryConfig = PlayerWeaponSecondaryConfig;
						break;
					}
					case AVPMENU_ALIENKEYCONFIG:
					{
						AlienInputPrimaryConfig   = PlayerInputPrimaryConfig;
						AlienInputSecondaryConfig = PlayerInputSecondaryConfig;
						AlienWeaponPrimaryConfig = PlayerWeaponPrimaryConfig;
						AlienWeaponSecondaryConfig = PlayerWeaponSecondaryConfig;
						break;
					}
					case AVPMENU_CONTROLS:
					{
						ControlMethods = PlayerControlMethods;
						break;
					}
					case AVPMENU_JOYSTICKCONTROLS:
					{
						JoystickControlMethods = PlayerJoystickControlMethods;
						break;
					}
					
					default:
						break;
				}

				if (AvPMenus.CurrentMenu==AVPMENU_MARINEKEYCONFIG
				 || AvPMenus.CurrentMenu==AVPMENU_PREDATORKEYCONFIG
				 || AvPMenus.CurrentMenu==AVPMENU_ALIENKEYCONFIG)
				{
					GlobalCommandPrimaryConfig = PlayerGlobalCommandPrimaryConfig;
					GlobalCommandSecondaryConfig = PlayerGlobalCommandSecondaryConfig;
                    AlwaysRunBySpecies[AlwaysRunSpecies_Temp]=AlwaysRun_Temp==1;
				}

				/* These options are shared profile preferences, edited by all three keyboard menus. */
				{
					extern int CrouchIsToggleKey_Temp;
					extern int UnlimitedSaveGames_Temp;
					extern int SmartJetpackEnabled_Temp;
					
					extern int SmartZoomEnabled_Temp;
					extern int FullVerticalPitchEnabled_Temp;
					extern int CrouchIsToggleKey;
					extern int UnlimitedSaveGames;
					extern int SmartJetpackEnabled;
					extern int MarineCrosshairThicknessMode;
					extern int SmartZoomEnabled;
					extern int FullVerticalPitchEnabled;
					CrouchIsToggleKey=CrouchIsToggleKey_Temp;
					UnlimitedSaveGames=UnlimitedSaveGames_Temp;
					FullVerticalPitchEnabled=FullVerticalPitchEnabled_Temp ? 1 : 0;
					if (AvPMenus.CurrentMenu==AVPMENU_MARINEKEYCONFIG)
					{
						SmartJetpackEnabled=SmartJetpackEnabled_Temp;
						
					}
					if (AvPMenus.CurrentMenu==AVPMENU_PREDATORKEYCONFIG)
					{
						SmartZoomEnabled=SmartZoomEnabled_Temp ? 1 : 0;
						if (!SmartZoomEnabled)
							NormalizePredatorSmartZoomDisabledBindings(&PredatorInputPrimaryConfig,&PredatorInputSecondaryConfig);
						/* The next visit rebuilds the list with or without Zoom holding. */
						SmartZoomBindingRowPublished=SmartZoomEnabled;
					}
				}

				SaveUserProfile(UserProfilePtr);

				if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				{
					SetupNewMenu(AVPMENU_INGAME);
				}
				else
				{
					SetupNewMenu(AVPMENU_OPTIONS);
				}
			}
			break;
		}
		case AVPMENU_ELEMENT_RESETKEYCONFIG:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				switch (AvPMenus.CurrentMenu)
				{
					case AVPMENU_MARINEKEYCONFIG:
					{
						PlayerInputPrimaryConfig = DefaultMarineInputPrimaryConfig;
						PlayerInputSecondaryConfig = DefaultMarineInputSecondaryConfig;
						PlayerWeaponPrimaryConfig = DefaultMarineWeaponPrimaryConfig;
						PlayerWeaponSecondaryConfig = DefaultMarineWeaponSecondaryConfig;
						extern int AutoWeaponChangeOn_Temp;
						extern int CrouchIsToggleKey_Temp;
						extern int UnlimitedSaveGames_Temp;
						extern int SmartJetpackEnabled_Temp;
						
						extern int SmartZoomEnabled_Temp;
						extern int FullVerticalPitchEnabled_Temp;
						AutoWeaponChangeOn_Temp=TRUE;
						CrouchIsToggleKey_Temp=0;
						UnlimitedSaveGames_Temp=0;
						FullVerticalPitchEnabled_Temp=0;
						SmartJetpackEnabled_Temp=0;
						
						break;
					}
					case AVPMENU_PREDATORKEYCONFIG:
					{
						PlayerPreviousVisionBindings[0]=KEY_X;PlayerPreviousVisionBindings[1]=KEY_VOID;
                        PlayerInputPrimaryConfig = DefaultPredatorInputPrimaryConfig;
						PlayerInputSecondaryConfig = DefaultPredatorInputSecondaryConfig;
						PlayerWeaponPrimaryConfig = DefaultPredatorWeaponPrimaryConfig;
						PlayerWeaponSecondaryConfig = DefaultPredatorWeaponSecondaryConfig;
					extern int CrouchIsToggleKey_Temp;
					extern int UnlimitedSaveGames_Temp;
					extern int SmartJetpackEnabled_Temp;
					extern int SmartZoomEnabled_Temp;
					extern int FullVerticalPitchEnabled_Temp;
					CrouchIsToggleKey_Temp=0;
					UnlimitedSaveGames_Temp=0;
					FullVerticalPitchEnabled_Temp=0;
						break;
					}
					case AVPMENU_ALIENKEYCONFIG:
					{
						PlayerInputPrimaryConfig = DefaultAlienInputPrimaryConfig;
						PlayerInputSecondaryConfig = DefaultAlienInputSecondaryConfig;
						PlayerWeaponPrimaryConfig = DefaultAlienWeaponPrimaryConfig;
						PlayerWeaponSecondaryConfig = DefaultAlienWeaponSecondaryConfig;
					extern int CrouchIsToggleKey_Temp;
					extern int UnlimitedSaveGames_Temp;
					extern int SmartJetpackEnabled_Temp;
					extern int SmartZoomEnabled_Temp;
					extern int FullVerticalPitchEnabled_Temp;
					CrouchIsToggleKey_Temp=0;
					UnlimitedSaveGames_Temp=0;
					FullVerticalPitchEnabled_Temp=0;
						break;
					}
					case AVPMENU_CONTROLS:
					{
						PlayerControlMethods = DefaultControlMethods;
						break;
					}
					case AVPMENU_JOYSTICKCONTROLS:
					{
						PlayerJoystickControlMethods = DefaultJoystickControlMethods;
						break;
					}
					
					default:
						break;
				}
				if (AvPMenus.CurrentMenu==AVPMENU_MARINEKEYCONFIG
				 || AvPMenus.CurrentMenu==AVPMENU_PREDATORKEYCONFIG
				 || AvPMenus.CurrentMenu==AVPMENU_ALIENKEYCONFIG)
				{
					PlayerGlobalCommandPrimaryConfig = DefaultGlobalCommandPrimaryConfig;
					PlayerGlobalCommandSecondaryConfig = DefaultGlobalCommandSecondaryConfig;
                    AlwaysRun_Temp=0;
				}
			}
			break;
		}

		case AVPMENU_ELEMENT_RESETMPCONFIG :
		{
			//reset the multiplayer configuration
			extern void SetDefaultMultiplayerConfig();
			SetDefaultMultiplayerConfig();
			break;
		}

		case AVPMENU_ELEMENT_SAVESETTINGS:
		{
			if (interactionID == AVPMENU_ELEMENT_INTERACTION_SELECT)
			{
				SetDetailLevelsFromMenu();
				/* Reuse this existing Detail Levels confirmation as the single
				 * user-facing point where a changed AA mode rebuilds OpenGL. */
				if (AvPMenus.CurrentMenu == AVPMENU_DETAILLEVELS)
				{
					RestartOpenGLVideoContextIfNeeded();
				}
				SaveUserProfile(UserProfilePtr);
				SaveDeviceAndVideoModePreferences();
				
				if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				{
					if(AvPMenus.CurrentMenu == AVPMENU_DETAILLEVELS)
					{
						SetupNewMenu(AVPMENU_INGAMEAVOPTIONS);
					}
					else
					{
						SetupNewMenu(AVPMENU_INGAME);
					}
				}
				else
				{
					SetupNewMenu(AvPMenusData[AvPMenus.CurrentMenu].ParentMenu);
				}
			}
			break;
		}
	
		case AVPMENU_ELEMENT_LOADGAME:
		{
			int slot = (AvPMenus.CurrentlySelectedElement == 0) ? QUICK_SAVE_SLOT : AvPMenus.CurrentlySelectedElement - 1;
			if(slot>=0 && slot<NUMBER_OF_SAVE_SLOTS)
			{
				if(SaveGameSlot[slot].SlotUsed)
				{
					//set the request
					LoadGameRequest = slot;
					EventLatencyLoadCheckpoint("menu load selected",slot);
					if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
					{
						//leave the menus
						AvPMenus.MenusState = MENUSSTATE_STARTGAME;	
					}
				}
			}
			break;
		}
		case AVPMENU_ELEMENT_SAVEGAME:
		{
			int slot = AvPMenus.CurrentlySelectedElement;
			if(slot>=0 && slot<NUMBER_OF_MANUAL_SAVE_SLOTS)
			{
				//set the request
				SaveGameRequest = slot;
				if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				{
					//leave the menus
					AvPMenus.MenusState = MENUSSTATE_STARTGAME;	
				}
			}
			break;
		}
	}
}


int LengthOfMenuText(char *textPtr);
int LengthOfSmallMenuText(char *textPtr);

/* Scale all horizontal menu anchors about the physical centre while the
   in-game text scope is active. */
static int ScaleInGameMenuTextX(int x)
{
	return MENU_CENTREX + MUL_FIXED(x-MENU_CENTREX, D3D_GetActiveConsoleTextScaleFixed());
}

static int Hardware_RenderScaledInGameMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format)
{
	return Hardware_RenderSmallMenuText(textPtr, ScaleInGameMenuTextX(x), y, alpha, format);
}

static int Hardware_RenderScaledInGameMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue)
{
	return Hardware_RenderSmallMenuText_Coloured(textPtr, ScaleInGameMenuTextX(x), y, alpha, format, red, green, blue);
}

extern void RenderOptimizedAmmoText(char *text,int x,int y,int alpha,int enabled);
static void RenderMenuElement(AVPMENU_ELEMENT *elementPtr, int e, int y)
{
 if(elementPtr->ElementID==AVPMENU_ELEMENT_TEXTSLIDER && elementPtr->c.SliderValuePtr==&OptimizedAmmo){
  char label[256];snprintf(label,sizeof label,"%s = %s",GetTextString(TEXTSTRING_OPTIMIZED_AMMO_NEW),GetTextString(OptimizedAmmo?TEXTSTRING_YES:TEXTSTRING_NO));
  RenderOptimizedAmmoText(label,MENU_CENTREX,y+12,elementPtr->Brightness,OptimizedAmmo);return;
 }
 if(elementPtr->ElementID==AVPMENU_ELEMENT_TEXTSLIDER && elementPtr->c.SliderValuePtr==&RetainPlayerState){
  char label[256];snprintf(label,sizeof label,"%s: %s",GetTextString(TEXTSTRING_RETAIN_STATE_NEW),
      GetTextString(RetainPlayerState==CAMPAIGN_RETAIN?TEXTSTRING_RETAIN_ON_NEW:(RetainPlayerState==CAMPAIGN_CHAINED?TEXTSTRING_RETAIN_CHAINED_NEW:TEXTSTRING_RETAIN_OFF_NEW)));
  RenderCampaignModeText(label,MENU_CENTREX,y+12,elementPtr->Brightness,RetainPlayerState);return;
 }
	if (!MenuElementIsVisible(elementPtr)) return;
	int (*RenderText)(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format);
	int (*RenderText_Coloured)(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int r, int g, int b);
	int (*MenuTextLength)(char *textPtr);
	
	if (AvPMenus.FontToUse==AVPMENU_FONT_BIG)
	{
		RenderText = RenderMenuText;
		MenuTextLength = LengthOfMenuText;
	}
	else
	{
		if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
		{
			if (InGameMenuTextUsesNativeAnchors)
			{
				RenderText = Hardware_RenderSmallMenuText;
				RenderText_Coloured = Hardware_RenderSmallMenuText_Coloured;
			}
			else
			{
				RenderText = Hardware_RenderScaledInGameMenuText;
				RenderText_Coloured = Hardware_RenderScaledInGameMenuText_Coloured;
			}
		}
		else
		{
			RenderText = RenderSmallMenuText;
			RenderText_Coloured = RenderSmallMenuText_Coloured;
		}
		MenuTextLength = LengthOfSmallMenuText;
	}

	switch(elementPtr->ElementID)
    {
        case AVPMENU_ELEMENT_SHAKE_PARAM:
        {
            int row=elementPtr->c.Value,i,scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
            char lines[64][512];int count=ShakeParamLabel(row,lines),barX=ScreenDescriptorBlock.SDB_Width/2-107,sliderY=y+count*MUL_FIXED(17,scale);
            double fraction=(ShakeValues[row]-ShakeParams[row].min)/(ShakeParams[row].max-ShakeParams[row].min);
            int x=barX+3+(int)(201*fraction);
            for(i=0;i<count;i++)RenderText(lines[i],ScreenDescriptorBlock.SDB_Width/2,y+i*MUL_FIXED(17,scale),elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
            if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS){
                D3D_SetInGameMenuTextRenderActive(0);D3D_DrawSliderBar(barX,sliderY+1,elementPtr->Brightness);D3D_DrawSlider(x,sliderY+4,elementPtr->Brightness);D3D_SetInGameMenuTextRenderActive(1);
            }else{DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR,barX,sliderY+1,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);DrawAvPMenuGfx(AVPMENUGFX_SLIDER,x,sliderY+4,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);}
            break;
        }
        case AVPMENU_ELEMENT_SHAKE_EXPORT:
            RenderText("export",ScreenDescriptorBlock.SDB_Width/2,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);break;
        case AVPMENU_ELEMENT_MOTION_PARAM:
        {
            int row=elementPtr->c.Value,i,scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
            char lines[64][512];int count=MotionParamLabel(row,lines),barX=ScreenDescriptorBlock.SDB_Width/2-107,sliderY=y+count*MUL_FIXED(17,scale);
            double fraction=(MotionValues[row]-MotionParams[row].min)/(MotionParams[row].max-MotionParams[row].min);
            int x=barX+3+(int)(201*fraction);
            for(i=0;i<count;i++)RenderText(lines[i],ScreenDescriptorBlock.SDB_Width/2,y+i*MUL_FIXED(17,scale),elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
            if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS){
                D3D_SetInGameMenuTextRenderActive(0);D3D_DrawSliderBar(barX,sliderY+1,elementPtr->Brightness);D3D_DrawSlider(x,sliderY+4,elementPtr->Brightness);D3D_SetInGameMenuTextRenderActive(1);
            }else{DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR,barX,sliderY+1,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);DrawAvPMenuGfx(AVPMENUGFX_SLIDER,x,sliderY+4,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);}
            break;
        }
        case AVPMENU_ELEMENT_DIFFICULTYLEVEL:
            if(elementPtr->a.TextDescription==TEXTSTRING_MOLECULAR_ACID_NEW){
                char label[256];snprintf(label,sizeof label,"%s  %s",GetTextString(TEXTSTRING_MOLECULAR_ACID_NEW),AcidMultiplierLabel(AcidMenuTier));
                RenderAcidMenuText(label,MENU_CENTREX,y,elementPtr->Brightness);
            }else RenderText(GetTextString(elementPtr->a.TextDescription),MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
            break;
        case AVPMENU_ELEMENT_MOTION_EXPORT:
            RenderText("export",ScreenDescriptorBlock.SDB_Width/2,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);break;
        case AVPMENU_ELEMENT_DEV_WEAPON_Z:
        {
            int row=elementPtr->c.Value,z=DevViewmodelValue(row);
            int barX=MENU_CENTREX-80;
            int scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
            int sliderY=y+MUL_FIXED(HUD_FONT_HEIGHT+2,scale);
            int x=barX+3+(int)(((int64_t)(z-DevViewmodelMinimum(row))*201)/(DevViewmodelMaximum(row)-DevViewmodelMinimum(row)));
            char label[96];DevViewmodelFormat(row,label,sizeof label);
            /* Long labels must also fit low resolutions at console scale x3. */
            if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS && MUL_FIXED(LengthOfSmallMenuText(label),scale)>ScreenDescriptorBlock.SDB_Width-32){
                D3D_SetInGameMenuTextRenderActive(0);
                Hardware_RenderSmallMenuText(label,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
                D3D_SetInGameMenuTextRenderActive(1);
            }else RenderText(label,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
            /* Slider geometry uses physical anchors, including its left toggle. */
            if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS)Hardware_RenderSmallMenuText(DevViewmodelEditing(row)?"[on]":"[off]",barX-8,sliderY,elementPtr->Brightness,AVPMENUFORMAT_RIGHTJUSTIFIED);
            else RenderText(DevViewmodelEditing(row)?"[on]":"[off]",barX-8,sliderY,elementPtr->Brightness,AVPMENUFORMAT_RIGHTJUSTIFIED);
            if(AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS){D3D_DrawSliderBar(barX,sliderY+1,elementPtr->Brightness);D3D_DrawSlider(x,sliderY+4,elementPtr->Brightness);}
            else{DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR,barX,sliderY+1,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);DrawAvPMenuGfx(AVPMENUGFX_SLIDER,x,sliderY+4,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);}
            break;
        }
        case AVPMENU_ELEMENT_DEV_EXPORT:
            RenderText("export",MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
            break;

		default:
		GLOBALASSERT("UNKNOWN MENU ELEMENT"==0);
		case AVPMENU_ELEMENT_GOTOMENU:
		case AVPMENU_ELEMENT_QUITGAME:
		case AVPMENU_ELEMENT_STARTMPGAME:
		case AVPMENU_ELEMENT_JOINMPGAME:
		case AVPMENU_ELEMENT_VIDEOMODEOK:
		case AVPMENU_ELEMENT_RESUMEGAME:
		case AVPMENU_ELEMENT_RESTARTGAME:
		case AVPMENU_ELEMENT_KEYCONFIGOK:
		case AVPMENU_ELEMENT_RESETKEYCONFIG:
		case AVPMENU_ELEMENT_STARTMARINEDEMO:
		case AVPMENU_ELEMENT_STARTPREDATORDEMO:
		case AVPMENU_ELEMENT_STARTALIENDEMO:
		case AVPMENU_ELEMENT_STARTLEVELWITHCHEAT:
		case AVPMENU_ELEMENT_SAVEMPCONFIG:
		case AVPMENU_ELEMENT_JOINLOBBIED:
		case AVPMENU_ELEMENT_CONNECTIONCHOICE :
		case AVPMENU_ELEMENT_USERPROFILE_DELETE:
		case AVPMENU_ELEMENT_RESETMPCONFIG:
		case AVPMENU_ELEMENT_DELETEMPCONFIG:
		case AVPMENU_ELEMENT_SAVESETTINGS:
		{
			char *textPtr = GetTextString(elementPtr->a.TextDescription);
			RenderText(textPtr,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
			break;
		}
		#if 0
		case AVPMENU_ELEMENT_STARTALIENGAME:
		case AVPMENU_ELEMENT_STARTMARINEGAME:
		case AVPMENU_ELEMENT_STARTPREDATORGAME:
		{
			char *textPtr = GetTextString(elementPtr->a.TextDescription);
			RenderText(textPtr,MENU_LEFTXEDGE,MENU_BOTTOMYEDGE,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}
	
		case AVPMENU_ELEMENT_MARINEEPISODE:
		case AVPMENU_ELEMENT_ALIENEPISODE:
		case AVPMENU_ELEMENT_PREDATOREPISODE:
		{
			char *textPtr = GetTextString(elementPtr->a.TextDescription+*(elementPtr->SliderValuePtr));
			RenderText(textPtr,MENU_LEFTXEDGE,MENU_TOPYEDGE,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}
		#endif

		case AVPMENU_ELEMENT_DUMMYTEXTSLIDER:
		case AVPMENU_ELEMENT_DUMMYTEXTSLIDER_POINTER:
		case AVPMENU_ELEMENT_TEXTSLIDER:
		case AVPMENU_ELEMENT_TEXTSLIDER_POINTER:
		case AVPMENU_ELEMENT_SPECIES_TEXTSLIDER:
		case AVPMENU_ELEMENT_CHEATMODE_TEXTSLIDER:
		case AVPMENU_ELEMENT_CHEATMODE_SPECIES_TEXTSLIDER:
		case AVPMENU_ELEMENT_CHEATMODE_ENVIRONMENT_TEXTSLIDER:
		{
			char *textPtr = "";
			if(elementPtr->ElementID == AVPMENU_ELEMENT_TEXTSLIDER_POINTER ||
			   elementPtr->ElementID == AVPMENU_ELEMENT_DUMMYTEXTSLIDER_POINTER)	
			{
				//we have a pointer to the strings rather than the first string index
				if(elementPtr->d.TextSliderStringPointer)
				{
					textPtr = elementPtr->d.TextSliderStringPointer[*(elementPtr->c.SliderValuePtr)];
				}
			}
			else
			{
				//we have the index of the first string
				if(elementPtr->c.SliderValuePtr==&ConsoleTextScaleMode && ConsoleTextScaleMode>=9)
                    textPtr=ConsoleTextScaleMode==9?"x0.8":"x0.9";
                else textPtr = GetTextString(elementPtr->d.FirstTextSliderString+*(elementPtr->c.SliderValuePtr));
			}
			
			if(elementPtr->a.TextDescription!=TEXTSTRING_BLANK)
			{
		
				if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				{
					RenderText
					(
						GetTextString(elementPtr->a.TextDescription),
						MENU_CENTREX-MENU_ELEMENT_SPACING,
						y,
						elementPtr->Brightness,
						AVPMENUFORMAT_RIGHTJUSTIFIED
					);
					RenderText
					(
						textPtr,
						MENU_CENTREX+MENU_ELEMENT_SPACING,
						y,
						elementPtr->Brightness,
						AVPMENUFORMAT_LEFTJUSTIFIED
					);

				}
				else
				{
					int length = MenuTextLength(textPtr);

					if (length>(ScreenDescriptorBlock.SDB_Width-MENU_CENTREX-MENU_ELEMENT_SPACING*2))
					{
						RenderText
						(
							GetTextString(elementPtr->a.TextDescription),
							ScreenDescriptorBlock.SDB_Width-MENU_ELEMENT_SPACING*2-length,
							y,
							elementPtr->Brightness,
							AVPMENUFORMAT_RIGHTJUSTIFIED
						);
						RenderText
						(
							textPtr,
							ScreenDescriptorBlock.SDB_Width-MENU_ELEMENT_SPACING,
							y,
							elementPtr->Brightness,
							AVPMENUFORMAT_RIGHTJUSTIFIED
						);
					}
					else
					{

						RenderText
						(
							GetTextString(elementPtr->a.TextDescription),
							MENU_CENTREX-MENU_ELEMENT_SPACING,
							y,																		 
							elementPtr->Brightness,
							AVPMENUFORMAT_RIGHTJUSTIFIED
						);
						RenderText
						(
							textPtr,
							MENU_CENTREX+MENU_ELEMENT_SPACING,
							y,
							elementPtr->Brightness,
							AVPMENUFORMAT_LEFTJUSTIFIED
						);
					}
				}
			}
			else
			{
				RenderText
				(
					textPtr,
					MENU_CENTREX,
					y,
					elementPtr->Brightness,
					AVPMENUFORMAT_CENTREJUSTIFIED
				);
			}
			break;
		}
		case AVPMENU_ELEMENT_DUMMYTEXTFIELD:
		case AVPMENU_ELEMENT_TEXTFIELD:
		{
            if (AvPMenus.CurrentMenu == AVPMENU_USERPROFILEENTERNAME)
            {
                char *visible=elementPtr->c.TextPtr;
                const int width=MENU_RIGHTXEDGE-MENU_LEFTXEDGE-40;
                while (*visible && LengthOfMenuText(visible)>width)
                {
                    const char *next=visible;
                    AvPTextNextGlyphConst(&next);
                    visible=(char*)next;
                }
                int x=RenderText(visible,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
                if (AvPMenus.UserEnteringText)
                {
                    int b=GetSin(CloakingPhase&4095);
                    RenderText("I",x,y,MUL_FIXED(b,b),AVPMENUFORMAT_CENTREJUSTIFIED);
                }
                AvPMenus.WidthLeftForText=4096;
                break;
            }

			if (elementPtr->a.TextDescription==TEXTSTRING_BLANK)
			{
				if (AvPMenus.UserEnteringText && e==AvPMenus.CurrentlySelectedElement)
				{
					int b = GetSin(CloakingPhase&4095);
					int x = RenderText(elementPtr->c.TextPtr,MENU_CENTREX,y,elementPtr->Brightness/2,AVPMENUFORMAT_CENTREJUSTIFIED);
					x = RenderText("I",x,y,MUL_FIXED(b,b),AVPMENUFORMAT_CENTREJUSTIFIED);

					//work out how much space was left over
					if(AvPMenus.PositionInTextField)
						AvPMenus.WidthLeftForText = MENU_RIGHTXEDGE -x;
					else
						AvPMenus.WidthLeftForText = MENU_RIGHTXEDGE -MENU_CENTREX;
				}
				else
				{
					RenderText(elementPtr->c.TextPtr,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
				}
			}
			else
			{
				RenderText(GetTextString(elementPtr->a.TextDescription),MENU_CENTREX-MENU_ELEMENT_SPACING,y,elementPtr->Brightness,AVPMENUFORMAT_RIGHTJUSTIFIED);
			
				if (AvPMenus.UserEnteringText && e==AvPMenus.CurrentlySelectedElement)
				{
					int b = GetSin(CloakingPhase&4095);
					int x = RenderText(elementPtr->c.TextPtr,MENU_CENTREX+MENU_ELEMENT_SPACING,y,elementPtr->Brightness/2,AVPMENUFORMAT_LEFTJUSTIFIED);
					x = RenderText("I",x,y,MUL_FIXED(b,b),AVPMENUFORMAT_LEFTJUSTIFIED);
					
					//work out how much space was left over
					if(AvPMenus.PositionInTextField)
						AvPMenus.WidthLeftForText = MENU_RIGHTXEDGE -x;
					else
						AvPMenus.WidthLeftForText = MENU_RIGHTXEDGE -(MENU_CENTREX+MENU_ELEMENT_SPACING);
				}
				else
				{
					RenderText(elementPtr->c.TextPtr,MENU_CENTREX+MENU_ELEMENT_SPACING,y,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
				}
			}
			break;
		}

		case AVPMENU_ELEMENT_TEXTFIELD_SMALLWRAPPED:
		{
			if (elementPtr->a.TextDescription==TEXTSTRING_BLANK)
			{
				RECT area;
				area.left=MENU_LEFTXEDGE;
				area.right=MENU_RIGHTXEDGE;
				area.top=y;
				area.bottom=y+4*HUD_FONT_HEIGHT;
				
				if (AvPMenus.UserEnteringText && e==AvPMenus.CurrentlySelectedElement)
				{
					int output_x,output_y;
					int b = GetSin(CloakingPhase&4095);
					RenderSmallFontString_Wrapped(elementPtr->c.TextPtr,&area,elementPtr->Brightness/2,&output_x,&output_y);
					output_x = RenderSmallMenuText("I",output_x,output_y,MUL_FIXED(b,b),AVPMENUFORMAT_LEFTJUSTIFIED);
					
					//work out how much space was left over
					if(area.bottom - output_y < HUD_FONT_HEIGHT)
					{
						//no space left
						AvPMenus.WidthLeftForText= 0;
					}
					else if(area.bottom - output_y >= 2*HUD_FONT_HEIGHT)
					{
						//at least a whole line left
						AvPMenus.WidthLeftForText= area.right-area.left;
					}
					else
					{
						//on the last line
						AvPMenus.WidthLeftForText= area.right-output_x;
					}
				}
				else
				{
					RenderSmallFontString_Wrapped(elementPtr->c.TextPtr,&area,elementPtr->Brightness,0,0);
				}
			}
			else
			{
				RECT area;
				area.left=MENU_CENTREX+MENU_ELEMENT_SPACING;
				area.right=MENU_RIGHTXEDGE;
				area.top=y;
				area.bottom=y+4*HUD_FONT_HEIGHT;
				
				RenderText(GetTextString(elementPtr->a.TextDescription),MENU_CENTREX-MENU_ELEMENT_SPACING,y+HUD_FONT_HEIGHT,elementPtr->Brightness,AVPMENUFORMAT_RIGHTJUSTIFIED);
			
				if (AvPMenus.UserEnteringText && e==AvPMenus.CurrentlySelectedElement)
				{
					int output_x,output_y;
					int b = GetSin(CloakingPhase&4095);
					RenderSmallFontString_Wrapped(elementPtr->c.TextPtr,&area,elementPtr->Brightness/2,&output_x,&output_y);
					output_x = RenderSmallMenuText("I",output_x,output_y,MUL_FIXED(b,b),AVPMENUFORMAT_LEFTJUSTIFIED);
					
					//work out how much space was left over
					if(area.bottom - output_y < HUD_FONT_HEIGHT)
					{
						//no space left
						AvPMenus.WidthLeftForText= 0;
					}
					else if(area.bottom - output_y >= 2*HUD_FONT_HEIGHT)
					{
						//at least a whole line left
						AvPMenus.WidthLeftForText= area.right-area.left;
					}
					else
					{
						//on the last line
						AvPMenus.WidthLeftForText= area.right-output_x;
					}
				}
				else
				{
					RenderSmallFontString_Wrapped(elementPtr->c.TextPtr,&area,elementPtr->Brightness,0,0);
				}
			}
			break;
		}

		case AVPMENU_ELEMENT_DUMMYNUMBERFIELD:
		case AVPMENU_ELEMENT_NUMBERFIELD:
		{
			{
				static char NumberString[50];
				if(*elementPtr->c.NumberPtr!=0 || elementPtr->NumberFieldZeroString==TEXTSTRING_BLANK)
				{
					sprintf(NumberString,"%d",*elementPtr->c.NumberPtr);
					if(elementPtr->d.NumberFieldUnitsString!=TEXTSTRING_BLANK)
					{
						//add the text for the unit type	
						strcat(NumberString," ");
						strcat(NumberString,GetTextString(elementPtr->d.NumberFieldUnitsString));
					}
				}
				else
				{
					//use the special string to represent 0
					sprintf(NumberString,"%s",GetTextString(elementPtr->NumberFieldZeroString));
				}
				
				if (elementPtr->a.TextDescription==TEXTSTRING_BLANK)
				{
					if (AvPMenus.UserEnteringNumber && e==AvPMenus.CurrentlySelectedElement)
					{
						int b = GetSin(CloakingPhase&4095);
						int x = RenderText(NumberString,MENU_CENTREX,y,elementPtr->Brightness/2,AVPMENUFORMAT_CENTREJUSTIFIED);
						RenderText("I",x,y,MUL_FIXED(b,b),AVPMENUFORMAT_CENTREJUSTIFIED);
					}
					else
					{
						RenderText(NumberString,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
					}
				}
				else
				{
					RenderText(GetTextString(elementPtr->a.TextDescription),MENU_CENTREX-MENU_ELEMENT_SPACING,y,elementPtr->Brightness,AVPMENUFORMAT_RIGHTJUSTIFIED);
			
					if (AvPMenus.UserEnteringNumber && e==AvPMenus.CurrentlySelectedElement)
					{
						int b = GetSin(CloakingPhase&4095);
						int x = RenderText(NumberString,MENU_CENTREX+MENU_ELEMENT_SPACING,y,elementPtr->Brightness/2,AVPMENUFORMAT_LEFTJUSTIFIED);
						RenderText("I",x,y,MUL_FIXED(b,b),AVPMENUFORMAT_LEFTJUSTIFIED);
					}
					else
					{
						RenderText(NumberString,MENU_CENTREX+MENU_ELEMENT_SPACING,y,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
					}
				}
			}
			break;
		}
		
		case AVPMENU_ELEMENT_GOTOMENU_GFX:
		{
			DrawAvPMenuGfx(elementPtr->a.GfxID,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
			break;
		}
		case AVPMENU_ELEMENT_LISTCHOICE:
		{
			RenderText(elementPtr->c.TextPtr,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
			break;
		}
		case AVPMENU_ELEMENT_LOADMPCONFIG:
		{
			RenderText(elementPtr->c.TextPtr,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
			break;
		}
		case AVPMENU_ELEMENT_LOADIPADDRESS:
		{
			RenderText(elementPtr->c.TextPtr,MENU_CENTREX,y,elementPtr->Brightness,AVPMENUFORMAT_CENTREJUSTIFIED);
			break;
		}
		
		case AVPMENU_ELEMENT_TOGGLE:
		{
			break;
		}
						case AVPMENU_ELEMENT_WEAPON_FOV:
		{
			char fovText[16];
			SynchroniseWeaponFOVForCurrentSpecies();
			int x = MENU_CENTREX+MENU_ELEMENT_SPACING+3;
			x += (201*(WeaponFOV-WEAPON_FOV_MINIMUM))/(WeaponFOVMaximumForCurrentSpecies()-WEAPON_FOV_MINIMUM);
			sprintf(fovText, "%d", WeaponFOV);
			RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
			{
				D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness);
				D3D_DrawSlider(x, y+4, elementPtr->Brightness);
			}
			else
			{
				DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR, MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
				DrawAvPMenuGfx(AVPMENUGFX_SLIDER, x, y+4, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			}
			RenderText(fovText, MENU_CENTREX+MENU_ELEMENT_SPACING+220, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}
		case AVPMENU_ELEMENT_FOV:
		{
			ClampPlayerFOVForCurrentSpecies();

char fovText[16];
			int minimum = PlayerFOVMinimumForCurrentSpecies();
			int maximum = PlayerFOVMaximumForCurrentSpecies();
			int x = MENU_CENTREX+MENU_ELEMENT_SPACING+3;
			x += (201*(PlayerFOV-minimum))/(maximum-minimum);
			sprintf(fovText, "%d", PlayerFOV);
			RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
			{
				D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness);
				D3D_DrawSlider(x, y+4, elementPtr->Brightness);
			}
			else
			{
				DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR, MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
				DrawAvPMenuGfx(AVPMENUGFX_SLIDER, x, y+4, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			}
			RenderText(fovText, MENU_CENTREX+MENU_ELEMENT_SPACING+220, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}
		case AVPMENU_ELEMENT_HUD_ANIMATION_INTENSITY:
		{
			char hudScaleText[16];
			
			int x = MENU_CENTREX + MENU_ELEMENT_SPACING + 3;
			x += (201 * ((*elementPtr->c.SliderValuePtr) - 1)) / (100 - 1);
			snprintf(hudScaleText, sizeof(hudScaleText), "%d", (*elementPtr->c.SliderValuePtr));
			RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS) { D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness); D3D_DrawSlider(x, y+4, elementPtr->Brightness); }
			else { DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR, MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED); DrawAvPMenuGfx(AVPMENUGFX_SLIDER, x, y+4, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED); }
			RenderText(hudScaleText, MENU_CENTREX+MENU_ELEMENT_SPACING+220, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}
				case AVPMENU_ELEMENT_HUD_SCALE:
		{
			char hudScaleText[16];
			HUD_SynchronisePerSpeciesOptions();
			int x = MENU_CENTREX + MENU_ELEMENT_SPACING + 3;
			x += (201 * (HUDUserScaleTenths - HUD_USER_SCALE_TENTHS_MINIMUM)) / (HUD_USER_SCALE_TENTHS_MAXIMUM - HUD_USER_SCALE_TENTHS_MINIMUM);
			snprintf(hudScaleText, sizeof(hudScaleText), "x%.1f", (double)HUDUserScaleTenths / 10.0);
			RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS) { D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness); D3D_DrawSlider(x, y+4, elementPtr->Brightness); }
			else { DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR, MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED); DrawAvPMenuGfx(AVPMENUGFX_SLIDER, x, y+4, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED); }
			RenderText(hudScaleText, MENU_CENTREX+MENU_ELEMENT_SPACING+220, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}
				case AVPMENU_ELEMENT_CROSSHAIR_SCALE:
		{
			HUD_ClampCrosshairScaleForCurrentSpecies();
			char crosshairScaleText[16];
			int x = MENU_CENTREX + MENU_ELEMENT_SPACING + 3;
			x += (201 * (CrosshairScaleTenths - CROSSHAIR_SCALE_TENTHS_MINIMUM)) / (CROSSHAIR_SCALE_TENTHS_MAXIMUM - CROSSHAIR_SCALE_TENTHS_MINIMUM);
			snprintf(crosshairScaleText, sizeof(crosshairScaleText), "x%.1f", (double)CrosshairScaleTenths / 10.0);
			RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS) { D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness); D3D_DrawSlider(x, y+4, elementPtr->Brightness); }
			else { DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR, MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED); DrawAvPMenuGfx(AVPMENUGFX_SLIDER, x, y+4, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED); }
			RenderText(crosshairScaleText, MENU_CENTREX+MENU_ELEMENT_SPACING+220, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}case AVPMENU_ELEMENT_HUD_ANCHORED:
		{
			char *hudAnchorText;
			switch (HUDAnchorMode)
			{
				case HUD_ANCHOR_16_9: hudAnchorText = GetTextString(TEXTSTRING_HUD_ANCHORED_16_9_NEW); break;
				case HUD_ANCHOR_UNLOCKED: hudAnchorText = GetTextString(TEXTSTRING_HUD_ANCHORED_UNLOCKED_NEW); break;
				case HUD_ANCHOR_4_3:
				default: hudAnchorText = GetTextString(TEXTSTRING_HUD_ANCHORED_4_3_NEW); break;
			}
			RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			RenderText(hudAnchorText, MENU_CENTREX+MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}
		case AVPMENU_ELEMENT_HUD_POSITION:
		{
			char hudPositionText[16];
			HUD_SynchronisePerSpeciesOptions();
			int x = MENU_CENTREX+MENU_ELEMENT_SPACING+3;
			x += (201*HUDAnchorPosition)/100;
			HUDPositionDescription(hudPositionText, sizeof(hudPositionText));
			RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
			if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
			{
				D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness);
				D3D_DrawSlider(x, y+4, elementPtr->Brightness);
			}
			else
			{
				DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR, MENU_CENTREX+MENU_ELEMENT_SPACING, y+1, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
				DrawAvPMenuGfx(AVPMENUGFX_SLIDER, x, y+4, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			}
			RenderText(hudPositionText, MENU_CENTREX+MENU_ELEMENT_SPACING+220, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}case AVPMENU_ELEMENT_ZOOM_SENSITIVITY:
		{
			char zoomSensitivityText[16];
			int x = MENU_CENTREX+MENU_ELEMENT_SPACING+3;
			x += (201*(*elementPtr->c.SliderValuePtr))/elementPtr->b.MaxSliderValue;
			sprintf(zoomSensitivityText,"%.1f",(float)(*elementPtr->c.SliderValuePtr)/10.0f);
			RenderText(GetTextString(elementPtr->a.TextDescription),MENU_CENTREX-MENU_ELEMENT_SPACING,y,elementPtr->Brightness,AVPMENUFORMAT_RIGHTJUSTIFIED);
			if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
			{
				D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING,y+1,elementPtr->Brightness);
				D3D_DrawSlider(x,y+4,elementPtr->Brightness);
			}
			else
			{
				DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR,MENU_CENTREX+MENU_ELEMENT_SPACING,y+1,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
				DrawAvPMenuGfx(AVPMENUGFX_SLIDER,x,y+4,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
			}
			RenderText(zoomSensitivityText,MENU_CENTREX+MENU_ELEMENT_SPACING+220,y,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
			break;
		}		case AVPMENU_ELEMENT_SLIDER:
		{
			int x = MENU_CENTREX+MENU_ELEMENT_SPACING+3;
			char sliderCounter[16];
			x+=(201*(*elementPtr->c.SliderValuePtr))/elementPtr->b.MaxSliderValue;
			RenderText(GetTextString(elementPtr->a.TextDescription),MENU_CENTREX-MENU_ELEMENT_SPACING,y,elementPtr->Brightness,AVPMENUFORMAT_RIGHTJUSTIFIED);
			if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
			{
				D3D_DrawSliderBar(MENU_CENTREX+MENU_ELEMENT_SPACING,y+1,elementPtr->Brightness);
				D3D_DrawSlider(x,y+4,elementPtr->Brightness);
			}
			else
			{
				DrawAvPMenuGfx(AVPMENUGFX_SLIDERBAR,MENU_CENTREX+MENU_ELEMENT_SPACING,y+1,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
				DrawAvPMenuGfx(AVPMENUGFX_SLIDER,x,y+4,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
			}
			if (VanillaSliderUsesLiveCounter(elementPtr->a.TextDescription))
			{
				FormatVanillaSliderCounter(elementPtr, sliderCounter, sizeof(sliderCounter));
				RenderText(sliderCounter,MENU_CENTREX+MENU_ELEMENT_SPACING+220,y,elementPtr->Brightness,AVPMENUFORMAT_LEFTJUSTIFIED);
			}


			break;
		}
		case AVPMENU_ELEMENT_VIDEO_RENDERER:
		case AVPMENU_ELEMENT_VIDEO_ASPECT:
case AVPMENU_ELEMENT_VIDEO_RESOLUTION:
case AVPMENU_ELEMENT_VIDEO_REFRESH:
case AVPMENU_ELEMENT_VIDEO_SCREENMODE:
case AVPMENU_ELEMENT_VIDEO_VSYNC:
case AVPMENU_ELEMENT_VIDEO_FPSCAP:
{
    char *videoValue;
    switch (elementPtr->ElementID)
    {
        case AVPMENU_ELEMENT_VIDEO_RENDERER: videoValue = GetVideoRendererDescription(); break;
        case AVPMENU_ELEMENT_VIDEO_ASPECT: videoValue = GetVideoAspectDescription(); break;
        case AVPMENU_ELEMENT_VIDEO_RESOLUTION: videoValue = GetVideoResolutionDescription(); break;
        case AVPMENU_ELEMENT_VIDEO_REFRESH: videoValue = GetVideoRefreshDescription(); break;
        case AVPMENU_ELEMENT_VIDEO_VSYNC: videoValue = GetVideoVSyncDescription(); break;
        case AVPMENU_ELEMENT_VIDEO_FPSCAP: videoValue = GetVideoFPSCapDescription(); break;
        default: videoValue = GetVideoScreenModeDescription(); break;
    }
    RenderText(GetTextString(elementPtr->a.TextDescription), MENU_CENTREX-MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_RIGHTJUSTIFIED);
    if (elementPtr->ElementID == AVPMENU_ELEMENT_VIDEO_FPSCAP && AvPMenus.UserEnteringNumber && e == AvPMenus.CurrentlySelectedElement)
    {
        const int valueX = MENU_CENTREX + MENU_ELEMENT_SPACING;
        int b = GetSin(CloakingPhase & 4095);

        if (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
        {
            RenderText(videoValue, valueX, y, elementPtr->Brightness / 2, AVPMENUFORMAT_LEFTJUSTIFIED);
            int renderedValueX = InGameMenuTextUsesNativeAnchors ? valueX : ScaleInGameMenuTextX(valueX);
            int valueWidth = MUL_FIXED(MenuTextLength(videoValue), D3D_GetActiveConsoleTextScaleFixed());
            /* Hardware_RenderSmallMenuText returns its start x, not its end x. */
            Hardware_RenderSmallMenuText("I", renderedValueX + valueWidth + 3, y, MUL_FIXED(b, b), AVPMENUFORMAT_LEFTJUSTIFIED);
        }
        else
        {
            int cursorX = RenderText(videoValue, valueX, y, elementPtr->Brightness / 2, AVPMENUFORMAT_LEFTJUSTIFIED);
            RenderText("I", cursorX + 3, y, MUL_FIXED(b, b), AVPMENUFORMAT_LEFTJUSTIFIED);
        }
    }
    else RenderText(videoValue, MENU_CENTREX+MENU_ELEMENT_SPACING, y, elementPtr->Brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
    break;
}case AVPMENU_ELEMENT_VIDEOMODE:
		{
/*			RenderText
			(
				GetTextString(elementPtr->a.TextDescription),
				MENU_CENTREX-MENU_ELEMENT_SPACING,
				y,
				elementPtr->Brightness,
				AVPMENUFORMAT_RIGHTJUSTIFIED
			);	 */
			RenderText
			(
				GetVideoModeDescription2(),
				MENU_CENTREX,
				y,
				elementPtr->Brightness,
				AVPMENUFORMAT_CENTREJUSTIFIED
			);
			RenderText
			(
				GetVideoModeDescription3(),
				MENU_CENTREX,
				y+MENU_FONT_HEIGHT+MENU_ELEMENT_SPACING,
				elementPtr->Brightness,
				AVPMENUFORMAT_CENTREJUSTIFIED
			);
			break;
		}
		case AVPMENU_ELEMENT_KEYCONFIG:
		{
			unsigned char *primaryKey = KeyConfigBindingPointer(1,e);
			unsigned char *secondaryKey = KeyConfigBindingPointer(0,e);
			if (e==AvPMenus.CurrentlySelectedElement)
			{
				int x,g;
				int highlightScale = D3D_GetActiveConsoleTextScaleFixed();
				if (KeyConfigSelectionColumn)
				{
					x = MENU_RIGHTXEDGE;
				}
				else
				{
					x = MENU_CENTREX;
				}
				if (AvPMenus.UserChangingKeyConfig)
				{
					g = 255;
				}
				else
				{
					g = 128;
				}

				if(AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS)
				{
					Hardware_RenderHighlightRectangle(x-MUL_FIXED(100,highlightScale),y-MUL_FIXED(4,highlightScale),x+MUL_FIXED(4,highlightScale),y+MUL_FIXED(19,highlightScale),0,g,0);
				}
				else
				{
					RenderHighlightRectangle(x-100,y-4,x+4,y+19,0,g,0);
				}
			}
			if (AvPMenus.UserChangingKeyConfig && e==AvPMenus.CurrentlySelectedElement)
			{
				int b = GetSin(CloakingPhase&4095);
				if (AvPMenus.ChangingPrimaryConfig)
				{
					RenderText("_",MENU_CENTREX,y,MUL_FIXED(b,b),AVPMENUFORMAT_RIGHTJUSTIFIED);
					RenderText
					(
						GetDescriptionOfKey(*secondaryKey),
						MENU_RIGHTXEDGE,
						y,
						elementPtr->Brightness,
						AVPMENUFORMAT_RIGHTJUSTIFIED
					);

				}
				else
				{
					RenderText
					(
						GetDescriptionOfKey(*primaryKey),
						MENU_CENTREX,
						y,
						elementPtr->Brightness,
						AVPMENUFORMAT_RIGHTJUSTIFIED
					);
					RenderText("_",MENU_RIGHTXEDGE,y,MUL_FIXED(b,b),AVPMENUFORMAT_RIGHTJUSTIFIED);

				}

			}
			else
			{
				if (MultipleAssignments[0][KeyConfigVisibleBindingIndex(e)])
				{
					RenderText_Coloured
					(
						GetDescriptionOfKey(*primaryKey),
						MENU_CENTREX,
						y,
						ONE_FIXED,
						AVPMENUFORMAT_RIGHTJUSTIFIED,
						ONE_FIXED,
						ONE_FIXED,
						0
					);
				}
				else
				{
					RenderText
					(
						GetDescriptionOfKey(*primaryKey),
						MENU_CENTREX,
						y,
						elementPtr->Brightness,
						AVPMENUFORMAT_RIGHTJUSTIFIED
					);
				}
				if (MultipleAssignments[1][KeyConfigVisibleBindingIndex(e)])
				{
					RenderText_Coloured
					(
						GetDescriptionOfKey(*secondaryKey),
						MENU_RIGHTXEDGE,
						y,
						ONE_FIXED,
						AVPMENUFORMAT_RIGHTJUSTIFIED,
						ONE_FIXED,
						ONE_FIXED,
						0
					);
				}
				else
				{
					RenderText
					(
						GetDescriptionOfKey(*secondaryKey),
						MENU_RIGHTXEDGE,
						y,
						elementPtr->Brightness,
						AVPMENUFORMAT_RIGHTJUSTIFIED
					);
				}
			}

            enum TEXTSTRING_ID bindingLabel=elementPtr->a.TextDescription;
            if(bindingLabel==TEXTSTRING_MARINE_KEY_WALK || bindingLabel==TEXTSTRING_PREDATOR_KEY_WALK || bindingLabel==TEXTSTRING_ALIEN_KEY_WALK)
                bindingLabel=AlwaysRun_Temp ? TEXTSTRING_WALK_ACTION_NEW : TEXTSTRING_RUN_ACTION_NEW;
			RenderText
			(
				GetTextString(bindingLabel),
				MENU_LEFTXEDGE,
				y,
				elementPtr->Brightness,
				AVPMENUFORMAT_LEFTJUSTIFIED
			);


		}
	}
}

static int HeightOfMenuElement(AVPMENU_ELEMENT *elementPtr)
{
 /* Both rows draw at y+12; one font height plus 5 leaves a five-pixel gap. */
 if(elementPtr->ElementID==AVPMENU_ELEMENT_TEXTSLIDER && elementPtr->c.SliderValuePtr==&RetainPlayerState)return HUD_FONT_HEIGHT+5;
 if(elementPtr->ElementID==AVPMENU_ELEMENT_TEXTSLIDER && elementPtr->c.SliderValuePtr==&OptimizedAmmo)return 40;
	if (!MenuElementIsVisible(elementPtr)) return 0;
	int h = MENU_ELEMENT_SPACING;

	switch(elementPtr->ElementID)
	{
        case AVPMENU_ELEMENT_SHAKE_PARAM: {
            char lines[64][512];int scale=AvPMenus.MenusState==MENUSSTATE_INGAMEMENUS?D3D_GetConsoleTextScaleFixed():ONE_FIXED;
            return ShakeParamLabel(elementPtr->c.Value,lines)*17+(22*ONE_FIXED+scale-1)/scale;
        }
        case AVPMENU_ELEMENT_MOTION_PARAM: {char lines[64][512];return MotionParamLabel(elementPtr->c.Value,lines)*17+22;}
        case AVPMENU_ELEMENT_DEV_WEAPON_Z: return HUD_FONT_HEIGHT*2+10;
		default:
		{
			if (AvPMenus.FontToUse==AVPMENU_FONT_BIG)
			{
				h += MENU_FONT_HEIGHT;
			}
			else
			{
				h = HUD_FONT_HEIGHT+4;
			}
			break;
		}
		case AVPMENU_ELEMENT_LOADGAME:
		case AVPMENU_ELEMENT_SAVEGAME:
		{			
			h= HUD_FONT_HEIGHT*2+4;
			break;
		}
		case AVPMENU_ELEMENT_VIDEOMODE:
		{
			h += MENU_FONT_HEIGHT;
			h *=2;
			break;
		}

		case AVPMENU_ELEMENT_TEXTFIELD_SMALLWRAPPED :
		{
			h+=HUD_FONT_HEIGHT*4;
			break;
		}

		#if 0
		case AVPMENU_ELEMENT_STARTALIENGAME:
		case AVPMENU_ELEMENT_STARTMARINEGAME:
		case AVPMENU_ELEMENT_STARTPREDATORGAME:
		#endif
		/*
		case AVPMENU_ELEMENT_DUMMYTEXTFIELD:
		case AVPMENU_ELEMENT_TEXTFIELD:
		{
			h += MENU_FONT_HEIGHT*2;
			break;
		}
		*/
		
		case AVPMENU_ELEMENT_GOTOMENU_GFX:
		{
			h += HeightOfMenuGfx(elementPtr->a.GfxID);
			break;
		}
#if 0
		case AVPMENU_ELEMENT_SLIDER:
		{
			h += HeightOfMenuGfx(AVPMENUGFX_SLIDER);
			break;
		}
#endif
	}
	return h;
}

static char *GetDescriptionOfKey(unsigned char key)
{
	static char KeyDescBuffer[2]="\0\0";
	static char MouseButton5Description[]="MOUSE5";
	char *textPtr;
	
	switch (key)
	{
		case KEY_UP:
			textPtr = GetTextString(TEXTSTRING_KEYS_UP);
			break;
		case KEY_DOWN:
			textPtr = GetTextString(TEXTSTRING_KEYS_DOWN);
			break;
		case KEY_LEFT:
			textPtr = GetTextString(TEXTSTRING_KEYS_LEFT);
			break;
		case KEY_RIGHT:
			textPtr = GetTextString(TEXTSTRING_KEYS_RIGHT);
			break;
		case KEY_CR:
			textPtr = GetTextString(TEXTSTRING_KEYS_RETURN);
			break;
		case KEY_TAB:
			textPtr = GetTextString(TEXTSTRING_KEYS_TAB);
			break;
		case KEY_INS:
			textPtr = GetTextString(TEXTSTRING_KEYS_INSERT);
			break;
		case KEY_DEL:
			textPtr = GetTextString(TEXTSTRING_KEYS_DELETE);
			break;
		case KEY_END:
			textPtr = GetTextString(TEXTSTRING_KEYS_END);
			break;
		case KEY_HOME:
			textPtr = GetTextString(TEXTSTRING_KEYS_HOME);
			break;
		case KEY_PAGEUP:
			textPtr = GetTextString(TEXTSTRING_KEYS_PGUP);
			break;
		case KEY_PAGEDOWN:
			textPtr = GetTextString(TEXTSTRING_KEYS_PGDOWN);
			break;
		case KEY_BACKSPACE:
			textPtr = GetTextString(TEXTSTRING_KEYS_BACKSP);
			break;
		case KEY_COMMA:
			textPtr = GetTextString(TEXTSTRING_KEYS_COMMA);
			break;
		case KEY_FSTOP:
			textPtr = GetTextString(TEXTSTRING_KEYS_PERIOD);
			break;
		case KEY_SPACE:
			textPtr = GetTextString(TEXTSTRING_KEYS_SPACE);
			break;
		case KEY_LMOUSE:
			textPtr = GetTextString(TEXTSTRING_KEYS_LMOUSE);
			break;
		case KEY_RMOUSE:
			textPtr = GetTextString(TEXTSTRING_KEYS_RMOUSE);
			break;
		case KEY_MMOUSE:
			textPtr = GetTextString(TEXTSTRING_KEYS_MMOUSE);
			break;
		case KEY_MOUSEBUTTON4:
			textPtr = GetTextString(TEXTSTRING_KEYS_MOUSEBUTTON4);
			break;
		case KEY_MOUSEBUTTON5:
			textPtr = MouseButton5Description;
			break;
		case KEY_MOUSEWHEELUP:
			textPtr = GetTextString(TEXTSTRING_KEYS_MOUSEWHEELUP);
			break;
		case KEY_MOUSEWHEELDOWN:
			textPtr = GetTextString(TEXTSTRING_KEYS_MOUSEWHEELDOWN);
			break;
		case KEY_LEFTALT:
			textPtr = GetTextString(TEXTSTRING_KEYS_LALT);
			break;
		case KEY_RIGHTALT:
			textPtr = GetTextString(TEXTSTRING_KEYS_RALT);
			break;
		case KEY_LEFTCTRL:
			textPtr = GetTextString(TEXTSTRING_KEYS_LCTRL);
			break;
		case KEY_RIGHTCTRL:
			textPtr = GetTextString(TEXTSTRING_KEYS_RCTRL);
			break;
		case KEY_LEFTSHIFT:
			textPtr = GetTextString(TEXTSTRING_KEYS_LSHIFT);
			break;
		case KEY_RIGHTSHIFT:
			textPtr = GetTextString(TEXTSTRING_KEYS_RSHIFT);
			break;
		case KEY_CAPS:
			textPtr = GetTextString(TEXTSTRING_KEYS_CAPS);
			break;
		case KEY_NUMLOCK:
			textPtr = GetTextString(TEXTSTRING_KEYS_NUMLOCK);
			break;
		case KEY_SCROLLOK:
			textPtr = GetTextString(TEXTSTRING_KEYS_SCRLOCK);
			break;
		case KEY_NUMPAD0:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD0);
			break;
		case KEY_NUMPAD1:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD1);
			break;
		case KEY_NUMPAD2:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD2);
			break;
		case KEY_NUMPAD3:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD3);
			break;
		case KEY_NUMPAD4:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD4);
			break;
		case KEY_NUMPAD5:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD5);
			break;
		case KEY_NUMPAD6:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD6);
			break;
		case KEY_NUMPAD7:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD7);
			break;
		case KEY_NUMPAD8:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD8);
			break;
		case KEY_NUMPAD9:
			textPtr = GetTextString(TEXTSTRING_KEYS_PAD9);
			break;
		case KEY_NUMPADSUB:
			textPtr = GetTextString(TEXTSTRING_KEYS_PADSUB);
			break;
		case KEY_NUMPADADD:
			textPtr = GetTextString(TEXTSTRING_KEYS_PADADD);
			break;
		case KEY_NUMPADDEL:
			textPtr = GetTextString(TEXTSTRING_KEYS_PADDEL);
			break;
		case KEY_JOYSTICK_BUTTON_1:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_1);
			break;
		case KEY_JOYSTICK_BUTTON_2:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_2);
			break;
		case KEY_JOYSTICK_BUTTON_3:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_3);
			break;
		case KEY_JOYSTICK_BUTTON_4:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_4);
			break;
		case KEY_JOYSTICK_BUTTON_5:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_5);
			break;
		case KEY_JOYSTICK_BUTTON_6:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_6);
			break;
		case KEY_JOYSTICK_BUTTON_7:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_7);
			break;
		case KEY_JOYSTICK_BUTTON_8:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_8);
			break;
		case KEY_JOYSTICK_BUTTON_9:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_9);
			break;
		case KEY_JOYSTICK_BUTTON_10:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_10);
			break;
		case KEY_JOYSTICK_BUTTON_11:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_11);
			break;
		case KEY_JOYSTICK_BUTTON_12:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_12);
			break;
		case KEY_JOYSTICK_BUTTON_13:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_13);
			break;
		case KEY_JOYSTICK_BUTTON_14:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_14);
			break;
		case KEY_JOYSTICK_BUTTON_15:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_15);
			break;
		case KEY_JOYSTICK_BUTTON_16:
			textPtr = GetTextString(TEXTSTRING_KEYS_JOYSTICKBUTTON_16);
			break;
		case KEY_LBRACKET:	
			textPtr = GetTextString(TEXTSTRING_KEYS_LBRACKET);
			break;
		case KEY_RBRACKET:	
			textPtr = GetTextString(TEXTSTRING_KEYS_RBRACKET);
			break;
		case KEY_SEMICOLON:	
			textPtr = GetTextString(TEXTSTRING_KEYS_SEMICOLON);
			break;
		case KEY_APOSTROPHE:	
			textPtr = GetTextString(TEXTSTRING_KEYS_APOSTROPHE);
			break;
		case KEY_GRAVE:		
			textPtr = GetTextString(TEXTSTRING_KEYS_GRAVE);
			break;
		case KEY_BACKSLASH:	
			textPtr = GetTextString(TEXTSTRING_KEYS_BACKSLASH);
			break;
		case KEY_SLASH:
			textPtr = GetTextString(TEXTSTRING_KEYS_SLASH);
			break;
		case KEY_NUMPADENTER:
			textPtr = GetTextString(TEXTSTRING_KEYS_NUMPADENTER);
			break;
		case KEY_NUMPADDIVIDE:
			textPtr = GetTextString(TEXTSTRING_KEYS_NUMPADDIVIDE);
			break;
		case KEY_NUMPADMULTIPLY:
			textPtr = GetTextString(TEXTSTRING_KEYS_NUMPADMULTIPLY);
			break;
		case KEY_CAPITAL:
			textPtr = GetTextString(TEXTSTRING_KEYS_CAPITAL);
			break;
		case KEY_MINUS:
			textPtr = GetTextString(TEXTSTRING_KEYS_MINUS);
			break;
		case KEY_EQUALS:
			textPtr = GetTextString(TEXTSTRING_KEYS_EQUALS);
			break;
		case KEY_LWIN:
			textPtr = GetTextString(TEXTSTRING_KEYS_LWIN);
			break;
		case KEY_RWIN:
			textPtr = GetTextString(TEXTSTRING_KEYS_RWIN);
			break;
		case KEY_APPS:
			textPtr = GetTextString(TEXTSTRING_KEYS_APPS);
			break;
		case KEY_F1:
			textPtr = GetTextString(TEXTSTRING_KEYS_F1);
			break;
		case KEY_F2:
			textPtr = GetTextString(TEXTSTRING_KEYS_F2);
			break;
		case KEY_F3:								 
			textPtr = GetTextString(TEXTSTRING_KEYS_F3);
			break;
		case KEY_F4:
			textPtr = GetTextString(TEXTSTRING_KEYS_F4);
			break;
		case KEY_F5:
			textPtr = GetTextString(TEXTSTRING_KEYS_F5);
			break;
		case KEY_F6:
			textPtr = GetTextString(TEXTSTRING_KEYS_F6);
			break;
		case KEY_F7:
			textPtr = GetTextString(TEXTSTRING_KEYS_F7);
			break;
		case KEY_F8:
			textPtr = GetTextString(TEXTSTRING_KEYS_F8);
			break;
		case KEY_F9:
			textPtr = GetTextString(TEXTSTRING_KEYS_F9);
			break;
		case KEY_F10:
			textPtr = GetTextString(TEXTSTRING_KEYS_F10);
			break;
		case KEY_F11:
			textPtr = GetTextString(TEXTSTRING_KEYS_F11);
			break;									  
		case KEY_F12:
			textPtr = GetTextString(TEXTSTRING_KEYS_F12);
			break;

		case KEY_A_UMLAUT:
			KeyDescBuffer[0] = 196; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_O_UMLAUT:
			KeyDescBuffer[0] = 214; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_U_UMLAUT:
			KeyDescBuffer[0] = 220; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_BETA:
			KeyDescBuffer[0] = 223; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_PLUS:
			KeyDescBuffer[0] = '+'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_HASH:
			KeyDescBuffer[0] = '#'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_UPSIDEDOWNEXCLAMATION:
			KeyDescBuffer[0] = 161; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_C_CEDILLA:
			KeyDescBuffer[0] = 199; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_N_TILDE:
			KeyDescBuffer[0] = 209; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_RIGHTBRACKET:
			KeyDescBuffer[0] = ')'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_ASTERISK:
			KeyDescBuffer[0] = '*'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_DOLLAR:
			KeyDescBuffer[0] = '$'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_U_GRAVE:
			KeyDescBuffer[0] = 217; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_EXCLAMATION:
			KeyDescBuffer[0] = '!'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_COLON:
			KeyDescBuffer[0] = ':';
			textPtr = KeyDescBuffer;
			break;
		case KEY_DIACRITIC_GRAVE:
			KeyDescBuffer[0] = 96; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_DIACRITIC_ACUTE:
			KeyDescBuffer[0] = 180; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_DIACRITIC_CARET:
			KeyDescBuffer[0] = '^'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_DIACRITIC_UMLAUT:
			KeyDescBuffer[0] = 168; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_ORDINAL:
			KeyDescBuffer[0] = 186; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;
		case KEY_LESSTHAN:
			KeyDescBuffer[0] = '<'; // Windows Latin 1 (ANSI)
			textPtr = KeyDescBuffer;
			break;

		case KEY_VOID:
			KeyDescBuffer[0] = 0;
			textPtr = KeyDescBuffer;
			break;


		default: // alpha-numeric case
		{
			if (key >= KEY_A && key <= KEY_Z)
			{
				KeyDescBuffer[0] = key - KEY_A + 'A';
			}
			else if (key >= KEY_0 && key <= KEY_9)
			{
				KeyDescBuffer[0] = key - KEY_0 + '0';
			}
			else // um, no idea - send blank
			{
				KeyDescBuffer[0] = 0;
			}
			textPtr = KeyDescBuffer;
			break;
		}

	}
	return textPtr;
}

/* Advance by the episode just played, never by the profile's unlock frontier.
 * A replay at an easier difficulty cannot lower the stored completion record. */
static int OkayToPlayNextEpisode(void)
{
 int *episode, basicCount, totalCount;
 enum AVPMENU_ID levelMenu;
 void (*setLevel)(int);
 switch(AvP.PlayerType){
 case I_Marine: episode=&MarineEpisodeToPlay;basicCount=MAX_NO_OF_BASIC_MARINE_EPISODES;
  totalCount=MAX_NO_OF_MARINE_EPISODES;levelMenu=AVPMENU_MARINELEVELS;setLevel=SetLevelToLoadForMarine;break;
 case I_Predator: episode=&PredatorEpisodeToPlay;basicCount=MAX_NO_OF_BASIC_PREDATOR_EPISODES;
  totalCount=MAX_NO_OF_PREDATOR_EPISODES;levelMenu=AVPMENU_PREDATORLEVELS;setLevel=SetLevelToLoadForPredator;break;
 case I_Alien: episode=&AlienEpisodeToPlay;basicCount=MAX_NO_OF_BASIC_ALIEN_EPISODES;
  totalCount=MAX_NO_OF_ALIEN_EPISODES;levelMenu=AVPMENU_ALIENLEVELS;setLevel=SetLevelToLoadForAlien;break;
 default: SetupNewMenu(AVPMENU_MAIN);return 0;
 }
 if(*episode<0||*episode>=totalCount){SetupNewMenu(AVPMENU_MAIN);return 0;}
 if(UserProfilePtr && !DebuggingCommandsActive){
  if(UserProfilePtr->LevelCompleted[AvP.PlayerType][*episode]<AcidCompletion())
   UserProfilePtr->LevelCompleted[AvP.PlayerType][*episode]=AcidCompletion();
  SaveUserProfile(UserProfilePtr);
 }
 /* Bonus maps are standalone; the campaign finale keeps its existing outro. */
 if(RetainPlayerState!=CAMPAIGN_OFF && *episode<basicCount-1){
  ++*episode;RememberEpisode(AvP.PlayerType,*episode);setLevel(*episode);
  /* Automatic continuation bypasses the briefing menu: prepare the next
   * episode text explicitly for loading and the post-load fade. */
  SetBriefingTextForEpisode(*episode, AvP.PlayerType);
  CampaignArmNextLevel();
  LoadGameRequest=SAVELOAD_REQUEST_NONE;
  AvPMenus.MenusState=MENUSSTATE_STARTGAME;
  return 1;
 }
 SetupNewMenu(levelMenu);
 return 0;
}


int NumberOfAvailableLevels(I_PLAYER_TYPE playerID)
{
	int i=0;
	int maximumLevel;
	int numberOfBasicLevels;
	int maxDifficulty=0x7fffffff;

	switch(playerID)
	{
		case I_Marine:
		{
			maximumLevel = MAX_NO_OF_MARINE_EPISODES-1;
			numberOfBasicLevels = MAX_NO_OF_BASIC_MARINE_EPISODES-1;
			break;
		}
		case I_Predator:
		{
			maximumLevel = MAX_NO_OF_PREDATOR_EPISODES-1;
			numberOfBasicLevels = MAX_NO_OF_BASIC_PREDATOR_EPISODES-1;
			break;
		}
		case I_Alien:
		{
			maximumLevel = MAX_NO_OF_ALIEN_EPISODES-1;
			numberOfBasicLevels = MAX_NO_OF_BASIC_ALIEN_EPISODES-1;
			break;
		}
	}

	#if 0 // force all level access
	while (i<maximumLevel && (UserProfilePtr->LevelCompleted[playerID][i]=3))
		i++;
	UserProfilePtr->LevelCompleted[playerID][i]=3;
	#else

	while (i<numberOfBasicLevels && UserProfilePtr->LevelCompleted[playerID][i])
		i++;
		
	MaximumSelectableLevel = i;
	{
		int l;
		for (l=0; l<=numberOfBasicLevels; l++)
		{
			if (UserProfilePtr->LevelCompleted[playerID][l]<maxDifficulty)
			{
				maxDifficulty = UserProfilePtr->LevelCompleted[playerID][l];
			}
		}
		switch(maxDifficulty)
		{
			case AVP_DIFFICULTY_LEVEL_EASY:
			{
				MaximumSelectableLevel = maximumLevel-3;
				i = maximumLevel;
				break;
			} 
			case AVP_DIFFICULTY_LEVEL_MEDIUM:
			{
				MaximumSelectableLevel = maximumLevel-1;
				i = maximumLevel;
				break;
			} 
			case AVP_DIFFICULTY_LEVEL_MOLECULAR_ACID:
			case AVP_DIFFICULTY_LEVEL_IMPOSSIBLE:
			case AVP_DIFFICULTY_LEVEL_HARD:
			{
				MaximumSelectableLevel = maximumLevel;
				i = maximumLevel;
				break;
			} 
			default:
				break;
		}
	}
	//while (i<maximumLevel && (UserProfilePtr->LevelCompleted[playerID][i]!=0))
	//	i++;
	#endif

	return i;
}
int LevelMostLikelyToPlay(I_PLAYER_TYPE playerID)
{
	int i=0;
	int maximumLevel;

	switch(playerID)
	{
		case I_Marine:
		{
			maximumLevel = MAX_NO_OF_MARINE_EPISODES-1;
			break;
		}
		case I_Predator:
		{
			maximumLevel = MAX_NO_OF_PREDATOR_EPISODES-1;
			break;
		}
		case I_Alien:
		{
			maximumLevel = MAX_NO_OF_ALIEN_EPISODES-1;
			break;
		}
	}

	while (i<maximumLevel && (UserProfilePtr->LevelCompleted[playerID][i]!=0))
		i++;

	return i;
}

int MaxDifficultyLevelAllowed(I_PLAYER_TYPE playerID, int level)
{
 int total,basic,available;
 switch(playerID){
 case I_Marine:total=MAX_NO_OF_MARINE_EPISODES;basic=MAX_NO_OF_BASIC_MARINE_EPISODES;break;
 case I_Predator:total=MAX_NO_OF_PREDATOR_EPISODES;basic=MAX_NO_OF_BASIC_PREDATOR_EPISODES;break;
 case I_Alien:total=MAX_NO_OF_ALIEN_EPISODES;basic=MAX_NO_OF_BASIC_ALIEN_EPISODES;break;
 default:return 3;
 }
 if(!UserProfilePtr || level<0 || level>=total)return 3;
 /* Completion records keep the highest difficulty reached per episode.
  * Director's Cut or better unlocks Molecular acid; Realistic unlocks
  * Impossible. Existing completion records satisfy the same rules. */
 if(UserProfilePtr->LevelCompleted[playerID][level]>=AVP_DIFFICULTY_LEVEL_HARD)return 5;
 if(UserProfilePtr->LevelCompleted[playerID][level]>=AVP_DIFFICULTY_LEVEL_MEDIUM)return 4;
 /* Bonus missions inherit advanced difficulties earned in this species.
  * Their own record alone could hide Molecular acid on an unlocked bonus. */
 if(level>=basic){
  int completed; available=3;
  for(completed=0;completed<total;completed++){
   int record=UserProfilePtr->LevelCompleted[playerID][completed];
   if(record>=AVP_DIFFICULTY_LEVEL_HARD)return 5;
   if(record>=AVP_DIFFICULTY_LEVEL_MEDIUM)available=4;
  }
  return available;
 }
 if(level==0)return 3;
 available=UserProfilePtr->LevelCompleted[playerID][level-1];
 if(available<1)available=1;
 if(available>3)available=3;
 return available;
}
void DisplayVideoModeUnavailableScreen(void)
{
	do
	{
		CheckForWindowsMessages();
		DrawMainMenusBackdrop();
		ReadUserInput();
		{
			RECT area;
			//draw the attached string at the bottom of the screen

			area.left=MENU_LEFTXEDGE;
			area.right=MENU_RIGHTXEDGE;
			area.top=0;
			area.bottom=ScreenDescriptorBlock.SDB_Height;

			RenderSmallFontString_Wrapped(GetTextString(TEXTSTRING_NOTENOUGHMEMORY),&area,BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT,0,0);
			
		}
				
		ShowMenuFrameRate();

		FlipBuffers();
		FrameCounterHandler();
		PlayMenuMusic();
	}
	while(!DebouncedGotAnyKey);
}


void CheckForCredits(void)
{
#if 0
	FILE *fp = OpenGameFile("credits.txt", FILEMODE_READONLY, FILETYPE_PERM);
	
	if (!fp)
	{
		char message[100];
		sprintf(message,"Unable to access credits.txt\n");
		MessageBox(NULL,message,TYRARGO_PROJECT_NAME " - Error",MB_OK+MB_SYSTEMMODAL);
		exit(0x111);
		return;
	}
	else
	{
		fclose(fp);
	}
#endif	
}

void DoCredits(void)
{
	int position = 300*2048;
	BOOL FinishedCredits = FALSE;
	
	char *creditsBufferPtr = LoadTextFile("credits.txt");
	char *creditsPtr;
	
	if (!creditsBufferPtr) return;

	if (!strncmp (creditsBufferPtr, "REBCRIF1", 8))
	{
		creditsPtr = (char*)HuffmanDecompress((HuffmanPackage*)(creditsBufferPtr)); 		
		DeallocateMem(creditsBufferPtr);
		creditsBufferPtr=creditsPtr;
	}
	else
	{
		creditsPtr = creditsBufferPtr;
	}


	do
	{
		CheckForWindowsMessages();
		DrawMainMenusBackdrop();
		ReadUserInput();
	
		FinishedCredits = !RollCreditsText(position,creditsPtr+4);
        RenderSmallMenuText("libsmacker (C) 2012-2021 Greg Kennedy / LGPL 2.1+",MENU_CENTREX,410,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
        RenderSmallMenuText("libbinkdec (C) 2011 Barry Duncan; FFmpeg contributors / LGPL 2.1+",MENU_CENTREX,425,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
        RenderSmallMenuText("OpenAL Soft: Chris Robinson / LGPL 2+; see THIRD_PARTY_NOTICES.md",MENU_CENTREX,440,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);
        RenderSmallMenuText("CRT: Inigo Quilez, Mattias Gustavsson, HelelSingh (see notices)",MENU_CENTREX,455,ONE_FIXED,AVPMENUFORMAT_CENTREJUSTIFIED);

		ShowMenuFrameRate();

		FlipBuffers();
		FrameCounterHandler();
		PlayMenuMusic();
	   	position -= RealFrameTime;
	}
	while(!DebouncedGotAnyKey && !FinishedCredits);

	UnloadTextFile("credits.txt",creditsPtr);
}

BOOL RollCreditsText(int position, unsigned char *textPtr)
{
	int y=0;

	while(*textPtr!='#')
	{
		char buffer1[96];
		char buffer2[96];
		int i;
		int centredText,splitText;
		
										
		{
			int yy = 60+y+position/2048;
			
			if (yy >= 60 && yy <= 400)
			{
				int b;
				{
					if (*textPtr=='}')
					{
						textPtr++;
						centredText = 1;
					}
					else
					{
						centredText = 0;
					}

					for (i=0; i<80 && (*textPtr!='\r')&&(*textPtr!='\n')&&(*textPtr!='\0')&&(*textPtr!='|'); i++)
					{
						buffer1[i] = *textPtr++;
		//				if (buffer1[i]>='a' && buffer1[i]<='z') buffer1[i] += 'A' - 'a';
					}
					buffer1[i]=0;

					if (*textPtr=='|')
					{
						textPtr++;
						for (i=0; i<80 && (*textPtr!='\r')&&(*textPtr!='\n')&&(*textPtr!='\0'); i++)
						{
							buffer2[i] = *textPtr++;
						}
						buffer2[i]=0;
						splitText = 1;
					}
					else
					{
						splitText = 0;
					}

					textPtr++;
					while(*textPtr=='\n') textPtr++;

				}
				
				if (yy<92)
				{
					b = (yy-60)*2048;
				}
				else if (yy>400-32)
				{
					b = (400-yy)*2048;
				}
				else 
				{
					b = ONE_FIXED;
				}

				if (splitText)
				{
					RenderSmallMenuText(buffer1,MENU_CENTREX-MENU_ELEMENT_SPACING,yy,b,AVPMENUFORMAT_RIGHTJUSTIFIED);
					RenderSmallMenuText(buffer2,MENU_CENTREX+MENU_ELEMENT_SPACING,yy,b,AVPMENUFORMAT_LEFTJUSTIFIED);
				}
				else
				{
					if (centredText)
					{
						RenderSmallMenuText(buffer1,MENU_CENTREX,yy,b,AVPMENUFORMAT_CENTREJUSTIFIED);
					}
					else
					{
						RenderSmallMenuText(buffer1,MENU_LEFTXEDGE,yy,b,AVPMENUFORMAT_LEFTJUSTIFIED);
					}
				}

			}
			else
			{
				while(*textPtr++!='\n');
			}
		}
		y+=15;
	}

	if(y+(position/2048)<0)
	{
		//finished
		return FALSE;
	}
	return TRUE;
}
/* KJL 17:50:06 24/06/98 - setup video mode for Menus */

extern int VideoModeTypeScreen;
extern int RasterisationRequestMode;
extern int VideoMode;
extern int WindowMode;
extern int WindowRequestMode;

extern int DXMemoryMode;
extern int DXMemoryRequestMode;

extern void SelectMenuDisplayMode(void)
{
    MinimizeAllImages();
	TimeStampedMessage("after MinimizeAllImages");
	MinimizeAllDDGraphics();
	TimeStampedMessage("after MinimizeAllDDGraphics");

    ReleaseDirect3DNotDDOrImages();
	TimeStampedMessage("after ReleaseDirect3DNotDDOrImages");

    finiObjectsExceptDD();
	TimeStampedMessage("after finiObjectsExceptDD");

  	finiObjects();
	SelectDirectDrawObject(NULL);

	DXMemoryRequestMode = RequestSystemMemoryAlways;
	DXMemoryMode = SystemMemoryPreferred;

	RasterisationRequestMode = RequestSoftwareRasterisation;
	VideoMode = VideoMode_DX_640x480x15;
	VideoModeTypeScreen = VideoModeType_15;
	WindowMode = WindowRequestMode;
	ScreenDescriptorBlock.SDB_Width = 640;
	ScreenDescriptorBlock.SDB_Height = 480;
	ScreenDescriptorBlock.SDB_ScreenDepth = VideoModeType_15;

	#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)
	ChangeDirectDrawObject();
	#endif
	TimeStampedMessage("after ChangeDirectDrawObject");

	GenerateDirectDrawSurface();
	TimeStampedMessage("after GenerateDirectDrawSurface");

	{
		extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
		Global_VDB_Ptr = 0;
	}
}






















#if 0
	 


struct STARDESC
{       
	int X;
	int Y;
	int Chasing;
	int Fleeing;
};
#define NUMBER_OF_STARS 128

struct STARDESC Star[NUMBER_OF_STARS];

static void DrawStar(int x, int y)
{
	extern unsigned char *ScreenBuffer;
	extern long BackBufferPitch;

	int xi,xf,xfm,yi,yf,yfm;

	xi=x/65536;
	yi=y/65536;
	xf = x - xi*65536;
	yf = y - yi*65536;
	xfm = ONE_FIXED - xf;
	yfm = ONE_FIXED - yf;

	*(unsigned short*)(ScreenBuffer + ((xi+0)*2 + (yi+0)*BackBufferPitch)) = (unsigned short)WhiteOfBrightness(MUL_FIXED(xfm,yfm));
	*(unsigned short*)(ScreenBuffer + ((xi+0)*2 + (yi+1)*BackBufferPitch)) = (unsigned short)WhiteOfBrightness(MUL_FIXED(xfm,yf));
	*(unsigned short*)(ScreenBuffer + ((xi+1)*2 + (yi+0)*BackBufferPitch)) = (unsigned short)WhiteOfBrightness(MUL_FIXED(xf,yfm));
	*(unsigned short*)(ScreenBuffer + ((xi+1)*2 + (yi+1)*BackBufferPitch)) = (unsigned short)WhiteOfBrightness(MUL_FIXED(xf,yf));

}
#endif

static void InitMainMenusBackdrop(void)
{
	#if 0
	int i;
	for(i=0; i<NUMBER_OF_STARS; i++)
	{
		Star[i].X = (FastRandom()&65535)*640;
		Star[i].Y = (FastRandom()&65535)*480;
		Star[i].Chasing = (FastRandom()&127);
		Star[i].Fleeing = (FastRandom()&127);
	}
	#endif
	#if 0
	{
		int x,y;
		for (x=0; x<128; x++)
		for (y=0; y<128; y++)
		CloudTable[x][y]=(FastRandom()&32767)+32768;
	}
	#endif
	StartMenuBackgroundBink();
}



static int ExpandedMainMenu(void){return 1;}
extern void ConfigureFrontendCanvas(int expanded);
extern int DrawSettingsMenuBackdrop(int biomech);
extern void DrawMainMenusBackdrop(void)
{
 ConfigureFrontendCanvas(ExpandedMainMenu());
 PlayMenuBackgroundBink(); /* Keep the established background ambience. */
 if(AvPMenus.CurrentMenu!=AVPMENU_MAINMENUAVOPTIONS && FMVBackend_DrawMenuBackground())return;
 if(DrawSettingsMenuBackdrop(AvPMenus.CurrentMenu==AVPMENU_MAINMENUAVOPTIONS))return;
 DrawAvPMenuGfx(AVPMENUGFX_BACKDROP,0,0,ONE_FIXED+1,AVPMENUFORMAT_LEFTJUSTIFIED);
}

int WhiteOfBrightness(int brightness)
{
	fprintf(stderr, "WhiteOfBrightness(%d)\n", brightness);
	
	return 0;
#if 0
	extern DDPIXELFORMAT DisplayPixelFormat;
	int a;

	a = MUL_FIXED(DisplayPixelFormat.dwRBitMask,brightness) & DisplayPixelFormat.dwRBitMask;
	a |= MUL_FIXED(DisplayPixelFormat.dwGBitMask,brightness) & DisplayPixelFormat.dwGBitMask;
	a |= MUL_FIXED(DisplayPixelFormat.dwBBitMask,brightness) & DisplayPixelFormat.dwBBitMask;

	return a;
#endif	
}

void RenderPixel(int x,int y,int r,int g,int b)
{
	fprintf(stderr, "RenderPixel(%d, %d, %d, %d, %d)\n", x, y, r, g, b);
#if 0
	extern DDPIXELFORMAT DisplayPixelFormat;
	extern unsigned char *ScreenBuffer;
	extern long BackBufferPitch;

	unsigned short colour;

	colour  = MUL_FIXED(DisplayPixelFormat.dwRBitMask,r<<8) & DisplayPixelFormat.dwRBitMask;
	colour |= MUL_FIXED(DisplayPixelFormat.dwGBitMask,g<<8) & DisplayPixelFormat.dwGBitMask;
	colour |= MUL_FIXED(DisplayPixelFormat.dwBBitMask,b<<8) & DisplayPixelFormat.dwBBitMask;
	
	
	*(unsigned short*) (ScreenBuffer + (x)*2 + (y)*BackBufferPitch)  = colour;
#endif	
}
#if 0
void BezierCurve(void)
{
	int i;
	extern unsigned char *ScreenBuffer;
	extern long BackBufferPitch;
	static b=0;
	for (i=0; i<=255; i++)
	{
		int u = ((i*65536)/255);
		int m = MUL_FIXED(u,u);
		int l = MUL_FIXED(2*u,ONE_FIXED-u);

		int a;
		
		a = MUL_FIXED(255,m)+MUL_FIXED(b,l);
		if (a<0) a=0;
		if (a>255) a=255;

		m = MUL_FIXED(a*256,a*256);
		l = MUL_FIXED(2*a*256,ONE_FIXED-a*256);

		a;
		
		a = MUL_FIXED(255,m)+MUL_FIXED(b,l);
		if (a<0) a=0;
		if (a>255) a=255;

   		*(unsigned short*)(ScreenBuffer + ((i+0)*2 + (400-a)*BackBufferPitch)) = 0xffff;
   		//*(unsigned short*)(ScreenBuffer + ((i+0)*2 + (400-i)*BackBufferPitch)) = 0xff;

   		*(unsigned short*)(ScreenBuffer + ((i+0)*2 + (400-i)*BackBufferPitch)) = 0xffff;
   		*(unsigned short*)(ScreenBuffer + ((i+50)*2 + (400-i)*BackBufferPitch)) = 0xffff;
   		*(unsigned short*)(ScreenBuffer + ((i+100)*2 + (400-i)*BackBufferPitch)) = 0xffff;


	}
//		PlayWithGammaSettings(b);
	b++;
	if (b>=255) b=0;
}			   
#endif



static void UpdateMultiplayerConfigurationMenu()
{
	/*update the displayed levels according to the cuurent game type*/
	
	AVPMENU_ELEMENT *elementPtr = AvPMenus.MenuElements;
	
	//search for the level name element
	do
	{
		GLOBALASSERT(elementPtr->ElementID!=AVPMENU_ELEMENT_ENDOFMENU);
		elementPtr++;

	}while(elementPtr->a.TextDescription!=TEXTSTRING_MULTIPLAYER_ENVIRONMENT);

	
	if(netGameData.skirmishMode)
	{
		//force 'cooperative' game if using skirmish mode
		netGameData.gameType=NGT_Coop;
	}
	
	if(netGameData.gameType!=NGT_Coop)
	{
		elementPtr->b.MaxSliderValue = NumMultiplayerLevels-1;
		elementPtr->d.TextSliderStringPointer = MultiplayerLevelNames;

		//make sure the level number is within bounds
		netGameData.levelNumber%=NumMultiplayerLevels;
	}
	else
	{
		elementPtr->b.MaxSliderValue = NumCoopLevels-1;
		elementPtr->d.TextSliderStringPointer = CoopLevelNames;

		//make sure the level number is within bounds
		netGameData.levelNumber%=NumCoopLevels;
	}


	//see if selected element is the gamestyle element
	elementPtr = &AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement];

	if(elementPtr->a.TextDescription==TEXTSTRING_MULTIPLAYER_GAMESTYLE)
	{
		//change the helpstring according to the game style
		switch(netGameData.gameType)
		{
			case NGT_Individual :
				elementPtr->HelpString=TEXTSTRING_MPHELP_GAMESTYLE_DEATHMATCH;
				break;

			case NGT_CoopDeathmatch :
				elementPtr->HelpString=TEXTSTRING_MPHELP_GAMESTYLE_DEATHMATCHTEAM;
				break;
			
			case NGT_LastManStanding :
				elementPtr->HelpString=TEXTSTRING_MPHELP_GAMESTYLE_LASTMANSTANDING;
				break;
			
			case NGT_PredatorTag :
				elementPtr->HelpString=TEXTSTRING_MPHELP_GAMESTYLE_PREDATORTAG;
				break;
			
			case NGT_Coop :
				elementPtr->HelpString=TEXTSTRING_MPHELP_GAMESTYLE_COOPERATIVE;
				break;

			case NGT_AlienTag :
				elementPtr->HelpString=TEXTSTRING_MPHELP_GAMESTYLE_ALIENTAG;
				break;
		}
	}

	//also need to fill in customLevelName
	strcpy(netGameData.customLevelName,GetCustomMultiplayerLevelName(netGameData.levelNumber,netGameData.gameType));

}

static void TestValidityOfCheatMenu(void)
{
	CheatMode_GetNextAllowedMode(AvPMenus.MenuElements[0].c.SliderValuePtr,TRUE);
	CheatMode_GetNextAllowedSpecies(AvPMenus.MenuElements[1].c.SliderValuePtr,TRUE);
	CheatMode_GetNextAllowedEnvironment(AvPMenus.MenuElements[2].c.SliderValuePtr,TRUE);
}

static unsigned char *BriefingTextString[5];
static unsigned char BlankLine[]="";

void SetBriefingTextForEpisode(int episode, I_PLAYER_TYPE playerID)
{	
	int i,s;

	switch(playerID)
	{
		default:
		case I_Marine:
		{
			s = TEXTSTRING_MARINELEVELSBRIEFING_1;
			break;
		}
		case I_Predator:
		{
			s = TEXTSTRING_PREDATORLEVELSBRIEFING_1;
			break;
		}
		case I_Alien:
		{
			s = TEXTSTRING_ALIENLEVELSBRIEFING_1;
			break;
		}
	}
	
	for(i=0; i<5; i++)
	{
		BriefingTextString[i] = GetTextString(s+i+ episode*5);
	}
}

static char MultiplayerBriefing[3][100];

static void AddMultiplayerBriefingString(const char* text)
{
	int shortest = 0;
	int shortest_length =1000;
	int i;

	for(i=0;i<3;i++)
	{
		int length = strlen(MultiplayerBriefing[i]);
		if(length<shortest_length)
		{
			shortest = i;
			shortest_length = length;
		}
	}

	if(shortest_length + 3 + strlen(text)>=100) return;

	if(shortest_length>0)
	{
		strcat(MultiplayerBriefing[shortest]," , ");
	}
	strcat(MultiplayerBriefing[shortest],text);
}

static BOOL IsFlamerInLevel(int level)
{
	if(level == AVP_ENVIRONMENT_SUBWAY_MP ||
	   level == AVP_ENVIRONMENT_SUBWAY_COOP)
	{
		return FALSE;
	}
	return TRUE;
}

static BOOL IsMinigunInLevel(int level)
{
	if(level == AVP_ENVIRONMENT_SEWER)
	{
		return FALSE;
	}
	return TRUE;
}

static BOOL IsSadarInLevel(int level)
{
	if(level == AVP_ENVIRONMENT_MASSACRE ||
	   level == AVP_ENVIRONMENT_MEATFACTORY_MP ||
	   level == AVP_ENVIRONMENT_MEATFACTORY_COOP ||
	   level == AVP_ENVIRONMENT_HADLEYSHOPE_MP ||
	   level == AVP_ENVIRONMENT_HADLEYSHOPE_COOP ||
	   level == AVP_ENVIRONMENT_HIVE ||
	   level == AVP_ENVIRONMENT_HIVE_COOP ||
	   level == AVP_ENVIRONMENT_KENS_COOP ||
	   level == AVP_ENVIRONMENT_LEADWORKS_MP ||
	   level == AVP_ENVIRONMENT_LEADWORKS_COOP)
	{
		return FALSE;
	}
	return TRUE;
}

static BOOL IsSkeeterInLevel(int level)
{
	if(level == AVP_ENVIRONMENT_LEADWORKS_MP ||
	   level == AVP_ENVIRONMENT_LEADWORKS_COOP)
	{
		return FALSE;
	}
	return TRUE;
}

static BOOL IsSmartgunInLevel(int level)
{
	if(level == AVP_ENVIRONMENT_NOSTROMO_MP) return FALSE;
	return TRUE;
}


static void SetBriefingTextForMultiplayer()
{
	int level = NumberForCurrentLevel();

	int num_not_available = 0;

	if(netGameData.customLevelName[0] != 0)
	{
		//custom level
		BriefingTextString[0] = netGameData.customLevelName;
		return;
	}
	
	MultiplayerBriefing[0][0] = 0;
	MultiplayerBriefing[1][0] = 0;
	MultiplayerBriefing[2][0] = 0;

	if(!netGameData.allowSmartgun || !IsSmartgunInLevel(level))
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_SMARTGUN));
		num_not_available++;
	}
	if(!netGameData.allowFlamer || !IsFlamerInLevel(level))
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_FLAMETHROWER));
		num_not_available++;
	}
	if(!netGameData.allowSadar || !IsSadarInLevel(level))
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_SADAR));
		num_not_available++;
	}
	if(!netGameData.allowGrenadeLauncher)
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_GRENADELAUNCHER));
		num_not_available++;
	}
	if(!netGameData.allowMinigun || !IsMinigunInLevel(level))
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_MINIGUN));
		num_not_available++;
	}
	if(!netGameData.allowSmartDisc || !IsSkeeterInLevel(level))
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_SKEETER));
		num_not_available++;
	}
	if(!netGameData.allowPistols)
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_MARINE_PISTOL));
		num_not_available++;
	}
	if(!netGameData.allowDisc)
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_DISC));
		num_not_available++;
	}
	if(!netGameData.allowPistol)
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_BRIEFING_PREDATOR_PISTOL));
		num_not_available++;
	}
	if(!netGameData.allowPlasmaCaster)
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_SHOULDERCANNON));
		num_not_available++;
	}
	if(!netGameData.allowSpeargun)
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_RIFLE));
		num_not_available++;
	}
	if(!netGameData.allowMedicomp)
	{
		AddMultiplayerBriefingString(GetTextString(TEXTSTRING_INGAME_MEDICOMP));
		num_not_available++;
	}

	if(netGameData.gameType!=NGT_Coop)
	{
		BriefingTextString[0] = GetTextString(netGameData.levelNumber + TEXTSTRING_MULTIPLAYERLEVELS_1);
		
	}
	else
	{
		BriefingTextString[0] = GetTextString(netGameData.levelNumber + TEXTSTRING_COOPLEVEL_1);
	}
	
	if(num_not_available)
	{
		BriefingTextString[1] = GetTextString(TEXTSTRING_BRIEFING_UNAVAILABLE_WEAPONS);
		BriefingTextString[2] = MultiplayerBriefing[0];
		BriefingTextString[3] = MultiplayerBriefing[1];
		BriefingTextString[4] = MultiplayerBriefing[2];
	}
}

void SetBriefingTextToBlank(void)
{	
	int i;

	for(i=0; i<5; i++)
	{
		BriefingTextString[i] = BlankLine;
	}
}

void RenderBriefingText(int centreY, int brightness)
{
	int lengthOfLongestLine=-1;
	int x,y,i;
	const int hardwareBriefing = (AvPMenus.MenusState != MENUSSTATE_MAINMENUS);
	const int TextScaleFixed = hardwareBriefing ? D3D_GetConsoleTextScaleFixed() : ONE_FIXED;

	for(i=0; i<5; i++)
	{
		int length = 0;
		{
			char *ptr = BriefingTextString[i];

			length = MUL_FIXED(AvPTextStringWidth(ptr, AAFontWidths), TextScaleFixed);
		}
		
		if (lengthOfLongestLine < length)
		{
			lengthOfLongestLine = length;
		}
	}

	x = (ScreenDescriptorBlock.SDB_Width-lengthOfLongestLine)/2;
	y = centreY - MUL_FIXED(3*HUD_FONT_HEIGHT, TextScaleFixed);
	if (hardwareBriefing) D3D_SetBriefingTextRenderActive(1);
	for(i=0; i<5; i++)
	{
		if (hardwareBriefing)
		{
			Hardware_RenderSmallMenuText(BriefingTextString[i], x, y, brightness, AVPMENUFORMAT_LEFTJUSTIFIED/*,MENU_CENTREY-60-100,MENU_CENTREY-60+180*/);
		}
		else
		{
			RenderSmallMenuText(BriefingTextString[i], x, y, brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
		}
		if (i) y += MUL_FIXED(HUD_FONT_HEIGHT, TextScaleFixed);
		else y += MUL_FIXED(HUD_FONT_HEIGHT*2, TextScaleFixed);
	}
	if (hardwareBriefing) D3D_SetBriefingTextRenderActive(0);
}


void CheckForKeysWithMultipleAssignments(void)
{
	int column,row;
	int rowCount=KeyConfigActionRowCount()+KeyConfigWeaponRowCount()+KeyConfigGlobalCommandRowCount();

	for (column=0; column<=1; column++)
	{
		for (row=0; row<MAX_KEYCONFIG_BINDINGS; row++) MultipleAssignments[column][row]=0;
	}

	for (column=0; column<=1; column++)
	{
		for (row=0; row<rowCount; row++)
		{
			int innerColumn,innerRow;
			int menuElementIndex=KeyConfigMenuElementIndexForBinding(row);
			unsigned char key=*KeyConfigBindingPointer(column==0,menuElementIndex);
			if (key==KEY_VOID) continue;
			for (innerColumn=0; innerColumn<=1; innerColumn++)
			for (innerRow=0; innerRow<rowCount; innerRow++)
			{
				int innerMenuElementIndex=KeyConfigMenuElementIndexForBinding(innerRow);
				if (innerColumn==column && innerRow==row) continue;
				if (key==*KeyConfigBindingPointer(innerColumn==0,innerMenuElementIndex))
				{
                    if (AvPMenus.CurrentMenu==AVPMENU_PREDATORKEYCONFIG && SmartZoomBindingRowPublished)
                    {
                        int action=AvPMenus.MenuElements[menuElementIndex].a.TextDescription-TEXTSTRING_PREDATOR_KEY_FORWARD;
                        int other=AvPMenus.MenuElements[innerMenuElementIndex].a.TextDescription-TEXTSTRING_PREDATOR_KEY_FORWARD;
                        if (((action==21 || action==22) && (other==16 || other==17 || other==19 || other==20)) ||
                            ((other==21 || other==22) && (action==16 || action==17 || action==19 || action==20))) continue;
                    }
					MultipleAssignments[column][row]=1;
				}
			}
		}
	}
}
void HandleCheatModeFeatures(void)
{
	switch(CheatMode_Active)
	{
		case CHEATMODE_WARPSPEED:
		{
			TimeScale = 2*ONE_FIXED;
			AcidSetTier(0);AvP.Difficulty = 0;
			break;
		}
		case CHEATMODE_LANDOFTHEGIANTS:
		{
			TimeScale =ONE_FIXED/2;
			break;
		}
		case CHEATMODE_IMPOSSIBLEMISSION:
		{
			AcidSetTier(0);AvP.Difficulty = 3;
			break;
		}
		case CHEATMODE_UNDERWATER:
		{
			TimeScale = (ONE_FIXED*7)/10;
			break;
		}
		default:
			break;
	}
}

void ShowMenuFrameRate(void)
{
#if 0
	extern NormalFrameTime;
	char buffer[8];
	
	sprintf(buffer,"%d fps",65536/NormalFrameTime);
	RenderSmallMenuText(buffer,20,20,ONE_FIXED,	AVPMENUFORMAT_LEFTJUSTIFIED);
#endif
}

#define MAX_ITEMS_IN_KEYBOARDENTRYQUEUE 8
static char KeyboardEntryQueue[MAX_ITEMS_IN_KEYBOARDENTRYQUEUE];
static int NumberOfItemsInKeyboardEntryQueue;
static int KeyboardEntryQueue_ProcessingIndex;

void KeyboardEntryQueue_Add(char c)
{
	SDL_StartTextInput(NULL);
	if (c<32) return;

	if (NumberOfItemsInKeyboardEntryQueue<MAX_ITEMS_IN_KEYBOARDENTRYQUEUE)
	{
		KeyboardEntryQueue[NumberOfItemsInKeyboardEntryQueue++] = c;
	}
}

static void KeyboardEntryQueue_Clear(void)
{
	int i;
	SDL_StartTextInput(NULL);
	for (i=0; i<MAX_ITEMS_IN_KEYBOARDENTRYQUEUE; i++)
	{
		KeyboardEntryQueue[i] = 0;
	}
	NumberOfItemsInKeyboardEntryQueue = 0;
}
  
static void KeyboardEntryQueue_StartProcessing(void)
{
	KeyboardEntryQueue_ProcessingIndex = 0;
}

static char KeyboardEntryQueue_ProcessCharacter(void)
{
	SDL_StartTextInput(NULL);
	if (KeyboardEntryQueue_ProcessingIndex >= NumberOfItemsInKeyboardEntryQueue)
	{
		return 0;
	}
	return KeyboardEntryQueue[KeyboardEntryQueue_ProcessingIndex++];
}



/*------------------------------------**
** Loading and saving main level info **
**------------------------------------*/


extern int AlienEpisodeToPlay;
extern int MarineEpisodeToPlay;
extern int PredatorEpisodeToPlay;


void SaveLevelHeader()
{
	LEVEL_SAVE_BLOCK* block = (LEVEL_SAVE_BLOCK*) GetPointerForSaveBlock(sizeof(LEVEL_SAVE_BLOCK));

	//fill in the header
	block->header.type = SaveBlock_MainHeader;
	block->header.size = sizeof(*block);

	//fill in the main block
	strncpy(block->AvP_Save_String,AcidActive()?"AVPSAVE1":"AVPSAVE0",8);
    block->AcidTier=AcidActive()?MolecularAcidTier:0;block->Reserved=0;

	block->Species = AvP.PlayerType;

	switch(block->Species)
	{
		case I_Marine :
			block->Episode = MarineEpisodeToPlay;
			break;

		case I_Alien :
			block->Episode = AlienEpisodeToPlay;
			break;

		case I_Predator :
			block->Episode = PredatorEpisodeToPlay;
			break;
	}

	block->ElapsedTime_Hours =(unsigned char) AvP.ElapsedHours;
	block->ElapsedTime_Minutes =(unsigned char) AvP.ElapsedMinutes;
	block->ElapsedTime_Seconds = AvP.ElapsedSeconds;

	block->Difficulty = AvP.Difficulty;
	block->NumberOfSavesLeft = (unsigned char) NumberOfSavesLeft;
}

void LoadLevelHeader(SAVE_BLOCK_HEADER* header)
{
	LEVEL_SAVE_BLOCK* block =(LEVEL_SAVE_BLOCK*) header;

	if(block->header.size!=sizeof(*block)) return;

	AvP.ElapsedHours = block->ElapsedTime_Hours;
	AvP.ElapsedMinutes = block->ElapsedTime_Minutes;
	AvP.ElapsedSeconds = block->ElapsedTime_Seconds;
}

static void GetHeaderInfoForSaveSlot(SAVE_SLOT_HEADER* save_slot, const GameDirectoryFile *gdf)
{
	LEVEL_SAVE_BLOCK block;
	unsigned int file_size;
	unsigned char filename[100];
	FILE *file;

	save_slot->SlotUsed = 0;

	sprintf(filename, "%s%s", USER_PROFILES_PATH, gdf->filename);
	file = OpenGameFile(filename, FILEMODE_READONLY, FILETYPE_CONFIG);

	if (file==NULL)
	{
		//failed to load (probably doesn't exist)
		return;
	}
	
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);
	
	if(file_size < sizeof(LEVEL_SAVE_BLOCK))
	{
		//obviously not much of a save file then...
		fclose(file);
		return;

	}

	save_slot->TimeStamp = gdf->timestamp;
   	
	//load the level header
	fread(&block, sizeof(block), 1, file);
	fclose(file);

	//a few checks
	if(block.header.type != SaveBlock_MainHeader ||
	   block.header.size != sizeof(block) ||
	   (strncmp(block.AvP_Save_String,"AVPSAVE0",8) && strncmp(block.AvP_Save_String,"AVPSAVE1",8)))
	{
		//no good.
		return;
	}	

	if(block.Difficulty>I_Impossible)return;
    if(!strncmp(block.AvP_Save_String,"AVPSAVE1",8) && (block.AcidTier<1||block.AcidTier>3||block.Difficulty!=I_Hard))return;
    //appears to be a reasonable file
	save_slot->SlotUsed = TRUE;

	//copy stuff from the block

	save_slot->Species = block.Species;
	save_slot->Episode = block.Episode;
	save_slot->ElapsedTime_Hours = block.ElapsedTime_Hours;
	save_slot->ElapsedTime_Minutes = block.ElapsedTime_Minutes;
	save_slot->ElapsedTime_Seconds = (block.ElapsedTime_Seconds >> 16);
	save_slot->Difficulty = block.Difficulty;
    save_slot->AcidTier=!strncmp(block.AvP_Save_String,"AVPSAVE1",8)?block.AcidTier:0;
	save_slot->SavesLeft = block.NumberOfSavesLeft;
}

void ScanSaveSlots(void)
{
	unsigned char pattern[100];
	void *gd;
	GameDirectoryFile *gdf;

	memset(SaveGameSlot, 0, sizeof(SaveGameSlot));
	sprintf(pattern, "%s_*.sav", UserProfilePtr->FullName);
	gd = OpenGameDirectory(USER_PROFILES_PATH, pattern, FILETYPE_CONFIG);
	if (gd == NULL)
		return;

	while ((gdf = ScanGameDirectory(gd)) != NULL)
	{
		const char *name;
		const char *slotText;
		int slotNumber = 0;

		if ((gdf->attr & FILEATTR_DIRECTORY) != 0 || (gdf->attr & FILEATTR_READABLE) == 0)
			continue;
		name = gdf->filename;
		if (strncmp(name, UserProfilePtr->FullName, strlen(UserProfilePtr->FullName)) != 0)
			continue;
		slotText = name + strlen(UserProfilePtr->FullName);
		if (*slotText++ != '_' || *slotText < '1' || *slotText > '9')
			continue;
		while (*slotText >= '0' && *slotText <= '9')
		{
			slotNumber = slotNumber * 10 + (*slotText - '0');
			slotText++;
		}
		if (strcmp(slotText, ".sav") != 0)
			continue;
		if (slotNumber >= 1 && slotNumber <= NUMBER_OF_SAVE_SLOTS)
			GetHeaderInfoForSaveSlot(&SaveGameSlot[slotNumber - 1], gdf);
	}
}

extern void GetFilenameForSaveSlot(int i, unsigned char *filenamePtr)
{
	sprintf(filenamePtr,"%s%s_%d.sav",USER_PROFILES_PATH,UserProfilePtr->FullName,i+1);
}

static void CheckForLoadGame()
{
	if(LoadGameRequest >=0 && LoadGameRequest<NUMBER_OF_SAVE_SLOTS)
	{
		SAVE_SLOT_HEADER* save_slot = &SaveGameSlot[LoadGameRequest];	
		if(save_slot->SlotUsed)
		{
			AvP.PlayerType = save_slot->Species;
			AvP.Difficulty = save_slot->Difficulty;
            AcidSetTier(save_slot->AcidTier);

			switch(AvP.PlayerType)
			{
				case I_Marine :
					MarineEpisodeToPlay = save_slot->Episode;
					RememberEpisode(I_Marine,MarineEpisodeToPlay);
                SetLevelToLoadForMarine(MarineEpisodeToPlay);
					break;

				case I_Alien :
					AlienEpisodeToPlay = save_slot->Episode;
					RememberEpisode(I_Alien,AlienEpisodeToPlay);
                SetLevelToLoadForAlien(AlienEpisodeToPlay);
				
					break;
	
				case I_Predator :
					PredatorEpisodeToPlay = save_slot->Episode;
					RememberEpisode(I_Predator,PredatorEpisodeToPlay);
                SetLevelToLoadForPredator(PredatorEpisodeToPlay);
					break;
		
			}
			SetBriefingTextForEpisode(save_slot->Episode, AvP.PlayerType);
			AvPMenus.MenusState = MENUSSTATE_STARTGAME;
		}
		else
		{
			//cancel request
			LoadGameRequest = SAVELOAD_REQUEST_NONE;
		}
	}
}


static void PasteFromClipboard(char* Text,int MaxTextLength)
{
	fprintf(stderr, "PasteFromClipboard(%p, %d)\n", Text, MaxTextLength);	
#if 0
	HANDLE hGlobal;
	if(!Text)
	{
		return;
	}

	if(IsClipboardFormatAvailable(CF_TEXT))
	{
		OpenClipboard(0);
		hGlobal = GetClipboardData(CF_TEXT);
		if(hGlobal)
		{
			char* pGlobal = GlobalLock(hGlobal);
			if(pGlobal)
			{
				strncpy(Text,pGlobal,MaxTextLength-1);
				Text[MaxTextLength-1] = 0;
				GlobalUnlock(hGlobal);
			}
		}
		CloseClipboard();
	}
#endif	
}



