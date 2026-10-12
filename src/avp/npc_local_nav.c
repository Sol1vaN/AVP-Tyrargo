/* Bounded, incremental walking search for geometry without useful waypoints.
 * Existing module routing remains responsible for the long-range destination.
 * No displacement is forced: dynamics/collision still resolve every step. */
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "stratdef.h"
#include "dynblock.h"
#include "los.h"
#include "npc_local_nav.h"
#include "bh_marin.h"
#include "dynamics.h"
#include <math.h>
#include <SDL3/SDL.h>
#include <string.h>
#include <stdlib.h>
extern int NormalFrameTime;
#define CELL 250
#define NPC_NAV_CONTACT_SKIN 10 /* matches dynamics.c COLLISION_GRANULARITY */
#define NPC_NAV_BODY_RADIUS (450-NPC_NAV_CONTACT_SKIN)
#define JUMP_CELLS 4
#define CENTRE (NPC_NAV_SIDE/2)
#define START (CENTRE*NPC_NAV_SIDE+CENTRE)
static int HorizontalDistance(VECTORCH *a,VECTORCH *b){VECTORCH d=*a;SubVector(b,&d);d.vy=0;return Magnitude(&d);}
void NPCNavReset(NPC_LOCAL_NAV *n){memset(n,0,sizeof(*n));n->allowance=1;}

/* Collision traversal uses shared scratch; jobs remain on the engine thread.
   Replay completed queries, yield before the next one, never accept an
   unchecked edge. Fair per-actor slices keep long jump tests off one frame. */
static NPC_LOCAL_NAV *jobNav;
static int jobCursor,jobYield,jobBudget,frameRays;
static Uint64 frameSpent;
void NPCNavFrameBegin(void){
 extern int NumActiveStBlocks;extern STRATEGYBLOCK*ActiveStBlockList[];static unsigned round;
 frameRays=0;frameSpent=0;int count=0;
 for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*s=ActiveStBlockList[i];if(s->I_SBtype==I_BehaviourMarine && s->SBdataptr && s->SBdptr)count++;}
 int quota=count?128/count:64;if(quota>64)quota=64;if(quota<1)quota=1;int ordinal=0;
 for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*s=ActiveStBlockList[i];if(s->I_SBtype==I_BehaviourMarine && s->SBdataptr && s->SBdptr){
  NPC_LOCAL_NAV*n=&((MARINE_STATUS_BLOCK*)s->SBdataptr)->localNav;
  n->frameIssued=0;n->allowance=count>128 && ((ordinal++ + round)%count)>=128?0:quota;
 }}round++;
}
static int NavRay(STRATEGYBLOCK*sb,VECTORCH origin,VECTORCH direction,NPC_NAV_RAY*out){
 if(jobNav){
  int i=jobCursor++;
  if(i<jobNav->rayCount){NPC_NAV_RAY*r=&jobNav->rays[i];
   if(!memcmp(&r->origin,&origin,sizeof origin)&&!memcmp(&r->direction,&direction,sizeof direction)){*out=*r;return 1;}
   jobNav->rayCount=i;
  }
  if(i>=1024 || jobBudget<=0 || frameRays>=128 || (jobNav->frameIssued && frameSpent>3000000)){jobYield=1;return 0;}
  jobBudget--;frameRays++;jobNav->frameIssued++;
  Uint64 started=SDL_GetTicksNS();
  FindPolygonInLineOfSight_Navigation(&direction,&origin,sb->SBdptr,Player);
  frameSpent+=SDL_GetTicksNS()-started;
  *out=(NPC_NAV_RAY){origin,direction,LOS_Point,LOS_ObjectNormal,LOS_ObjectHitPtr!=NULL,LOS_Lambda};jobNav->rays[i]=*out;jobNav->rayCount=i+1;return 1;
 }
 FindPolygonInLineOfSight_Navigation(&direction,&origin,sb->SBdptr,Player);
 *out=(NPC_NAV_RAY){origin,direction,LOS_Point,LOS_ObjectNormal,LOS_ObjectHitPtr!=NULL,LOS_Lambda};return 1;
}
static void JobStart(NPC_LOCAL_NAV*n){jobNav=n;jobCursor=jobYield=0;jobBudget=n->allowance;}
static void JobEnd(void){if(jobNav)jobNav->allowance=jobBudget;jobNav=NULL;}
static int FloorRay(STRATEGYBLOCK*sb,VECTORCH origin,int range,VECTORCH*point,VECTORCH*normal){
 NPC_NAV_RAY r;if(!NavRay(sb,origin,(VECTORCH){0,ONE_FIXED,0},&r)||!r.hit||r.normal.vy>-40000||r.lambda>range)return 0;
 *point=r.point;if(normal)*normal=r.normal;return 1;
}

