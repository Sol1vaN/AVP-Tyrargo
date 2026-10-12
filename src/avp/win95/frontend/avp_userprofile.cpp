#include "event_latency.h"
#include "camera_motion.h"
/* KJL 15:17:31 10/12/98 - user profile stuff */
#include "list_tem.hpp"
extern "C"
{
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "stratdef.h"

#include "avp_userprofile.h"
#include "player_fov.h"
#include "d3d_hud.h"
#include "language.h"
#include "gammacontrol.h"
#include "psnd.h"
#include "cd_player.h"
#include "fmv_backend.h"
#include "fmv.h"
#include "screenshot_image.h"

#define UseLocalAssert Yes
#include "ourasert.h"

 // Edmond
#include "pldnet.h"
#include <time.h>
#include <stddef.h>

static int LoadUserProfiles(void);

static void EmptyUserProfilesList(void);
static void InsertProfileIntoList(AVP_USER_PROFILE *profilePtr);
static int ProfileIsMoreRecent(AVP_USER_PROFILE *profilePtr, AVP_USER_PROFILE *profileToTestAgainstPtr);
static void MigratePredatorWeaponBindings(AVP_USER_PROFILE *profilePtr);

static void SetDefaultProfileOptions(AVP_USER_PROFILE *profilePtr);
static int ProfileUsesWeaponBindingsV17(const AVP_USER_PROFILE *profilePtr);

extern int SmackerSoundVolume;
extern int EffectsSoundVolume;
extern int MoviesAreActive;
extern int IntroOutroMoviesAreActive;
extern char MP_PlayerName[];
extern int AutoWeaponChangeOn;
extern int CrouchIsToggleKey;
extern int UnlimitedSaveGames;
extern int SmartJetpackEnabled;
extern int SmartZoomEnabled;
extern int FullVerticalPitchEnabled;
extern int FMVScaleMode;
extern int FPSDisplayPosition;
extern int MonitorNoSignalEffectMode;
extern int ConsoleTextScaleMode;
extern int MarineCrosshairThicknessMode;
extern int NotificationMessageDurationMode;
extern int MusicPlayMode;
extern int CRTFilterEnabled;
extern int HUDAnchorMode;
extern int HUDAnchorPosition;
extern int HUDAnchorUnlockedPosition;
extern int HUDUserScaleTenths;
extern int HUDTextureFilterMode;
#define HUD_ANCHOR_PROFILE_MARKER 0x5A
#define PLAYER_FOV_PROFILE_MARKER 0x5B
#define PROFILE_SPARE_CROSSHAIR_SCALE_MARKER 0x20
#define PROFILE_SPARE_CROSSHAIR_SCALE_MASK 0x1E
#define PROFILE_SPARE_NOISE_SCALE_HALF 0x02
#define PROFILE_SPARE_HUD_TEXTURE_NEAREST 0x40
#define PROFILE_SPARE_HUD_TEXTURE_FILTER_MARKER 0x80
#define PROFILE_MONITOR_FPS_MARKER 0x4E465000
#define PROFILE_MONITOR_FPS_MASK   0xFFFFFF00
#define PROFILE_MONITOR_FPS_DURATION_MARKER 0x4E464000
#define PROFILE_MONITOR_FPS_DURATION_MASK   0xFFFFC000
/* Compatibility signature written by Probe 105 before its mask alignment was fixed. */
#define PROFILE_MONITOR_FPS_DURATION_V105_MARKER 0x4E465000
#define PROFILE_MONITOR_NO_SIGNAL_EFFECT_MARKER 0x4E534730
/* Bits 10-11 store the base scale, bit 12 selects x3.0, and bit 13 marks
   the five-value layout. Notification duration occupies only bits 8-9. */
#define PROFILE_TEXT_SCALE_MASK       0x00000C00
#define PROFILE_TEXT_SCALE_SHIFT      10
#define PROFILE_TEXT_SCALE_X30_FLAG   0x00001000
#define PROFILE_TEXT_SCALE_V2_MARKER  0x00002000
/* Nine scales, four bits; low ten bits keep monitor/FPS/duration unchanged. */
#define PROFILE_TEXT_SCALE_V3_MARKER  0x4E470000
/* Bit 6 of unused[1]'s high byte is free beside crosshair/music metadata. */
#define PROFILE_CRT_FILTER_FLAG       0x40
/* High byte of unused[4]; low 24 bits continue holding the three HUD values. */
#define PROFILE_VERTICAL_PITCH_MARKER 0x56000000
#define PROFILE_VERTICAL_PITCH_MASK   0xFE000000
#define PROFILE_VERTICAL_PITCH_FULL   0x01000000
/* SpareBits has three bits. Its third bit marks the Probe 070 FMV profile
   packing. The alien reload migration byte stores the five FMV modes plus
   mouse zoom without changing the on-disk profile layout. */
#define PROFILE_SPARE_FMV_SCALE_V2_MARKER 0x04
#define FMV_SCALE_PROFILE_V2_MARKER_BASE 0x20
#define FMV_SCALE_PROFILE_V2_STRIDE 21
#define FMV_SCALE_PROFILE_V2_MOUSE_MAX 20
#define FMV_SCALE_PROFILE_V2_MARKER_MAX 0x88
/* Probe 069's three-mode packing remains readable. */
#define FMV_SCALE_PROFILE_LEGACY_MARKER_BASE 0x40
#define FMV_SCALE_PROFILE_LEGACY_MODE_SHIFT 5
#define FMV_SCALE_PROFILE_LEGACY_MODE_BASE 2
#define FMV_SCALE_PROFILE_LEGACY_MOUSE_MASK 0x1F
#define FMV_SCALE_PROFILE_LEGACY_MARKER_MAX 0x94
#define PROFILE_META_FMV_MODE_MARKER_BASE 0xC0 /* Probe 071: ten whole-number modes. */
#define PROFILE_META_FMV_MODE_MARKER_MAX 0xC9
#define PROFILE_META_FMV_MODE_P072_MARKER_BASE 0xD0 /* Probe 072: sixteen fractional modes. */
#define PROFILE_META_FMV_MODE_P072_MARKER_MAX 0xDF
#define PROFILE_META_FMV_MODE_MARKER_SLOT 10
#define PROFILE_PER_SPECIES_UI_SCALERS_MARKER 0x4E415637
/* unused[1] packs three low-byte FOV values; its high byte was unoccupied. */
#define PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_BASE 0xA4
#define PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_MAX 0xA6
#define PROFILE_MUSIC_PLAY_MODE_MASK 0x18
#define PROFILE_MUSIC_PLAY_MODE_SHIFT 3
#define GLOBAL_COMMAND_BINDINGS_PROFILE_PRIMARY_BASE MAX_NUMBER_OF_CHEATMODES
#define GLOBAL_COMMAND_BINDINGS_PROFILE_SECONDARY_BASE (MAX_NUMBER_OF_CHEATMODES+NUMBER_OF_GLOBAL_COMMAND_BINDINGS)
#define GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER_INDEX (MAX_NUMBER_OF_CHEATMODES+(2*NUMBER_OF_GLOBAL_COMMAND_BINDINGS))
#define GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER 0xC6

static void RestoreGlobalCommandBindingsFromProfile(AVP_USER_PROFILE *profilePtr)
{
	if (profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER_INDEX] == GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER)
	{
		memcpy(&GlobalCommandPrimaryConfig,&profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_PRIMARY_BASE],sizeof(GlobalCommandPrimaryConfig));
		memcpy(&GlobalCommandSecondaryConfig,&profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_SECONDARY_BASE],sizeof(GlobalCommandSecondaryConfig));
	}
	else
	{
		GlobalCommandPrimaryConfig=DefaultGlobalCommandPrimaryConfig;
		GlobalCommandSecondaryConfig=DefaultGlobalCommandSecondaryConfig;
		memcpy(&profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_PRIMARY_BASE],&GlobalCommandPrimaryConfig,sizeof(GlobalCommandPrimaryConfig));
		memcpy(&profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_SECONDARY_BASE],&GlobalCommandSecondaryConfig,sizeof(GlobalCommandSecondaryConfig));
		profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER_INDEX]=GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER;
	}
}

static void StoreGlobalCommandBindingsInProfile(AVP_USER_PROFILE *profilePtr)
{
	memcpy(&profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_PRIMARY_BASE],&GlobalCommandPrimaryConfig,sizeof(GlobalCommandPrimaryConfig));
	memcpy(&profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_SECONDARY_BASE],&GlobalCommandSecondaryConfig,sizeof(GlobalCommandSecondaryConfig));
	profilePtr->CheatMode[GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER_INDEX]=GLOBAL_COMMAND_BINDINGS_PROFILE_MARKER;
}

List<AVP_USER_PROFILE *> UserProfilesList;
static AVP_USER_PROFILE DefaultUserProfile = 
{
	"",
};
static AVP_USER_PROFILE *CurrentUserProfilePtr;

