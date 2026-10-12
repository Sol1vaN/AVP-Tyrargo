#include "tyrargo_instrumentation.h"
/* Windows Red Book CD-DA backend for SDL3 builds.
   SDL removed its CD API after SDL 1.2; the former SDL3 branch was a no-op. */
#include <windows.h>
#include <mmsystem.h>
#include <winioctl.h>
#include <ntddcdrm.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "win95/cd_player.h"
#include "cdplayer.h"

#define CDDA_NO_DEVICE ((MCIDEVICEID)0)
/* Original AvP music catalogue uses Red Book tracks 1 through 15. */
#define CDDA_REQUIRED_TRACK_COUNT 15

static MCIDEVICEID CDDAMciDevice = CDDA_NO_DEVICE;
static int CDDAEnabled = 0;
static int CDDAPlaying = 0;
static int CDDALooping = 0;
static int CDDATrack = 0;
static int CDDAVolume = CDDA_VOLUME_DEFAULT;
static DWORD CDDALastOpenAttempt = 0;
static char CDDADrive[3] = "";
/* Estimated wake-up for the music scheduler; actual EOF is worker-owned. */
static DWORD CDDATrackEndTick = 0;
static int CDDAPlaybackEnded = 1;
/* I11 diagnostic counters: read-only telemetry, no logging in this module. */
static unsigned long CDDAMciCommandCount = 0;
static unsigned long CDDAMciStatusCount = 0;
/* I12: last synchronous Red Book start result, read by diagnostics. */
static int CDDALastStartTrack = 0;
static int CDDALastStartFailureStage = 0;
static unsigned long CDDALastStartFailureError = 0;

int CDPlayerVolume = CDDA_VOLUME_DEFAULT;
int CDTrackMax = 0;

/* Probe 139 tracing is intentionally disabled after validation.  The call sites
   remain harmless no-ops so they can be re-enabled for a future diagnosis. */
void CDDA_RandomMusicTracef(const char *format, ...)
{
	(void)format;
}
static MCIERROR CDDA_SendCommand(MCIDEVICEID device, UINT command, DWORD_PTR flags, DWORD_PTR parameters)
{
#if TYRARGO_RENDER_TRACE
	++CDDAMciCommandCount;
	if (command == MCI_STATUS) ++CDDAMciStatusCount;
#endif
	return mciSendCommandA(device, command, flags, parameters);
}
static void CDDARecordStartFailure(int track, int stage, MCIERROR error)
{
	CDDALastStartTrack = track;
	CDDALastStartFailureStage = stage;
	CDDALastStartFailureError = (unsigned long)error;
}

static void CDDAClearStartFailure(int track)
{
	CDDALastStartTrack = track;
	CDDALastStartFailureStage = 0;
	CDDALastStartFailureError = 0;
}

/* A private SDL stream gives CD music its own gain. The former WinMM
   "Compact Disc" mixer control can alias a shared output on modern drivers.
   Raw sectors are read only by the worker; the gameplay status path is local. */
#define CDDA_SECTOR_BYTES 2352
#define CDDA_READ_SECTORS 16
#define CDDA_QUEUE_BYTES (44100 * 4 / 2)
static SDL_AudioStream *CDDAStream;
static HANDLE CDDARawDrive = INVALID_HANDLE_VALUE;
static HANDLE CDDAWorker, CDDAStopEvent, CDDAReadyEvent;
static volatile LONG CDDAWorkerEnded = 1, CDDAWorkerError;
static LONG CDDAFirstSector, CDDALastSector;
static int CDDAOwnAudio;

static int CDDARawIo(DWORD command, void *input, DWORD inputSize,
                     void *output, DWORD outputSize, DWORD *received)
{
    OVERLAPPED io;
    HANDLE events[2];
    DWORD error, wait;
    int ok;
    ZeroMemory(&io, sizeof io);
    io.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!io.hEvent) return 0;
    ok = DeviceIoControl(CDDARawDrive, command, input, inputSize,
                         output, outputSize, received, &io);
    error = ok ? ERROR_SUCCESS : GetLastError();
    if (!ok && error == ERROR_IO_PENDING) {
        events[0] = CDDAStopEvent; events[1] = io.hEvent;
        wait = WaitForMultipleObjects(2, events, FALSE, 5000);
        if (wait != WAIT_OBJECT_0 + 1) {
            /* Drain cancellation before releasing the OVERLAPPED or its buffer. */
            CancelIoEx(CDDARawDrive, &io);
            GetOverlappedResult(CDDARawDrive, &io, received, TRUE);
            error = wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : ERROR_OPERATION_ABORTED;
            ok = 0;
        } else {
            ok = GetOverlappedResult(CDDARawDrive, &io, received, FALSE);
            error = ok ? ERROR_SUCCESS : GetLastError();
        }
    }
    CloseHandle(io.hEvent);
    SetLastError(error);
    return ok;
}