/* Native cuboids stand on their highest supported foot corner, even on
   a smooth ramp. Keep navigation elevations in that same coordinate space. */
static int FloorSupport(STRATEGYBLOCK*sb,VECTORCH origin,int range,VECTORCH*point,VECTORCH*normal){
 static const int corners[5][2]={{0,0},{-450,-450},{-450,450},{450,-450},{450,450}};
 int found=0;VECTORCH support=origin,bestNormal={0,-ONE_FIXED,0};
 for(int k=0;k<5;k++){VECTORCH probe=origin,hit,n;probe.vx+=corners[k][0];probe.vz+=corners[k][1];
  if(FloorRay(sb,probe,range,&hit,&n) && (!found || hit.vy<support.vy)){found=1;support.vy=hit.vy;bestNormal=n;}
  if(jobYield)return 0;
 }
 if(found){*point=support;if(normal)*normal=bestNormal;}return found;
}

static int ClearRay(STRATEGYBLOCK *sb,VECTORCH a,VECTORCH b){
 VECTORCH direction=b;SubVector(&a,&direction);int distance=Magnitude(&direction);if(!distance)return 1;
 Normalise(&direction);NPC_NAV_RAY r;if(!NavRay(sb,a,direction,&r))return 0;
 return !r.hit || r.lambda>=distance-2;
}
/* Keep the ray shell inside the native 10 mm collision skin. Dynamics
   allows that contact tolerance; exact face rays otherwise trap legal poses. */
