/* A squad goal persists across combat states; movement is still native dynamics. */
static int SquadRay(STRATEGYBLOCK*sb,VECTORCH from,VECTORCH to,DISPLAYBLOCK*ignore){
 VECTORCH direction=to;SubVector(&from,&direction);int distance=Magnitude(&direction);if(!distance)return 1;Normalise(&direction);
 FindPolygonInLineOfSight_TwoIgnores(&direction,&from,0,sb->SBdptr,ignore);
 return !LOS_ObjectHitPtr || LOS_Lambda>=distance-30;
}
static int SquadFireClear(STRATEGYBLOCK*sb,STRATEGYBLOCK*target,int crouch){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;VECTORCH eye,end;
 if(m->My_Gunflash_Section)eye=m->My_Gunflash_Section->World_Offset;else GetTargetingPointOfObject_Far(sb,&eye);
 if(crouch)eye.vy+=550;NPCGetTargetPosition(&end,target);
 return SquadRay(sb,eye,end,target->SBdptr);
}
static void SquadSwitch(STRATEGYBLOCK*sb,int slot){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;PLAYER_WEAPON_DATA*w=&q->weapons[slot];
 MARINE_WEAPON_DATA*weapon=GetThisNPCMarineWeapon(NPCInventoryNativeWeapon(w->WeaponIDNumber));if(!weapon)return;
 SECTION*root=GetNamedHierarchyFromLibrary(weapon->Riffname,weapon->HierarchyName);if(!root)return;
 EndMarineMuzzleFlash(sb);
 const char*names[]={"sprintheaddelta","Minigun","HitDelta","Elevation"};
 for(int i=0;i<4;i++)if(Get_Delta_Sequence(&m->HModelController,(char*)names[i]))Remove_Delta_Sequence(&m->HModelController,(char*)names[i]);
 m->HModelController.Sequence_Type=HMSQT_MarineStand;m->HModelController.Sub_Sequence=MSSS_Standard;
 Transmogrify_HModels(sb,&m->HModelController,root,0,1,0);m->My_Weapon=weapon;
 m->My_Gunflash_Section=weapon->GunflashName?GetThisSectionData(m->HModelController.section_data,weapon->GunflashName):NULL;
 m->My_Elevation_Section=weapon->ElevationSection?GetThisSectionData(m->HModelController.section_data,weapon->ElevationSection):NULL;
 if(weapon->UseElevation && HModelSequence_Exists(&m->HModelController,HMSQT_MarineStand,MSSS_Elevation)){DELTA_CONTROLLER*d=Add_Delta_Sequence(&m->HModelController,"Elevation",HMSQT_MarineStand,MSSS_Elevation,0);if(d)d->timer=32767;}
 if(weapon->id==MNPCW_Minigun && HModelSequence_Exists(&m->HModelController,HMSQT_MarineStand,MSSS_Minigun_Delta)){DELTA_CONTROLLER*d=Add_Delta_Sequence(&m->HModelController,"Minigun",HMSQT_MarineStand,MSSS_Minigun_Delta,ONE_FIXED/8);if(d){d->Playing=0;d->Looped=1;}}
 if(HModelSequence_Exists(&m->HModelController,HMSQT_MarineStand,MSSS_HitChestFront)){DELTA_CONTROLLER*d=Add_Delta_Sequence(&m->HModelController,"HitDelta",HMSQT_MarineStand,MSSS_HitChestFront,ONE_FIXED/4);if(d)d->Playing=0;}
 q->activeSlot=slot;m->clipammo=w->PrimaryRoundsRemaining/ONE_FIXED;
 if(w->WeaponIDNumber==WEAPON_TWO_PISTOLS)m->clipammo+=w->SecondaryRoundsRemaining/ONE_FIXED;
 if(w->WeaponIDNumber==WEAPON_FLAMETHROWER)m->clipammo=(int)(((int64_t)w->PrimaryRoundsRemaining*weapon->clip_size)/TemplateAmmo[AMMO_FLAMETHROWER].AmmoPerMagazine);
 if(w->WeaponIDNumber==WEAPON_CUDGEL)m->clipammo=-1;
 m->smartAimValid=0;m->volleySize=0;m->lasthitsection=NULL;m->weapon_variable=m->weapon_variable2=0;
 DeInitialise_HModel(&m->HModelController);ProveHModel_Far(&m->HModelController,sb);Marine_Enter_Wait_State(sb);
}
static int SquadSafeFloor(STRATEGYBLOCK*sb,VECTORCH*point){
 VECTORCH origin=*point,down={0,ONE_FIXED,0};origin.vy-=1200;
 FindPolygonInLineOfSight_Navigation(&down,&origin,sb->SBdptr,Player);
 if(!LOS_ObjectHitPtr || LOS_ObjectNormal.vy>-40000 || LOS_Lambda>2200)return 0;
 point->vy=LOS_Point.vy;return ModuleFromPosition(point,sb->containingModule)!=NULL;
}

