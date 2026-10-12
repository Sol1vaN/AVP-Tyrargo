
extern enum WEAPON_ID MarineWeaponKey[MAX_NO_OF_WEAPON_SLOTS];
extern MARINE_WEAPON_DATA *GetThisNPCMarineWeapon(MARINE_NPC_WEAPONS id);
int NPCInventoryNativeWeapon(int w){switch(w){
 case WEAPON_PULSERIFLE:return MNPCW_PulseRifle;case WEAPON_SMARTGUN:return MNPCW_Smartgun;
 case WEAPON_FLAMETHROWER:return MNPCW_Flamethrower;case WEAPON_SADAR:return MNPCW_SADAR;
 case WEAPON_GRENADELAUNCHER:return MNPCW_GrenadeLauncher;case WEAPON_MINIGUN:return MNPCW_Minigun;
 case WEAPON_MARINE_PISTOL:return MNPCW_PistolMarine;case WEAPON_TWO_PISTOLS:return MNPCW_TwoPistols;
 case WEAPON_FRISBEE_LAUNCHER:return MNPCW_Skeeter;case WEAPON_CUDGEL:return MNPCW_End+1;
 default:return -1;}}
int NPCInventoryPlayerWeapon(int w){switch(w){
 case MNPCW_PulseRifle:return WEAPON_PULSERIFLE;case MNPCW_Smartgun:return WEAPON_SMARTGUN;
 case MNPCW_Flamethrower:return WEAPON_FLAMETHROWER;case MNPCW_SADAR:return WEAPON_SADAR;
 case MNPCW_GrenadeLauncher:return WEAPON_GRENADELAUNCHER;case MNPCW_Minigun:return WEAPON_MINIGUN;
 case MNPCW_PistolMarine:return WEAPON_MARINE_PISTOL;case MNPCW_TwoPistols:return WEAPON_TWO_PISTOLS;
 case MNPCW_Skeeter:return WEAPON_FRISBEE_LAUNCHER;default:return WEAPON_CUDGEL;}}
static int NPCSlot(NPC_MARINE_SQUAD*q,int weapon){for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++)if(q->weapons[i].WeaponIDNumber==weapon)return i;return -1;}
static unsigned int NPCClipFixed(MARINE_STATUS_BLOCK*m){if(m->clipammo<=0)return 0;
 return m->My_Weapon->id==MNPCW_Flamethrower?(unsigned int)(((int64_t)m->clipammo*TemplateAmmo[AMMO_FLAMETHROWER].AmmoPerMagazine)/m->My_Weapon->clip_size):(unsigned int)m->clipammo*ONE_FIXED;}
static int NPCClipNative(MARINE_STATUS_BLOCK*m,PLAYER_WEAPON_DATA*w){
 if(w->WeaponIDNumber==WEAPON_CUDGEL)return -1;
 if(m->My_Weapon->id==MNPCW_Flamethrower)return (int)(((int64_t)w->PrimaryRoundsRemaining*m->My_Weapon->clip_size)/TemplateAmmo[AMMO_FLAMETHROWER].AmmoPerMagazine);
 if(w->WeaponIDNumber==WEAPON_TWO_PISTOLS)return (w->PrimaryRoundsRemaining+w->SecondaryRoundsRemaining)/ONE_FIXED;
 return w->PrimaryRoundsRemaining/ONE_FIXED;}