static int ConvertProbe071FMVScaleMode(int oldMode)
{
	/* Preserve exact intent of Probe 071 profiles after fractional slots were
	   added ahead of their original whole-number positions. */
	static const int conversion[10] = {
		FMV_SCALE_PANORAMIC_X1, FMV_SCALE_PANORAMIC_X2,
		FMV_SCALE_PANORAMIC_X3, FMV_SCALE_PANORAMIC_X4,
		FMV_SCALE_ULTRAWIDE_X1, FMV_SCALE_ULTRAWIDE_X2,
		FMV_SCALE_ULTRAWIDE_X3, FMV_SCALE_ULTRAWIDE_X4,
		FMV_SCALE_FULLSCREEN_WIDESCREEN, FMV_SCALE_FULLSCREEN_ULTRAWIDE
	};
	if (oldMode < 0 || oldMode >= 10) return FMV_SCALE_FULLSCREEN_WIDESCREEN;
	return conversion[oldMode];
}
static int ReadFMVScaleModeFromProfile(const AVP_USER_PROFILE *profilePtr, int *mouseZoomTenthsPtr)
{
	unsigned char marker = profilePtr->AlienInputPrimaryConfig.ReloadBindingProfileMarker;
	int mouseZoom = DEFAULT_MOUSE_ZOOM_SENSITIVITY_TENTHS;
	int mode = FMV_SCALE_FULLSCREEN_WIDESCREEN;
	int modeFromMetadata = -1;

	if (ProfileUsesWeaponBindingsV17(profilePtr))
	{
		const unsigned char *weaponData = (const unsigned char *)profilePtr->Padding;
		weaponData += sizeof(MarineWeaponPrimaryConfig);
		weaponData += sizeof(MarineWeaponSecondaryConfig);
		unsigned char modeMarker = weaponData[PROFILE_META_FMV_MODE_MARKER_SLOT];
		if (modeMarker >= PROFILE_META_FMV_MODE_P072_MARKER_BASE
		 && modeMarker <= PROFILE_META_FMV_MODE_P072_MARKER_MAX)
			modeFromMetadata = (int)(modeMarker - PROFILE_META_FMV_MODE_P072_MARKER_BASE);
		else if (modeMarker >= PROFILE_META_FMV_MODE_MARKER_BASE
		 && modeMarker <= PROFILE_META_FMV_MODE_MARKER_MAX)
			modeFromMetadata = ConvertProbe071FMVScaleMode((int)(modeMarker - PROFILE_META_FMV_MODE_MARKER_BASE));
	}

	if ((profilePtr->SpareBits & PROFILE_SPARE_FMV_SCALE_V2_MARKER)
	 && marker >= FMV_SCALE_PROFILE_V2_MARKER_BASE
	 && marker <= FMV_SCALE_PROFILE_V2_MARKER_MAX)
	{
		int packed = (int)marker - FMV_SCALE_PROFILE_V2_MARKER_BASE;
		int savedMode = packed / FMV_SCALE_PROFILE_V2_STRIDE;
		int savedMouseZoom = packed % FMV_SCALE_PROFILE_V2_STRIDE;
		if (savedMode >= 0 && savedMode <= 4)
		{
			mouseZoom = savedMouseZoom;
			if (modeFromMetadata < 0)
			{
				/* Probe 070: old x1/x2/x3 retain their whole-number meanings. */
				mode = (savedMode == 0) ? FMV_SCALE_PANORAMIC_X1 :
					(savedMode == 1) ? FMV_SCALE_PANORAMIC_X2 :
					(savedMode == 2) ? FMV_SCALE_PANORAMIC_X3 :
					(savedMode == 3) ? FMV_SCALE_FULLSCREEN_WIDESCREEN : FMV_SCALE_FULLSCREEN_ULTRAWIDE;
			}
		}
	}
	else if (marker >= 0xB0 && marker <= 0xC4)
	{
		/* Probe 001..068 and Probe 071 use this zoom-only packing. */
		mouseZoom = (int)(marker - 0xB0);
	}
	else
	{
		/* Probe 069's pre-070 three-mode packing remains readable. */
		int savedMode = ((int)(marker >> FMV_SCALE_PROFILE_LEGACY_MODE_SHIFT)
			- FMV_SCALE_PROFILE_LEGACY_MODE_BASE);
		int savedMouseZoom = (int)(marker & FMV_SCALE_PROFILE_LEGACY_MOUSE_MASK);
		if (marker >= FMV_SCALE_PROFILE_LEGACY_MARKER_BASE
		 && marker <= FMV_SCALE_PROFILE_LEGACY_MARKER_MAX
		 && savedMode >= 0 && savedMode <= 2
		 && savedMouseZoom <= FMV_SCALE_PROFILE_V2_MOUSE_MAX)
		{
			mouseZoom = savedMouseZoom;
			if (modeFromMetadata < 0)
				mode = (savedMode == 0) ? FMV_SCALE_PANORAMIC_X1 : FMV_SCALE_FULLSCREEN_WIDESCREEN;
		}
	}
	if (modeFromMetadata >= FMV_SCALE_PANORAMIC_X1
	 && modeFromMetadata <= FMV_SCALE_FULLSCREEN_ULTRAWIDE)
		mode = modeFromMetadata;
	if (mouseZoomTenthsPtr) *mouseZoomTenthsPtr = mouseZoom;
	return mode;
}
extern void LoadSettingsForStartup(void)
{
	/* The intro runs before profile selection. Read only the newest profile and
	   leave the list empty, restoring the FMV scaler and CRT filter settings. */
	FMVScaleMode = FMV_SCALE_ULTRAWIDE_X25;
	CRTFilterEnabled = 0;
	EmptyUserProfilesList();
	LoadUserProfiles();
	if (UserProfilesList.size())
	{
		const AVP_USER_PROFILE *profilePtr = UserProfilesList.first_entry();
		unsigned int packedMarker = ((unsigned int)profilePtr->unused[1] >> 24) & 0xffu;
		unsigned int thicknessMarker = packedMarker & ~(PROFILE_MUSIC_PLAY_MODE_MASK | PROFILE_CRT_FILTER_FLAG);
		FMVScaleMode = ReadFMVScaleModeFromProfile(profilePtr,0);
		if (profilePtr->unused[0] == PROFILE_PER_SPECIES_UI_SCALERS_MARKER
		 && thicknessMarker >= PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_BASE
		 && thicknessMarker <= PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_MAX)
			CRTFilterEnabled = (packedMarker & PROFILE_CRT_FILTER_FLAG) ? 1 : 0;
	}
	EmptyUserProfilesList();
}

extern void ExamineSavedUserProfiles(void)
{
	// delete any existing profiles
	EmptyUserProfilesList();
	
	// load any available user profiles
	LoadUserProfiles();
	
	// make a fake entry to allow creating new user profiles
	AVP_USER_PROFILE *profilePtr = new AVP_USER_PROFILE;
	*profilePtr = DefaultUserProfile;

	profilePtr->FileTime = time(NULL);

	strncpy(profilePtr->FullName,GetTextString(TEXTSTRING_USERPROFILE_NEW),MAX_SIZE_OF_USERS_NAME);
	profilePtr->FullName[MAX_SIZE_OF_USERS_NAME]=0;
	SetDefaultProfileOptions(profilePtr);

	InsertProfileIntoList(profilePtr);
}

extern int NumberOfUserProfiles(void)
{
	int n = UserProfilesList.size();

	LOCALASSERT(n>0);

	return n-1;
}

extern AVP_USER_PROFILE *GetFirstUserProfile(void)
{
	CurrentUserProfilePtr=UserProfilesList.first_entry();
	return CurrentUserProfilePtr;
}

extern AVP_USER_PROFILE *GetNextUserProfile(void)
{
	if (CurrentUserProfilePtr == UserProfilesList.last_entry())
	{
		CurrentUserProfilePtr = UserProfilesList.first_entry();
	}
	else
	{
		CurrentUserProfilePtr = UserProfilesList.next_entry(CurrentUserProfilePtr);
	}
	return CurrentUserProfilePtr;
}

static void EmptyUserProfilesList(void)
{
	while (UserProfilesList.size())
	{
		delete UserProfilesList.first_entry();
		UserProfilesList.delete_first_entry();
	}
}

/* Versioned suffix: legacy bytes, signature, 64-byte name. Old profiles
   need no migration on disk. Truncated/unrecognised suffixes use legacy Name. */