static DWORD WINAPI CDDAPump(LPVOID unused)
{
    unsigned char data[CDDA_READ_SECTORS * CDDA_SECTOR_BYTES];
    LONG sector = CDDAFirstSector;
    int started = 0;
    (void)unused;
    while (WaitForSingleObject(CDDAStopEvent, 0) == WAIT_TIMEOUT) {
        int queued = SDL_GetAudioStreamQueued(CDDAStream);
        if (queued < 0) { InterlockedExchange(&CDDAWorkerError, ERROR_READ_FAULT); break; }
        if (sector >= CDDALastSector) {
            if (!queued) {
                /* The device may still hold its final callback buffer. */
                WaitForSingleObject(CDDAStopEvent, 100);
                break;
            }
            WaitForSingleObject(CDDAStopEvent, 10);
            continue;
        }
        if (queued >= CDDA_QUEUE_BYTES) { WaitForSingleObject(CDDAStopEvent, 10); continue; }
        {
            RAW_READ_INFO readInfo;
            DWORD received = 0, count = CDDALastSector - sector;
            if (count > CDDA_READ_SECTORS) count = CDDA_READ_SECTORS;
            ZeroMemory(&readInfo, sizeof readInfo);
            readInfo.DiskOffset.QuadPart = (LONGLONG)sector * 2048;
            readInfo.SectorCount = count;
            readInfo.TrackMode = CDDA;
            if (!CDDARawIo(IOCTL_CDROM_RAW_READ, &readInfo, sizeof readInfo,
                           data, count * CDDA_SECTOR_BYTES, &received) || received != count * CDDA_SECTOR_BYTES) {
                DWORD error = GetLastError();
                InterlockedExchange(&CDDAWorkerError, error ? error : ERROR_READ_FAULT);
                break;
            }
            /* Windows CDDA raw reads expose interleaved signed little-endian PCM. */
            if (!SDL_PutAudioStreamData(CDDAStream, data, received)) {
                InterlockedExchange(&CDDAWorkerError, ERROR_WRITE_FAULT); break;
            }
            sector += count;
            if (sector == CDDALastSector && !SDL_FlushAudioStream(CDDAStream)) {
                InterlockedExchange(&CDDAWorkerError, ERROR_WRITE_FAULT); break;
            }
            if (!started) { started = 1; SetEvent(CDDAReadyEvent); }
        }
    }
    InterlockedExchange(&CDDAWorkerEnded, 1);
    SetEvent(CDDAReadyEvent);
    return 0;
}

static void CDDAStopStream(void)
{
    if (CDDAStopEvent) SetEvent(CDDAStopEvent);
    if (CDDAWorker) {
        WaitForSingleObject(CDDAWorker, INFINITE);
        CloseHandle(CDDAWorker); CDDAWorker = NULL;
    }
    if (CDDAStream) { SDL_DestroyAudioStream(CDDAStream); CDDAStream = NULL; }
    if (CDDARawDrive != INVALID_HANDLE_VALUE) { CloseHandle(CDDARawDrive); CDDARawDrive = INVALID_HANDLE_VALUE; }
    if (CDDAReadyEvent) { CloseHandle(CDDAReadyEvent); CDDAReadyEvent = NULL; }
    if (CDDAStopEvent) { CloseHandle(CDDAStopEvent); CDDAStopEvent = NULL; }
    InterlockedExchange(&CDDAWorkerEnded, 1);
}

static LONG CDDASector(const TRACK_DATA *track)
{
    return ((track->Address[1] * 60 + track->Address[2]) * 75 + track->Address[3]) - 150;
}