static void NPCGrenadeFields(GRENADE_LAUNCHER_DATA*g,int ammo,unsigned int**rounds,unsigned short**mags){
 switch(ammo){case AMMO_FLARE_GRENADE:*rounds=&g->FlareRoundsRemaining;*mags=&g->FlareMagazinesRemaining;break;case AMMO_FRAGMENTATION_GRENADE:*rounds=&g->FragmentationRoundsRemaining;*mags=&g->FragmentationMagazinesRemaining;break;case AMMO_PROXIMITY_GRENADE:*rounds=&g->ProximityRoundsRemaining;*mags=&g->ProximityMagazinesRemaining;break;default:*rounds=&g->StandardRoundsRemaining;*mags=&g->StandardMagazinesRemaining;break;}
}
int NPCInventoryGrenadeType(STRATEGYBLOCK*sb){NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;switch(q->grenades.SelectedAmmo){case AMMO_FRAGMENTATION_GRENADE:return I_BehaviourClusterGrenade;case AMMO_PROXIMITY_GRENADE:return I_BehaviourProximityGrenade;case AMMO_FLARE_GRENADE:return I_BehaviourFlareGrenade;default:return I_BehaviourGrenade;}}
void NPCInventoryInit(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;if(q->initialized)return;
 memset(q,0,sizeof(*q));q->initialized=1;q->activeSlot=-1;
 NPC_DATA*n=GetThisNpcData(I_NPC_Marine);q->maxHealth=n->StartingStats.Health*ONE_FIXED;
 q->maxArmour=AcidResourceCapacity(n->StartingStats.Armour)*ONE_FIXED;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++)q->weapons[i].WeaponIDNumber=MarineWeaponKey[i];
 int slot=NPCSlot(q,NPCInventoryPlayerWeapon(m->My_Weapon->id));
 if(slot>=0){q->activeSlot=slot;PLAYER_WEAPON_DATA*w=&q->weapons[slot];w->Possessed=1;w->PrimaryRoundsRemaining=NPCClipFixed(m);
  if(w->WeaponIDNumber==WEAPON_TWO_PISTOLS){w->SecondaryRoundsRemaining=w->PrimaryRoundsRemaining/2;w->PrimaryRoundsRemaining-=w->SecondaryRoundsRemaining;}
  w->PrimaryMagazinesRemaining=2;
  if(w->WeaponIDNumber==WEAPON_PULSERIFLE){w->SecondaryRoundsRemaining=TemplateAmmo[AMMO_PULSE_GRENADE].AmmoPerMagazine;w->SecondaryMagazinesRemaining=2;}
 }
 q->weapons[NPCSlot(q,WEAPON_CUDGEL)].Possessed=1;
 q->grenades.SelectedAmmo=AMMO_GRENADE;
}
void NPCInventorySync(STRATEGYBLOCK*sb){MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
 if(!q->initialized || q->activeSlot<0)return;PLAYER_WEAPON_DATA*w=&q->weapons[q->activeSlot];
 if(NPCInventoryNativeWeapon(w->WeaponIDNumber)!=m->My_Weapon->id){
  int slot=NPCSlot(q,NPCInventoryPlayerWeapon(m->My_Weapon->id));if(slot<0 || !q->weapons[slot].Possessed)return;
  q->activeSlot=slot;w=&q->weapons[slot];m->clipammo=NPCClipNative(m,w);
 }
 unsigned int rounds=NPCClipFixed(m);
 if(w->WeaponIDNumber==WEAPON_TWO_PISTOLS){w->PrimaryRoundsRemaining=(rounds/ONE_FIXED/2)*ONE_FIXED;w->SecondaryRoundsRemaining=rounds-w->PrimaryRoundsRemaining;}
 else w->PrimaryRoundsRemaining=rounds;
 if(w->WeaponIDNumber==WEAPON_GRENADELAUNCHER){unsigned int*r;unsigned short*c;NPCGrenadeFields(&q->grenades,q->grenades.SelectedAmmo,&r,&c);*r=w->PrimaryRoundsRemaining;w->PrimaryMagazinesRemaining=*c;}
}
/* The native reload's optimized partial magazines and Acid rounding are per actor. */
typedef struct {GRENADE_LAUNCHER_DATA grenades;unsigned int deficit[OPTIMIZED_AMMO_CHANNELS];int remainder[ACID_PICKUP_CHANNELS];} NPC_PICKUP_CONTEXT;
static void NPCBegin(STRATEGYBLOCK*sb,PLAYER_STATUS*proxy,NPC_PICKUP_CONTEXT*old){
 NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;
 memset(proxy,0,sizeof(*proxy));memcpy(proxy->WeaponSlot,q->weapons,sizeof q->weapons);
 old->grenades=GrenadeLauncherData;memcpy(old->deficit,OptimizedAmmoDeficit,sizeof old->deficit);memcpy(old->remainder,AcidPickupRemainders,sizeof old->remainder);
 GrenadeLauncherData=q->grenades;memcpy(OptimizedAmmoDeficit,q->deficits,sizeof q->deficits);memcpy(AcidPickupRemainders,q->pickupRemainders,sizeof q->pickupRemainders);
 InventoryNPC=sb;InventoryRecipient=proxy;
}
static void NPCEnd(STRATEGYBLOCK*sb,PLAYER_STATUS*proxy,NPC_PICKUP_CONTEXT*old){
 NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;memcpy(q->weapons,proxy->WeaponSlot,sizeof q->weapons);
 q->grenades=GrenadeLauncherData;memcpy(q->deficits,OptimizedAmmoDeficit,sizeof q->deficits);memcpy(q->pickupRemainders,AcidPickupRemainders,sizeof q->pickupRemainders);
 InventoryNPC=NULL;InventoryRecipient=NULL;GrenadeLauncherData=old->grenades;memcpy(OptimizedAmmoDeficit,old->deficit,sizeof old->deficit);memcpy(AcidPickupRemainders,old->remainder,sizeof old->remainder);
}
int NPCInventoryReload(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;if(!NPCSupportCanCommand(sb))return m->My_Weapon->clip_size;
 NPCInventoryInit(sb);NPCInventorySync(sb);NPC_MARINE_SQUAD*q=&m->squad;if(q->activeSlot<0)return 0;
 if(q->weapons[q->activeSlot].WeaponIDNumber==WEAPON_GRENADELAUNCHER){
  unsigned int*r;unsigned short*c;NPCGrenadeFields(&q->grenades,q->grenades.SelectedAmmo,&r,&c);
  if(!*r && !*c){int types[]={AMMO_GRENADE,AMMO_FRAGMENTATION_GRENADE,AMMO_PROXIMITY_GRENADE,AMMO_FLARE_GRENADE};for(int a=0;a<4;a++){NPCGrenadeFields(&q->grenades,types[a],&r,&c);if(*r || *c){q->grenades.SelectedAmmo=types[a];break;}}}
 }
 PLAYER_STATUS proxy;NPC_PICKUP_CONTEXT old;NPCBegin(sb,&proxy,&old);PLAYER_WEAPON_DATA*w=&proxy.WeaponSlot[q->activeSlot];
 if(w->WeaponIDNumber==WEAPON_GRENADELAUNCHER){unsigned int*r;unsigned short*c;NPCGrenadeFields(&GrenadeLauncherData,GrenadeLauncherData.SelectedAmmo,&r,&c);w->PrimaryRoundsRemaining=*r;w->PrimaryMagazinesRemaining=*c;}
 ReloadWeaponMagazine(w,0);
 if(w->WeaponIDNumber==WEAPON_GRENADELAUNCHER){unsigned int*r;unsigned short*c;NPCGrenadeFields(&GrenadeLauncherData,GrenadeLauncherData.SelectedAmmo,&r,&c);*r=w->PrimaryRoundsRemaining;*c=w->PrimaryMagazinesRemaining;}
 if(w->WeaponIDNumber==WEAPON_TWO_PISTOLS || (w->WeaponIDNumber==WEAPON_PULSERIFLE && !w->SecondaryRoundsRemaining))ReloadWeaponMagazine(w,1);
 NPCEnd(sb,&proxy,&old);return NPCClipNative(m,&q->weapons[q->activeSlot]);
}
static int NPCAmmoLow(NPC_MARINE_SQUAD*q,int slot,int secondary){
 if(slot<0)return 0;PLAYER_WEAPON_DATA*w=&q->weapons[slot];int id=w->WeaponIDNumber;if(id<0)return 0;
 int ammo=secondary?TemplateWeapon[id].SecondaryAmmoID:TemplateWeapon[id].PrimaryAmmoID;if(ammo<0)return 0;
 if(id==WEAPON_GRENADELAUNCHER && !secondary)ammo=q->grenades.SelectedAmmo;
 int64_t cap=AcidMagazineRounds(ammo,TemplateAmmo[ammo].AmmoPerMagazine);
 if(cap<=0)return 0;
 int64_t amount=secondary?w->SecondaryRoundsRemaining:w->PrimaryRoundsRemaining;
 unsigned int mags=secondary?w->SecondaryMagazinesRemaining:w->PrimaryMagazinesRemaining;
 if(id==WEAPON_GRENADELAUNCHER && !secondary){switch(q->grenades.SelectedAmmo){
 case AMMO_GRENADE:mags=q->grenades.StandardMagazinesRemaining;break;case AMMO_FLARE_GRENADE:mags=q->grenades.FlareMagazinesRemaining;break;
 case AMMO_FRAGMENTATION_GRENADE:mags=q->grenades.FragmentationMagazinesRemaining;break;case AMMO_PROXIMITY_GRENADE:mags=q->grenades.ProximityMagazinesRemaining;break;}}
 amount+=(int64_t)mags*cap;
 int channel=id*2+(secondary!=0);if(id==WEAPON_GRENADELAUNCHER && !secondary)channel=q->grenades.SelectedAmmo==AMMO_FLARE_GRENADE?61:q->grenades.SelectedAmmo==AMMO_PROXIMITY_GRENADE?62:q->grenades.SelectedAmmo==AMMO_FRAGMENTATION_GRENADE?63:60;
 if(mags && channel>=0 && channel<OPTIMIZED_AMMO_CHANNELS && q->deficits[channel]<(unsigned int)cap)amount-=q->deficits[channel];return amount*100<((int64_t)AcidResourceCapacity(99)+1)*cap*30;
}