#define PROFILE_LEGACY_BYTES offsetof(AVP_USER_PROFILE, FullName)
static_assert(PROFILE_LEGACY_BYTES == 44560, "Profile prefix layout changed");
static const unsigned char ProfileExtensionTag[8] = {'T','Y','R','P','R','F',1,0};
static const unsigned char ProfileMotionTag[8]={'T','Y','R','M','O','T',2,0};
static const unsigned char ProfileEpisodeTag[8]={'T','Y','R','E','P','I',1,0};
static const unsigned char ProfileRunTag[8]={'T','Y','R','R','U','N',1,0};
static int ReadProfileRecord(FILE *file, AVP_USER_PROFILE *profile)
{
    unsigned char tag[8];
    char name[MAX_SIZE_OF_USERS_NAME+1];
    unsigned char bindings[2];
    memset(profile, 0, sizeof(*profile));
    profile->HUDShakeIntensityValue=profile->HUDSwayIntensityValue=profile->HUDBobbingIntensityValue=50;
    profile->PreviousVisionBindings[0]=profile->PreviousVisionBindings[1]=KEY_VOID;
    if (fread(profile, 1, PROFILE_LEGACY_BYTES, file) != PROFILE_LEGACY_BYTES) return 0;
    profile->Name[LEGACY_SIZE_OF_USERS_NAME] = 0;
    strcpy(profile->FullName, profile->Name);
    if (fread(tag,1,sizeof(tag),file)==sizeof(tag) && !memcmp(tag,ProfileExtensionTag,sizeof(tag)) &&
        fread(name,1,sizeof(name),file)==sizeof(name) && fread(bindings,1,2,file)==2)
    {
        name[MAX_SIZE_OF_USERS_NAME]=0;
        /* Names become filenames; a damaged suffix must never escape profiles. */
        if (name[0] && !strpbrk(name, "/\\:*?\"<>|") && !strchr(name,'\n') && !strchr(name,'\r'))
        {
            strcpy(profile->FullName,name);
            memcpy(profile->PreviousVisionBindings,bindings,2);
            unsigned char motionTag[8],values[3];
            if(fread(motionTag,1,8,file)==8 && !memcmp(motionTag,ProfileMotionTag,6) && motionTag[7]==0){
                if(motionTag[6]==1){
                    /* v195 single control becomes Shake; new independent controls default to 50. */
                    if(fread(values,1,1,file)==1 && values[0]>=1 && values[0]<=100)
                        profile->HUDShakeIntensityValue=values[0];
                }else if(motionTag[6]==2 && fread(values,1,3,file)==3){
                    if(values[0]>=1 && values[0]<=100)profile->HUDShakeIntensityValue=values[0];
                    if(values[1]>=1 && values[1]<=100)profile->HUDSwayIntensityValue=values[1];
                    if(values[2]>=1 && values[2]<=100)profile->HUDBobbingIntensityValue=values[2];
                }
            }
        }
    }
    unsigned char episodeTag[8],episodes[3];
    if(fread(episodeTag,1,8,file)==8 && !memcmp(episodeTag,ProfileEpisodeTag,8) &&
       fread(episodes,1,3,file)==3)memcpy(profile->LastSelectedEpisode,episodes,3);
    unsigned char runTag[8],runValues[3];
    if(fread(runTag,1,8,file)==8 && !memcmp(runTag,ProfileRunTag,8) && fread(runValues,1,3,file)==3)
        for(int species=0;species<3;species++)profile->AlwaysRun[species]=runValues[species]==1;

    return 1;
}
static int WriteProfileRecord(FILE *file, AVP_USER_PROFILE *profile)
{
    profile->FullName[MAX_SIZE_OF_USERS_NAME]=0;
    char legacyName[LEGACY_SIZE_OF_USERS_NAME+1]={0};
    strncpy(legacyName,profile->FullName,LEGACY_SIZE_OF_USERS_NAME);
    profile->Name[LEGACY_SIZE_OF_USERS_NAME]=0;
    /* Preserve legacy padding when the short name did not change. */
    if(strcmp(profile->Name,legacyName))memcpy(profile->Name,legacyName,sizeof(legacyName));
    return fwrite(profile,1,PROFILE_LEGACY_BYTES,file)==PROFILE_LEGACY_BYTES &&
        fwrite(ProfileExtensionTag,1,sizeof(ProfileExtensionTag),file)==sizeof(ProfileExtensionTag) &&
        fwrite(profile->FullName,1,sizeof(profile->FullName),file)==sizeof(profile->FullName) &&
        fwrite(profile->PreviousVisionBindings,1,2,file)==2 &&
        fwrite(ProfileMotionTag,1,8,file)==8 &&
        fwrite(&profile->HUDShakeIntensityValue,1,1,file)==1 &&
        fwrite(&profile->HUDSwayIntensityValue,1,1,file)==1 &&
        fwrite(&profile->HUDBobbingIntensityValue,1,1,file)==1 &&
        fwrite(ProfileEpisodeTag,1,8,file)==8 &&
        fwrite(profile->LastSelectedEpisode,1,3,file)==3 &&
        fwrite(ProfileRunTag,1,8,file)==8 && fwrite(profile->AlwaysRun,1,3,file)==3;
}

extern int SaveUserProfileLatencyBody(AVP_USER_PROFILE *profilePtr)
{
	char *filename = new char [strlen(USER_PROFILES_PATH)+strlen(profilePtr->FullName)+strlen(USER_PROFILES_SUFFIX)+1];
	strcpy(filename,USER_PROFILES_PATH);
	strcat(filename,profilePtr->FullName);
	strcat(filename,USER_PROFILES_SUFFIX);

	FILE* file=OpenGameFile(filename, FILEMODE_WRITEONLY, FILETYPE_CONFIG);
	delete [] filename;
	if (!file) return 0;
	
	{ EVENT_TICK stamp=EventLatencyBegin(); SaveSettingsToUserProfile(profilePtr); EventLatencyEnd(stamp,"profile snapshot",0,0,0,profilePtr->FullName,0); }
 EVENT_TICK writeStamp=EventLatencyBegin();
	int ok = WriteProfileRecord(file,profilePtr);
 EventLatencyEnd(writeStamp,"profile serialization/write",ok,0,0,profilePtr->FullName,0);
 EVENT_TICK closeStamp=EventLatencyBegin();
	if (fclose(file) != 0) ok = 0;
 EventLatencyEnd(closeStamp,"profile close/flush",ok,0,0,profilePtr->FullName,0);

	return ok;
}
extern int SaveUserProfile(AVP_USER_PROFILE *profilePtr)
{
 EVENT_TICK stamp=EventLatencyBegin();
 int result=SaveUserProfileLatencyBody(profilePtr);
 EventLatencyEnd(stamp,"save profile total",result,0,0,profilePtr->FullName,2);
 return result;
}

extern void DeleteUserProfile(int number)
{
	AVP_USER_PROFILE *profilePtr = GetFirstUserProfile();

	for (int i=0; i<number; i++) profilePtr = GetNextUserProfile();

	char *filename = new char [strlen(USER_PROFILES_PATH)+strlen(profilePtr->FullName)+strlen(USER_PROFILES_SUFFIX)+1];
	strcpy(filename,USER_PROFILES_PATH);
	strcat(filename,profilePtr->FullName);
	strcat(filename,USER_PROFILES_SUFFIX);

	DeleteGameFile(filename);

	delete [] filename;
	{
		int i;
		filename = new char [100];

		for (i=0; i<NUMBER_OF_SAVE_SLOTS; i++)
		{
			sprintf(filename,"%s%s_%d.sav",USER_PROFILES_PATH,profilePtr->FullName,i+1);
			DeleteGameFile(filename);
		}
		delete [] filename;
	}
}

static void InsertProfileIntoList(AVP_USER_PROFILE *profilePtr)
{
	if (UserProfilesList.size())
	{
		AVP_USER_PROFILE *profileInListPtr = GetFirstUserProfile();

		for (int i=0; i<UserProfilesList.size(); i++, profileInListPtr = GetNextUserProfile())
		{
			if (ProfileIsMoreRecent(profilePtr,profileInListPtr))
			{
				UserProfilesList.add_entry_before(profilePtr,profileInListPtr);
				return;
			}
		}
	}
	UserProfilesList.add_entry(profilePtr);
}

static int ProfileIsMoreRecent(AVP_USER_PROFILE *profilePtr, AVP_USER_PROFILE *profileToTestAgainstPtr)
{
	if (difftime(profilePtr->FileTime, profileToTestAgainstPtr->FileTime) > 0.0) {
		return 1; /* first file newer than file to test */
	} else {
		return 0;
	}
}

static int UserProfileHasVisibleName(AVP_USER_PROFILE *profilePtr)
{
	int i;
	int hasVisibleCharacter = 0;

	/* Profile files are binary and older/corrupt records need not contain
	   a terminator in this field. Bound it before using it as UI text. */
	profilePtr->FullName[sizeof(profilePtr->FullName)-1] = 0;
	for (i = 0; profilePtr->FullName[i]; ++i)
	{
		if ((unsigned char)profilePtr->FullName[i] > ' ')
		{
			hasVisibleCharacter = 1;
			break;
		}
	}

	return hasVisibleCharacter;
}
static int LoadUserProfiles(void)
{
	void *gd;
	GameDirectoryFile *gdf;
	
	gd = OpenGameDirectory(USER_PROFILES_PATH, USER_PROFILES_WILDCARD_NAME, FILETYPE_CONFIG);
	if (gd == NULL) {
		CreateGameDirectory(USER_PROFILES_PATH); /* maybe it didn't exist.. */
		return 0;
	}

	int nPathLen = strlen(USER_PROFILES_PATH);
	
	while ((gdf = ScanGameDirectory(gd)) != NULL) {
		if ((gdf->attr & FILEATTR_DIRECTORY) != 0)
			continue;
		if ((gdf->attr & FILEATTR_READABLE) == 0)
			continue;
		
		char * pszFullPath = new char [nPathLen+strlen(gdf->filename)+1];
		strcpy(pszFullPath, USER_PROFILES_PATH);
		strcat(pszFullPath, gdf->filename);
			
		FILE *rif_file;
		rif_file = OpenGameFile(pszFullPath, FILEMODE_READONLY, FILETYPE_CONFIG);
		if(rif_file==NULL)
		{
			delete[] pszFullPath;
			continue;
		}

		AVP_USER_PROFILE *profilePtr = new AVP_USER_PROFILE;
			
		if (!ReadProfileRecord(rif_file, profilePtr))
		{
	       		fclose(rif_file);
			delete[] pszFullPath;
			delete profilePtr;
			continue;
		}

		profilePtr->FileTime = gdf->timestamp;
	
		if (!UserProfileHasVisibleName(profilePtr))
		{
			fclose(rif_file);
			delete[] pszFullPath;
			delete profilePtr;
			continue;
		}

		InsertProfileIntoList(profilePtr);
		fclose(rif_file);
		delete[] pszFullPath;
	}
	
	CloseGameDirectory(gd);
	
	return 1;
}

