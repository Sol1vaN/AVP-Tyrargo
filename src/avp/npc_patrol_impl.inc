
/* Shared exploration coordinator. All collision/movement remains on the
   engine thread. Graph and grid work is incremental; neither is tied to the
   human's position. Runtime pointers are rebuilt after load. */
#include <stdlib.h>
#include "pfarlocs.h"
#include "pheromon.h"
#include <limits.h>
#include <stdint.h>
#define PATROL_CELL 5000
#define PATROL_TEAM_GAP 6500
typedef struct {int x,z,module,visits,blocked,failures;} PATROL_SECTOR;
static struct {
 int age,tick,phase,goalValid,goalAge,lastDistance,stalled,graphRetry,regroup,regroupTime,yieldHold,paceSlow,graphRead,graphWrite,graphActive,graphBest,gridCursor;
 char leader[SB_NAME_LENGTH];VECTORCH anchor,goal,progress,heading,trail[96];int trailCount,trailStart,destination;AIMODULE*graph;
 int *seen,*queue,*parent,*visits;int capacity;PATROL_SECTOR*sectors;int sectorCount,sectorCapacity;
} patrol;
static int PatrolX(int x){return x>=0?x/PATROL_CELL:(x-PATROL_CELL+1)/PATROL_CELL;}
/* Validate the address before reading it. A stale module is never a valid
   index merely because its freed memory once contained an in-range number. */