/* Spatial supply bays: hold the turn through collection and departure. */
typedef struct {int used,timer,exiting,radius;VECTORCH centre;char owner[SB_NAME_LENGTH];} NPC_SUPPLY_BAY;
static NPC_SUPPLY_BAY supplyBays[16];
void NPCInventoryReset(void){memset(supplyBays,0,sizeof supplyBays);}
static void NPCLoadout(STRATEGYBLOCK*sb){
 NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;if(q->loadoutReady && q->loadoutTimer>0)return;
 int first=!q->loadoutReady;q->loadoutReady=1;q->loadoutTimer=ONE_FIXED/2;int owned=0;unsigned int available=0;
 for(int j=0;j<MAX_NO_OF_WEAPON_SLOTS;j++)if(q->weapons[j].Possessed && q->weapons[j].WeaponIDNumber!=WEAPON_CUDGEL)owned++;
 if(first)q->additionalPicked=owned>1?owned-1:0;
 STRATEGYBLOCK*skipped=q->pickupSkipTimer>0?FindSBWithName(q->pickupSkipName):NULL;
 int skippedWeapon=skipped && skipped->I_SBtype==I_BehaviourInanimateObject && skipped->SBdataptr && ((INANIMATEOBJECT_STATUSBLOCK*)skipped->SBdataptr)->typeId==IOT_Weapon?((INANIMATEOBJECT_STATUSBLOCK*)skipped->SBdataptr)->subType:-1;
 for(int i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK*o=ActiveStBlockList[i];if(o->I_SBtype!=I_BehaviourInanimateObject || !o->SBdataptr || !o->DynPtr || o->SBflags.please_destroy_me)continue;
  INANIMATEOBJECT_STATUSBLOCK*d=o->SBdataptr;if(d->subType==skippedWeapon || d->typeId!=IOT_Weapon || d->event_target || d->respawnTimer || (q->pickupSkipTimer>0 && !memcmp(o->SBname,q->pickupSkipName,SB_NAME_LENGTH)))continue;
  int nearby=VectorDistance(&o->DynPtr->Position,&sb->DynPtr->Position)<=14000;
  for(int b=0;!nearby && b<16;b++)if(supplyBays[b].used && VectorDistance(&sb->DynPtr->Position,&supplyBays[b].centre)<supplyBays[b].radius+3500 && VectorDistance(&o->DynPtr->Position,&supplyBays[b].centre)<supplyBays[b].radius)nearby=1;
  if(!nearby)continue;
  int slot=NPCSlot(q,d->subType);if(slot>=0 && !q->weapons[slot].Possessed && d->subType!=WEAPON_CUDGEL && NPCInventoryNativeWeapon(d->subType)>=0)available|=1u<<slot;
 }
 q->loadoutMask&=available;available&=~q->loadoutMask;int chosen=0;for(int j=0;j<MAX_NO_OF_WEAPON_SLOTS;j++)if(q->loadoutMask&(1u<<j))chosen++;
 for(int n=q->additionalPicked+chosen;n<2 && available;n++){
  int count=0;for(int j=0;j<MAX_NO_OF_WEAPON_SLOTS;j++)if(available&(1u<<j))count++;
  int rank=FastRandom()%count;for(int j=0;j<MAX_NO_OF_WEAPON_SLOTS;j++)if(available&(1u<<j)){if(!rank--){q->loadoutMask|=1u<<j;available&=~(1u<<j);break;}}
 }
 /* If no supplies were nearby, retry when the squad reaches a supply bay. */
 if(!q->loadoutMask && q->additionalPicked<2)q->loadoutReady=0;
}
static NPC_SUPPLY_BAY*NPCBay(STRATEGYBLOCK*item){
 if(item->SBname[0]=='N' && item->SBname[1]=='W')return NULL; /* A corpse drop is not an APC rack. */
 int freeSlot=-1;
 for(int i=0;i<16;i++){if(!supplyBays[i].used){if(freeSlot<0)freeSlot=i;continue;}if(VectorDistance(&supplyBays[i].centre,&item->DynPtr->Position)<10000)return &supplyBays[i];}
 if(freeSlot<0)return NULL;NPC_SUPPLY_BAY*b=&supplyBays[freeSlot];b->used=1;b->centre=item->DynPtr->Position;
 /* The rack's centre comes from the whole nearby supply group, not the
    first weapon at one end of a long vehicle. */
 int64_t x=0,y=0,z=0;int count=0;
 for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*o=ActiveStBlockList[i];
  if(o->I_SBtype!=I_BehaviourInanimateObject || !o->DynPtr || !o->SBdataptr || o->SBflags.please_destroy_me)continue;
  INANIMATEOBJECT_STATUSBLOCK*d=o->SBdataptr;
  if(d->typeId!=IOT_Weapon || d->event_target || d->respawnTimer || VectorDistance(&o->DynPtr->Position,&item->DynPtr->Position)>10000)continue;
  x+=o->DynPtr->Position.vx;y+=o->DynPtr->Position.vy;z+=o->DynPtr->Position.vz;count++;
 }
 if(count)b->centre=(VECTORCH){x/count,y/count,z/count};b->radius=4500;
 for(int i=0;i<NumActiveStBlocks;i++){STRATEGYBLOCK*o=ActiveStBlockList[i];
  if(o->I_SBtype!=I_BehaviourInanimateObject || !o->DynPtr || !o->SBdataptr || ((INANIMATEOBJECT_STATUSBLOCK*)o->SBdataptr)->typeId!=IOT_Weapon || VectorDistance(&o->DynPtr->Position,&item->DynPtr->Position)>10000)continue;
  VECTORCH delta=o->DynPtr->Position;SubVector(&b->centre,&delta);delta.vy=0;int r=Magnitude(&delta)+1500;if(r>b->radius)b->radius=r;
 }
 return b;
}
static int NPCBayOnRaisedFloor(STRATEGYBLOCK*sb,NPC_SUPPLY_BAY*b){
 if(!sb || !sb->DynPtr)return 0;
 for(COLLISIONREPORT*c=sb->DynPtr->CollisionReportPtr;c;c=c->NextCollisionReportPtr){STRATEGYBLOCK*o=c->ObstacleSBPtr;
  if(c->ObstacleNormal.vy<-40000 && o && o->I_SBtype==I_BehaviourInanimateObject && o->DynPtr && o->DynPtr->IsStatic && !o->DynPtr->IsPickupObject && VectorDistance(&o->DynPtr->Position,&b->centre)<b->radius+1000)return 1;
 }return 0;
}
/* A rack task belongs to the human's current supply area, not to the
   marine's unfinished weapon quota after the human has left. */