static int BodyLink(STRATEGYBLOCK*sb,VECTORCH a,VECTORCH b,int drop,int height){
 static const int corners[5][2]={{0,0},{-NPC_NAV_BODY_RADIUS,-NPC_NAV_BODY_RADIUS},{-NPC_NAV_BODY_RADIUS,NPC_NAV_BODY_RADIUS},{NPC_NAV_BODY_RADIUS,-NPC_NAV_BODY_RADIUS},{NPC_NAV_BODY_RADIUS,NPC_NAV_BODY_RADIUS}};
 for(int c=0;c<5;c++){
  VECTORCH from=a,to=b;from.vy-=height;to.vy-=height;from.vx+=corners[c][0];from.vz+=corners[c][1];to.vx+=corners[c][0];to.vz+=corners[c][1];
  if(drop>350){VECTORCH edge=to;edge.vy=from.vy;if(!ClearRay(sb,from,edge)||!ClearRay(sb,edge,to))return 0;}
  else if(!ClearRay(sb,from,to))return 0;
  if(height>1100){VECTORCH feet=from;feet.vy+=height-100;if(!ClearRay(sb,feet,from))return 0;
   feet=to;feet.vy+=height-100;if(!ClearRay(sb,feet,to))return 0;}
 }
 return 1;
}
static int WalkLink(STRATEGYBLOCK *sb,VECTORCH a,VECTORCH *b,int floor,int*crouch){
 *crouch=0;
 if(floor){VECTORCH origin=*b;origin.vy=a.vy-1200;
  if(!FloorSupport(sb,origin,2250,b,NULL))return 0;
 }
 int drop=b->vy-a.vy;
 int ramp=0;VECTORCH middle=a,normal={0,-ONE_FIXED,0};middle.vx+=(b->vx-a.vx)/2;middle.vz+=(b->vz-a.vz)/2;middle.vy-=1200;
 VECTORCH ground;if(FloorSupport(sb,middle,2250,&ground,&normal)){
  int expected=a.vy+drop/2;
  ramp=abs(ground.vy-expected)<100 && normal.vy<-40000;
 }
 if(jobYield)return 0;
 if(!ramp && (drop<-350 || drop>1000))return 0;
 if(ramp)drop=0; /* Follow the continuous support plane through both ends. */

 if(drop<-350 || drop>1000)return 0;
 /* The native collider is an axis-aligned box, not a cylinder: diagonal
    movement must clear its four corners at feet, torso and head. */
 if(!BodyLink(sb,a,*b,drop,100)||!BodyLink(sb,a,*b,drop,1100))return 0;
 if(!BodyLink(sb,a,*b,drop,1950)){if(!BodyLink(sb,a,*b,drop,1200))return 0;*crouch=1;}
 return 1;
}
/* A human jump has a finite height and must clear both body and landing. */
static int JumpCrouch,JumpImpulse,JumpBoost;
static int JumpArc(STRATEGYBLOCK*sb,VECTORCH a,VECTORCH*b,int head){
 if(sb->I_SBtype!=I_BehaviourMarine)return 0;
 VECTORCH origin=*b,down={0,ONE_FIXED,0};origin.vy=a.vy-1500;
 if(!FloorRay(sb,origin,2500,b,NULL))return 0;
 static const int supportCorners[4][2]={{-450,-450},{-450,450},{450,-450},{450,450}};
 for(int k=0;k<4;k++){VECTORCH probe=origin,point;probe.vx+=supportCorners[k][0];probe.vz+=supportCorners[k][1];
  if(FloorRay(sb,probe,2500,&point,NULL) && point.vy<b->vy)b->vy=point.vy;
  if(jobYield)return 0;
 }
 int drop=b->vy-a.vy;if(drop<-1250 || drop>1000)return 0;
 /* Use the lowest ballistic arc that clears this rise. Full-strength
    jumps under a vehicle roof reject a perfectly reachable low step. */
 double height=drop<0?-drop+75:150;
 if(height<150)height=150;
 double impulse=sqrt(2.0*GRAVITY_STRENGTH*height);
 if(impulse>8500)return 0;if(JumpBoost)impulse=8500;JumpImpulse=(int)ceil(impulse);impulse=JumpImpulse;
 double discriminant=impulse*impulse+2.0*GRAVITY_STRENGTH*drop;
 if(discriminant<=0)return 0;
 double duration=(impulse+sqrt(discriminant))/GRAVITY_STRENGTH;
 static const int corners[5][2]={{0,0},{-NPC_NAV_BODY_RADIUS,-NPC_NAV_BODY_RADIUS},{-NPC_NAV_BODY_RADIUS,NPC_NAV_BODY_RADIUS},{NPC_NAV_BODY_RADIUS,-NPC_NAV_BODY_RADIUS},{NPC_NAV_BODY_RADIUS,NPC_NAV_BODY_RADIUS}};
 for(int side=0;side<5;side++)for(int h=0;h<3;h++){
  VECTORCH last=a;last.vy-=h==0?head:h==1?1050:250;
  last.vx+=corners[side][0];last.vz+=corners[side][1];
  for(int i=1;i<=12;i++){
   double time=duration*i/12.0;VECTORCH point=a;
   point.vx+=(int)lround((b->vx-a.vx)*i/12.0);point.vz+=(int)lround((b->vz-a.vz)*i/12.0);
   point.vy+=(int)lround(-impulse*time+GRAVITY_STRENGTH*time*time/2)-(h==0?head:h==1?1050:250);
   point.vx+=corners[side][0];point.vz+=corners[side][1];
   if(!ClearRay(sb,last,point))return 0;
   if(h==0){VECTORCH foot=point;foot.vy+=head-100;if(!ClearRay(sb,foot,point))return 0;}
   last=point;
  }
 }
 return 1;
}
static int JumpLink(STRATEGYBLOCK*sb,VECTORCH a,VECTORCH*b){
 VECTORCH landing=*b;JumpCrouch=0;
 for(JumpBoost=0;JumpBoost<2;JumpBoost++){
  *b=landing;if(JumpArc(sb,a,b,1950))return 1;if(jobYield)return 0;
  *b=landing;if(JumpArc(sb,a,b,1200)){JumpCrouch=1;return 1;}if(jobYield)return 0;
 }return 0;
}