static int CDDAStartStream(int track, DWORD *duration)
{
    char path[] = "\\\\.\\X:";
    CDROM_TOC toc;
    SDL_AudioSpec spec = {SDL_AUDIO_S16LE, 2, 44100};
    DWORD received, error = ERROR_GEN_FAILURE;
    int index;
    CDDAStopStream();
    InterlockedExchange(&CDDAWorkerError, 0);
    CDDAStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    CDDAReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!CDDAStopEvent || !CDDAReadyEvent) { error = GetLastError(); goto fail; }
    path[4] = CDDADrive[0];
    CDDARawDrive = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (CDDARawDrive == INVALID_HANDLE_VALUE) { error = GetLastError(); goto fail; }
    ZeroMemory(&toc, sizeof toc);
    if (!CDDARawIo(IOCTL_CDROM_READ_TOC, NULL, 0, &toc, sizeof toc, &received)) { error = GetLastError(); goto fail; }
    index = track - toc.FirstTrack;
    if (toc.FirstTrack < 1 || toc.LastTrack > 99 || track < toc.FirstTrack || track > toc.LastTrack ||
        received < 4 + (DWORD)(toc.LastTrack - toc.FirstTrack + 2) * sizeof(TRACK_DATA) ||
        (toc.TrackData[index].Control & 4)) { error = ERROR_INVALID_DATA; goto fail; }
    CDDAFirstSector = CDDASector(&toc.TrackData[index]);
    CDDALastSector = CDDASector(&toc.TrackData[index + 1]);
    if (CDDAFirstSector < 0 || CDDALastSector <= CDDAFirstSector) { error = ERROR_INVALID_DATA; goto fail; }
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) goto fail;
        CDDAOwnAudio = 1;
    }
    CDDAStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!CDDAStream || !SDL_SetAudioStreamGain(CDDAStream, CDDAVolume / (float)CDDA_VOLUME_MAX)) goto fail;
    InterlockedExchange(&CDDAWorkerEnded, 0);
    CDDAWorker = CreateThread(NULL, 0, CDDAPump, NULL, 0, NULL);
    if (!CDDAWorker) { error = GetLastError(); goto fail; }
    if (WaitForSingleObject(CDDAReadyEvent, 6000) != WAIT_OBJECT_0) { error = ERROR_TIMEOUT; goto fail; }
    error = (DWORD)InterlockedCompareExchange(&CDDAWorkerError, 0, 0);
    if (error || InterlockedCompareExchange(&CDDAWorkerEnded, 0, 0)) { if (!error) error = ERROR_READ_FAULT; goto fail; }
    if (!SDL_ResumeAudioStreamDevice(CDDAStream)) { error = ERROR_GEN_FAILURE; goto fail; }
    *duration = (DWORD)(((LONGLONG)(CDDALastSector - CDDAFirstSector) * 1000 + 74) / 75);
    return 1;
fail:
    CDDAStopStream();
    SetLastError(error);
    return 0;
}

static void CDDAApplyMusicVolume(int volume)
{
    if (CDDAStream) SDL_SetAudioStreamGain(CDDAStream, volume / (float)CDDA_VOLUME_MAX);
}
static int CDDAEnsureDevice(void)
{
	DWORD driveMask;
	int index;
	MCIERROR lastError = 0;
	DWORD now;

	if (CDDAMciDevice)
	{
		return 1;
	}

	now = timeGetTime();
	if (CDDALastOpenAttempt && (now - CDDALastOpenAttempt) < 1000)
	{
		CDDA_RandomMusicTracef("RML_MCI_OPEN_THROTTLED");
		return 0;
	}
	CDDALastOpenAttempt = now;
	CDDA_RandomMusicTracef("RML_MCI_OPEN_BEGIN");

	driveMask = GetLogicalDrives();
	for (index = 0; index < 26; ++index)
	{
		char root[4];
		char element[3];
		MCI_OPEN_PARMSA openParms;
		MCIERROR error;

		if (!(driveMask & (1UL << index))) continue;
		root[0] = (char)('A' + index);
		root[1] = ':';
		root[2] = 92;
		root[3] = 0;
		if (GetDriveTypeA(root) != DRIVE_CDROM) continue;

		element[0] = root[0];
		element[1] = ':';
		element[2] = 0;
		ZeroMemory(&openParms, sizeof(openParms));
		openParms.lpstrDeviceType = "cdaudio";
		openParms.lpstrElementName = element;
		error = CDDA_SendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&openParms);
		if (!error)
		{
			MCI_STATUS_PARMS statusParms;
			ZeroMemory(&statusParms, sizeof(statusParms));
			statusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
			error = CDDA_SendCommand(openParms.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&statusParms);
			if (!error && (int)statusParms.dwReturn >= CDDA_REQUIRED_TRACK_COUNT)
			{
				CDDAMciDevice = openParms.wDeviceID;
				CDDADrive[0] = element[0];
				CDDADrive[1] = ':';
				CDDADrive[2] = 0;
				CDDA_RandomMusicTracef("RML_MCI_OPEN_OK drive=%s device=%lu tracks=%lu", CDDADrive, (unsigned long)CDDAMciDevice, (unsigned long)statusParms.dwReturn);
				return 1;
			}
			/* This is a data/foreign disc, not the complete AvP Red Book catalogue. */
			CDDA_SendCommand(openParms.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
			CDDA_RandomMusicTracef("RML_MCI_OPEN_REJECTED drive=%s tracks=%lu error=%lu", element, (unsigned long)statusParms.dwReturn, (unsigned long)error);
			lastError = error;
			continue;
		}
		CDDA_RandomMusicTracef("RML_MCI_OPEN_FAIL drive=%s error=%lu", element, (unsigned long)error);
		lastError = error;
	}
	CDDA_RandomMusicTracef("RML_MCI_OPEN_UNAVAILABLE last_error=%lu", (unsigned long)lastError);
	return 0;
}
/* Forget a device that has reached an unusable MCI state, without disabling
   CD audio globally. A later start can then open the drive afresh. */
