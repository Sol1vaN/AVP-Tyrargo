#include "event_latency.h"
#if TYRARGO_MEASURE_LATENCY
/* Main-thread CPU wall times, inclusive (nested durations must not be added).
 * No allocations, GL synchronization, disk writes or sound calls while playing.
 * Keep slow frames plus nearby event context; frame pacing is measured separately. */
#include "event_latency.h"
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_init.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "files.h"
#include "audio_latency.h"
#include "tyrargo_version.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif
/* UTC start + process ID + precise start ticks identify one process execution.
 * Repeated flushes append under the same session; SDL relative times remain useful. */
const char *EventLatencySession(void){
 static char session[144];
 if(!session[0]){
#ifdef _WIN32
  FILETIME ft;SYSTEMTIME utc;ULARGE_INTEGER ticks;
  GetSystemTimeAsFileTime(&ft);FileTimeToSystemTime(&ft,&utc);
  ticks.LowPart=ft.dwLowDateTime;ticks.HighPart=ft.dwHighDateTime;
  snprintf(session,sizeof session,"utc=%04u-%02u-%02uT%02u:%02u:%02u.%03uZ pid=%lu run=%016llx",
   utc.wYear,utc.wMonth,utc.wDay,utc.wHour,utc.wMinute,utc.wSecond,utc.wMilliseconds,
   (unsigned long)GetCurrentProcessId(),(unsigned long long)ticks.QuadPart);
#else
  time_t now=time(0);struct tm *utc=gmtime(&now);char stamp[32]="unavailable";
  if(utc)strftime(stamp,sizeof stamp,"%Y-%m-%dT%H:%M:%SZ",utc);
  snprintf(session,sizeof session,"utc=%s run=%016llx",stamp,SDL_GetTicksNS());
#endif
 }
 return session;
}
#define FRAME_EVENTS 128
#define HISTORY_EVENTS 8192
typedef struct {
 EVENT_TICK workingSet,privateBytes,pageFaults,readBytes,writeBytes;
 int memoryValid,ioValid;
} PROCESS_SAMPLE;
static void readProcessSample(PROCESS_SAMPLE *sample){
 memset(sample,0,sizeof(*sample));
#ifdef _WIN32
 PROCESS_MEMORY_COUNTERS_EX memory;IO_COUNTERS io;
 memset(&memory,0,sizeof(memory));memory.cb=sizeof(memory);
 if(K32GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&memory,sizeof(memory))){
  sample->memoryValid=1;sample->workingSet=memory.WorkingSetSize;
  sample->privateBytes=memory.PrivateUsage;sample->pageFaults=memory.PageFaultCount;
 }
 if(GetProcessIoCounters(GetCurrentProcess(),&io)){
  sample->ioValid=1;sample->readBytes=io.ReadTransferCount;sample->writeBytes=io.WriteTransferCount;
 }