static void InitialiseReloadBinding(PLAYER_INPUT_CONFIGURATION *inputConfig)
{
	inputConfig->ReloadWeapon = KEY_VOID;
	inputConfig->ReloadBindingProfileMarker = RELOAD_BINDING_PROFILE_MARKER;
}

/* AVP_USER_PROFILE remains byte-for-byte stable. Padding is exclusively the
   six 11-slot binding maps plus their eight-byte signature. */
#define WEAPON_BINDING_PROFILE_STORAGE_BYTES (6*NUMBER_OF_WEAPON_BINDINGS)
#define PROFILE_META_HUD_MARKER 0x5A
#define PROFILE_META_HUD_SCALE_MARKER 0x5D
#define PROFILE_META_CROSSHAIR_SCALE_MARKER 0x5E
#define PROFILE_META_CROSSHAIR_SCALE_MARKER_SLOT 9
#define PROFILE_META_CROSSHAIR_SCALE_MARINE_VALUE_SLOT 9
#define PROFILE_META_CROSSHAIR_SCALE_PREDATOR_VALUE_SLOT 9
#define PROFILE_META_HUD_SCALE_VALUE_SLOT 9
#define PROFILE_META_HUD_SCALE_MARKER_SLOT 10
#define PROFILE_META_FOV_LEGACY_MARKER 0x5B
#define PROFILE_META_FOV_MARKER 0x5C
#define PROFILE_META_HUD_MARKER_SLOT 9
#define PROFILE_META_HUD_MODE_SLOT 10
#define PROFILE_META_HUD_POSITION_SLOT 9
#define PROFILE_META_HUD_UNLOCKED_SLOT 10
#define PROFILE_META_FOV_MARKER_SLOT 10
#define PROFILE_META_FOV_VALUE_SLOT 10
#define PROFILE_META_FOV_ALIEN_VALUE_SLOT 10
static const unsigned char WeaponBindingProfileSignatureV15[8] = {'N','A','V','P','W','B','1','5'};
static const unsigned char WeaponBindingProfileSignatureV16[8] = {'N','A','V','P','W','B','1','6'};
static const unsigned char WeaponBindingProfileSignature[8] = {'N','A','V','P','W','B','1','7'};

static int ProfileHasWeaponBindingSignature(const AVP_USER_PROFILE *profilePtr, const unsigned char *signature)
{
	return memcmp(profilePtr->Padding+WEAPON_BINDING_PROFILE_STORAGE_BYTES,signature,8)==0;
}

static int ProfileUsesWeaponBindingsV17(const AVP_USER_PROFILE *profilePtr)
{
	return ProfileHasWeaponBindingSignature(profilePtr,WeaponBindingProfileSignature);
}

static int ProfileUsesWeaponBindingsV16(const AVP_USER_PROFILE *profilePtr)
{
	return ProfileHasWeaponBindingSignature(profilePtr,WeaponBindingProfileSignatureV16);
}

static int PackThreeUIValues(int marineValue, int predatorValue, int alienValue)
{
	return (marineValue & 0xff) | ((predatorValue & 0xff) << 8) | ((alienValue & 0xff) << 16);
}
static int ReadPackedUIValue(int packedValues, int species)
{
	if (species == I_Predator) return (packedValues >> 8) & 0xff;
	if (species == I_Alien) return (packedValues >> 16) & 0xff;
	return packedValues & 0xff;
}
static void StorePerSpeciesUIScalersInProfile(AVP_USER_PROFILE *profilePtr)
{
	HUD_SynchronisePerSpeciesOptions();
    profilePtr->HUDShakeIntensityValue=(unsigned char)(HUDShakeIntensity>=1 && HUDShakeIntensity<=100?HUDShakeIntensity:50);
    profilePtr->HUDSwayIntensityValue=(unsigned char)(HUDSwayIntensity>=1 && HUDSwayIntensity<=100?HUDSwayIntensity:50);
    profilePtr->HUDBobbingIntensityValue=(unsigned char)(HUDBobbingIntensity>=1 && HUDBobbingIntensity<=100?HUDBobbingIntensity:50);

	SynchroniseWeaponFOVForCurrentSpecies();
	profilePtr->unused[0] = PROFILE_PER_SPECIES_UI_SCALERS_MARKER;
	if (MarineCrosshairThicknessMode < MARINE_CROSSHAIR_THICKNESS_X10 || MarineCrosshairThicknessMode > MARINE_CROSSHAIR_THICKNESS_X20)
		MarineCrosshairThicknessMode = MARINE_CROSSHAIR_THICKNESS_X10;
	if (MusicPlayMode < 0 || MusicPlayMode > 2) MusicPlayMode = 1;
	if (CRTFilterEnabled != 0) CRTFilterEnabled = 1;
	profilePtr->unused[1] = (int)(((unsigned int)PackThreeUIValues(WeaponFOVMarine, WeaponFOVPredator, WeaponFOVAlien) & 0x00ffffffu) | ((unsigned int)((PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_BASE + MarineCrosshairThicknessMode) | ((MusicPlayMode << PROFILE_MUSIC_PLAY_MODE_SHIFT) & PROFILE_MUSIC_PLAY_MODE_MASK) | (CRTFilterEnabled ? PROFILE_CRT_FILTER_FLAG : 0)) << 24));
	profilePtr->unused[2] = (int)CameraMotionPack(PackThreeUIValues(HUDUserScaleMarineTenths, HUDUserScalePredatorTenths, HUDUserScaleAlienTenths), MarineCameraMode);
	/* 0xA0..A2 identifies a screenshot preference without changing the profile layout. */
	profilePtr->unused[3] = (int)((unsigned int)PackThreeUIValues(HUDAnchorPositionMarine, HUDAnchorPositionPredator, HUDAnchorPositionAlien) |
        ((0xA0u + (unsigned int)(ScreenshotFormat>=0 && ScreenshotFormat<SCREENSHOT_FORMAT_COUNT ? ScreenshotFormat : SCREENSHOT_PNG))<<24));
	profilePtr->unused[4] = PackThreeUIValues(HUDAnchorUnlockedPositionMarine, HUDAnchorUnlockedPositionPredator, HUDAnchorUnlockedPositionAlien);
}
static void RestorePerSpeciesUIScalersFromProfile(const AVP_USER_PROFILE *profilePtr)
{
	MarineCameraMode = 0;
    HUDShakeIntensity = profilePtr->HUDShakeIntensityValue>=1 && profilePtr->HUDShakeIntensityValue<=100 ? profilePtr->HUDShakeIntensityValue:50;
    HUDSwayIntensity=profilePtr->HUDSwayIntensityValue>=1 && profilePtr->HUDSwayIntensityValue<=100 ? profilePtr->HUDSwayIntensityValue:50;
    HUDBobbingIntensity=profilePtr->HUDBobbingIntensityValue>=1 && profilePtr->HUDBobbingIntensityValue<=100 ? profilePtr->HUDBobbingIntensityValue:50;

	if (profilePtr->unused[0] == PROFILE_PER_SPECIES_UI_SCALERS_MARKER)
	{
		WeaponFOVMarine = ReadPackedUIValue(profilePtr->unused[1], I_Marine);
		WeaponFOVPredator = ReadPackedUIValue(profilePtr->unused[1], I_Predator);
		WeaponFOVAlien = ReadPackedUIValue(profilePtr->unused[1], I_Alien);
		{
			unsigned int marineCrosshairThicknessMarker = ((unsigned int)profilePtr->unused[1] >> 24) & 0xffu;
			unsigned int thicknessMarker = marineCrosshairThicknessMarker & ~(PROFILE_MUSIC_PLAY_MODE_MASK | PROFILE_CRT_FILTER_FLAG);
			if (thicknessMarker >= PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_BASE && thicknessMarker <= PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_MAX)
			{
				MarineCrosshairThicknessMode = (int)(thicknessMarker - PROFILE_MARINE_CROSSHAIR_THICKNESS_MARKER_BASE);
				MusicPlayMode = (int)((marineCrosshairThicknessMarker & PROFILE_MUSIC_PLAY_MODE_MASK) >> PROFILE_MUSIC_PLAY_MODE_SHIFT);
			CRTFilterEnabled = (marineCrosshairThicknessMarker & PROFILE_CRT_FILTER_FLAG) ? 1 : 0;
			}
			else
			{
				MarineCrosshairThicknessMode = MARINE_CROSSHAIR_THICKNESS_X10;
				MusicPlayMode = 1;
				CRTFilterEnabled = 0;
			}
		}
		MarineCameraMode = CameraMotionUnpack((unsigned int)profilePtr->unused[2]);
	HUDUserScaleMarineTenths = ReadPackedUIValue(profilePtr->unused[2], I_Marine);
		HUDUserScalePredatorTenths = ReadPackedUIValue(profilePtr->unused[2], I_Predator);
		HUDUserScaleAlienTenths = ReadPackedUIValue(profilePtr->unused[2], I_Alien);
		HUDAnchorPositionMarine = ReadPackedUIValue(profilePtr->unused[3], I_Marine);
		HUDAnchorPositionPredator = ReadPackedUIValue(profilePtr->unused[3], I_Predator);
		HUDAnchorPositionAlien = ReadPackedUIValue(profilePtr->unused[3], I_Alien);
		HUDAnchorUnlockedPositionMarine = ReadPackedUIValue(profilePtr->unused[4], I_Marine);
		HUDAnchorUnlockedPositionPredator = ReadPackedUIValue(profilePtr->unused[4], I_Predator);
		HUDAnchorUnlockedPositionAlien = ReadPackedUIValue(profilePtr->unused[4], I_Alien);
	}
	else
	{
		MarineCrosshairThicknessMode = MARINE_CROSSHAIR_THICKNESS_X10;
		MusicPlayMode = 1;
		CRTFilterEnabled = 0;
		WeaponFOVMarine = WeaponFOVPredator = WeaponFOVAlien = WeaponFOV;
		HUDUserScaleMarineTenths = HUDUserScalePredatorTenths = HUDUserScaleAlienTenths = HUDUserScaleTenths;
		HUDAnchorPositionMarine = HUDAnchorPositionPredator = HUDAnchorPositionAlien = HUDAnchorPosition;
		HUDAnchorUnlockedPositionMarine = HUDAnchorUnlockedPositionPredator = HUDAnchorUnlockedPositionAlien = HUDAnchorUnlockedPosition;
	}
	HUD_RestorePerSpeciesOptions();
	RestoreWeaponFOVForCurrentSpecies();
}
static void StoreModernOptionsInWeaponMetadata(void)
{
	/* These slots are not published for direct selection on any species. */
	AlienWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_MARKER_SLOT] = PROFILE_META_HUD_MARKER;
	AlienWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_MODE_SLOT] = (unsigned char)HUDAnchorMode;
	AlienWeaponSecondaryConfig.Weapon[PROFILE_META_HUD_POSITION_SLOT] = (unsigned char)HUDAnchorPosition;
	AlienWeaponSecondaryConfig.Weapon[PROFILE_META_HUD_UNLOCKED_SLOT] = (unsigned char)HUDAnchorUnlockedPosition;
	HUD_GetUserScaleFixed();
	MarineWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_SCALE_VALUE_SLOT] = (unsigned char)HUDUserScaleTenths;
	MarineWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_SCALE_MARKER_SLOT] = PROFILE_META_HUD_SCALE_MARKER;
	HUD_ClampCrosshairScaleForCurrentSpecies();
	MarineWeaponSecondaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_MARKER_SLOT] = PROFILE_META_CROSSHAIR_SCALE_MARKER;
	PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_MARINE_VALUE_SLOT] = (unsigned char)CrosshairScaleMarineTenths;
	PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_PREDATOR_VALUE_SLOT] = (unsigned char)CrosshairScalePredatorTenths;
	/* Keep world FOV independent for alien and the shared marine/predator group. */
	ClampPlayerFOVForCurrentSpecies();
	PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_FOV_MARKER_SLOT] = (unsigned char)(PROFILE_META_FMV_MODE_P072_MARKER_BASE + FMVScaleMode);
	PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_VALUE_SLOT] = (unsigned char)PlayerFOVMarinePredator;
	MarineWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_ALIEN_VALUE_SLOT] = (unsigned char)PlayerFOVAlien;
}
static void InitialiseWeaponBindings(void)
{
	MarineWeaponPrimaryConfig = DefaultMarineWeaponPrimaryConfig;
	MarineWeaponSecondaryConfig = DefaultMarineWeaponSecondaryConfig;
	PredatorWeaponPrimaryConfig = DefaultPredatorWeaponPrimaryConfig;
	PredatorWeaponSecondaryConfig = DefaultPredatorWeaponSecondaryConfig;
	AlienWeaponPrimaryConfig = DefaultAlienWeaponPrimaryConfig;
	AlienWeaponSecondaryConfig = DefaultAlienWeaponSecondaryConfig;
}

