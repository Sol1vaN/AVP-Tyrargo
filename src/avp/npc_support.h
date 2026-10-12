#ifndef NPC_SUPPORT_H
#define NPC_SUPPORT_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { char name[SB_NAME_LENGTH]; } NPC_DAMAGE_SOURCE;
typedef struct { int alive,onFire; AVP_BEHAVIOUR_TYPE type; NPC_DAMAGE_SOURCE source; } NPC_DAMAGE_EVENT;
NPC_DAMAGE_SOURCE NPCSupportPushSource(STRATEGYBLOCK *sb);
NPC_DAMAGE_SOURCE NPCSupportCurrentSource(void);
NPC_DAMAGE_SOURCE NPCSupportPushOwner(NPC_DAMAGE_SOURCE owner);
NPC_DAMAGE_SOURCE NPCSupportPushBurnSource(STRATEGYBLOCK *sb);
void NPCSupportIgnite(STRATEGYBLOCK *sb);
void NPCSupportPopSource(NPC_DAMAGE_SOURCE previous);
void NPCSupportInheritSource(STRATEGYBLOCK *sb);
NPC_DAMAGE_EVENT NPCSupportBeforeDamage(STRATEGYBLOCK *sb);
void NPCSupportAfterDamage(STRATEGYBLOCK *sb,NPC_DAMAGE_EVENT event);
void NPCSupportFrameBegin(void);
VECTORCH NPCSupportPatrolGoal(STRATEGYBLOCK *sb);
int NPCSupportPatrolPace(STRATEGYBLOCK*sb);
int NPCSupportPatrolTravel(STRATEGYBLOCK *sb,VECTORCH *velocity);
void NPCSupportPatrolReset(void);
void NPCSupportPatrolFrame(void);
AIMODULE* NPCSupportPatrolNext(AIMODULE*source,AIMODULE*destination);
AIMODULE *NPCSupportAlienHuntModule(STRATEGYBLOCK *sb);
void NPCSupportUpdate(STRATEGYBLOCK *sb);
int NPCSupportSmartgunHostile(STRATEGYBLOCK *shooter,STRATEGYBLOCK *target);
int NPCSupportSentryGoal(STRATEGYBLOCK *sb,VECTORCH *goal);
int NPCSupportPredatorRevealed(STRATEGYBLOCK *target);
int NPCSupportCanCommand(STRATEGYBLOCK *sb);
void NPCSupportUseInput(int pressed);
void NPCSupportCommand(STRATEGYBLOCK *sb);
void NPCSupportReset(void);
void NPCSupportSave(void);
struct save_block_header;
void NPCSupportLoad(struct save_block_header *header);
void NPCSupportPatrolSave(void);
int NPCSupportPatrolLoad(struct save_block_header *header);
#ifdef __cplusplus
}
#endif
#endif