static int PatrolIndex(AIMODULE*m){
 if(!m || !AIModuleArray || patrol.graph!=AIModuleArray || patrol.capacity!=AIModuleArraySize || patrol.capacity<=0)return -1;
 uintptr_t base=(uintptr_t)AIModuleArray,address=(uintptr_t)m;
 if(address<base)return -1;
 uintptr_t offset=address-base;
 if(offset>=(size_t)patrol.capacity*sizeof(*m) || offset%sizeof(*m))return -1;
 int index=(int)(offset/sizeof(*m));return AIModuleArray[index].m_index==index?index:-1;
}
static int PatrolGraphCurrent(void){
 if(patrol.capacity && (patrol.graph!=AIModuleArray || patrol.capacity!=AIModuleArraySize))NPCSupportPatrolReset();
 return AIModuleArray && AIModuleArraySize>0 && patrol.capacity>0;
}
static int PatrolSector(int x,int z,int module,int create){
 for(int i=0;i<patrol.sectorCount;i++)if(patrol.sectors[i].x==x && patrol.sectors[i].z==z && patrol.sectors[i].module==module)return i;
 if(!create)return -1;
 if(patrol.sectorCount==patrol.sectorCapacity){int size=patrol.sectorCapacity?patrol.sectorCapacity*2:64;PATROL_SECTOR*next=realloc(patrol.sectors,(size_t)size*sizeof(*next));if(!next)return -1;patrol.sectors=next;patrol.sectorCapacity=size;}
 int index=patrol.sectorCount++;patrol.sectors[index]=(PATROL_SECTOR){x,z,module,0,0};return index;
}
void NPCSupportPatrolReset(void){free(patrol.seen);free(patrol.queue);free(patrol.parent);free(patrol.visits);free(patrol.sectors);memset(&patrol,0,sizeof patrol);patrol.destination=-1;}
void NPCSupportPatrolFrame(void){
 PatrolGraphCurrent();
 if(AvP.Network!=I_No_Network || AvP.PlayerType!=I_Marine)return;
 STRATEGYBLOCK*leader=NULL;
 for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*s=ActiveStBlockList[i];if(NPCSupportCanCommand(s) && !s->supportFollow && (!leader || memcmp(s->SBname,leader->SBname,SB_NAME_LENGTH)<0))leader=s;}
 if(!leader){patrol.tick=0;return;}
 if(!patrol.capacity && AIModuleArray && AIModuleArraySize>0){
  patrol.graph=AIModuleArray;patrol.destination=-1;patrol.capacity=AIModuleArraySize;patrol.seen=calloc(patrol.capacity,sizeof(int));patrol.queue=calloc(patrol.capacity,sizeof(int));patrol.parent=calloc(patrol.capacity,sizeof(int));patrol.visits=calloc(patrol.capacity,sizeof(int));
  if(!patrol.seen || !patrol.queue || !patrol.parent || !patrol.visits){NPCSupportPatrolReset();return;}
  patrol.anchor=leader->DynPtr->Position;
 }
 if(!patrol.capacity)return;
 if(memcmp(patrol.leader,leader->SBname,SB_NAME_LENGTH)){patrol.destination=-1;memcpy(patrol.leader,leader->SBname,SB_NAME_LENGTH);patrol.goalValid=patrol.graphActive=patrol.gridCursor=patrol.trailCount=patrol.trailStart=0;}
 VECTORCH travel=leader->DynPtr->Position;
 VECTORCH last=patrol.trailCount?patrol.trail[patrol.trailCount-1]:travel;
 if(leader->DynPtr->IsInContactWithFloor && (!patrol.trailCount || VectorDistance(&travel,&last)>800)){
  if(patrol.trailCount==96){memmove(patrol.trail,patrol.trail+1,95*sizeof(VECTORCH));patrol.trailCount=95;patrol.trailStart++;}
  patrol.trail[patrol.trailCount++]=leader->DynPtr->Position;
 }
 if(patrol.age<INT_MAX-NormalFrameTime)patrol.age+=NormalFrameTime;
 int phase=patrol.age<10*ONE_FIXED?0:patrol.age<30*ONE_FIXED?1:2;
 if(phase!=patrol.phase){patrol.phase=phase;patrol.goalValid=patrol.graphActive=0;patrol.gridCursor=0;}
 patrol.goalAge+=NormalFrameTime;if(patrol.graphRetry>0)patrol.graphRetry-=NormalFrameTime;patrol.tick-=NormalFrameTime;
 if(patrol.tick>0)return;patrol.tick=ONE_FIXED/4;
 for(int i=0;i<patrol.sectorCount;i++)if(patrol.sectors[i].blocked>0)patrol.sectors[i].blocked-=ONE_FIXED/4;
 for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*s=ActiveStBlockList[i];if(!NPCSupportCanCommand(s) || s->supportFollow || !s->containingModule)continue;
  int m=PatrolIndex(s->containingModule->m_aimodule);if(m<0)continue;if(!patrol.visits[m])patrol.visits[m]=1;
  int index=PatrolSector(PatrolX(s->DynPtr->Position.vx),PatrolX(s->DynPtr->Position.vz),m,1);
  if(index>=0 && !patrol.sectors[index].visits){patrol.sectors[index].visits=1;patrol.stalled=0;}
 }
}
static void PatrolBeginGraph(AIMODULE*source){
 memset(patrol.seen,0,patrol.capacity*sizeof(int));int index=PatrolIndex(source);patrol.graphRead=patrol.graphWrite=0;patrol.graphBest=-1;
 if(index<0)return;patrol.queue[patrol.graphWrite++]=index;patrol.seen[index]=1;patrol.parent[index]=-1;patrol.graphActive=1;
}
static int PatrolGraphStep(STRATEGYBLOCK*leader){
 int budget=32;
 while(patrol.graphRead<patrol.graphWrite && budget--){
  int index=patrol.queue[patrol.graphRead++];AIMODULE*from=&AIModuleArray[index];
  if(from->m_link_ptrs)for(AIMODULE**link=from->m_link_ptrs;*link;link++){
   AIMODULE*to=*link;int next=PatrolIndex(to);if(next<0 || patrol.seen[next] || !AIModuleIsPhysical(to) || !AIModuleAdmitsPheromones(to))continue;
   FARENTRYPOINT*ep=GetAIModuleEP(to,from);if(!ep || ep->alien_only || !CheckAdjacencyValidity(to,from,0))continue;
   patrol.seen[next]=1;patrol.parent[next]=index;patrol.queue[patrol.graphWrite++]=next;
   VECTORCH point=ep->position;AddVector(&to->m_world,&point);
   if(patrol.phase<2 && VectorDistance(&point,&patrol.anchor)>(patrol.phase?22000:6500))continue;
   /* Prefer an unvisited connected room before repeating an explored room. */
   int sector=PatrolSector(PatrolX(point.vx),PatrolX(point.vz),next,0);
   if(!patrol.visits[next] && patrol.graphBest<0 && (sector<0 || patrol.sectors[sector].blocked<=0))patrol.graphBest=next;
  }
 }
 if(patrol.graphRead<patrol.graphWrite)return 0;
 patrol.graphActive=0;patrol.graphRetry=ONE_FIXED;
 if(patrol.graphBest>=0){int target=patrol.graphBest,parent=patrol.parent[target];FARENTRYPOINT*ep=GetAIModuleEP(&AIModuleArray[target],&AIModuleArray[parent]);
  if(ep){patrol.destination=target;patrol.goal=ep->position;AddVector(&AIModuleArray[target].m_world,&patrol.goal);patrol.goalValid=1;patrol.goalAge=patrol.stalled=0;patrol.progress=leader->DynPtr->Position;patrol.lastDistance=INT_MAX;return 1;}}
 return 0;
}
static int PatrolFloor(STRATEGYBLOCK*sb,VECTORCH*point){
 if(!sb->SBdptr)return 1;VECTORCH origin=*point;origin.vy=sb->DynPtr->Position.vy-1800;VECTORCH down={0,ONE_FIXED,0};
 FindPolygonInLineOfSight_Navigation(&down,&origin,sb->SBdptr,Player);
 if(!LOS_ObjectHitPtr || LOS_ObjectNormal.vy>-40000 || LOS_Lambda>4000)return 0;
 point->vy=LOS_Point.vy;return ModuleFromPosition(point,sb->containingModule)!=NULL;
}
static void PatrolGridStep(STRATEGYBLOCK*leader){
 /* Grow a frontier around cells actually traversed, including large maps
    that put the entire outdoor terrain in a single AI module. */
 int module=PatrolIndex(leader->containingModule->m_aimodule);if(module<0)return;
 int best=-1,score=INT_MAX;VECTORCH goal={0,0,0};
 for(int n=0;n<32 && patrol.sectorCount;n++){
  int seed=patrol.gridCursor++%patrol.sectorCount;PATROL_SECTOR cell=patrol.sectors[seed];if(cell.module!=module || !cell.visits)continue;
  for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++){
   if(!dx&&!dz)continue;int x=cell.x+dx,z=cell.z+dz;VECTORCH point={x*PATROL_CELL+PATROL_CELL/2,leader->DynPtr->Position.vy,z*PATROL_CELL+PATROL_CELL/2};
   MODULE*room=ModuleFromPosition(&point,leader->containingModule);if(!room || room->m_aimodule!=leader->containingModule->m_aimodule)continue;
   if(patrol.phase<2 && VectorDistance(&point,&patrol.anchor)>(patrol.phase?22000:6500))continue;
   int index=PatrolSector(x,z,module,0);if(index>=0 && (patrol.sectors[index].visits || patrol.sectors[index].blocked>0))continue;
   int cost=VectorDistance(&point,&leader->DynPtr->Position);if(cost<score){score=cost;best=PatrolSector(x,z,module,1);goal=point;}
  }
 }
 if(best>=0){if(PatrolFloor(leader,&goal)){patrol.goal=goal;patrol.destination=module;patrol.goalValid=1;patrol.goalAge=patrol.stalled=0;patrol.progress=leader->DynPtr->Position;patrol.lastDistance=INT_MAX;}
  else patrol.sectors[best].blocked=15*ONE_FIXED;}
}
VECTORCH NPCSupportPatrolGoal(STRATEGYBLOCK*sb){
 if(!PatrolGraphCurrent())return sb->DynPtr->Position;
 if(patrol.goalValid && (patrol.destination<0 || patrol.destination>=patrol.capacity)){patrol.goalValid=patrol.graphActive=0;patrol.destination=-1;}
 STRATEGYBLOCK*leader=FindSBWithName(patrol.leader);if(!leader || !patrol.capacity || !sb->containingModule)return sb->DynPtr->Position;
 STRATEGYBLOCK*buddy=NULL;int distance=0;
 for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*s=ActiveStBlockList[i];if(s!=leader && NPCSupportCanCommand(s) && !s->supportFollow){int d=VectorDistance(&s->DynPtr->Position,&leader->DynPtr->Position);if(d>distance){distance=d;buddy=s;}}}
 if(sb!=leader){
  MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_LOCAL_NAV*n=&m->localNav;
  VECTORCH goal=leader->DynPtr->Position,offset=sb->DynPtr->Position;SubVector(&goal,&offset);offset.vy=0;
  /* Regroup is a new task, not another historic breadcrumb. Discard the
     completed route once at the transition and plan towards the partner. */
  if(Magnitude(&offset)>PATROL_TEAM_GAP || (n->patrolRegroup && Magnitude(&offset)>=4000)){
   if(!n->patrolRegroup){NPCNavReset(n);n->patrolRegroup=1;}
   n->yieldTime=0;return goal;
  }
  if(n->patrolRegroup){NPCNavReset(n);n->patrolCursorValid=0;}

  if(n->yieldTime>0){n->yieldTime-=NormalFrameTime;VECTORCH d=n->yieldGoal;SubVector(&sb->DynPtr->Position,&d);d.vy=0;
   if(Magnitude(&d)>400 && n->yieldTime>0)return n->yieldGoal;n->yieldTime=0;}
  VECTORCH heading=leader->DynPtr->LinVelocity;heading.vy=0;if(Magnitude(&heading)>500){Normalise(&heading);patrol.heading=heading;}
  heading=patrol.heading;int ahead=MUL_FIXED(offset.vx,heading.vx)+MUL_FIXED(offset.vz,heading.vz);
  if(Magnitude(&offset)<1800 && ahead>0){
   /* Commit to a single yielding destination rather than moving it every
      frame; the leader waits while this checked manoeuvre is completed. */
   VECTORCH side={-heading.vz,0,heading.vx};int dot=MUL_FIXED(side.vx,offset.vx)+MUL_FIXED(side.vz,offset.vz);if((dot<0) ^ (n->yieldSide++ & 1)){side.vx=-side.vx;side.vz=-side.vz;}
   n->yieldGoal=sb->DynPtr->Position;n->yieldGoal.vx+=MUL_FIXED(side.vx,2000);n->yieldGoal.vz+=MUL_FIXED(side.vz,2000);n->yieldTime=3*ONE_FIXED;return n->yieldGoal;
  }
  /* Follow positions actually traversed by the leader, including vehicle
     corners. A moving offset alone shortcuts those corners into obstacles. */
  if(patrol.trailCount){int end=patrol.trailCount-1,length=VectorDistance(&goal,&patrol.trail[end]);
   while(end>0 && length<2500){length+=VectorDistance(&patrol.trail[end],&patrol.trail[end-1]);end--;}
   int nearest=n->patrolCursor-patrol.trailStart;
   if(!n->patrolCursorValid || nearest<0 || nearest>=patrol.trailCount){int best=INT_MAX;nearest=0;
    for(int i=0;i<=end;i++){int d=VectorDistance(&sb->DynPtr->Position,&patrol.trail[i]);if(d<best){best=d;nearest=i;}}
    n->patrolCursorValid=1;
   }
   /* Close companions join the current trailing segment. Keeping a historic
      cursor after the leader turns back sends the follower away from its
      partner. The native planner still checks the route around corners. */
   if(Magnitude(&offset)<4000)nearest=end;
   if(nearest>end)nearest=end;
   while(nearest<end && VectorDistance(&sb->DynPtr->Position,&patrol.trail[nearest])<850)nearest++;
   n->patrolCursor=patrol.trailStart+nearest;goal=patrol.trail[nearest];
  }else{Normalise(&offset);goal.vx+=MUL_FIXED(offset.vx,2200);goal.vz+=MUL_FIXED(offset.vz,2200);}
  return goal;
 }
 if(buddy && distance>PATROL_TEAM_GAP && !patrol.regroup){
  patrol.regroup=1;NPCNavReset(&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->localNav);
 }
 if(!buddy || distance<4000){if(patrol.regroup)NPCNavReset(&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->localNav);patrol.regroup=0;patrol.regroupTime=0;}
 if(!buddy || distance>=2400)patrol.yieldHold=0;
 int yielding=buddy && distance<2400 && ((MARINE_STATUS_BLOCK*)buddy->SBdataptr)->localNav.yieldTime>0;
 if(yielding && patrol.yieldHold<2*ONE_FIXED)patrol.yieldHold+=NormalFrameTime;
 /* Courtesy is a short hand-off, never an indefinite hold that also pauses
    exploration recovery when the other side is obstructed. */
 if(patrol.regroup || (yielding && patrol.yieldHold<ONE_FIXED)){
  if(patrol.goalAge>=NormalFrameTime)patrol.goalAge-=NormalFrameTime;
  if(patrol.regroup){patrol.regroupTime+=NormalFrameTime;
   /* A follower blocked on a slope must not leave the leader waiting forever.
      Walk back to reunite using the same collision planner, then resume the
      exploration destination. No relocation or collision bypass. */
   if(patrol.regroupTime>5*ONE_FIXED && buddy){
    if(patrol.regroupTime-NormalFrameTime<=5*ONE_FIXED)NPCNavReset(&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->localNav);
    return buddy->DynPtr->Position;
   }
  }return sb->DynPtr->Position;
 } /* Let the follower catch up along the same route. */
 if(patrol.goalValid){
  VECTORCH delta=patrol.goal;SubVector(&sb->DynPtr->Position,&delta);delta.vy=0;
  MARINE_STATUS_BLOCK*m=sb->SBdataptr;
  int interrupted=m->Target!=NULL || FindSBWithName(m->squad.pickupName)!=NULL;
  if(interrupted){if(patrol.goalAge>=NormalFrameTime)patrol.goalAge-=NormalFrameTime;patrol.stalled=0;patrol.progress=sb->DynPtr->Position;}
  else if(Magnitude(&delta)<patrol.lastDistance-500){patrol.lastDistance=Magnitude(&delta);patrol.stalled=0;}
  else patrol.stalled+=NormalFrameTime;
  if(Magnitude(&delta)<850 && (PatrolIndex(sb->containingModule->m_aimodule)==patrol.destination || abs(sb->DynPtr->Position.vy-patrol.goal.vy)<1000)){int m=patrol.destination;if(m>=0)patrol.visits[m]++;
   int index=PatrolSector(PatrolX(patrol.goal.vx),PatrolX(patrol.goal.vz),m,1);if(index>=0)patrol.sectors[index].visits++;patrol.goalValid=0;}
  else if(patrol.goalAge>90*ONE_FIXED || patrol.stalled>8*ONE_FIXED){int m=patrol.destination,index=PatrolSector(PatrolX(patrol.goal.vx),PatrolX(patrol.goal.vz),m,1);if(index>=0){PATROL_SECTOR*sector=&patrol.sectors[index];if(sector->failures<4)sector->failures++;sector->blocked=(60+60*sector->failures)*ONE_FIXED;}patrol.goalValid=0;}
 }
 if(!patrol.goalValid){
  if(!patrol.graphActive && patrol.graphRetry<=0)PatrolBeginGraph(sb->containingModule->m_aimodule);
  if(!patrol.graphActive || !PatrolGraphStep(sb))PatrolGridStep(sb);
 }
 return patrol.goalValid?patrol.goal:sb->DynPtr->Position;
}

