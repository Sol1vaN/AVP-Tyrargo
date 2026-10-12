extern "C"
{
#include "3dc.h"
#include "ourasert.h"
#include "psndplat.h"
#include "dxlog.h"
#include "cd_player.h"
#include "avp_menus.h"
#include "gamedef.h"

#include "avp_envinfo.h"
};

#include "list_tem.hpp"
#include <windows.h>
#include <stdio.h>

//lists of tracks for each level
List<int> LevelCDTracks[AVP_ENVIRONMENT_END_OF_LIST];

//lists of tracks for each species in multiplayer games
List<int> MultiplayerCDTracks[3];

static int LastTrackChosen=-1;
/* This is global rather than species-specific. */
int MusicPlayMode=1; /* repeat with random silences */
static DWORD RandomMusicSilenceDeadline=0;
static int RandomMusicSilenceActive=0;
static DWORD MusicRepeatRetryDeadline=0;
/* Next local music-state transition; never an MCI polling cadence. */
static DWORD NextMusicActionTick=0;
static int RandomMusicPlaybackObserved=0;
#if 0 /* Archived probe 140 telemetry: deliberately unavailable in releases. */
/* I11: automatic once-per-second telemetry. No hotkey, no re-open/close cycle. */
static FILE *Probe140I11MusicTrace=0;
static DWORD Probe140I11WindowStart=0;
static unsigned long Probe140I11DispatchCalls=0;
static unsigned long Probe140I11DeadlineGated=0;
static unsigned long Probe140I11FullDispatches=0;
static unsigned long Probe140I11LastMciCommands=0;
static unsigned long Probe140I11LastMciStatus=0;