void NPCNavStop(STRATEGYBLOCK*sb){
 VECTORCH velocity={0,0,0};
 if(sb->DynPtr->IsInContactWithFloor){
  for(COLLISIONREPORT*c=sb->DynPtr->CollisionReportPtr;c;c=c->NextCollisionReportPtr){VECTORCH n=c->ObstacleNormal;
   if(n.vy>=-40000)continue;VECTORCH impulse=sb->DynPtr->LinImpulse;
   impulse.vy+=MUL_FIXED(GRAVITY_STRENGTH,NormalFrameTime);
   int64_t square=(int64_t)n.vx*n.vx+(int64_t)n.vy*n.vy+(int64_t)n.vz*n.vz;
   int64_t dot=(int64_t)n.vx*impulse.vx+(int64_t)n.vy*impulse.vy+(int64_t)n.vz*impulse.vz;
   if(square){VECTORCH drift={impulse.vx-(int)(dot*n.vx/square),impulse.vy-(int)(dot*n.vy/square),impulse.vz-(int)(dot*n.vz/square)};
    if(Magnitude(&drift)<7000)velocity=(VECTORCH){-drift.vx,-drift.vy,-drift.vz};}break;
  }
 }
 sb->DynPtr->LinVelocity=velocity;
}
static void Begin(STRATEGYBLOCK*sb,NPC_LOCAL_NAV*n,VECTORCH origin,VECTORCH goal){
 n->basisX=(VECTORCH){ONE_FIXED,0,0};n->basisZ=(VECTORCH){0,0,ONE_FIXED};
 /* Align samples with solid scenery already touched by native dynamics.
    Narrow rotated doorways must not fall between an arbitrary world grid. */
 for(COLLISIONREPORT*c=sb->DynPtr->CollisionReportPtr;c;c=c->NextCollisionReportPtr){
  STRATEGYBLOCK*o=c->ObstacleSBPtr;if(!o || o->I_SBtype!=I_BehaviourInanimateObject || !o->DynPtr || !o->DynPtr->IsStatic || o->DynPtr->IsPickupObject)continue;
  n->basisX=(VECTORCH){o->DynPtr->OrientMat.mat11,0,o->DynPtr->OrientMat.mat13};Normalise(&n->basisX);
  if(n->basisX.vx || n->basisX.vz)n->basisZ=(VECTORCH){-n->basisX.vz,0,n->basisX.vx};else n->basisX=(VECTORCH){ONE_FIXED,0,0};break;
 }
 n->partial=0;n->rayCount=n->jobKind=0;n->expanding=-1;n->expandingJump=0;n->neighbor=0;n->origin=origin;n->goal=goal;n->state=1;n->count=n->step=0;n->progressTime=0;n->progress=origin;
 for(int i=0;i<NPC_NAV_NODES;i++){n->nodes[i].cost=-1;n->nodes[i].parent=-1;n->nodes[i].closed=0;n->nodes[i].jump=0;n->nodes[i].crouch=0;n->nodes[i].pendingJump=0;}
 n->nodes[START].cost=0;n->nodes[START].position=origin;
}
static void Complete(NPC_LOCAL_NAV*n,int index,VECTORCH position){
 int continued=n->partial;
 VECTORCH reverse[NPC_NAV_PATH];int jumping[NPC_NAV_PATH],crawling[NPC_NAV_PATH];int count=0;
 while(index!=START && index>=0 && count<NPC_NAV_PATH){crawling[count]=n->nodes[index].crouch;jumping[count]=n->nodes[index].jump;reverse[count++]=n->nodes[index].position;index=n->nodes[index].parent;}
 if(index!=START || !count){n->state=0;n->checkTimer=ONE_FIXED;return;}
 n->count=0;n->step=0;n->state=2;n->partial=0;
 /* Consecutive checked edges on the same straight support plane form one
    segment. Retain turns, ledges, crouch changes and every jump separately. */
 VECTORCH anchor=n->origin;
 for(int i=count-1;i>=0;){int last=i;
  while(last>0 && !jumping[i] && !jumping[last-1] && crawling[last-1]==crawling[i]){
   VECTORCH a=reverse[i],b=reverse[last-1];SubVector(&anchor,&a);SubVector(&reverse[last],&b);
   int64_t limit=(int64_t)Magnitude(&a)*Magnitude(&b)/500;
   if((int64_t)a.vx*b.vx+(int64_t)a.vy*b.vy+(int64_t)a.vz*b.vz<=0 ||
    llabs((int64_t)a.vx*b.vy-(int64_t)a.vy*b.vx)>limit ||
    llabs((int64_t)a.vx*b.vz-(int64_t)a.vz*b.vx)>limit ||
    llabs((int64_t)a.vy*b.vz-(int64_t)a.vz*b.vy)>limit)break;
   last--;
  }
  int k=n->count++;n->path[k]=reverse[last];n->jump[k]=jumping[i];n->crouch[k]=crawling[i];anchor=reverse[last];i=last-1;
 }
 /* A new frontier path includes its already traversed prefix. Start at the
    closest checked walking segment, never turn back to the old first corner. */
 if(continued){double nearest=150.0*150.0;int first=-1;
  VECTORCH a=n->origin;
  for(int i=0;i<n->count;i++){VECTORCH b=n->path[i];if(n->jump[i])break;
   double x=b.vx-a.vx,z=b.vz-a.vz,l=x*x+z*z;
   double k=l?((position.vx-a.vx)*x+(position.vz-a.vz)*z)/l:0;if(k<0)k=0;if(k>1)k=1;
   double dx=position.vx-a.vx-k*x,dz=position.vz-a.vz-k*z,d=dx*dx+dz*dz;
   if(d<=nearest){nearest=d;first=i;}a=b;
  }if(first<0){n->state=n->count=n->step=n->walkValid=n->checkTimer=0;}else n->step=first;
 }
}
static VECTORCH GridPoint(NPC_LOCAL_NAV*n,int x,int z,int height){
 int a=(x-CENTRE)*CELL,b=(z-CENTRE)*CELL;
 return (VECTORCH){n->origin.vx+MUL_FIXED(n->basisX.vx,a)+MUL_FIXED(n->basisZ.vx,b),height,n->origin.vz+MUL_FIXED(n->basisX.vz,a)+MUL_FIXED(n->basisZ.vz,b)};
}
static void Expand(STRATEGYBLOCK*sb,NPC_LOCAL_NAV*n){
 int best=n->expanding,score=2147483647,fresh=best<0;
 if(best<0){
  int jumpIndex=-1;
  /* Expensive arcs enter the same cost frontier as walking. A wall beside
     an available walk no longer consumes hundreds of rays before that walk
     is considered. All jump candidates remain available. */
  for(int i=0;i<NPC_NAV_NODES;i++)if(n->nodes[i].cost>=0){
   NPC_NAV_NODE*v=&n->nodes[i];
   if(!v->closed){int f=v->cost+2*HorizontalDistance(&v->position,&n->goal);if(f<score){score=f;best=i;jumpIndex=-1;}}
   int x=i%NPC_NAV_SIDE,z=i/NPC_NAV_SIDE;
   for(int k=0;k<9*JUMP_CELLS;k++)if(v->pendingJump&(UINT64_C(1)<<k)){
    int dir=k%9,stride=k/9+1,dx=dir%3-1,dz=dir/3-1,nx=x+dx*stride,nz=z+dz*stride;
    if(nx<0 || nz<0 || nx>=NPC_NAV_SIDE || nz>=NPC_NAV_SIDE)continue;
    NPC_NAV_NODE*next=&n->nodes[nz*NPC_NAV_SIDE+nx];int cost=v->cost+(dx&&dz?354:250)*stride+3200;
    if(next->closed || (next->cost>=0 && next->cost<=cost))continue;
    VECTORCH point=GridPoint(n,nx,nz,v->position.vy);
    int f=cost+2*HorizontalDistance(&point,&n->goal);if(f<score){score=f;best=i;jumpIndex=k;}
   }
  }
  if(best<0){n->state=0;n->checkTimer=ONE_FIXED;return;}
  n->expanding=best;n->expandingJump=jumpIndex>=0;n->neighbor=jumpIndex>=0?jumpIndex:0;
 }
 int x=best%NPC_NAV_SIDE,z=best/NPC_NAV_SIDE;NPC_NAV_NODE*node=&n->nodes[best];
 int distance=HorizontalDistance(&node->position,&n->goal),reached=!n->goalKind && distance<600 && abs(node->position.vy-n->goal.vy)<600;
 if(!n->expandingJump && (fresh || n->jobKind==3) && n->goalKind && distance<950 && n->goal.vy>=node->position.vy-2134 && n->goal.vy<=node->position.vy+150){
  n->jobKind=3;VECTORCH eye=node->position;int top=sb->SBdptr->ObMinY;if(top>-1934)top=-1934;eye.vy+=top+200;JobStart(n);reached=ClearRay(sb,eye,n->goal);JobEnd();
  if(jobYield)return;n->jobKind=n->rayCount=0;
 }
 if(!n->expandingJump && best!=START && (reached || ((x==0 || z==0 || x==NPC_NAV_SIDE-1 || z==NPC_NAV_SIDE-1) && HorizontalDistance(&n->origin,&n->goal)>CELL*CENTRE))){Complete(n,best,sb->DynPtr->Position);return;}

 if(fresh && !n->expandingJump && best!=START && !n->count && HorizontalDistance(&node->position,&sb->DynPtr->Position)>=200){
  int walk=1,index=best;while(index!=START && index>=0){if(n->nodes[index].jump){walk=0;break;}index=n->nodes[index].parent;}
  if(walk && index==START){Complete(n,best,sb->DynPtr->Position);n->state=1;n->partial=1;}
 }
 for(int budget=0;budget<8 && (n->expandingJump?budget==0:n->neighbor<9);budget++){
  int index=n->neighbor++,dir=index%9,dx=dir%3-1,dz=dir/3-1;
  if(!dx&&!dz){budget--;continue;}int stride=n->expandingJump?index/9+1:1;int nx=x+dx*stride,nz=z+dz*stride;
  if(nx<0 || nz<0 || nx>=NPC_NAV_SIDE || nz>=NPC_NAV_SIDE)continue;
  NPC_NAV_NODE*next=&n->nodes[nz*NPC_NAV_SIDE+nx];
  int jump=n->expandingJump,cost=node->cost+(dx&&dz?354:250)*stride+(jump?3200:0);
  if(next->closed || (next->cost>=0 && next->cost<=cost)){if(jump)break;continue;}
  VECTORCH goal=GridPoint(n,nx,nz,node->position.vy);
  int rejected=0;for(int k=0;k<8;k++)if(n->blockedTime[k]>0 && abs(goal.vy-n->blocked[k].vy)<500 && HorizontalDistance(&goal,&n->blocked[k])<250){rejected=1;break;}
  if(rejected){node->pendingJump&=~(UINT64_C(1)<<index);if(jump)break;continue;}
  int crouch=0;JobStart(n);int valid=jump?JumpLink(sb,node->position,&goal):WalkLink(sb,node->position,&goal,1,&crouch);
  if(jump && valid)crouch=JumpCrouch;JobEnd();if(jobYield){n->neighbor=index;return;}n->rayCount=0;
  if(jump)node->pendingJump&=~(UINT64_C(1)<<index);else if(!valid)for(int stride=1;stride<=JUMP_CELLS;stride++)node->pendingJump|=UINT64_C(1)<<((stride-1)*9+index);
  if(valid){next->cost=cost;next->parent=best;next->position=goal;next->jump=jump;next->crouch=crouch;}
  if(jump)break;
 }
 if(n->expandingJump){n->expanding=-1;n->expandingJump=0;}
 else if(n->neighbor==9){node->closed=1;n->expanding=-1;}
}
/* A yielded collision job is not a command to stop. Only reuse a fully
   checked segment while the actor is still inside that corridor. */
