/* Activate a placed enemy's extra cohort once, after its script has awakened it.
 * Existing generators already scale their cadence and limits. No allies cloned. */
static int acidPopulationRemainder;
static int AcidSpawnPosition(STRATEGYBLOCK*sb,VECTORCH*point){
 static const int x[8]={1,1,0,-1,-1,-1,0,1},z[8]={0,1,1,1,0,-1,-1,-1};
 for(int k=0;k<8;k++){
  VECTORCH candidate=sb->DynPtr->Position;candidate.vx+=x[k]*1800;candidate.vz+=z[k]*1800;
  VECTORCH top=candidate,down={0,ONE_FIXED,0};top.vy-=1600;
  FindPolygonInLineOfSight_TwoIgnores(&down,&top,0,sb->SBdptr,Player);
  if(!LOS_ObjectHitPtr || LOS_ObjectNormal.vy>-40000 || LOS_Lambda>3100)continue;
  candidate.vy=LOS_Point.vy;MODULE*module=ModuleFromPosition(&candidate,sb->containingModule);if(!module || !module->m_aimodule)continue;
  int valid=1;for(int i=0;i<NumActiveStBlocks;i++){
   STRATEGYBLOCK*o=ActiveStBlockList[i];if(!o->DynPtr || NPC_IsDead(o))continue;
   if((o->I_SBtype==I_BehaviourAlien || o->I_SBtype==I_BehaviourPredator || o->I_SBtype==I_BehaviourMarine || o==Player->ObStrategyBlock) && VectorDistance(&candidate,&o->DynPtr->Position)<1000){valid=0;break;}
  }if(!valid)continue;
  VECTORCH from=sb->DynPtr->Position;from.vy-=1000;VECTORCH to=candidate;to.vy-=1000;VECTORCH ray=to;SubVector(&from,&ray);int length=Magnitude(&ray);Normalise(&ray);
  FindPolygonInLineOfSight_TwoIgnores(&ray,&from,0,sb->SBdptr,Player);if(LOS_ObjectHitPtr && LOS_Lambda<length)continue;
  /* Clearance at both head and shoulders, plus a non-penetrating spawn origin. */
  for(int j=0;j<4;j++){VECTORCH a=candidate,b=candidate;a.vy-=1700;b.vy-=1700;b.vx+=j<2?(j?450:-450):0;b.vz+=j>=2?(j==2?450:-450):0;ray=b;SubVector(&a,&ray);Normalise(&ray);
   FindPolygonInLineOfSight_TwoIgnores(&ray,&a,0,sb->SBdptr,Player);if(LOS_ObjectHitPtr && LOS_Lambda<450){valid=0;break;}}
  if(valid){*point=candidate;return 1;}
 }
 return 0;
}
static void AcidPopulationUpdate(STRATEGYBLOCK*sb){
 if(!AcidActive() || !sb->SBdptr || sb->acidPopulationState<0 || NPC_IsDead(sb))return;
 if(sb->I_SBtype!=I_BehaviourAlien && sb->I_SBtype!=I_BehaviourPredator)return;
 if(sb->I_SBtype==I_BehaviourAlien){ALIEN_STATUS_BLOCK*a=sb->SBdataptr;if(a->generator_sbptr || a->BehaviourState==ABS_Wait || a->BehaviourState==ABS_Dormant)return;}
 if(!sb->acidPopulationState){static const int bonus[4]={0,5,10,15};acidPopulationRemainder+=bonus[MolecularAcidTier];int count=acidPopulationRemainder/10;acidPopulationRemainder%=10;sb->acidPopulationState=count?count+1:-1;}
 if(sb->acidPopulationState<0)return;
 sb->acidPopulationTimer-=NormalFrameTime;if(sb->acidPopulationTimer>0)return;sb->acidPopulationTimer=ONE_FIXED;
 VECTORCH position;if(!AcidSpawnPosition(sb,&position))return;
 STRATEGYBLOCK*clone=CreateActiveStrategyBlock();if(!clone)return;InitialiseSBValues(clone);AssignNewSBName(clone);clone->I_SBtype=sb->I_SBtype;
 char cloneName[SB_NAME_LENGTH];memcpy(cloneName,clone->SBname,SB_NAME_LENGTH);
 clone->maintainVisibility=1;clone->acidPopulationState=-1;
 if(sb->I_SBtype==I_BehaviourAlien){TOOLS_DATA_ALIEN data={0};data.position=position;data.shapeIndex=sb->shapeIndex;data.type=((ALIEN_STATUS_BLOCK*)sb->SBdataptr)->Type;data.starteuler=sb->DynPtr->OrientEuler;memcpy(data.nameID,clone->SBname,SB_NAME_LENGTH);InitAlienBehaviour(&data,clone);}
 else{PREDATOR_STATUS_BLOCK*pred=sb->SBdataptr;TOOLS_DATA_PREDATOR data={0};data.position=position;data.shapeIndex=sb->shapeIndex;data.predator_number=pred->personalNumber;data.primary=pred->PrimaryWeapon;data.secondary=pred->SecondaryWeapon;data.path=data.stepnumber=-1;memcpy(data.nameID,clone->SBname,SB_NAME_LENGTH);InitPredatorBehaviour(&data,clone);}
 clone=FindSBWithName(cloneName);
 if(clone && clone->SBdataptr && clone->DynPtr){clone->containingModule=ModuleFromPosition(&position,sb->containingModule);clone->acidPopulationState=-1;if(--sb->acidPopulationState==1)sb->acidPopulationState=-1;}
}
typedef struct {SAVE_BLOCK_HEADER header;int schema;char name[SB_NAME_LENGTH];int state,timer,remainder;} ACID_POPULATION_SAVE;
static void AcidPopulationSave(STRATEGYBLOCK*s){
 if(s->I_SBtype!=I_BehaviourAlien && s->I_SBtype!=I_BehaviourPredator)return;
 ACID_POPULATION_SAVE*b=GetPointerForSaveBlock(sizeof(*b));memset(b,0,sizeof(*b));b->header.type=SaveBlock_NPCSupport;b->header.size=sizeof(*b);b->schema=246;memcpy(b->name,s->SBname,SB_NAME_LENGTH);b->state=s->acidPopulationState;b->timer=s->acidPopulationTimer;b->remainder=acidPopulationRemainder;
}
static int AcidPopulationLoad(SAVE_BLOCK_HEADER*h){
 if(h->size!=sizeof(ACID_POPULATION_SAVE) || *(int*)(h+1)!=246)return 0;
 ACID_POPULATION_SAVE*b=(ACID_POPULATION_SAVE*)h;STRATEGYBLOCK*s=FindSBWithName(b->name);
 if(b->state>=-1 && b->state<=3 && b->remainder>=0 && b->remainder<10){acidPopulationRemainder=b->remainder;if(s){s->acidPopulationState=b->state;s->acidPopulationTimer=b->timer;}}
 return 1;
}