static void Probe140I11LogMusicDispatch(DWORD now)
{
	int enabled, trackCount, playing, track, failureTrack, failureStage;
	unsigned long deadline, mciCommands, mciStatus, failureError;
	if (!Probe140I11WindowStart)
	{
		Probe140I11WindowStart=now;
		CDDA_GetDispatchDebugState(&enabled, &trackCount, &playing, &track, &deadline, &Probe140I11LastMciCommands, &Probe140I11LastMciStatus);
		Probe140I11MusicTrace=fopen("logs\\probe140_music_dispatch_state.log", "a");
		if (Probe140I11MusicTrace) { fprintf(Probe140I11MusicTrace, "I12 begin tick=%lu enabled=%d tracks=%d playing=%d track=%d deadline=%lu\n", (unsigned long)now, enabled, trackCount, playing, track, deadline); fflush(Probe140I11MusicTrace); }
		return;
	}
	if ((LONG)(now-Probe140I11WindowStart)<1000) return;
	CDDA_GetDispatchDebugState(&enabled, &trackCount, &playing, &track, &deadline, &mciCommands, &mciStatus);
	CDDA_GetLastStartFailure(&failureTrack, &failureStage, &failureError);
	if (Probe140I11MusicTrace) { fprintf(Probe140I11MusicTrace, "I12 tick=%lu calls=%lu gated=%lu full=%lu enabled=%d tracks=%d playing=%d track=%d deadline=%lu mci_commands_delta=%lu mci_status_delta=%lu failure_track=%d failure_stage=%d failure_error=%lu\n", (unsigned long)now, Probe140I11DispatchCalls, Probe140I11DeadlineGated, Probe140I11FullDispatches, enabled, trackCount, playing, track, deadline, mciCommands-Probe140I11LastMciCommands, mciStatus-Probe140I11LastMciStatus, failureTrack, failureStage, failureError); fflush(Probe140I11MusicTrace); }
	Probe140I11WindowStart=now;
	Probe140I11DispatchCalls=0; Probe140I11DeadlineGated=0; Probe140I11FullDispatches=0;
	Probe140I11LastMciCommands=mciCommands; Probe140I11LastMciStatus=mciStatus;
}
#endif
extern "C"
{

void EmptyCDTrackList()
{
	for(int i=0;i<AVP_ENVIRONMENT_END_OF_LIST;i++)
	{
		while(LevelCDTracks[i].size()) LevelCDTracks[i].delete_first_entry();
	}

	for(int i=0;i<3;i++)
	{
		while(MultiplayerCDTracks[i].size()) MultiplayerCDTracks[i].delete_first_entry();
	}
}

#define CDTrackFileName "cd tracks.txt"


static void ExtractTracksForLevel(char* & buffer,List<int> & track_list)
{
	//search for a line starting with a #
	while(*buffer)
	{
		if(*buffer=='#') break;
		//search for next line
		while(*buffer)
		{
			if(*buffer=='\n')
			{
				buffer++;
				if(*buffer=='\r') buffer++;
				break;
			}
			buffer++;
		}

	}
	
	while(*buffer)
	{
		//search for a track number or comment
		if(*buffer==';')
		{
			//comment , so no further info on this line
			break;
		}
		else if(*buffer=='\n' || *buffer=='\r')
		{
			//reached end of line
			break;
		}
		else if(*buffer>='0' && *buffer<='9')
		{
			int track=-1;
			//find a number , add it to the list
			sscanf(buffer,"%d",&track);

			if(track>=0)
			{
				track_list.add_entry(track);
			}

			//skip to the next non numerical character
			while(*buffer>='0' && *buffer<='9') buffer++;
		}
		else
		{
			buffer++;
		}
	}

	//go to the next line
	while(*buffer)
	{
		if(*buffer=='\n')
		{
			buffer++;
			if(*buffer=='\r') buffer++;
			break;
		}
		buffer++;
	}
	
}

void LoadCDTrackList()
{
	//clear out the old list first
	EmptyCDTrackList();

	FILE *file = OpenGameFile(CDTrackFileName, FILEMODE_READONLY, FILETYPE_OPTIONAL);
	
	if(file==NULL)
	{
		LOGDXFMT(("Failed to open %s",CDTrackFileName));
		return;
	}

	char* buffer;
	int file_size;

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	rewind(file);
	
	//copy the file contents into a buffer
	buffer=new char[file_size+1];
	fread(buffer, 1, file_size, file);
	buffer[file_size]='\0';
	fclose(file);
	
	char* bufferptr=buffer;


	//first extract the multiplayer tracks
	for(int i=0;i<3;i++)
	{
		ExtractTracksForLevel(bufferptr,MultiplayerCDTracks[i]);
	}
	
	//now the level tracks
	for(int i=0 ;i<AVP_ENVIRONMENT_END_OF_LIST;i++)
	{
		ExtractTracksForLevel(bufferptr,LevelCDTracks[i]);
	}
	

	delete [] buffer;
}

static unsigned int TrackSelectCounter=0;

static BOOL PickCDTrack(List<int>& track_list)
{
	//make sure we have some tracks in the list
	if(!track_list.size()) return FALSE;

	//pick the next track in the list
	unsigned int index=TrackSelectCounter % track_list.size();

	TrackSelectCounter++;

	// A selection becomes current only after Red Book confirms the start.
	int requestedTrack=track_list[index];
	if(!CDDA_PlayVerified(requestedTrack))
	{
		CDDA_RandomMusicTracef("RML_SCHED_PICK_REJECTED track=%d list_size=%d", requestedTrack, track_list.size());
		return FALSE;
	}

	LastTrackChosen = requestedTrack;
	CDDA_RandomMusicTracef("RML_SCHED_PICK track=%d list_size=%d", LastTrackChosen, track_list.size());
	return TRUE;	
}


void CheckCDAndChooseTrackIfNeeded()
{
	static enum playertypes lastPlayerType;
	DWORD now=GetTickCount();
	DWORD playbackDeadline;
	int trackIsPlaying;
	/* I13: temporary automatic dispatcher logger disabled. */
	
#if 0 /* Archived measurement counter; not part of music scheduling. */
++Probe140I11DispatchCalls;
#endif
	if(NextMusicActionTick && (LONG)(now-NextMusicActionTick)<0)
	{
		
#if 0 /* Archived measurement counter; not part of music scheduling. */
++Probe140I11DeadlineGated;
#endif
		return;
	}
	
#if 0 /* Archived measurement counter; not part of music scheduling. */
++Probe140I11FullDispatches;
#endif
	NextMusicActionTick=0;
	if(!CDDA_IsOn()) return;
	trackIsPlaying=CDDA_IsPlaying();
	if(trackIsPlaying)
	{
		int activeTrack = CDDA_GetCurrentTrack();
		/* Capture tracks started by the original map bootstrap as well as by this selector. */
		if(activeTrack>0 && LastTrackChosen!=activeTrack)
		{
			LastTrackChosen=activeTrack;
			CDDA_RandomMusicTracef("RML_SCHED_CAPTURE_ACTIVE track=%d", LastTrackChosen);
		}
		/* Only schedule silence after the backend actually observed playback. */
		if(!RandomMusicPlaybackObserved)
		{
			CDDA_RandomMusicTracef("RML_SCHED_PLAYBACK_OBSERVED track=%d mode=%d", LastTrackChosen, MusicPlayMode);
		}
		RandomMusicPlaybackObserved=1;
		playbackDeadline=(DWORD)CDDA_GetPlaybackDeadline();
		NextMusicActionTick=playbackDeadline ? playbackDeadline : now+1000;
		if(AvP.Network == I_No_Network || AvP.PlayerType==lastPlayerType)
			return;
		if(MultiplayerCDTracks[AvP.PlayerType].contains(LastTrackChosen))
			return;
	}
	else if(RandomMusicPlaybackObserved)
	{
		/* A mode change is effective immediately, even while a random silence was pending. */
		if(MusicPlayMode<=0)
		{
			RandomMusicSilenceDeadline=0;
			RandomMusicSilenceActive=0;
			NextMusicActionTick=now+1000;
			return;
		}
		if(LastTrackChosen<0) return;

		if(MusicPlayMode>=2)
		{
			/* Classic loop: restart this map's already selected track without silence. */
			if(MusicRepeatRetryDeadline && (LONG)(GetTickCount()-MusicRepeatRetryDeadline)<0) return;
			MusicRepeatRetryDeadline=0;
			RandomMusicSilenceDeadline=0;
			RandomMusicSilenceActive=0;
			CDDA_RandomMusicTracef("RML_SCHED_ALWAYS_REPEAT track=%d", LastTrackChosen);
			if(CDDA_PlayVerified(LastTrackChosen))
			{
				RandomMusicPlaybackObserved=0;
				CDDA_RandomMusicTracef("RML_SCHED_REPEAT_ACCEPTED track=%d mode=always", LastTrackChosen);
			}
			else
			{
				/* Keep retrying the exact same track, but do not spin every frame. */
				MusicRepeatRetryDeadline=now+1000;
				NextMusicActionTick=MusicRepeatRetryDeadline;
				CDDA_RandomMusicTracef("RML_SCHED_REPEAT_RETRY track=%d mode=always delay_seconds=1", LastTrackChosen);
			}
			return;
		}

		if(!RandomMusicSilenceActive)
		{
			DWORD silenceSeconds=(DWORD)(5+(FastRandom()%296));
			/* Uniform, inclusive 5..300 seconds; only silence is random. */
			RandomMusicSilenceDeadline=now + silenceSeconds*1000;
			NextMusicActionTick=RandomMusicSilenceDeadline;
			RandomMusicSilenceActive=1;
			CDDA_RandomMusicTracef("RML_SCHED_SILENCE_BEGIN track=%d seconds=%lu", LastTrackChosen, (unsigned long)silenceSeconds);
			return;
		}
		if((LONG)(now-RandomMusicSilenceDeadline)<0) { NextMusicActionTick=RandomMusicSilenceDeadline; return; }
		CDDA_RandomMusicTracef("RML_SCHED_SILENCE_EXPIRED track=%d", LastTrackChosen);
		/* Do not discard the pending repeat unless Red Book playback has
		   actually started. Failed MCI starts stay retryable. */
		if(CDDA_PlayVerified(LastTrackChosen))
		{
			RandomMusicPlaybackObserved=0;
			RandomMusicSilenceDeadline=0;
			RandomMusicSilenceActive=0;
			CDDA_RandomMusicTracef("RML_SCHED_REPEAT_ACCEPTED track=%d mode=random", LastTrackChosen);
		}
		else
		{
			RandomMusicSilenceDeadline=now+1000;
			NextMusicActionTick=RandomMusicSilenceDeadline;
			CDDA_RandomMusicTracef("RML_SCHED_REPEAT_RETRY track=%d mode=random delay_seconds=1", LastTrackChosen);
		}
		return;
	}

	if(AvP.Network == I_No_Network)
	{
		int level=NumberForCurrentLevel();
		if(level>=0 && level<AVP_ENVIRONMENT_END_OF_LIST)
		{
			if(!PickCDTrack(LevelCDTracks[level]))
			{
				/* Never retry a failed synchronous MCI start on the next render frame. */
				NextMusicActionTick=now+1000;
			}
			return;
		}
	}
	lastPlayerType=AvP.PlayerType;
	if(!PickCDTrack(MultiplayerCDTracks[AvP.PlayerType]))
	{
		NextMusicActionTick=now+1000;
	}
}

void ResetCDPlayForLevel()
{
	//check the number of tracks available while we're at it
	CDDA_CheckNumberOfTracks();

	TrackSelectCounter=0;
	LastTrackChosen=-1;
	RandomMusicSilenceDeadline=0;
	RandomMusicSilenceActive=0;
	RandomMusicPlaybackObserved=0;
	NextMusicActionTick=0;
	CDDA_RandomMusicTracef("RML_SCHED_RESET_LEVEL");
	CDDA_Stop();
}

};




