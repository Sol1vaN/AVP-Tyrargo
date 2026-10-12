#include "audio_latency.h"
#if TYRARGO_MEASURE_LATENCY
#include "audio_latency.h"
#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <string.h>
#include "files.h"
#include "tyrargo_version.h"
#define LATENCY_CAPACITY 128
typedef struct {
 unsigned long long timestamp,elapsed;
 int sound;
 char stage[40],asset[160];
} LATENCY_EVENT;
static LATENCY_EVENT events[LATENCY_CAPACITY];
static unsigned int count,dropped;
#define DROP_CAPACITY 64
typedef struct {int sound;unsigned int count;char reason[40];} DROP_EVENT;
static DROP_EVENT drops[DROP_CAPACITY];
static unsigned int dropCount,dropOverflow;
void AudioPlaybackDrop(int sound,const char *reason)
{
 unsigned int i;
 for(i=0;i<dropCount;i++)if(drops[i].sound==sound && !strcmp(drops[i].reason,reason)){
  drops[i].count++;return;
 }
 if(dropCount==DROP_CAPACITY){dropOverflow++;return;}
 drops[dropCount].sound=sound;drops[dropCount].count=1;
 snprintf(drops[dropCount].reason,sizeof(drops[dropCount].reason),"%s",reason);
 dropCount++;
}
unsigned long long AudioLatencyNow(void){(void)EventLatencySession();return SDL_GetTicksNS();}
void AudioLatencyRecord(unsigned long long start,const char *stage,int sound,const char *asset)
{
 unsigned long long now=AudioLatencyNow(),elapsed=now-start;
 LATENCY_EVENT *e;
 if(elapsed<20000000ULL)return;
 if(count==LATENCY_CAPACITY){dropped++;return;}
 e=&events[count++];e->timestamp=now;e->elapsed=elapsed;e->sound=sound;
 snprintf(e->stage,sizeof(e->stage),"%s",stage?stage:"");
 snprintf(e->asset,sizeof(e->asset),"%s",asset?asset:"");
}
void AudioLatencyFlush(void)
{
 FILE *f;unsigned int i;
 if(!count && !dropCount && !dropOverflow)return;
 f=OpenGameFile("sfx_latency.log",FILEMODE_APPEND,FILETYPE_CONFIG);
 if(!f)return;
 fprintf(f,"\nAVP: Tyrargo audio latency %s (>20ms), events=%u dropped=%u\nsession %s\n",TYRARGO_VERSION,count,dropped,EventLatencySession());
 for(i=0;i<count;i++)fprintf(f,"t=%.3fs %s %.3fms sound=%d %s\n",
  events[i].timestamp/1e9,events[i].stage,events[i].elapsed/1e6,events[i].sound,events[i].asset);
 for(i=0;i<dropCount;i++)fprintf(f,"playback rejected sound=%d count=%u reason=%s\n",
  drops[i].sound,drops[i].count,drops[i].reason);
 if(dropOverflow)fprintf(f,"playback rejected unclassified=%u\n",dropOverflow);
 fclose(f);count=0;dropped=0;dropCount=0;dropOverflow=0;
}

#endif
