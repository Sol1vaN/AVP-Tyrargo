#ifndef AVP_AUDIO_LATENCY_H
#define AVP_AUDIO_LATENCY_H
#ifdef __cplusplus
extern "C" {
#endif
#include "event_latency.h"
#if TYRARGO_MEASURE_LATENCY
/* Main/game thread only. Record never allocates or performs I/O. */
unsigned long long AudioLatencyNow(void);
void AudioLatencyRecord(unsigned long long start,const char *stage,int sound,const char *asset);
/* Call only at loading/shutdown boundaries, never from gameplay playback. */
void AudioLatencyFlush(void);
void AudioPlaybackDrop(int sound,const char *reason);
#else
#define AudioLatencyNow() 0ULL
#define AudioLatencyRecord(...) ((void)0)
#define AudioPlaybackDrop(...) ((void)0)
#define AudioLatencyFlush() ((void)0)
#endif
#ifdef __cplusplus
}
#endif
#endif