static void SaveWeaponBindingsToProfile(AVP_USER_PROFILE *profilePtr)
{
	unsigned char *data=(unsigned char *)profilePtr->Padding;
	memcpy(data,&MarineWeaponPrimaryConfig,sizeof(MarineWeaponPrimaryConfig)); data+=sizeof(MarineWeaponPrimaryConfig);
	memcpy(data,&MarineWeaponSecondaryConfig,sizeof(MarineWeaponSecondaryConfig)); data+=sizeof(MarineWeaponSecondaryConfig);
	memcpy(data,&PredatorWeaponPrimaryConfig,sizeof(PredatorWeaponPrimaryConfig)); data+=sizeof(PredatorWeaponPrimaryConfig);
	memcpy(data,&PredatorWeaponSecondaryConfig,sizeof(PredatorWeaponSecondaryConfig)); data+=sizeof(PredatorWeaponSecondaryConfig);
	memcpy(data,&AlienWeaponPrimaryConfig,sizeof(AlienWeaponPrimaryConfig)); data+=sizeof(AlienWeaponPrimaryConfig);
	memcpy(data,&AlienWeaponSecondaryConfig,sizeof(AlienWeaponSecondaryConfig));
	memcpy(profilePtr->Padding+WEAPON_BINDING_PROFILE_STORAGE_BYTES,WeaponBindingProfileSignature,sizeof(WeaponBindingProfileSignature));
}

static int ProfileHasWeaponBindings(const AVP_USER_PROFILE *profilePtr)
{
	return (ProfileUsesWeaponBindingsV17(profilePtr)
		|| ProfileUsesWeaponBindingsV16(profilePtr)
		|| ProfileHasWeaponBindingSignature(profilePtr,WeaponBindingProfileSignatureV15));
}

static int ProfileNeedsPredatorBindingMigration(const AVP_USER_PROFILE *profilePtr)
{
	return ProfileHasWeaponBindingSignature(profilePtr,WeaponBindingProfileSignatureV15);
}
static void LoadWeaponBindingsFromProfile(const AVP_USER_PROFILE *profilePtr)
{
	const unsigned char *data=(const unsigned char *)profilePtr->Padding;
	memcpy(&MarineWeaponPrimaryConfig,data,sizeof(MarineWeaponPrimaryConfig)); data+=sizeof(MarineWeaponPrimaryConfig);
	memcpy(&MarineWeaponSecondaryConfig,data,sizeof(MarineWeaponSecondaryConfig)); data+=sizeof(MarineWeaponSecondaryConfig);
	memcpy(&PredatorWeaponPrimaryConfig,data,sizeof(PredatorWeaponPrimaryConfig)); data+=sizeof(PredatorWeaponPrimaryConfig);
	memcpy(&PredatorWeaponSecondaryConfig,data,sizeof(PredatorWeaponSecondaryConfig)); data+=sizeof(PredatorWeaponSecondaryConfig);
	memcpy(&AlienWeaponPrimaryConfig,data,sizeof(AlienWeaponPrimaryConfig)); data+=sizeof(AlienWeaponPrimaryConfig);
	memcpy(&AlienWeaponSecondaryConfig,data,sizeof(AlienWeaponSecondaryConfig));
}
/* Probe 019: repair the old Predator menu layout once.  The legacy extra
   reload row stored wristblade in ReloadWeapon and shifted the visible weapon
   rows; slot 4 is the unpublished pistol and must not retain the old Disc key. */
static void MigratePredatorWeaponBindings(AVP_USER_PROFILE *profilePtr)
{
	unsigned char oldPrimary[6];
	unsigned char oldSecondary[6];

	memcpy(oldPrimary,PredatorWeaponPrimaryConfig.Weapon,sizeof(oldPrimary));
	memcpy(oldSecondary,PredatorWeaponSecondaryConfig.Weapon,sizeof(oldSecondary));

	PredatorWeaponPrimaryConfig.Weapon[0]=profilePtr->PredatorInputPrimaryConfig.ReloadWeapon;
	PredatorWeaponSecondaryConfig.Weapon[0]=profilePtr->PredatorInputSecondaryConfig.ReloadWeapon;
	PredatorWeaponPrimaryConfig.Weapon[1]=oldPrimary[0];
	PredatorWeaponSecondaryConfig.Weapon[1]=oldSecondary[0];
	PredatorWeaponPrimaryConfig.Weapon[2]=oldPrimary[1];
	PredatorWeaponSecondaryConfig.Weapon[2]=oldSecondary[1];
	PredatorWeaponPrimaryConfig.Weapon[3]=oldPrimary[2];
	PredatorWeaponSecondaryConfig.Weapon[3]=oldSecondary[2];
	PredatorWeaponPrimaryConfig.Weapon[4]=KEY_VOID;
	PredatorWeaponSecondaryConfig.Weapon[4]=KEY_VOID;
	PredatorWeaponPrimaryConfig.Weapon[5]=oldPrimary[4];
	PredatorWeaponSecondaryConfig.Weapon[5]=oldSecondary[4];

	profilePtr->PredatorInputPrimaryConfig.ReloadWeapon=KEY_VOID;
	profilePtr->PredatorInputSecondaryConfig.ReloadWeapon=KEY_VOID;
}

