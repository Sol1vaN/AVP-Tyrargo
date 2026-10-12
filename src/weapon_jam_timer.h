#ifndef TYRARGO_WEAPON_JAM_TIMER_H
#define TYRARGO_WEAPON_JAM_TIMER_H
#include <stdint.h>
#include <math.h>
typedef struct WEAPON_JAM_TIMER {
 double remaining;
 uint32_t randomState;
 int armed;
} WEAPON_JAM_TIMER;
static inline void WeaponJamReset(WEAPON_JAM_TIMER *t) {
 t->remaining=0.0; t->armed=0;
}
static inline int WeaponJamReady(WEAPON_JAM_TIMER *t,int elapsedFixed,int probability) {
 if(elapsedFixed<=0 || probability<=0)return 0;
 if(probability>=65536)return 1;
 if(!t->armed) {
  uint32_t x=t->randomState;
  if(!x)x=0x6d2b79f5u;
  x^=x<<13; x^=x>>17; x^=x<<5; t->randomState=x;
  /* Open interval, full 32-bit precision; no per-frame RNG/threshold rounding.
     Reference: original p per update at 100 updates/sec.
     Time advances ONLY when primary fire is eligible, never while idle,
     reloading, jammed, out of ammo, or in a paused menu. */
  double u=((double)x+0.5)/4294967296.0;
  t->remaining=-log(u)/(-100.0*log1p(-(double)probability/65536.0));
  t->armed=1;
 }
 t->remaining-=(double)elapsedFixed/65536.0;
 if(t->remaining>0.0)return 0;
 WeaponJamReset(t);
 return 1; /* Existing jam state owns recovery; no catch-up burst. */
}
#endif
