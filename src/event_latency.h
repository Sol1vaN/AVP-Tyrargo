#ifndef TYRARGO_EVENT_LATENCY_H
#define TYRARGO_EVENT_LATENCY_H
#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned long long EVENT_TICK;
enum EVENT_WORK {
 EL_MODULES, EL_EXTENDED_MODULES, EL_OBJECTS, EL_MODEL_SECTIONS,
 EL_SHAPE_POINTS, EL_SHAPE_POLYGONS, EL_BATCHES, EL_TRIANGLES,
 EL_WORK_COUNT
};
/* Probe 226: bounded in-memory hitch diagnostics, including frontend/focus. */
#define TYRARGO_MEASURE_LATENCY 1
#if TYRARGO_MEASURE_LATENCY
const char *EventLatencySession(void);
void EventLatencyPlayer(int x,int y,int z,int weapon);
void EventLatencyCount(enum EVENT_WORK counter,unsigned amount);
void EventLatencyFrameBegin(const char *map,int module);
void EventLatencyFrameEnd(void);
void EventLatencyFlush(void);
void EventLatencyLoadCheckpoint(const char *stage,int detail);
void EventLatencyShutdown(void);
void EventLatencyWindowState(int focused,int minimized);
void EventLatencyMenu(int menu,int selection);
void EventLatencyNote(const char *stage,int type,int message,const char *resource);
EVENT_TICK EventLatencyBegin(void);
EVENT_TICK EventLatencyId(const char *binaryName);
void EventLatencyEnd(EVENT_TICK start,const char *stage,int type,int message,EVENT_TICK id,const char *resource,int marker);
#else
#define EventLatencySession() "disabled"
#define EventLatencyPlayer(...) ((void)0)
#define EventLatencyCount(...) ((void)0)
#define EventLatencyFrameBegin(...) ((void)0)
#define EventLatencyFrameEnd() ((void)0)
#define EventLatencyFlush() ((void)0)
#define EventLatencyLoadCheckpoint(...) ((void)0)
#define EventLatencyShutdown() ((void)0)
#define EventLatencyWindowState(...) ((void)0)
#define EventLatencyMenu(...) ((void)0)
#define EventLatencyNote(...) ((void)0)
#define EventLatencyBegin() 0ULL
#define EventLatencyId(...) 0ULL
#define EventLatencyEnd(...) ((void)0)
#endif
#ifdef __cplusplus
}
#endif
#endif