static VECTORCH SquadFollowGoal(STRATEGYBLOCK*sb){
 VECTORCH goal=Player->ObStrategyBlock->DynPtr->Position;
 NPC_LOCAL_NAV*n=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->localNav;
 if(n->followSlotValid){AddVector(&n->followSlot,&goal);return goal;}
 int rank=0;for(int i=0;i<NumActiveStBlocks;i++)if(ActiveStBlockList[i]!=sb && NPCSupportCanCommand(ActiveStBlockList[i]) && ActiveStBlockList[i]->supportFollow && memcmp(ActiveStBlockList[i]->SBname,sb->SBname,SB_NAME_LENGTH)<0)rank++;
 /* Stable world-space lanes avoid circling when the human rotates in place.
    The collision planner still chooses the route to each slot. */
 int angle=((rank%2)*2048+(rank/2)*512+((MARINE_STATUS_BLOCK*)sb->SBdataptr)->localNav.followOffset)&4095;goal.vx+=MUL_FIXED(GetSin(angle),2400);goal.vz+=MUL_FIXED(GetCos(angle),2400);
 return goal;
}
static VECTORCH SquadPatrol(STRATEGYBLOCK*sb){
 return sb->supportFollow?SquadFollowGoal(sb):NPCSupportPatrolGoal(sb);
}
static int SquadPace(STRATEGYBLOCK*sb,VECTORCH direction,int remaining){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
 int sprint=q->moveStyle==MMS_Sprint;int desired=remaining>(sprint?3000:4500)?7000:3000;
 if(!sb->supportFollow && !m->Target){VECTORCH leaderTravel={0,0,0};
  if(remaining>550 && NPCSupportPatrolTravel(sb,&leaderTravel) && Magnitude(&leaderTravel)>4800)desired=7000;
 }
 q->paceTimer-=NormalFrameTime;
 VECTORCH drift=direction;SubVector(&q->paceDirection,&drift);
 if(q->paceTimer>0 && Magnitude(&drift)<12000 && (remaining>4500 || remaining<3000 || desired==q->desiredPace))desired=q->desiredPace;
 else {q->paceTimer=ONE_FIXED/8;q->paceDirection=direction;VECTORCH from=sb->DynPtr->Position;from.vy-=950;
 for(int range=800;range<=1800;range+=1000){VECTORCH to=from;to.vx+=MUL_FIXED(direction.vx,range);to.vz+=MUL_FIXED(direction.vz,range);
  if(m->localNav.count && m->localNav.step<m->localNav.count){VECTORCH end=m->localNav.path[m->localNav.step];SubVector(&sb->DynPtr->Position,&end);end.vy=0;if(Magnitude(&end)<range)break;}
  if(!SquadRay(sb,from,to,Player)){desired=range==800?1600:2600;break;}
  VECTORCH foot=to;foot.vy=sb->DynPtr->Position.vy;if(!SquadSafeFloor(sb,&foot)){desired=1600;break;}
 }
 q->desiredPace=desired;}
 if(!sb->supportFollow && !m->Target){int cap=NPCSupportPatrolPace(sb);if(desired>cap)desired=cap;}
 int step=MUL_FIXED(10000,NormalFrameTime);if(step<1)step=1;
 if(q->speed<desired){q->speed+=step;if(q->speed>desired)q->speed=desired;}else{q->speed-=step;if(q->speed<desired)q->speed=desired;}
 int running=q->moveStyle==MMS_Sprint;
 q->moveStyle=q->speed>(running?3600:4800)?MMS_Sprint:m->Target?MMS_Combat:MMS_Alert;return q->speed;
}
static int MarineSupportControl(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;
 if(!sb->containingModule || !NPCSupportCanCommand(sb) || !Player || !Player->ObStrategyBlock->DynPtr || !Player->ObStrategyBlock->containingModule)return 0;
 NPCInventoryInit(sb);NPCInventorySync(sb);NPC_MARINE_SQUAD*q=&m->squad;
  m->sentryGoalTimer-=NormalFrameTime;
 /* Legacy discharge states zero vertical speed: never run them mid jump. */
 if(q->jumpTimer>0 && !sb->DynPtr->IsInContactWithFloor){q->jumpTimer-=NormalFrameTime;if(q->jumpTimer<0)q->jumpTimer=0;m->IAmCrouched=q->movementCrouch;m->nearSpeed=q->airborneSpeed;m->acceleration=10000;NPCSetVelocity(sb,&q->airborneDirection,q->airborneSpeed);HandleMovingAnimations(sb);return 1;}
 if(m->Target && NPC_IsDead(m->Target))m->Target=NULL;
 q->enemyMemory-=NormalFrameTime;q->sightTimer-=NormalFrameTime;
 if(q->sightTimer<=0){q->sightTimer=ONE_FIXED/8;
  if(m->Target){q->enemyMemory=5*ONE_FIXED;q->enemyPosition=m->Target->DynPtr->Position;memcpy(q->enemyName,m->Target->SBname,SB_NAME_LENGTH);}
  else for(int i=0;i<NumActiveStBlocks;i++){
   STRATEGYBLOCK*ally=ActiveStBlockList[i];if(ally==sb || !NPCSupportCanCommand(ally) || VectorDistance(&ally->DynPtr->Position,&sb->DynPtr->Position)>30000)continue;
   MARINE_STATUS_BLOCK*a=ally->SBdataptr;STRATEGYBLOCK*enemy=a->Target;
   if(!enemy || NPC_IsDead(enemy) || !enemy->DynPtr)continue;
   q->enemyMemory=5*ONE_FIXED;q->enemyPosition=enemy->DynPtr->Position;memcpy(q->enemyName,enemy->SBname,SB_NAME_LENGTH);
   if(enemy->SBdptr && MarineCanSeeObject(sb,enemy)){m->Target=enemy;memcpy(m->Target_SBname,enemy->SBname,SB_NAME_LENGTH);}
   break;
  }
 }
 int range=m->Target?VectorDistance(&sb->DynPtr->Position,&m->Target->DynPtr->Position):10000;
 int transient=m->behaviourState==MBS_Reloading||m->behaviourState==MBS_PanicReloading||m->behaviourState==MBS_PumpAction||m->behaviourState==MBS_GetWeapon;
 if(!transient){int slot=NPCInventoryChooseWeapon(sb,range);if(slot>=0)SquadSwitch(sb,slot);}
 switch(m->behaviourState){case MBS_Reloading:case MBS_PanicReloading:case MBS_PumpAction:case MBS_GetWeapon:case MBS_Taunting:case MBS_AcidAvoidance:
  /* Near actors outside the view still have to finish their reload/taunt.
     Pose evaluation is guarded by GlobalFrameCounter, so rendering cannot
     advance the animation twice. */
  ProveHModel_Far(&m->HModelController,sb);WanderMission_Control(sb);return 1;default:break;}
 if(m->clipammo==0 && (m->My_Weapon->id==MNPCW_SADAR || m->My_Weapon->id==MNPCW_Skeeter)){m->clipammo=NPCInventoryReload(sb);if(m->clipammo>0)Marine_Enter_Wait_State(sb);}
 if(m->clipammo==0 && q->activeSlot>=0 && (q->weapons[q->activeSlot].PrimaryMagazinesRemaining || q->weapons[q->activeSlot].SecondaryMagazinesRemaining)){Marine_Enter_Reload_State(sb);return 1;}
 /* A fixed formation slot can be inside a wall even though the human is
    reachable. Try the next stable slot only after sustained lack of actual
    progress nearby; the ordinary collision planner validates the new route. */
 NPC_LOCAL_NAV*followNav=&m->localNav;
 VECTORCH followError={0,0,0};int followHeight=0;
 if(sb->supportFollow){followError=SquadFollowGoal(sb);SubVector(&sb->DynPtr->Position,&followError);followHeight=abs(followError.vy);followError.vy=0;}
 int followBlocked=followHeight>=600 || Magnitude(&followError)>(followNav->arrivalHold?1150:650);
 if(sb->supportFollow && !m->Target && followBlocked && sb->DynPtr->IsInContactWithFloor && VectorDistance(&sb->DynPtr->Position,&Player->ObStrategyBlock->DynPtr->Position)<6000){
  VECTORCH progress=sb->DynPtr->Position;SubVector(&followNav->followProgress,&progress);progress.vy=0;
  VECTORCH walking=sb->DynPtr->LinVelocity;walking.vy=0;
  /* Gravity can slide a motionless actor by centimetres every frame. Only
     commanded movement credits navigation progress; retain native sliding. */
  if(Magnitude(&progress)>200 && Magnitude(&walking)>200){followNav->followProgress=sb->DynPtr->Position;followNav->followStalled=0;}
  else followNav->followStalled+=NormalFrameTime;
  if(followNav->followStalled>2*ONE_FIXED){
   VECTORCH slot=sb->DynPtr->Position;SubVector(&Player->ObStrategyBlock->DynPtr->Position,&slot);
   VECTORCH horizontal=slot;horizontal.vy=0;int gap=Magnitude(&horizontal);
   VECTORCH eye=sb->DynPtr->Position,end=Player->ObStrategyBlock->DynPtr->Position;eye.vy-=1200;end.vy-=1200;
   int cover=gap>1800 && gap<3500 && SquadRay(sb,eye,end,Player);
   int offset=(followNav->followOffset+512)&4095;NPCNavReset(followNav);
   /* A real collision-valid position with sight of the human is usable
      cover. Keep its relative offset instead of chasing a blocked ideal
      slot forever. Otherwise try the next slot through ordinary navigation. */
   followNav->followSlotValid=cover;if(cover)followNav->followSlot=slot;
   followNav->followOffset=offset;followNav->followProgress=sb->DynPtr->Position;
  }
 }else followNav->followStalled=0;
 int looting=0;VECTORCH goal=SquadPatrol(sb);
 int tether=VectorDistance(&sb->DynPtr->Position,&Player->ObStrategyBlock->DynPtr->Position);
 /* Supply duty is local to the human. A follow command must not leave a
    marine searching an abandoned rack or waiting for its lottery turn. */
 int resupply=!sb->supportFollow && NPCInventoryCanResupply(sb);
 if(!m->Target && resupply)NPCSupportSentryGoal(sb,&goal);
  if(!m->Target && q->enemyMemory>0){STRATEGYBLOCK*enemy=FindSBWithName(q->enemyName);if(!enemy || NPC_IsDead(enemy))q->enemyMemory=0;else if(!sb->supportFollow || VectorDistance(&q->enemyPosition,&Player->ObStrategyBlock->DynPtr->Position)<18000)goal=q->enemyPosition;}
 int combat=m->Target!=NULL;
 int shoot=0;q->retreating=0;int regroup=sb->supportFollow && VectorDistance(&sb->DynPtr->Position,&Player->ObStrategyBlock->DynPtr->Position)>8000;
 if(combat){
  int clear=sb->SBdptr && m->Target->SBdptr && SquadFireClear(sb,m->Target,0);
  int max=m->My_Weapon->MaxRange;
  shoot=clear && (max<0 || range<max) && range>=m->My_Weapon->MinRange && m->clipammo!=0;
  if(shoot && (m->My_Weapon->id==MNPCW_SADAR || m->My_Weapon->id==MNPCW_GrenadeLauncher)){
   MATRIXCH trajectory;VECTORCH origin=m->My_Gunflash_Section?m->My_Gunflash_Section->World_Offset:sb->DynPtr->Position;
   shoot=MarineProjectileAim(sb,&origin,m->My_Weapon->id==MNPCW_GrenadeLauncher,&trajectory);
  }
  q->retreating=range<3000 && m->My_Weapon->id!=MNPCW_End+1;
  if(shoot){
   /* Face the threat before native firing visibility checks. Keep weapon-specific
      attack sequences and their tween/keyframe state intact. */
   if(m->behaviourState!=MBS_Firing){
    VECTORCH face=m->Target->DynPtr->Position;SubVector(&sb->DynPtr->Position,&face);face.vy=0;
    if(!NPCOrientateToVector(sb,&face,NPC_TURNRATE,NULL)){NPCSetVelocity(sb,&face,0);return 1;}
    Marine_Enter_Firing_State(sb);
   }
   VECTORCH movingVelocity=sb->DynPtr->LinVelocity;
   WanderMission_Control(sb);NPCInventorySync(sb);
   if(!q->retreating && !regroup)return 1;
   /* Native fire functions stop feet while firing. Tactical movement must
      accelerate from the preceding velocity, not restart at zero each shot. */
   sb->DynPtr->LinVelocity=movingVelocity;
   VECTORCH away=sb->DynPtr->Position;SubVector(&m->Target->DynPtr->Position,&away);away.vy=0;Normalise(&away);
   goal=sb->DynPtr->Position;goal.vx+=MUL_FIXED(away.vx,3500);goal.vz+=MUL_FIXED(away.vz,3500);if(regroup)goal=SquadFollowGoal(sb);
  }else{
   q->tacticalTimer-=NormalFrameTime;
   if(regroup)q->tacticalTimer=0;
   if(q->tacticalTimer<=0){q->tacticalTimer=ONE_FIXED;q->tacticalGoal=m->Target->DynPtr->Position;
    /* A blocking vehicle/wall is a navigation problem, not a reason to fire forever. */
    if(!clear && sb->supportFollow && VectorDistance(&sb->DynPtr->Position,&Player->ObStrategyBlock->DynPtr->Position)>3000)q->tacticalGoal=SquadFollowGoal(sb);
   }
   goal=q->tacticalGoal;
  }
 }else if(sb->SBdptr && resupply && q->enemyMemory<=0 && !NPCSupportSentryGoal(sb,&goal)){STRATEGYBLOCK*item=NPCInventoryNearby(sb);if(item){goal=item->DynPtr->Position;looting=1;}
  if(NPCInventoryApproach(sb,&goal))looting=0;}
 MODULE*targetModule=ModuleFromPosition(&goal,Player->ObStrategyBlock->containingModule);AIMODULE*source=sb->containingModule->m_aimodule,*destination=targetModule?targetModule->m_aimodule:NULL;
 VECTORCH separation=goal;SubVector(&sb->DynPtr->Position,&separation);separation.vy=0;int distance=Magnitude(&separation);
 if(!source || !destination)return 0;
 int nearGoal=combat?650:looting?200:sb->supportFollow?650:550;
 int corridor=source==destination || (distance<60000 && IsModuleVisibleFromModule(sb->containingModule,targetModule));
 int following=sb->supportFollow && !combat && !looting;
 if(!following)m->localNav.arrivalHold=0;
 if(following){if(distance<nearGoal)m->localNav.arrivalHold=1;else if(distance>nearGoal+500)m->localNav.arrivalHold=0;}
 if(corridor && abs(goal.vy-sb->DynPtr->Position.vy)<600 && (distance<nearGoal || (following && m->localNav.arrivalHold)) && !q->retreating){
  /* Track actual human travel within the formation, including native ramp
     sliding. Repeated full stops/catch-up bursts look indecisive even when the
     human has released movement keys. No human position/physics is changed. */
  VECTORCH travel={0,0,0};
  if(following && NormalFrameTime>0){DYNAMICSBLOCK*d=Player->ObStrategyBlock->DynPtr;
   travel.vx=(int)((int64_t)(d->Position.vx-d->PrevPosition.vx)*ONE_FIXED/NormalFrameTime);
   travel.vz=(int)((int64_t)(d->Position.vz-d->PrevPosition.vz)*ONE_FIXED/NormalFrameTime);
   if(distance>350){VECTORCH correction=separation;Normalise(&correction);int speed=(distance-350)*2;
    travel.vx+=MUL_FIXED(correction.vx,speed);travel.vz+=MUL_FIXED(correction.vz,speed);}
  }
  if(!following && !combat && !looting)NPCSupportPatrolTravel(sb,&travel);
  int speed=Magnitude(&travel);q->moveStyle=speed>4800?MMS_Sprint:MMS_Alert;
  if(speed>100 && sb->DynPtr->IsInContactWithFloor){Normalise(&travel);if(speed>(following?3000:7000))speed=following?3000:7000;
   q->speed=speed;m->nearSpeed=speed;m->acceleration=10000;NPCSetVelocity(sb,&travel,speed);
   NPCOrientateToVector(sb,&travel,NPC_TURNRATE,NULL);m->behaviourState=MBS_Responding;HandleMovingAnimations(sb);
  }else{NPCNavStop(sb);q->speed=0;if(m->behaviourState!=MBS_Waiting)Marine_Enter_Wait_State(sb);HandleWaitingAnimations(sb);}
  return 1;}
 AIMODULE*next=NULL;
 if(source!=destination && !corridor){
  m->supportRouteTimer-=NormalFrameTime;
  if(m->supportSource!=source || m->supportDestination!=destination || m->supportRouteTimer<=0){m->supportSource=source;m->supportDestination=destination;m->supportRouteTimer=ONE_FIXED/2;m->supportNext=sb->supportFollow?NULL:NPCSupportPatrolNext(source,destination);if(!m->supportNext)m->supportNext=GetNextModuleForLink(source,destination,32,0);InitWaypointManager(&m->waypointManager);}
  next=m->supportNext;if(!next){sb->DynPtr->LinVelocity.vx=sb->DynPtr->LinVelocity.vz=0;return 1;}
  FARENTRYPOINT*ep=GetAIModuleEP(next,source);if(!ep || ep->alien_only)return 1;goal=ep->position;AddVector(&next->m_world,&goal);
 }else{m->supportSource=source;m->supportDestination=destination;}
 if(!sb->SBdptr){m->stateTimer-=NormalFrameTime;if(next && m->stateTimer<=0){ProcessFarMarineTargetModule(sb,next);m->stateTimer=MARINE_FAR_MOVE_TIME;}return 1;}
 if(!shoot){EndMarineMuzzleFlash(sb);m->behaviourState=MBS_Responding;m->IAmCrouched=0;m->gotapoint=0;}
 VECTORCH direction;
 if(q->jumpTimer>0){q->jumpTimer-=NormalFrameTime;if(q->jumpTimer<0)q->jumpTimer=0;}
 if(!sb->DynPtr->IsInContactWithFloor && q->jumpTimer>0)direction=q->airborneDirection;
 else {m->localNav.itemGoal=looting;
  if(!NPCNavDirection(sb,&m->localNav,&goal,&direction)){
   /* The legacy floor planner is anchored to the HUMAN floor polygon even
      when its target argument is a pickup or exit. This route has already
      selected/validated its own destination; retain that destination. */
   direction=goal;SubVector(&sb->DynPtr->Position,&direction);direction.vy=0;Normalise(&direction);
  }
 }
 q->movementCrouch=m->localNav.crouched;m->IAmCrouched=q->movementCrouch;
 int pace=(!sb->DynPtr->IsInContactWithFloor && q->jumpTimer>0)?q->airborneSpeed:SquadPace(sb,direction,distance);if(m->localNav.count && m->localNav.step<m->localNav.count){
 VECTORCH delta=m->localNav.path[m->localNav.step];SubVector(&sb->DynPtr->Position,&delta);delta.vy=0;
 int remaining=Magnitude(&delta);
 if(m->localNav.step+1<m->localNav.count){
  VECTORCH next=m->localNav.path[m->localNav.step+1];SubVector(&m->localNav.path[m->localNav.step],&next);next.vy=0;Normalise(&next);
  int alignment=MUL_FIXED(direction.vx,next.vx)+MUL_FIXED(direction.vz,next.vz);
  if(alignment<50000){int limit=1800+remaining*3;if(pace>limit)pace=limit;}
 }else{int limit=1000+remaining*4;if(pace>limit)pace=limit;}
 }if(q->movementCrouch && pace>2200)pace=2200;m->nearSpeed=pace;m->acceleration=10000;NPCSetVelocity(sb,&direction,pace);
 if(!shoot)NPCOrientateToVector(sb,&direction,NPC_TURNRATE,NULL);
 if(!shoot){HandleMovingAnimations(sb);CentreMarineElevation(sb);}
 return 1;
}