/* Match the leader's real movement at the trailing slot, including native
   slope motion, instead of stopping at each new breadcrumb. */
int NPCSupportPatrolTravel(STRATEGYBLOCK*sb,VECTORCH*velocity){
 STRATEGYBLOCK*leader=FindSBWithName(patrol.leader);if(!leader || leader==sb || sb->supportFollow || NormalFrameTime<=0 || abs(leader->DynPtr->Position.vy-sb->DynPtr->Position.vy)>1800 || ((MARINE_STATUS_BLOCK*)sb->SBdataptr)->localNav.yieldTime>0)return 0;
 VECTORCH gap=sb->DynPtr->Position;SubVector(&leader->DynPtr->Position,&gap);gap.vy=0;
 if(Magnitude(&gap)<2000 || Magnitude(&gap)>6500)return 0;
 velocity->vx=(int)((int64_t)(leader->DynPtr->Position.vx-leader->DynPtr->PrevPosition.vx)*ONE_FIXED/NormalFrameTime);
 velocity->vz=(int)((int64_t)(leader->DynPtr->Position.vz-leader->DynPtr->PrevPosition.vz)*ONE_FIXED/NormalFrameTime);velocity->vy=0;return 1;
}

/* Versioned optional block, separate from every native actor save layout. */
typedef struct {SAVE_BLOCK_HEADER header;int schema,age,phase,capacity,sectorCount,goalValid,goalAge,destination;char leader[SB_NAME_LENGTH];VECTORCH anchor,goal;} PATROL_SAVE;
void NPCSupportPatrolSave(void){
 if(!PatrolGraphCurrent())return;
 size_t size=sizeof(PATROL_SAVE)+(size_t)patrol.capacity*sizeof(int)+(size_t)patrol.sectorCount*sizeof(PATROL_SECTOR);
 PATROL_SAVE*b=GetPointerForSaveBlock(size);memset(b,0,sizeof(*b));b->header.type=SaveBlock_NPCSupport;b->header.size=size;b->schema=248;b->age=patrol.age;b->phase=patrol.phase;b->capacity=patrol.capacity;b->sectorCount=patrol.sectorCount;b->goalValid=patrol.goalValid;b->goalAge=patrol.goalAge;b->destination=patrol.destination;b->anchor=patrol.anchor;b->goal=patrol.goal;memcpy(b->leader,patrol.leader,SB_NAME_LENGTH);
 memcpy(b+1,patrol.visits,patrol.capacity*sizeof(int));if(patrol.sectorCount)memcpy((char*)(b+1)+patrol.capacity*sizeof(int),patrol.sectors,patrol.sectorCount*sizeof(PATROL_SECTOR));
}
int NPCSupportPatrolLoad(SAVE_BLOCK_HEADER*header){
 if(header->size<sizeof(PATROL_SAVE) || *(int*)(header+1)!=248)return 0;
 PATROL_SAVE*b=(PATROL_SAVE*)header;if(!AIModuleArray || b->capacity!=AIModuleArraySize || b->capacity<=0 || b->sectorCount<0 || b->sectorCount>131072 || b->age<0 || b->phase<0 || b->phase>2 || b->goalAge<0)return 1;
 size_t size=sizeof(*b)+(size_t)b->capacity*sizeof(int)+(size_t)b->sectorCount*sizeof(PATROL_SECTOR);if(size!=header->size)return 1;
 NPCSupportPatrolReset();patrol.graph=AIModuleArray;patrol.capacity=b->capacity;patrol.seen=calloc(patrol.capacity,sizeof(int));patrol.queue=calloc(patrol.capacity,sizeof(int));patrol.parent=calloc(patrol.capacity,sizeof(int));patrol.visits=calloc(patrol.capacity,sizeof(int));
 patrol.sectorCapacity=b->sectorCount;patrol.sectorCount=b->sectorCount;patrol.sectors=b->sectorCount?calloc(b->sectorCount,sizeof(PATROL_SECTOR)):NULL;
 if(!patrol.seen || !patrol.queue || !patrol.parent || !patrol.visits || (b->sectorCount && !patrol.sectors)){NPCSupportPatrolReset();return 1;}
 patrol.lastDistance=INT_MAX;patrol.age=b->age;patrol.phase=b->phase;patrol.goalValid=b->goalValid==1;patrol.goalAge=b->goalAge;patrol.anchor=b->anchor;patrol.goal=b->goal;memcpy(patrol.leader,b->leader,SB_NAME_LENGTH);
 patrol.destination=b->destination>=0 && b->destination<patrol.capacity?b->destination:-1;
 if(patrol.destination<0)patrol.goalValid=0;
 memcpy(patrol.visits,b+1,patrol.capacity*sizeof(int));if(patrol.sectorCount)memcpy(patrol.sectors,(char*)(b+1)+patrol.capacity*sizeof(int),patrol.sectorCount*sizeof(PATROL_SECTOR));return 1;
}