static void CDDAReleaseDevice(void)
{
	CDDAStopStream();
	if (CDDAMciDevice != CDDA_NO_DEVICE)
	{
		CDDA_SendCommand(CDDAMciDevice, MCI_CLOSE, MCI_WAIT, 0);
		CDDAMciDevice = CDDA_NO_DEVICE;
	}
	CDDADrive[0] = 0;
	CDDATrackEndTick = 0;
	CDDAPlaybackEnded = 1;
	CDDAPlaying = 0;
	CDTrackMax = 0;
	/* Recovery is allowed immediately; do not inherit the normal open throttle. */
	CDDALastOpenAttempt = 0;
}

static int CDDAGetTrackCount(void)
{
	MCI_STATUS_PARMS statusParms;
	MCIERROR error;
	if (!CDDAEnsureDevice()) return 0;
	ZeroMemory(&statusParms, sizeof(statusParms));
	statusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	error = CDDA_SendCommand(CDDAMciDevice, MCI_STATUS, MCI_STATUS_ITEM,
		(DWORD_PTR)&statusParms);
	if (error)
	{
		return 0;
	}
	return (int)statusParms.dwReturn;
}

static int CDDAStartTrack(int track)
{
    DWORD duration;
    CDDAClearStartFailure(track);
    if (!CDDAEnsureDevice()) { CDDARecordStartFailure(track, 1, 0); return 0; }
    if (track <= 0 || track > CDTrackMax) { CDDARecordStartFailure(track, 2, 0); return 0; }
    if (!CDDAStartStream(track, &duration)) {
        CDDARecordStartFailure(track, 6, GetLastError()); return 0;
    }
    CDDATrackEndTick = timeGetTime() + duration + 500;
    CDDATrack = track;
    CDDAPlaybackEnded = 0;
    CDDAPlaying = 1;
    return 1;
}

/* Retry an explicit start once after reopening the catalogue device. */
static int CDDAStartTrackWithRecovery(int track)
{
	if (CDDAStartTrack(track))
	{
		CDDA_RandomMusicTracef("RML_MCI_PRIMARY_OK track=%d", track);
		return 1;
	}

	CDDA_RandomMusicTracef("RML_MCI_PRIMARY_FAIL track=%d recovery=reopen", track);
	CDDAReleaseDevice();
	if (!CDDAEnsureDevice())
	{
		CDDA_RandomMusicTracef("RML_MCI_RECOVERY_FAIL stage=open track=%d", track);
		return 0;
	}
	CDTrackMax = CDDAGetTrackCount();
	if (track <= 0 || track > CDTrackMax)
	{
		CDDA_RandomMusicTracef("RML_MCI_RECOVERY_FAIL stage=track_range track=%d max=%d", track, CDTrackMax);
		return 0;
	}
	if (CDDAStartTrack(track))
	{
		CDDA_RandomMusicTracef("RML_MCI_RECOVERY_OK track=%d", track);
		return 1;
	}
	CDDA_RandomMusicTracef("RML_MCI_RECOVERY_FAIL stage=play track=%d", track);
	return 0;
}

void CDDA_Start(void)
{
	CDDA_RandomMusicTracef("RML_RUN_START");
	CDDAEnabled = 1;
	CDDAPlaying = 0;
	CDDALooping = 0;
	CDDATrack = 0;
	CDDATrackEndTick = 0;
	CDDAPlaybackEnded = 1;
	CDDAVolume = CDPlayerVolume;
	CDDA_CheckNumberOfTracks();
	CDDAApplyMusicVolume(CDDAVolume);
}

void CDDA_End(void)
{
	CDDA_Stop();
	CDDAReleaseDevice();
	CDDAEnabled = 0;
	CDTrackMax = 0;
	if (CDDAOwnAudio) { SDL_QuitSubSystem(SDL_INIT_AUDIO); CDDAOwnAudio = 0; }
}

void CDDA_Management(void)
{
	(void)CDDA_IsPlaying();
}