static void SetDefaultProfileOptions(AVP_USER_PROFILE *profilePtr)
{
    int previousAlwaysRun[3];memcpy(previousAlwaysRun,AlwaysRunBySpecies,sizeof previousAlwaysRun);
    memset(AlwaysRunBySpecies,0,sizeof AlwaysRunBySpecies);
    const int savedScreenshotFormat=ScreenshotFormat;
    ScreenshotFormat=SCREENSHOT_PNG;
    PredatorPreviousVisionBindings[0]=KEY_X;PredatorPreviousVisionBindings[1]=KEY_VOID;
	// set Gamma
	RequestedGammaSetting = 128;
	
	// controls
	MarineInputPrimaryConfig = DefaultMarineInputPrimaryConfig;
	MarineInputSecondaryConfig = DefaultMarineInputSecondaryConfig;
	PredatorInputPrimaryConfig = DefaultPredatorInputPrimaryConfig;
	PredatorInputSecondaryConfig = DefaultPredatorInputSecondaryConfig;
	AlienInputPrimaryConfig = DefaultAlienInputPrimaryConfig;
	AlienInputSecondaryConfig = DefaultAlienInputSecondaryConfig;
	/* Calibrated reload bindings already carry their migration marker. */
	GlobalCommandPrimaryConfig = DefaultGlobalCommandPrimaryConfig;
	GlobalCommandSecondaryConfig = DefaultGlobalCommandSecondaryConfig;
	InitialiseWeaponBindings();
	MouseZoomSensitivityTenths = DEFAULT_MOUSE_ZOOM_SENSITIVITY_TENTHS;
	ControlMethods = DefaultControlMethods;
	JoystickControlMethods = DefaultJoystickControlMethods;
	
	SmackerSoundVolume = ONE_FIXED/512;
	EffectsSoundVolume = VOLUME_DEFAULT;
	CDPlayerVolume = CDDA_VOLUME_DEFAULT;
	MoviesAreActive = 1;
	IntroOutroMoviesAreActive = 1; 
	AutoWeaponChangeOn = TRUE;
	CrouchIsToggleKey = 0;
	UnlimitedSaveGames = 0;
	SmartJetpackEnabled = 0;
	SmartZoomEnabled = 0;
	HUDAnchorMode = HUD_ANCHOR_16_9;
	HUDAnchorPosition = 0;
	HUDAnchorUnlockedPosition = 0;
	HUDAnchorPositionMarine = HUDAnchorPositionPredator = HUDAnchorPositionAlien = 0;
	HUDAnchorUnlockedPositionMarine = HUDAnchorUnlockedPositionPredator = HUDAnchorUnlockedPositionAlien = 0;
	HUDUserScaleTenths = 7;
	HUDUserScaleMarineTenths = 7;
	HUDUserScalePredatorTenths = HUDUserScaleAlienTenths = 7;
	NoiseOverlayScaleMode = NOISE_OVERLAY_SCALE_1X;
	FPSDisplayPosition = FPS_DISPLAY_OFF;
	NotificationMessageDurationMode = 0;
	MusicPlayMode = 1;
	/* CRT is global. Preserve the read-only startup preference while the profile
	   picker creates its temporary default entry; selecting a profile restores
	   that profile's persisted value normally. */
	MonitorNoSignalEffectMode = MONITOR_NO_SIGNAL_RGB_NOISE;
	ConsoleTextScaleMode = 2;
	HUDTextureFilterMode = HUD_TEXTURE_FILTER_BILINEAR;
	FMVScaleMode = FMV_SCALE_ULTRAWIDE_X25;
	MarineCameraMode = 7;
    HUDShakeIntensity = HUDSwayIntensity = HUDBobbingIntensity = 50;
	CrosshairScaleTenths = CROSSHAIR_SCALE_TENTHS_DEFAULT;
	CrosshairScaleMarineTenths = CROSSHAIR_SCALE_TENTHS_DEFAULT;
	CrosshairScalePredatorTenths = 20;
	FullVerticalPitchEnabled = 0;
	MarineCrosshairThicknessMode = MARINE_CROSSHAIR_THICKNESS_X10;
	HUD_RestoreCrosshairScaleForCurrentSpecies();
	PlayerFOV = PLAYER_FOV_DEFAULT;
	PlayerFOVMarinePredator = PLAYER_FOV_MARINE_DEFAULT;
	PlayerFOVAlien = PLAYER_FOV_ALIEN_DEFAULT;
	
	WeaponFOV = WEAPON_FOV_DEFAULT;
	WeaponFOVMarine = WEAPON_FOV_MARINE_PREDATOR_DEFAULT;
	WeaponFOVPredator = WEAPON_FOV_MARINE_PREDATOR_DEFAULT;
	WeaponFOVAlien = WEAPON_FOV_ALIEN_DEFAULT;
	RestoreWeaponFOVForCurrentSpecies();
	HUD_RestorePerSpeciesOptions();
	
	WeaponFOVStyle = WEAPON_FOV_STYLE_STRETCHED;
	
	strcpy(MP_PlayerName, "Player");

	SetToDefaultDetailLevels();
	
	{
		int a,b;

		for (a=0; a<I_MaxDifficulties; a++) {
			for (b=0; b<AVP_ENVIRONMENT_END_OF_LIST; b++) {
				profilePtr->PersonalBests[a][b]=DefaultLevelGameStats;
			}
		}
	}

	SaveSettingsToUserProfile(profilePtr);
    memcpy(AlwaysRunBySpecies,previousAlwaysRun,sizeof previousAlwaysRun);
    ScreenshotFormat=savedScreenshotFormat; /* Temporary new-profile entry must not overwrite saved global preferences. */
}
			