int NPCInventoryCanResupply(STRATEGYBLOCK*sb){
 NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;
 int tether=VectorDistance(&sb->DynPtr->Position,&Player->ObStrategyBlock->DynPtr->Position);
 if(!sb->supportFollow)return 1;
 STRATEGYBLOCK*item=FindSBWithName(q->pickupName);
 if(item && item->DynPtr && item->I_SBtype==I_BehaviourInanimateObject && item->SBdataptr && ((INANIMATEOBJECT_STATUSBLOCK*)item->SBdataptr)->typeId==IOT_Weapon && VectorDistance(&item->DynPtr->Position,&sb->DynPtr->Position)<14000)NPCBay(item);
 for(int i=0;i<16;i++){NPC_SUPPLY_BAY*b=&supplyBays[i];if(b->used && q->additionalPicked<2 && VectorDistance(&sb->DynPtr->Position,&b->centre)<b->radius+3500 && VectorDistance(&Player->ObStrategyBlock->DynPtr->Position,&b->centre)<b->radius+(sb->supportFollow?3500:8000))return 1;}
 return sb->supportFollow && tether<3500;
}
static int NPCBayTurn(STRATEGYBLOCK*sb,NPC_SUPPLY_BAY*b){
 if(!b)return 1;
 STRATEGYBLOCK*owner=FindSBWithName(b->owner);
 if(!NPCSupportCanCommand(owner) || !NPCInventoryCanResupply(owner) || ((MARINE_STATUS_BLOCK*)owner->SBdataptr)->Target || (b->timer<=0 && !b->exiting && !NPCBayOnRaisedFloor(owner,b)) || (b->exiting && VectorDistance(&owner->DynPtr->Position,&b->centre)>b->radius+500)){
  memset(b->owner,0,SB_NAME_LENGTH);b->exiting=0;owner=NULL;
 }
 if(!owner){
  STRATEGYBLOCK*winner=NULL;int count=0;
  for(int i=0;i<NumActiveStBlocks;i++){
   STRATEGYBLOCK*s=ActiveStBlockList[i];if(!NPCSupportCanCommand(s) || ((MARINE_STATUS_BLOCK*)s->SBdataptr)->Target || VectorDistance(&s->DynPtr->Position,&b->centre)>b->radius+3500)continue;
   if(!NPCInventoryCanResupply(s))continue;
   NPCInventoryInit(s);NPCLoadout(s);NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)s->SBdataptr)->squad;
   if(q->additionalPicked>=2 || !q->loadoutMask)continue;
   if(FastRandom()%++count==0)winner=s;
  }
  if(winner){memcpy(b->owner,winner->SBname,SB_NAME_LENGTH);b->timer=30*ONE_FIXED;owner=winner;}
 }
 
 if(VectorDistance(&Player->ObStrategyBlock->DynPtr->Position,&b->centre)<b->radius+1000)return 0;
 /* An NPC already on the rack's raised floor must leave before the
    lottery winner approaches. The initial map occupant is not the winner
    automatically; it clears the doorway when another marine wins. */
 if(owner==sb && !NPCBayOnRaisedFloor(sb,b))for(int i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK*other=ActiveStBlockList[i];if(other!=sb && NPCSupportCanCommand(other) && NPCBayOnRaisedFloor(other,b))return 0;
 }
 return owner==sb && !b->exiting;
}
static VECTORCH NPCBayWait(STRATEGYBLOCK*sb,NPC_SUPPLY_BAY*b){
 /* Wait on the human's side of the supplies, rather than behind the APC
    wall selected by a radial projection from an interior NPC position. */
 VECTORCH away=Player->ObStrategyBlock->DynPtr->Position;SubVector(&b->centre,&away);away.vy=0;
 if(!away.vx&&!away.vz){away=sb->DynPtr->Position;SubVector(&b->centre,&away);away.vy=0;}
 if(!away.vx&&!away.vz)away.vx=ONE_FIXED;Normalise(&away);
 VECTORCH goal=b->centre;goal.vx+=MUL_FIXED(away.vx,b->radius+1500);goal.vz+=MUL_FIXED(away.vz,b->radius+1500);
 /* Two lanes leave the approach to the human and active quartermaster. */
 int lane=((unsigned char)sb->SBname[7]&1)?1000:-1000;
 goal.vx+=MUL_FIXED(away.vz,lane);goal.vz-=MUL_FIXED(away.vx,lane);goal.vy=sb->DynPtr->Position.vy;return goal;
}
int NPCInventoryApproach(STRATEGYBLOCK*sb,VECTORCH*goal){
 NPC_MARINE_SQUAD*loadout=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;
 /* Courtesy applies to collecting the two additional weapons, not to every
    later traversal of the supply area's large enclosing sphere. An equipped
    patrol must keep its exploration goal even when it passes beside the APC. */
 if(loadout->additionalPicked>=2){
  for(int i=0;i<16;i++){NPC_SUPPLY_BAY*b=&supplyBays[i];
   if(b->used && !memcmp(b->owner,sb->SBname,SB_NAME_LENGTH)){
    if(NPCBayOnRaisedFloor(sb,b))b->exiting=1;
    else{memset(b->owner,0,SB_NAME_LENGTH);b->exiting=0;}
   }
  }return 0;
 }
 for(int i=0;i<16;i++){
  NPC_SUPPLY_BAY*b=&supplyBays[i];if(!b->used || VectorDistance(&sb->DynPtr->Position,&b->centre)>b->radius+3500)continue;
  int turn=NPCBayTurn(sb,b);STRATEGYBLOCK*owner=FindSBWithName(b->owner);NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;
  if(owner==sb && q->additionalPicked>=2)b->exiting=1;
  if((owner==sb && b->exiting) || (!turn && (q->additionalPicked<2 || VectorDistance(&sb->DynPtr->Position,&b->centre)<b->radius+500))){*goal=NPCBayWait(sb,b);return 1;}
  if(turn && q->additionalPicked<2 && !FindSBWithName(q->pickupName)){*goal=b->centre;return 1;}
 }
 return 0;
}