int CDDA_GetCurrentTrack(void)
{
	return CDDATrack;
}

unsigned long CDDA_GetPlaybackDeadline(void)
{
	/* Purely local: callers may use this from the render loop safely. */
	if (!CDDAPlaying || CDDAPlaybackEnded) return 0;
	return (unsigned long)CDDATrackEndTick;
}

void CDDA_GetLastStartFailure(int *track, int *stage, unsigned long *error)
{
	if (track) *track = CDDALastStartTrack;
	if (stage) *stage = CDDALastStartFailureStage;
	if (error) *error = CDDALastStartFailureError;
}

void CDDA_GetDispatchDebugState(int *enabled, int *trackCount, int *playing, int *track,
	unsigned long *deadline, unsigned long *mciCommands, unsigned long *mciStatus)
{
	if (enabled) *enabled = CDDAEnabled;
	if (trackCount) *trackCount = CDTrackMax;
	if (playing) *playing = CDDAPlaying && !CDDAPlaybackEnded;
	if (track) *track = CDDATrack;
	if (deadline) *deadline = (unsigned long)CDDATrackEndTick;
	if (mciCommands) *mciCommands = CDDAMciCommandCount;
	if (mciStatus) *mciStatus = CDDAMciStatusCount;
}

void CheckCDVolume(void)
{
	if (CDPlayerVolume != CDDAVolume) CDDA_ChangeVolume(CDPlayerVolume);
}

void CDDA_ChangeVolume(int volume)
{
	if (volume < CDDA_VOLUME_MIN) volume = CDDA_VOLUME_MIN;
	if (volume > CDDA_VOLUME_MAX) volume = CDDA_VOLUME_MAX;

	CDPlayerVolume = volume;
	CDDAVolume = volume;

	/* Only this music stream: never the device gain, session, or SFX listener. */
	CDDAApplyMusicVolume(volume);
}
int CDDA_GetCurrentVolumeSetting(void)
{
	return CDDAVolume;
}

void CDDA_SwitchOn(void)
{
	CDDAEnabled = 1;
	CDDA_CheckNumberOfTracks();
}

void CDDA_SwitchOff(void)
{
	CDDA_Stop();
	CDDAEnabled = 0;
}

int CDDA_CheckNumberOfTracks(void)
{
	CDTrackMax = CDDAGetTrackCount();
	/* A compatible device is admitted only after the 15-track catalog check. */
	if (CDTrackMax < CDDA_REQUIRED_TRACK_COUNT)
	{
		CDTrackMax = 0;
		return 0;
	}
	return CDTrackMax;
}

int CDDA_IsOn(void)
{
	/* This is queried from the gameplay loop. Device discovery and MCI_STATUS
	   must happen only at startup, explicit enable, or level reset; never here. */
	return CDDAEnabled && CDTrackMax > 0;
}

int CDDA_IsPlaying(void)
{
    if (!CDDAEnabled || !CDDAPlaying || CDDAPlaybackEnded) return 0;
    /* No drive calls here. EOF comes from the worker after its queue drains. */
    if (!InterlockedCompareExchange(&CDDAWorkerEnded, 0, 0)) return 1;
    CDDATrackEndTick = 0;
    CDDAPlaybackEnded = 1;
    CDDAPlaying = 0;
    if (CDDALooping && CDDATrack > 0) CDDAPlaying = CDDAStartTrackWithRecovery(CDDATrack);
    return CDDAPlaying;
}
int CDDA_PlayVerified(int track)
{
	int started;
	CDDA_RandomMusicTracef("RML_REPEAT_REQUEST track=%d enabled=%d max=%d", track, CDDAEnabled, CDTrackMax);
	if (!CDDA_IsOn())
	{
		CDDA_RandomMusicTracef("RML_REPEAT_ABORT reason=cdda_off track=%d", track);
		return 0;
	}
	CDDA_Stop();
	CDDALooping = 0;
	started = CDDAStartTrackWithRecovery(track);
	CDDA_RandomMusicTracef("RML_REPEAT_RESULT track=%d started=%d", track, started);
	return started;
}

void CDDA_Play(int track)
{
	(void)CDDA_PlayVerified(track);
}

void CDDA_PlayLoop(int track)
{
	if (!CDDA_IsOn()) return;
	CDDA_Stop();
	CDDALooping = 1;
	CDDAStartTrackWithRecovery(track);
}

void CDDA_Stop(void)
{
    CDDAStopStream();
    CDDATrackEndTick = 0;
    CDDAPlaybackEnded = 1;
    CDDAPlaying = 0;
    CDDALooping = 0;
}
