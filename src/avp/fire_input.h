#ifndef TYRARGO_FIRE_INPUT_H
#define TYRARGO_FIRE_INPUT_H
#include <string.h>
typedef struct {
 int weapon,primaryLatched,secondaryLatched,secondaryDown,secondaryQueued;
} FIRE_INPUT_STATE;
static void FireInputReset(FIRE_INPUT_STATE *s)
{ memset(s,0,sizeof(*s));s->weapon=-1; }
static void FireInputUpdate(FIRE_INPUT_STATE *s,int weapon,int primary,int secondary,
                            int dt,int allowQueue)
{
 if(s->weapon!=weapon){FireInputReset(s);s->weapon=weapon;}
 if(!primary)s->primaryLatched=0;
 if(!secondary)s->secondaryLatched=0;
 if(dt>0 && s->secondaryQueued>0){s->secondaryQueued-=dt;if(s->secondaryQueued<0)s->secondaryQueued=0;}
 if(!allowQueue)s->secondaryQueued=0;
 else if(secondary && !s->secondaryDown)s->secondaryQueued=65536/4;
 s->secondaryDown=secondary;
}
static int FireInputPrimary(const FIRE_INPUT_STATE *s,int down,int automatic)
{ return down && (automatic || !s->primaryLatched); }
static int FireInputSecondary(const FIRE_INPUT_STATE *s,int down,int automatic)
{ return s->secondaryQueued>0 || (down && (automatic || !s->secondaryLatched)); }
#endif
