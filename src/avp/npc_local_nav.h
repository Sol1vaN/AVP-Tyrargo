#ifndef NPC_LOCAL_NAV_H
#define NPC_LOCAL_NAV_H
#define NPC_NAV_SIDE 75
#define NPC_NAV_NODES (NPC_NAV_SIDE*NPC_NAV_SIDE)
#define NPC_NAV_PATH 96
typedef struct {int cost,parent,closed,jump,crouch;uint64_t pendingJump;VECTORCH position;} NPC_NAV_NODE;
typedef struct {VECTORCH origin,direction,point,normal;int hit,lambda;} NPC_NAV_RAY;
typedef struct {
 /* Runtime only: keep traversing the last fully checked walking segment. */
 int walkValid,walkCrouch,arrivalHold,partial,yieldTime,yieldSide,patrolCursor,patrolCursorValid,patrolRegroup;VECTORCH walkFrom,walkEnd,yieldGoal;
 int followOffset,followStalled,followSlotValid;VECTORCH followProgress,followSlot;
 int allowance,rayCount,jobKind,frameIssued,itemGoal,goalKind;VECTORCH jobFrom,jobTo;NPC_NAV_RAY rays[1024];
 int state,checkTimer,count,step,progressTime,expanding,expandingJump,neighbor,crouched;VECTORCH origin,goal,progress,basisX,basisZ;
 VECTORCH blocked[8];int blockedTime[8],blockedCursor;
 NPC_NAV_NODE nodes[NPC_NAV_NODES];VECTORCH path[NPC_NAV_PATH];int jump[NPC_NAV_PATH],crouch[NPC_NAV_PATH];
} NPC_LOCAL_NAV;
void NPCNavFrameBegin(void);
void NPCNavStop(STRATEGYBLOCK *sb);
void NPCNavReset(NPC_LOCAL_NAV *nav);
int NPCNavDirection(STRATEGYBLOCK *sb,NPC_LOCAL_NAV *nav,VECTORCH *goal,VECTORCH *direction);
#endif