/* Reuse the full graph route rather than the legacy 32-hop search limit. */
AIMODULE* NPCSupportPatrolNext(AIMODULE*source,AIMODULE*destination){
 if(!PatrolGraphCurrent())return NULL;
 int from=PatrolIndex(source),to=PatrolIndex(destination);
 if(from<0 || to<0 || !patrol.seen || !patrol.seen[to])return NULL;
 for(int budget=patrol.capacity;budget>0 && to>=0;budget--){
  int parent=patrol.parent[to];if(parent==from)return &AIModuleArray[to];to=parent;
 }
 return NULL;
}

/* Keep sprinting together before the hard regroup distance is reached.
   Hysteresis is independent of the animation's walk/run transition. */
int NPCSupportPatrolPace(STRATEGYBLOCK*sb){
 STRATEGYBLOCK*leader=FindSBWithName(patrol.leader);if(!leader || leader!=sb || sb->supportFollow)return 7000;
 int distance=0;for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*buddy=ActiveStBlockList[i];if(buddy==leader || !NPCSupportCanCommand(buddy) || buddy->supportFollow)continue;
  int gap=VectorDistance(&buddy->DynPtr->Position,&leader->DynPtr->Position);if(gap>distance)distance=gap;
 }
 if(distance>4500)patrol.paceSlow=1;else if(distance<3000)patrol.paceSlow=0;
 return patrol.paceSlow?5000:7000;
}