int NPCInventoryWanted(STRATEGYBLOCK*sb,STRATEGYBLOCK*item){
 if(!item || item->I_SBtype!=I_BehaviourInanimateObject || !item->SBdataptr || item->SBflags.please_destroy_me)return 0;
 INANIMATEOBJECT_STATUSBLOCK*o=item->SBdataptr;if(o->respawnTimer || o->event_target)return 0; /* Leave mission/script objectives for humans. */
 NPCInventoryInit(sb);NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;
 if(o->typeId==IOT_Health)return (int64_t)sb->SBDamageBlock.Health*100<(int64_t)q->maxHealth*60;
 if(o->typeId==IOT_Armour)return (int64_t)sb->SBDamageBlock.Armour*100<(int64_t)q->maxArmour*60;
 if(o->typeId==IOT_Weapon){int slot=NPCSlot(q,o->subType);if(slot<0 || NPCInventoryNativeWeapon(o->subType)<0)return 0;
  NPCLoadout(sb);if(!q->weapons[slot].Possessed)return q->additionalPicked<2 && (q->loadoutMask&(1u<<slot));
  /* Repeated pistol pickups do not bypass the two distinct weapon quota. */
  return 0; /* Weapon duplicates remain for the human; ammo pickups restock NPCs. */}
 if(o->typeId==IOT_Ammo){for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){int id=q->weapons[i].WeaponIDNumber;if(id<0 || !q->weapons[i].Possessed)continue;
  if(TemplateWeapon[id].PrimaryAmmoID==o->subType && NPCAmmoLow(q,i,0))return 1;
  if(TemplateWeapon[id].SecondaryAmmoID==o->subType && NPCAmmoLow(q,i,1))return 1;
 }if(o->subType==AMMO_FLARE_GRENADE || o->subType==AMMO_FRAGMENTATION_GRENADE || o->subType==AMMO_PROXIMITY_GRENADE)return (o->subType==AMMO_FLARE_GRENADE?q->grenades.FlareMagazinesRemaining:o->subType==AMMO_FRAGMENTATION_GRENADE?q->grenades.FragmentationMagazinesRemaining:q->grenades.ProximityMagazinesRemaining)*100<AcidResourceCapacity(99)*30;}
 return 0;
}
int NPCInventoryPickup(STRATEGYBLOCK*sb,STRATEGYBLOCK*item){
 if(!NPCInventoryWanted(sb,item))return 0;MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;INANIMATEOBJECT_STATUSBLOCK*o=item->SBdataptr;int accepted=0;
 int newWeapon=o->typeId==IOT_Weapon && !q->weapons[NPCSlot(q,o->subType)].Possessed;
 NPC_SUPPLY_BAY*bay=o->typeId==IOT_Weapon?NPCBay(item):NULL;if(newWeapon && !NPCBayTurn(sb,bay))return 0;
 PLAYER_STATUS proxy;NPC_PICKUP_CONTEXT old;NPCBegin(sb,&proxy,&old);
 switch(o->typeId){case IOT_Weapon:accepted=AbleToPickupWeapon(o->subType);break;case IOT_Ammo:accepted=AbleToPickupAmmo(o->subType);break;
 case IOT_Health:sb->SBDamageBlock.Health=q->maxHealth;sb->SBDamageBlock.IsOnFire=0;accepted=1;break;
 case IOT_Armour:sb->SBDamageBlock.Armour=q->maxArmour;accepted=1;break;default:break;}
 if(q->activeSlot>=0 && proxy.WeaponSlot[q->activeSlot].WeaponIDNumber==WEAPON_GRENADELAUNCHER){PLAYER_WEAPON_DATA*w=&proxy.WeaponSlot[q->activeSlot];unsigned int*r;unsigned short*c;NPCGrenadeFields(&GrenadeLauncherData,GrenadeLauncherData.SelectedAmmo,&r,&c);w->PrimaryMagazinesRemaining=*c;}
 NPCEnd(sb,&proxy,&old);
 if(accepted){if(newWeapon){if(bay)bay->timer=30*ONE_FIXED;q->additionalPicked++;q->loadoutMask&=~(1u<<NPCSlot(q,o->subType));if(bay && q->additionalPicked>=2)bay->exiting=1;}if(q->activeSlot>=0)m->clipammo=NPCClipNative(m,&q->weapons[q->activeSlot]);RemovePickedUpObjectForActor(item,sb);}
 return accepted;
}
/* Match the human's shelf reach without changing the NPC collider. The
   actual eye-to-item segment must remain unobstructed, even for weapons. */