extern void GetSettingsFromUserProfile(void)
{
    for(int species=0;species<3;species++)AlwaysRunBySpecies[species]=UserProfilePtr->AlwaysRun[species]==1;
    memcpy(PredatorPreviousVisionBindings,UserProfilePtr->PreviousVisionBindings,2);
    unsigned int screenshotTag=(unsigned int)UserProfilePtr->unused[3]>>24;
    /* Legacy profiles keep the existing global setting; new profiles explicitly store PNG. */
    if(screenshotTag>=0xA0u && screenshotTag<0xA0u+SCREENSHOT_FORMAT_COUNT)
        ScreenshotFormat=(int)(screenshotTag-0xA0u);
	RequestedGammaSetting = UserProfilePtr->GammaSetting;
	RestoreGlobalCommandBindingsFromProfile(UserProfilePtr);

	/* Existing profiles have no reload marker. Initialise only once, then preserve
	   the user's subsequent binding in the same unchanged profile layout. */
	if (UserProfilePtr->MarineInputPrimaryConfig.ReloadBindingProfileMarker
		!= RELOAD_BINDING_PROFILE_MARKER)
	{
		InitialiseReloadBinding(&UserProfilePtr->MarineInputPrimaryConfig);
		InitialiseReloadBinding(&UserProfilePtr->MarineInputSecondaryConfig);
	}

	/* Old profiles receive defaults once; later loads restore the saved maps. */
if (ProfileHasWeaponBindings(UserProfilePtr))
{
	int needsPredatorMigration=ProfileNeedsPredatorBindingMigration(UserProfilePtr);
	LoadWeaponBindingsFromProfile(UserProfilePtr);
	if (needsPredatorMigration)
	{
		MigratePredatorWeaponBindings(UserProfilePtr);
	}
}
else
{
	InitialiseWeaponBindings();
	SaveWeaponBindingsToProfile(UserProfilePtr);
}

	MarineInputPrimaryConfig = 		UserProfilePtr->MarineInputPrimaryConfig;
	MarineInputSecondaryConfig = 	UserProfilePtr->MarineInputSecondaryConfig;
	PredatorInputPrimaryConfig = 	UserProfilePtr->PredatorInputPrimaryConfig;
	PredatorInputSecondaryConfig = 	UserProfilePtr->PredatorInputSecondaryConfig;
	AlienInputPrimaryConfig = 		UserProfilePtr->AlienInputPrimaryConfig;
	AlienInputSecondaryConfig = 	UserProfilePtr->AlienInputSecondaryConfig;
	ControlMethods = 				UserProfilePtr->ControlMethods;
	{
		FMVScaleMode = ReadFMVScaleModeFromProfile(UserProfilePtr, &MouseZoomSensitivityTenths);
	}
	JoystickControlMethods = 		UserProfilePtr->JoystickControlMethods;
	MenuDetailLevelOptions = 		UserProfilePtr->DetailLevelSettings;
	SmackerSoundVolume =			UserProfilePtr->SmackerSoundVolume;
	EffectsSoundVolume =			UserProfilePtr->EffectsSoundVolume;
	CDPlayerVolume = 				UserProfilePtr->CDPlayerVolume;
	MoviesAreActive =				UserProfilePtr->MoviesAreActive;
	IntroOutroMoviesAreActive =		UserProfilePtr->IntroOutroMoviesAreActive;
	AutoWeaponChangeOn = 			!UserProfilePtr->AutoWeaponChangeDisabled;
	CrouchIsToggleKey = 			UserProfilePtr->CrouchToggleEnabled;
	UnlimitedSaveGames = 			UserProfilePtr->UnlimitedSaveGamesEnabled;
	SmartJetpackEnabled = 			UserProfilePtr->SmartJetpackEnabled;
	SmartZoomEnabled =             UserProfilePtr->SmartZoomEnabled;
	/* Restore the anchored HUD options before its independent scale. */
	if (ProfileUsesWeaponBindingsV17(UserProfilePtr)
	 && AlienWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_MARKER_SLOT] == PROFILE_META_HUD_MARKER
	 && AlienWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_MODE_SLOT] <= HUD_ANCHOR_UNLOCKED)
	{
		HUDAnchorMode = AlienWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_MODE_SLOT];
		HUDAnchorPosition = AlienWeaponSecondaryConfig.Weapon[PROFILE_META_HUD_POSITION_SLOT];
		HUDAnchorUnlockedPosition = AlienWeaponSecondaryConfig.Weapon[PROFILE_META_HUD_UNLOCKED_SLOT];
	}
	else { HUDAnchorMode = HUD_ANCHOR_4_3; HUDAnchorPosition = 0; HUDAnchorUnlockedPosition = 0;
	HUDUserScaleTenths = HUD_USER_SCALE_TENTHS_DEFAULT; }
	if (HUDAnchorPosition < 0 || HUDAnchorPosition > 100) HUDAnchorPosition = 0;
	if (HUDAnchorUnlockedPosition < 0 || HUDAnchorUnlockedPosition > 100) HUDAnchorUnlockedPosition = 0;
	HUDUserScaleTenths = HUD_USER_SCALE_TENTHS_DEFAULT;
	if (ProfileUsesWeaponBindingsV17(UserProfilePtr)
	 && MarineWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_SCALE_MARKER_SLOT] == PROFILE_META_HUD_SCALE_MARKER
	 && MarineWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_SCALE_VALUE_SLOT] >= HUD_USER_SCALE_TENTHS_MINIMUM
	 && MarineWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_SCALE_VALUE_SLOT] <= HUD_USER_SCALE_TENTHS_MAXIMUM)
		HUDUserScaleTenths = MarineWeaponPrimaryConfig.Weapon[PROFILE_META_HUD_SCALE_VALUE_SLOT];
	else HUDUserScaleTenths = HUD_USER_SCALE_TENTHS_DEFAULT;
	/* v1.7 stored one FOV. v1.8 metadata stores both species groups without
	   changing the profile binary layout. */
	if (ProfileUsesWeaponBindingsV17(UserProfilePtr)
	 && (PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_FOV_MARKER_SLOT] == PROFILE_META_FOV_MARKER || ((PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_FOV_MARKER_SLOT] >= PROFILE_META_FMV_MODE_MARKER_BASE && PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_FOV_MARKER_SLOT] <= PROFILE_META_FMV_MODE_MARKER_MAX) || (PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_FOV_MARKER_SLOT] >= PROFILE_META_FMV_MODE_P072_MARKER_BASE && PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_FOV_MARKER_SLOT] <= PROFILE_META_FMV_MODE_P072_MARKER_MAX)))
	 && PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_VALUE_SLOT] >= PLAYER_FOV_MARINE_PREDATOR_MINIMUM
	 && PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_VALUE_SLOT] <= 120
	 && MarineWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_ALIEN_VALUE_SLOT] >= PLAYER_FOV_ALIEN_DEFAULT
	 && MarineWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_ALIEN_VALUE_SLOT] <= PLAYER_FOV_MAXIMUM)
	{
		PlayerFOVMarinePredator = PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_VALUE_SLOT];
		PlayerFOVAlien = MarineWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_ALIEN_VALUE_SLOT];
	}
	else
	{
		int legacyFOV = PLAYER_FOV_DEFAULT;
		if (ProfileUsesWeaponBindingsV17(UserProfilePtr)
		 && PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_FOV_MARKER_SLOT] == PROFILE_META_FOV_LEGACY_MARKER
		 && PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_VALUE_SLOT] >= PLAYER_FOV_MINIMUM
		 && PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_VALUE_SLOT] <= PLAYER_FOV_MAXIMUM)
			legacyFOV = PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_FOV_VALUE_SLOT];
		else if (UserProfilePtr->Padding[4] == PLAYER_FOV_PROFILE_MARKER
		 && UserProfilePtr->Padding[5] >= PLAYER_FOV_MINIMUM
		 && UserProfilePtr->Padding[5] <= PLAYER_FOV_MAXIMUM)
			legacyFOV = UserProfilePtr->Padding[5];
		PlayerFOVMarinePredator = (legacyFOV >= PLAYER_FOV_MARINE_PREDATOR_MINIMUM && legacyFOV <= 120) ? legacyFOV : PLAYER_FOV_MARINE_DEFAULT;
		PlayerFOVAlien = (legacyFOV >= PLAYER_FOV_ALIEN_DEFAULT) ? legacyFOV : PLAYER_FOV_ALIEN_DEFAULT;
	}
	PlayerFOV = PlayerFOVMarinePredator;
	/* Alien secondary has no reload action. Keep the profile size unchanged by
	   reusing its reserved migration marker for the weapon-view FOV. */
	if (UserProfilePtr->AlienInputSecondaryConfig.ReloadBindingProfileMarker >= WEAPON_FOV_PROFILE_MINIMUM
	 && UserProfilePtr->AlienInputSecondaryConfig.ReloadBindingProfileMarker <= WEAPON_FOV_PROFILE_MAXIMUM)
	{
		WeaponFOV = UserProfilePtr->AlienInputSecondaryConfig.ReloadBindingProfileMarker;
	}
	else
	{
		WeaponFOV = WEAPON_FOV_DEFAULT;
	}
	WeaponFOVStyle = (UserProfilePtr->SpareBits & 1) ? WEAPON_FOV_STYLE_STRETCHED : WEAPON_FOV_STYLE_EXACT;
	RestorePerSpeciesUIScalersFromProfile(UserProfilePtr);
	FullVerticalPitchEnabled = ((UserProfilePtr->unused[4] & PROFILE_VERTICAL_PITCH_MASK) == PROFILE_VERTICAL_PITCH_MARKER && (UserProfilePtr->unused[4] & PROFILE_VERTICAL_PITCH_FULL)) ? 1 : 0;
	NoiseOverlayScaleMode = (UserProfilePtr->SpareBits & PROFILE_SPARE_NOISE_SCALE_HALF) ? NOISE_OVERLAY_SCALE_HALF : NOISE_OVERLAY_SCALE_1X;
	FPSDisplayPosition = FPS_DISPLAY_OFF;
	NotificationMessageDurationMode = 0;
	ConsoleTextScaleMode = 1;
	if ((UserProfilePtr->unused[5] & PROFILE_MONITOR_FPS_DURATION_MASK)==PROFILE_TEXT_SCALE_V3_MARKER)
    {
        MonitorNoSignalEffectMode=(UserProfilePtr->unused[5]>>4)&0x0f;
        FPSDisplayPosition=UserProfilePtr->unused[5]&0x0f;
        NotificationMessageDurationMode=(UserProfilePtr->unused[5]>>8)&3;
        ConsoleTextScaleMode=(UserProfilePtr->unused[5]>>10)&15;
    }
    else if (((UserProfilePtr->unused[5] & PROFILE_MONITOR_FPS_DURATION_MASK) == PROFILE_MONITOR_FPS_DURATION_MARKER) ||
		((UserProfilePtr->unused[5] & PROFILE_MONITOR_FPS_DURATION_MASK) == PROFILE_MONITOR_FPS_DURATION_V105_MARKER))
	{
		MonitorNoSignalEffectMode = (UserProfilePtr->unused[5] >> 4) & 0x0f;
		FPSDisplayPosition = UserProfilePtr->unused[5] & 0x0f;
		NotificationMessageDurationMode = (UserProfilePtr->unused[5] >> 8) & 0x03;
		ConsoleTextScaleMode = (UserProfilePtr->unused[5] & PROFILE_TEXT_SCALE_MASK) >> PROFILE_TEXT_SCALE_SHIFT;
		if (UserProfilePtr->unused[5] & PROFILE_TEXT_SCALE_V2_MARKER)
		{
			if (UserProfilePtr->unused[5] & PROFILE_TEXT_SCALE_X30_FLAG) ConsoleTextScaleMode = 4;
		}
		else if (ConsoleTextScaleMode >= 2)
		{
			/* Probe 133 iteration 03 stored x2.5/x3.0 as 2/3. */
			ConsoleTextScaleMode++;
		}
	}
	else if ((UserProfilePtr->unused[5] & PROFILE_MONITOR_FPS_MASK) == PROFILE_MONITOR_FPS_MARKER)
	{
		MonitorNoSignalEffectMode = (UserProfilePtr->unused[5] >> 4) & 0x0f;
		FPSDisplayPosition = UserProfilePtr->unused[5] & 0x0f;
	}
	else
	{
		/* Probe 094 stored only the monitor effect in this reserved word. */
		MonitorNoSignalEffectMode = (UserProfilePtr->unused[5] == (PROFILE_MONITOR_NO_SIGNAL_EFFECT_MARKER + MONITOR_NO_SIGNAL_RGB_SCANLINES)) ? MONITOR_NO_SIGNAL_RGB_SCANLINES : MONITOR_NO_SIGNAL_RGB_NOISE;
	}
	if (MonitorNoSignalEffectMode != MONITOR_NO_SIGNAL_RGB_SCANLINES) MonitorNoSignalEffectMode = MONITOR_NO_SIGNAL_RGB_NOISE;
	if (FPSDisplayPosition < FPS_DISPLAY_OFF || FPSDisplayPosition > FPS_DISPLAY_BOTTOM_LEFT) FPSDisplayPosition = FPS_DISPLAY_OFF;
	if (NotificationMessageDurationMode < 0 || NotificationMessageDurationMode > 3) NotificationMessageDurationMode = 0;
	if (ConsoleTextScaleMode < 0 || ConsoleTextScaleMode > 10) ConsoleTextScaleMode = 1;
	if (MusicPlayMode < 0 || MusicPlayMode > 2) MusicPlayMode = 1;
	if (CRTFilterEnabled != 0) CRTFilterEnabled = 1;
	HUDTextureFilterMode = ((UserProfilePtr->SpareBits & PROFILE_SPARE_HUD_TEXTURE_FILTER_MARKER)
		&& (UserProfilePtr->SpareBits & PROFILE_SPARE_HUD_TEXTURE_NEAREST))
		? HUD_TEXTURE_FILTER_NEAREST : HUD_TEXTURE_FILTER_BILINEAR;

	CrosshairScaleMarineTenths = CROSSHAIR_SCALE_TENTHS_DEFAULT;
	CrosshairScalePredatorTenths = CROSSHAIR_SCALE_TENTHS_DEFAULT;
	if (ProfileUsesWeaponBindingsV17(UserProfilePtr) && MarineWeaponSecondaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_MARKER_SLOT] == PROFILE_META_CROSSHAIR_SCALE_MARKER && PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_MARINE_VALUE_SLOT] >= CROSSHAIR_SCALE_TENTHS_MINIMUM && PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_MARINE_VALUE_SLOT] <= CROSSHAIR_SCALE_TENTHS_MAXIMUM && PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_PREDATOR_VALUE_SLOT] >= CROSSHAIR_SCALE_TENTHS_MINIMUM && PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_PREDATOR_VALUE_SLOT] <= CROSSHAIR_SCALE_TENTHS_MAXIMUM)
	{
		CrosshairScaleMarineTenths = PredatorWeaponPrimaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_MARINE_VALUE_SLOT];
		CrosshairScalePredatorTenths = PredatorWeaponSecondaryConfig.Weapon[PROFILE_META_CROSSHAIR_SCALE_PREDATOR_VALUE_SLOT];
	}
	else if (UserProfilePtr->SpareBits & PROFILE_SPARE_CROSSHAIR_SCALE_MARKER)
	{
		int legacyCrosshairScale = (UserProfilePtr->SpareBits & PROFILE_SPARE_CROSSHAIR_SCALE_MASK) >> 1;
		if (legacyCrosshairScale >= 3 && legacyCrosshairScale <= 13)
		{
			if (legacyCrosshairScale < CROSSHAIR_SCALE_TENTHS_MINIMUM) legacyCrosshairScale = CROSSHAIR_SCALE_TENTHS_MINIMUM;
			CrosshairScaleMarineTenths = legacyCrosshairScale;
			CrosshairScalePredatorTenths = legacyCrosshairScale;
		}
	}
	HUD_RestoreCrosshairScaleForCurrentSpecies();

	/* Version 1.6 wrote HUD/FOV into Padding, corrupting Marine slots 0?5.
	   Restore those six defaults once, then promote the profile to v1.7. */
	if (ProfileUsesWeaponBindingsV16(UserProfilePtr))
	{
		memcpy(MarineWeaponPrimaryConfig.Weapon,DefaultMarineWeaponPrimaryConfig.Weapon,6);
	}
	if (!ProfileUsesWeaponBindingsV17(UserProfilePtr))
	{
		StoreModernOptionsInWeaponMetadata();
		SaveWeaponBindingsToProfile(UserProfilePtr);
	}
	RestorePlayerFOVForCurrentSpecies();
	strncpy(MP_PlayerName,UserProfilePtr->MultiplayerCallsign,15);

	SetDetailLevelsFromMenu();
}

