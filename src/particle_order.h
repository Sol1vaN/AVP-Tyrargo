#ifndef AVP_PARTICLE_ORDER_H
#define AVP_PARTICLE_ORDER_H
#define PARTICLE_ORDER_MAX_PARTICLES 5000
#define PARTICLE_ORDER_MAX_PLANES 1001
/* One entry for every translucent polygon, followed by the final near threshold. */
typedef struct PARTICLE_RENDER_ORDER {
    int threshold[PARTICLE_ORDER_MAX_PLANES];
    int head[PARTICLE_ORDER_MAX_PLANES],tail[PARTICLE_ORDER_MAX_PLANES];
    int next[PARTICLE_ORDER_MAX_PARTICLES];
    int count,slot,knownParticles,active;
} PARTICLE_RENDER_ORDER;
static void ParticleOrderBegin(PARTICLE_RENDER_ORDER *o,const int *threshold,int count)
{
    o->active=0;
    if(count<=1||count>PARTICLE_ORDER_MAX_PLANES)return;
    for(int i=0;i<count;i++){
        if(i && threshold[i]>threshold[i-1])return;
        o->threshold[i]=threshold[i];o->head[i]=o->tail[i]=-1;
    }
    o->count=count;o->slot=0;o->knownParticles=0;o->active=1;
}
static void ParticleOrderQueue(PARTICLE_RENDER_ORDER *o,int index,int depth)
{
    int lo=o->slot,hi=o->count;
    while(lo<hi){int mid=lo+(hi-lo)/2;if(depth>o->threshold[mid])hi=mid;else lo=mid+1;}
    if(lo==o->count)return;
    o->next[index]=-1;
    if(o->tail[lo]>=0)o->next[o->tail[lo]]=index;else o->head[lo]=index;
    o->tail[lo]=index;
}
#endif