#endif
}
typedef struct {
 EVENT_TICK timestamp,elapsed,frame,id;
 int module,type,message;
 int playerX,playerY,playerZ,weapon,playerValid;
 EVENT_TICK cpuNS;
 int cpuValid,isFrame;
 int focused,minimized,menu,selection;
 PROCESS_SAMPLE process;
 EVENT_TICK faultsDelta,readsDelta,writesDelta;
 int deltaValid,ioDeltaValid;
 unsigned work[EL_WORK_COUNT];
 char stage[36],resource[160],map[64];
} EVENT_RECORD;
static EVENT_RECORD pending[FRAME_EVENTS],recent[FRAME_EVENTS],history[HISTORY_EVENTS];
static unsigned recentCount;
static EVENT_TICK recentFrame;
static unsigned count,writeIndex,stored,overwritten,pendingDropped,sessionDropped;
static EVENT_TICK frameNumber,frameStart;
static int active,marked,frameModule,hadFrames;
static char frameMap[64];
static unsigned frameWork[EL_WORK_COUNT];
static int playerX,playerY,playerZ,playerWeapon,playerValid;
void EventLatencyPlayer(int x,int y,int z,int weapon){
 if(!SDL_IsMainThread() || !active)return;
 playerX=x;playerY=y;playerZ=z;playerWeapon=weapon;playerValid=1;
}
static EVENT_TICK frameCPU;
static int frameCPUValid;
static PROCESS_SAMPLE frameProcess;
static int focused=-1,minimized=-1,frameMenu=-1,frameSelection=-1;
void EventLatencyWindowState(int focus,int mini){if(SDL_IsMainThread()){focused=focus;minimized=mini;}}
void EventLatencyMenu(int menu,int selection){if(SDL_IsMainThread()){frameMenu=menu;frameSelection=selection;}}
static int readMainThreadCPU(EVENT_TICK *ns){
#ifdef _WIN32
 FILETIME creation,exitTime,kernel,user;
 ULARGE_INTEGER k,u;
 if(!GetThreadTimes(GetCurrentThread(),&creation,&exitTime,&kernel,&user))return 0;
 k.LowPart=kernel.dwLowDateTime;k.HighPart=kernel.dwHighDateTime;
 u.LowPart=user.dwLowDateTime;u.HighPart=user.dwHighDateTime;
 *ns=(k.QuadPart+u.QuadPart)*100ULL;return 1;
#else
 (void)ns;return 0;
#endif
}
void EventLatencyCount(enum EVENT_WORK counter,unsigned amount){
 if(!SDL_IsMainThread() || !active || counter<0 || counter>=EL_WORK_COUNT)return;
 frameWork[counter]+=amount;
}
static void copyText(char *dst,unsigned size,const char *src){
 unsigned n=0;if(src)while(n+1<size && src[n]){dst[n]=src[n];n++;}dst[n]=0;
}
EVENT_TICK EventLatencyBegin(void){
 if(!SDL_IsMainThread() || !active)return 0;
 return SDL_GetTicksNS();
}
EVENT_TICK EventLatencyId(const char *name){
 EVENT_TICK id=0;if(name)memcpy(&id,name,8);return id;
}
void EventLatencyEnd(EVENT_TICK start,const char *stage,int type,int message,EVENT_TICK id,const char *resource,int marker){
 EVENT_TICK now;EVENT_RECORD *e;
 if(!start || !SDL_IsMainThread() || !active)return;
 now=SDL_GetTicksNS();
 if(marker>marked)marked=marker;
 if(!marker && now-start<2000000ULL)return;
 if(count==FRAME_EVENTS){pendingDropped++;return;}
 e=&pending[count++];memset(e,0,sizeof(*e));e->timestamp=now;e->elapsed=now-start;e->frame=frameNumber;e->id=id;
 e->module=frameModule;e->type=type;e->message=message;
 e->playerX=playerX;e->playerY=playerY;e->playerZ=playerZ;e->weapon=playerWeapon;e->playerValid=playerValid;
 e->cpuNS=0;e->cpuValid=e->isFrame=0;
 copyText(e->stage,sizeof e->stage,stage);copyText(e->resource,sizeof e->resource,resource);
 copyText(e->map,sizeof e->map,frameMap);
 e->focused=focused;e->minimized=minimized;e->menu=frameMenu;e->selection=frameSelection;
 if(marker>=2)readProcessSample(&e->process);
}
void EventLatencyNote(const char *stage,int type,int message,const char *resource){
 EventLatencyEnd(EventLatencyBegin(),stage,type,message,0,resource,2);
}
void EventLatencyFrameBegin(const char *map,int module){
 if(!SDL_IsMainThread())return;
 /* A loop must finish its frame before another loop takes ownership. */
 if(active)EventLatencyFrameEnd();
 (void)EventLatencySession();playerValid=0;
 active=1;count=marked=pendingDropped=0;frameModule=module;frameMenu=frameSelection=-1;
 copyText(frameMap,sizeof frameMap,map);frameNumber++;frameStart=SDL_GetTicksNS();hadFrames=1;
 memset(frameWork,0,sizeof frameWork);frameCPUValid=readMainThreadCPU(&frameCPU);
 readProcessSample(&frameProcess);
}
static void appendRecords(const EVENT_RECORD *records,unsigned n){
 unsigned i;
 for(i=0;i<n;i++){
  history[writeIndex]=records[i];writeIndex=(writeIndex+1)%HISTORY_EVENTS;
  if(stored<HISTORY_EVENTS)stored++;else overwritten++;
 }
}
void EventLatencyFrameEnd(void){
 EVENT_TICK elapsed,cpuEnd=0;
 int cpuEndValid;
 if(!SDL_IsMainThread() || !active)return;
 elapsed=SDL_GetTicksNS()-frameStart;
 cpuEndValid=readMainThreadCPU(&cpuEnd);
 /* Capture 50-72 FPS regressions too: their 14-20ms frames escaped 25ms.
  * Keep frontend/unfocused capture at the original threshold. */
 if(elapsed>=25000000ULL || marked>=2 || (playerValid && focused==1 && frameMenu<0 && elapsed>=12500000ULL)){
  /* Keep a recent spawn/trigger even when its first costly draw is next frame.
   * Fast recurring triggers cannot evict a captured hitch from history. */
  if(recentCount && frameNumber-recentFrame<=2)appendRecords(recent,recentCount);
  recentCount=0;
  if(count==FRAME_EVENTS){count--;pendingDropped++;}
  EventLatencyEnd(frameStart,"frame incl presentation/pacing",0,0,0,0,1);
  pending[count-1].isFrame=1;
  pending[count-1].cpuValid=frameCPUValid && cpuEndValid && cpuEnd>=frameCPU;
  if(pending[count-1].cpuValid)pending[count-1].cpuNS=cpuEnd-frameCPU;
  memcpy(pending[count-1].work,frameWork,sizeof frameWork);
  readProcessSample(&pending[count-1].process);
  if(frameProcess.memoryValid && pending[count-1].process.memoryValid){
   pending[count-1].deltaValid=1;
   /* Windows counts both soft and hard page faults; a fault is not proof of disk paging. */
   pending[count-1].faultsDelta=(unsigned)(pending[count-1].process.pageFaults-frameProcess.pageFaults);
  }
  if(frameProcess.ioValid && pending[count-1].process.ioValid){
   pending[count-1].ioDeltaValid=1;
   pending[count-1].readsDelta=pending[count-1].process.readBytes-frameProcess.readBytes;
   pending[count-1].writesDelta=pending[count-1].process.writeBytes-frameProcess.writeBytes;
  }
  appendRecords(pending,count);sessionDropped+=pendingDropped;
 }else if(marked){
  memcpy(recent,pending,count*sizeof(*recent));recentCount=count;recentFrame=frameNumber;
 }
 active=0;
}
void EventLatencyShutdown(void){if(!SDL_IsMainThread())return;EventLatencyFrameEnd();EventLatencyFlush();AudioLatencyFlush();}
/* Explicit modal-load checkpoints; ordinary gameplay recording stays in memory. */
void EventLatencyLoadCheckpoint(const char *stage,int detail){
 FILE *f;
 if(!SDL_IsMainThread())return;
 f=OpenGameFile("load_progress.log",FILEMODE_APPEND,FILETYPE_CONFIG);
 if(!f)return;
 fprintf(f,"%s %s t=%.6f stage=%s detail=%d\n",TYRARGO_DISPLAY_VERSION,
  EventLatencySession(),(double)SDL_GetTicksNS()/1000000000.0,stage,detail);
 fclose(f);
}
void EventLatencyFlush(void){
 FILE *f;unsigned i,first;
 if(!SDL_IsMainThread() || active || !hadFrames)return;
 f=OpenGameFile("event_latency.log",FILEMODE_APPEND,FILETYPE_CONFIG);
 if(!f)return;
 fprintf(f,"\nAVP: Tyrargo event latency %s | records=%u overwritten=%u omitted=%u\n",
  TYRARGO_VERSION,stored,overwritten,sessionDropped);
 fprintf(f,"session %s\n",EventLatencySession());
 fprintf(f,"Inclusive CPU wall times, NOT GPU timings. Nested times overlap. Present/pacing can include VSync/limiter waits. CPU below is MAIN THREAD only (coarse OS accounting), not GPU time. Counts are submitted workload, not visible pixels. Page faults include soft/hard faults; process I/O totals are not exclusively disk I/O. Focus/minimization and menu IDs identify intentional waits. No disk writes during frames.\n");
 first=(writeIndex+HISTORY_EVENTS-stored)%HISTORY_EVENTS;
 for(i=0;i<stored;i++){
  const EVENT_RECORD *e=&history[(first+i)%HISTORY_EVENTS];
  fprintf(f,"t=%.3f frame=%llu map=%s module=%d | %s %.3fms type=%d message=%d id=%016llx %s\n",
   e->timestamp/1e9,e->frame,e->map,e->module,e->stage,e->elapsed/1e6,e->type,e->message,e->id,e->resource);
  fprintf(f,"  focused=%d minimized=%d menu=%d selection=%d\n",e->focused,e->minimized,e->menu,e->selection);
  if(e->isFrame && e->playerValid)fprintf(f,"  player_mm=(%d,%d,%d) weapon=%d\n",e->playerX,e->playerY,e->playerZ,e->weapon);
  if(e->process.memoryValid)fprintf(f,"  working_set_MiB=%.3f private_MiB=%.3f page_faults_total=%llu\n",e->process.workingSet/1048576.0,e->process.privateBytes/1048576.0,e->process.pageFaults);
  if(e->deltaValid)fprintf(f,"  frame_page_faults=%llu\n",e->faultsDelta);
  if(e->ioDeltaValid)fprintf(f,"  process_read_delta_bytes=%llu process_write_delta_bytes=%llu\n",e->readsDelta,e->writesDelta);
  if(e->isFrame){
   if(e->cpuValid)fprintf(f,"  main_thread_cpu=%.3fms",e->cpuNS/1e6);
   else fprintf(f,"  main_thread_cpu=unavailable");
   fprintf(f," sectors=%u extended=%u objects=%u sections=%u shape_points=%u shape_polygons=%u legacy_batches=%u legacy_triangles=%u\n",
    e->work[EL_MODULES],e->work[EL_EXTENDED_MODULES],e->work[EL_OBJECTS],e->work[EL_MODEL_SECTIONS],
    e->work[EL_SHAPE_POINTS],e->work[EL_SHAPE_POLYGONS],e->work[EL_BATCHES],e->work[EL_TRIANGLES]);
  }
 }
 fclose(f);stored=writeIndex=overwritten=sessionDropped=hadFrames=recentCount=0;
}

#endif