extern void SaveSettingsToUserProfile(AVP_USER_PROFILE *profilePtr)
{
    for(int species=0;species<3;species++)profilePtr->AlwaysRun[species]=AlwaysRunBySpecies[species]==1;
    memcpy(profilePtr->PreviousVisionBindings,PredatorPreviousVisionBindings,2);
	profilePtr->GammaSetting = RequestedGammaSetting;
	StoreGlobalCommandBindingsInProfile(profilePtr);

	profilePtr->MarineInputPrimaryConfig =		MarineInputPrimaryConfig;
	profilePtr->MarineInputSecondaryConfig =	MarineInputSecondaryConfig;
	profilePtr->PredatorInputPrimaryConfig =	PredatorInputPrimaryConfig;
	profilePtr->PredatorInputSecondaryConfig =	PredatorInputSecondaryConfig;
	profilePtr->AlienInputPrimaryConfig =		AlienInputPrimaryConfig;
	profilePtr->AlienInputSecondaryConfig =		AlienInputSecondaryConfig;
	/* Five FMV modes and mouse zoom fit in the alien-only migration byte. */
	if (MouseZoomSensitivityTenths < 0 || MouseZoomSensitivityTenths > FMV_SCALE_PROFILE_V2_MOUSE_MAX)
		MouseZoomSensitivityTenths = DEFAULT_MOUSE_ZOOM_SENSITIVITY_TENTHS;
	if (FMVScaleMode < FMV_SCALE_PANORAMIC_X1 || FMVScaleMode > FMV_SCALE_FULLSCREEN_ULTRAWIDE)
		FMVScaleMode = FMV_SCALE_ULTRAWIDE_X25;
	/* FMV mode is stored in weapon metadata; retain the stable zoom-only byte. */
	profilePtr->AlienInputPrimaryConfig.ReloadBindingProfileMarker = (unsigned char)(0xB0 + MouseZoomSensitivityTenths);
	profilePtr->AlienInputSecondaryConfig.ReloadBindingProfileMarker = (unsigned char)WeaponFOV;
	StorePerSpeciesUIScalersInProfile(profilePtr);
	FullVerticalPitchEnabled = FullVerticalPitchEnabled ? 1 : 0;
	profilePtr->unused[4] = (profilePtr->unused[4] & 0x00FFFFFF) | PROFILE_VERTICAL_PITCH_MARKER | (FullVerticalPitchEnabled ? PROFILE_VERTICAL_PITCH_FULL : 0);
	HUD_ClampCrosshairScaleForCurrentSpecies();
	if (NoiseOverlayScaleMode != NOISE_OVERLAY_SCALE_HALF) NoiseOverlayScaleMode = NOISE_OVERLAY_SCALE_1X;
	if (MonitorNoSignalEffectMode != MONITOR_NO_SIGNAL_RGB_SCANLINES) MonitorNoSignalEffectMode = MONITOR_NO_SIGNAL_RGB_NOISE;
	if (FPSDisplayPosition < FPS_DISPLAY_OFF || FPSDisplayPosition > FPS_DISPLAY_BOTTOM_LEFT) FPSDisplayPosition = FPS_DISPLAY_OFF;
	if (NotificationMessageDurationMode < 0 || NotificationMessageDurationMode > 3) NotificationMessageDurationMode = 0;
	if (ConsoleTextScaleMode < 0 || ConsoleTextScaleMode > 10) ConsoleTextScaleMode = 1;
	profilePtr->unused[5] = PROFILE_TEXT_SCALE_V3_MARKER | ((NotificationMessageDurationMode & 0x03) << 8) | ((ConsoleTextScaleMode & 0x0f) << PROFILE_TEXT_SCALE_SHIFT) | ((MonitorNoSignalEffectMode & 0x0f) << 4) | (FPSDisplayPosition & 0x0f);
	if (HUDTextureFilterMode != HUD_TEXTURE_FILTER_NEAREST) HUDTextureFilterMode = HUD_TEXTURE_FILTER_BILINEAR;
	profilePtr->SpareBits = (unsigned char)((profilePtr->SpareBits & ~0xCF) | (WeaponFOVStyle == WEAPON_FOV_STYLE_STRETCHED ? 1 : 0) | (NoiseOverlayScaleMode == NOISE_OVERLAY_SCALE_HALF ? PROFILE_SPARE_NOISE_SCALE_HALF : 0) | (HUDTextureFilterMode == HUD_TEXTURE_FILTER_NEAREST ? PROFILE_SPARE_HUD_TEXTURE_NEAREST : 0) | PROFILE_SPARE_HUD_TEXTURE_FILTER_MARKER | PROFILE_SPARE_FMV_SCALE_V2_MARKER);
	StoreModernOptionsInWeaponMetadata();
		SaveWeaponBindingsToProfile(profilePtr);
profilePtr->ControlMethods =				ControlMethods;
	profilePtr->JoystickControlMethods =		JoystickControlMethods;
	profilePtr->DetailLevelSettings =			MenuDetailLevelOptions;
	profilePtr->SmackerSoundVolume =			SmackerSoundVolume;	
	profilePtr->EffectsSoundVolume =			EffectsSoundVolume;
	profilePtr->CDPlayerVolume = 				CDPlayerVolume;
	profilePtr->MoviesAreActive =				MoviesAreActive;
	profilePtr->IntroOutroMoviesAreActive =		IntroOutroMoviesAreActive;
	profilePtr->AutoWeaponChangeDisabled=   !AutoWeaponChangeOn;
	profilePtr->CrouchToggleEnabled = CrouchIsToggleKey ? 1 : 0;
	profilePtr->UnlimitedSaveGamesEnabled = UnlimitedSaveGames ? 1 : 0;
	profilePtr->SmartJetpackEnabled = SmartJetpackEnabled ? 1 : 0;
	profilePtr->SmartZoomEnabled = SmartZoomEnabled ? 1 : 0;
	strncpy(profilePtr->MultiplayerCallsign,MP_PlayerName,15);
}

extern void FixCheatModesInUserProfile(AVP_USER_PROFILE *profilePtr)
{
	int a;

	for (a=0; a<MAX_NUMBER_OF_CHEATMODES; a++) {
		if (profilePtr->CheatMode[a]==2) {
			profilePtr->CheatMode[a]=1;
		}
	}

}

}; // extern "C"