static int NPCItemReachable(STRATEGYBLOCK*sb,STRATEGYBLOCK*item){
 VECTORCH eye=sb->DynPtr->Position,ray=item->DynPtr->Position;
 int top=sb->SBdptr->ObMinY;if(top>-1934)top=-1934;
 if(!NPCPickupWithinReach((double)ray.vx-eye.vx,(double)ray.vy-eye.vy,(double)ray.vz-eye.vz,top,sb->SBdptr->ObMaxY))return 0;
 eye.vy+=sb->SBdptr->ObMinY+200;SubVector(&eye,&ray);int distance=Magnitude(&ray);if(!distance)return 1;Normalise(&ray);
 VECTORCH point=LOS_Point,normal=LOS_ObjectNormal;DISPLAYBLOCK*hit=LOS_ObjectHitPtr;SECTION_DATA*section=LOS_HModel_Section;int lambda=LOS_Lambda;
 FindPolygonInLineOfSight_Navigation(&ray,&eye,sb->SBdptr,Player);
 int visible=!LOS_ObjectHitPtr || LOS_Lambda>=distance-30;
 LOS_Point=point;LOS_ObjectNormal=normal;LOS_ObjectHitPtr=hit;LOS_HModel_Section=section;LOS_Lambda=lambda;return visible;
}
STRATEGYBLOCK*NPCInventoryNearby(STRATEGYBLOCK*sb){
 NPC_MARINE_SQUAD*q=&((MARINE_STATUS_BLOCK*)sb->SBdataptr)->squad;
 /* Once equipped, take useful supplies along the patrol instead of repeatedly
    abandoning exploration for a distant rack. An exhausted loadout can still
    plan a longer resupply trip. Thresholds/quota/courtesy remain unchanged. */
 int radius=14000,armed=0;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){PLAYER_WEAPON_DATA*w=&q->weapons[i];
  if(w->Possessed && w->WeaponIDNumber!=WEAPON_CUDGEL && (w->PrimaryRoundsRemaining || w->PrimaryMagazinesRemaining || w->SecondaryRoundsRemaining || w->SecondaryMagazinesRemaining))armed=1;
 }
 if(q->additionalPicked>=2 && armed)radius=2500;
 q->loadoutTimer-=NormalFrameTime;
 if(q->pickupSkipTimer>0){q->pickupSkipTimer-=NormalFrameTime;if(q->pickupSkipTimer<=0)memset(q->pickupSkipName,0,SB_NAME_LENGTH);}
 for(int i=0;i<16;i++)if(supplyBays[i].used && !memcmp(supplyBays[i].owner,sb->SBname,SB_NAME_LENGTH))supplyBays[i].timer-=NormalFrameTime;
 STRATEGYBLOCK*cached=FindSBWithName(q->pickupName);
 if(cached && (!cached->DynPtr || !NPCInventoryWanted(sb,cached) || VectorDistance(&sb->DynPtr->Position,&cached->DynPtr->Position)>radius))cached=NULL;
 if(cached){int d=VectorDistance(&sb->DynPtr->Position,&cached->DynPtr->Position);
  if(d<q->pickupDistance-200){q->pickupDistance=d;q->pickupProgress=12*ONE_FIXED;}else q->pickupProgress-=NormalFrameTime;
  if(q->pickupProgress<=0){memcpy(q->pickupSkipName,cached->SBname,SB_NAME_LENGTH);q->pickupSkipTimer=5*ONE_FIXED;q->loadoutTimer=0;cached=NULL;memset(q->pickupName,0,SB_NAME_LENGTH);}
 }
 if(cached){INANIMATEOBJECT_STATUSBLOCK*d=cached->SBdataptr;if(d->typeId==IOT_Weapon && !NPCBayTurn(sb,NPCBay(cached)))cached=NULL;}
 q->pickupTimer-=NormalFrameTime;if(q->pickupTimer>0)return cached;q->pickupTimer=ONE_FIXED/4;
 if(cached){
  if(NPCItemReachable(sb,cached)){
   if(NPCInventoryPickup(sb,cached)){cached=NULL;memset(q->pickupName,0,SB_NAME_LENGTH);}
  }
  if(cached)return cached;
 }
 STRATEGYBLOCK*best=NULL;int distance=radius;
 for(int i=0;i<NumActiveStBlocks;i++){
  STRATEGYBLOCK*o=ActiveStBlockList[i];if(!o->DynPtr || !o->SBdptr || (q->pickupSkipTimer>0 && !memcmp(o->SBname,q->pickupSkipName,SB_NAME_LENGTH)) || !NPCInventoryWanted(sb,o))continue;
  int d=VectorDistance(&sb->DynPtr->Position,&o->DynPtr->Position);if(d>=distance)continue;
  VECTORCH eye;GetTargetingPointOfObject_Far(sb,&eye);VECTORCH ray=o->DynPtr->Position;SubVector(&eye,&ray);int rayLength=Magnitude(&ray);Normalise(&ray);
  /* A supply may be behind the APC hull: route to it instead of rejecting
     it just because there is no direct eye-to-pickup line of sight. */
  INANIMATEOBJECT_STATUSBLOCK*data=o->SBdataptr;
  if(data->typeId!=IOT_Weapon){FindPolygonInLineOfSight_TwoIgnores(&ray,&eye,0,sb->SBdptr,o->SBdptr);if(LOS_ObjectHitPtr && LOS_Lambda<rayLength-30)continue;}
  else {NPC_SUPPLY_BAY*b=NPCBay(o);if(!NPCBayTurn(sb,b))continue;}
  if(NPCItemReachable(sb,o)){if(NPCInventoryPickup(sb,o)){i--;continue;}}
  else {best=o;distance=d;}
 }
 if(best){if(memcmp(q->pickupName,best->SBname,SB_NAME_LENGTH)){q->pickupDistance=distance;q->pickupProgress=12*ONE_FIXED;}memcpy(q->pickupName,best->SBname,SB_NAME_LENGTH);}else memset(q->pickupName,0,SB_NAME_LENGTH);
 return best;
}
int NPCInventoryChooseWeapon(STRATEGYBLOCK*sb,int range){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;NPCInventorySync(sb);
 q->weaponTimer-=NormalFrameTime;if(q->weaponTimer>0 && m->clipammo!=0)return -1;q->weaponTimer=ONE_FIXED;
 int best=-1,score=-999;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++){
  PLAYER_WEAPON_DATA*w=&q->weapons[i];int id=w->WeaponIDNumber,n=NPCInventoryNativeWeapon(id);if(!w->Possessed||n<0)continue;
  int extraGrenades=id==WEAPON_GRENADELAUNCHER && (q->grenades.StandardMagazinesRemaining||q->grenades.FragmentationMagazinesRemaining||q->grenades.ProximityMagazinesRemaining||q->grenades.FlareMagazinesRemaining||q->grenades.FragmentationRoundsRemaining||q->grenades.ProximityRoundsRemaining||q->grenades.FlareRoundsRemaining);
  if(id!=WEAPON_CUDGEL && !extraGrenades && !w->PrimaryRoundsRemaining && !w->SecondaryRoundsRemaining && !w->PrimaryMagazinesRemaining && !w->SecondaryMagazinesRemaining)continue;
  int value=id==WEAPON_SMARTGUN?85:id==WEAPON_PULSERIFLE?80:id==WEAPON_MINIGUN?75:id==WEAPON_TWO_PISTOLS?55:id==WEAPON_MARINE_PISTOL?40:id==WEAPON_CUDGEL?0:60;
  if(id==WEAPON_FLAMETHROWER)value=range<6500?90:10;
  if(id==WEAPON_SADAR || id==WEAPON_GRENADELAUNCHER || id==WEAPON_FRISBEE_LAUNCHER)value=range>10000?95:-50;
  if(i==q->activeSlot)value+=8; /* hysteresis: no frame-by-frame weapon churn */
  if(value>score){score=value;best=i;}
 }
 return best==q->activeSlot?-1:best;
}
