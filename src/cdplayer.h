#ifndef CDPLAYER_H
#define CDPLAYER_H

void CDDA_Start();
void CDDA_Stop();
void CDDA_End();
void CheckCDVolume();
int CDDA_GetCurrentTrack(void);
unsigned long CDDA_GetPlaybackDeadline(void);
int CDDA_PlayVerified(int track);
void CDDA_RandomMusicTracef(const char *format, ...);
void CDDA_GetLastStartFailure(int *track, int *stage, unsigned long *error);

#endif