static int ContinueWalk(NPC_LOCAL_NAV*n,VECTORCH position,VECTORCH*direction){
 if(!n->walkValid)return 0;
 VECTORCH segment=n->walkEnd,offset=position;SubVector(&n->walkFrom,&segment);SubVector(&n->walkFrom,&offset);segment.vy=offset.vy=0;
 int64_t length=(int64_t)segment.vx*segment.vx+(int64_t)segment.vz*segment.vz;
 int64_t along=(int64_t)offset.vx*segment.vx+(int64_t)offset.vz*segment.vz;
 int64_t cross=(int64_t)offset.vx*segment.vz-(int64_t)offset.vz*segment.vx;
 if(length<=0 || (double)along < -50.0*sqrt((double)length) || along>=length || (double)cross*cross>(double)length*150*150 || HorizontalDistance(&position,&n->walkEnd)<80){n->walkValid=0;return 0;}
 *direction=n->walkEnd;SubVector(&position,direction);direction->vy=0;Normalise(direction);n->crouched=n->walkCrouch;return 1;
}
int NPCNavDirection(STRATEGYBLOCK *sb,NPC_LOCAL_NAV*n,VECTORCH *goal,VECTORCH *direction){
 if(!sb->SBdptr || !sb->DynPtr->IsInContactWithFloor)return 0;
 VECTORCH position=sb->DynPtr->Position;int retries=0;
retry:
 for(int k=0;k<8;k++)if(n->blockedTime[k]>0)n->blockedTime[k]-=NormalFrameTime;
 if(n->goalKind!=n->itemGoal || (n->itemGoal && n->state && HorizontalDistance(goal,&n->goal)>500)){
  n->walkValid=0;n->state=n->jobKind=n->rayCount=n->checkTimer=0;
 }n->goalKind=n->itemGoal;
 /* Moving squad leaders must not starve an incremental search by resetting
    it every two metres. Geometry/costs stay valid when the goal changes.
    Finish the current bounded path, then plan from the actual new position. */
 if(n->state && HorizontalDistance(goal,&n->goal)>2000)n->goal=*goal;
 if(!n->state){
  int wasWalking=n->walkValid;VECTORCH checkedDirection;int walking=ContinueWalk(n,position,&checkedDirection);
  if(wasWalking && !walking)n->checkTimer=0;
  n->checkTimer-=NormalFrameTime;
  int remaining=walking?HorizontalDistance(&position,&n->walkEnd):0;
  if(n->jobKind!=1 && walking && remaining>900 && n->checkTimer>0){*direction=checkedDirection;return 1;}
  if(n->jobKind!=1 && !walking && n->checkTimer>0){*direction=(VECTORCH){0,0,0};return 1;}
  if(n->jobKind!=1){n->jobFrom=position;n->jobTo=*goal;
   VECTORCH ahead=*goal;SubVector(&position,&ahead);ahead.vy=0;int distance=Magnitude(&ahead);
   if(distance>1200){Normalise(&ahead);n->jobTo=position;n->jobTo.vx+=MUL_FIXED(ahead.vx,1200);n->jobTo.vz+=MUL_FIXED(ahead.vz,1200);}
   n->rayCount=0;n->jobKind=1;}
  VECTORCH end=n->jobTo;int crouch=0;JobStart(n);int clear=WalkLink(sb,n->jobFrom,&end,1,&crouch);JobEnd();
  if(jobYield){if(walking)*direction=checkedDirection;else *direction=(VECTORCH){0,0,0};return 1;}
  n->jobKind=n->rayCount=0;n->checkTimer=ONE_FIXED/8;
  if(clear){n->walkFrom=n->jobFrom;n->walkEnd=end;n->walkCrouch=crouch;n->walkValid=1;
   if(ContinueWalk(n,position,direction))return 1;
   n->checkTimer=0;*direction=(VECTORCH){0,0,0};return 1;}
  n->walkValid=0;Begin(sb,n,position,*goal);
 }
 if(n->state==1){
  /* Edges resume at the exact next collision query after yielding. */
  Expand(sb,n);if(n->state!=2 && !n->count){*direction=(VECTORCH){0,0,0};return n->state!=0;}
 }
 if(n->state==2 || (n->state==1 && n->count)){
  
  while(n->step<n->count){
   VECTORCH a=n->step?n->path[n->step-1]:n->origin,b=n->path[n->step],edge=b,offset=position;SubVector(&a,&edge);SubVector(&b,&offset);edge.vy=offset.vy=0;
   int64_t length=(int64_t)edge.vx*edge.vx+(int64_t)edge.vz*edge.vz,along=(int64_t)edge.vx*offset.vx+(int64_t)edge.vz*offset.vz,cross=(int64_t)edge.vx*offset.vz-(int64_t)edge.vz*offset.vx;
   if((HorizontalDistance(&position,&b)<160 && abs(position.vy-b.vy)<500) || (!n->jump[n->step] && abs(position.vy-b.vy)<500 && length>0 && along>=0 && (double)cross*cross<(double)length*450*450))n->step++;else break;
  }
  if(n->step==n->count){n->count=n->step=0;if(!n->partial)n->state=0;n->checkTimer=0;n->walkValid=0;if(!n->partial && !retries++){goto retry;}*direction=(VECTORCH){0,0,0};return 1;}
  if(HorizontalDistance(&position,&n->progress)>250){n->progress=position;n->progressTime=0;}
  else n->progressTime+=NormalFrameTime;
  if(n->progressTime>2*ONE_FIXED){
   /* Native collision is the final authority. Do not repeatedly choose the
      same mesh-clear edge that the real collider cannot traverse. */
   int k=n->blockedCursor++%8;n->blocked[k]=n->path[n->step];n->blockedTime[k]=10*ONE_FIXED;
   Begin(sb,n,position,*goal);*direction=(VECTORCH){0,0,0};return 1;}
  n->crouched=n->crouch[n->step];
  if(n->jump[n->step] && sb->I_SBtype==I_BehaviourMarine){
    MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
    VECTORCH landing=n->path[n->step];
    int ready=0;
    if(q->jumpTimer<=0){
     if(n->jobKind!=2){n->jobFrom=position;n->jobTo=landing;n->rayCount=0;n->jobKind=2;}
     landing=n->jobTo;JobStart(n);ready=JumpLink(sb,n->jobFrom,&landing);JobEnd();
     if(jobYield){*direction=(VECTORCH){0,0,0};return 1;}n->jobKind=n->rayCount=0;
    }
    if(ready){
     if(JumpCrouch)n->crouched=1;
     double time=(JumpImpulse+sqrt((double)JumpImpulse*JumpImpulse+2.0*GRAVITY_STRENGTH*(landing.vy-position.vy)))/GRAVITY_STRENGTH;
     sb->DynPtr->LinImpulse.vy=-JumpImpulse;sb->DynPtr->IsInContactWithFloor=0;sb->DynPtr->TimeNotInContactWithFloor=0;
     q->airborneSpeed=(int)(HorizontalDistance(&position,&landing)/time);
     q->jumpTimer=(int)(time*ONE_FIXED)+ONE_FIXED/4;q->airborneDirection=landing;SubVector(&position,&q->airborneDirection);q->airborneDirection.vy=0;Normalise(&q->airborneDirection);
     sb->DynPtr->LinVelocity.vx=MUL_FIXED(q->airborneDirection.vx,q->airborneSpeed);sb->DynPtr->LinVelocity.vz=MUL_FIXED(q->airborneDirection.vz,q->airborneSpeed);NPCMotionReset(&m->precision);
     n->jump[n->step]=0;
    }
   }
   *direction=n->path[n->step];SubVector(&position,direction);direction->vy=0;Normalise(direction);return 1;
 }
 return 0;
}
