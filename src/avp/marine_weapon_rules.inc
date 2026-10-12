static int MarinePistolChannel(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;
 return m->My_Weapon->id==MNPCW_TwoPistols && m->My_Gunflash_Section && !strcmp(m->My_Gunflash_Section->sempai->Section_Name,"dum flash");
}
/* Use player timing/ammunition definitions without replacing the human inventory.
 * Per-slot/channel clocks survive target changes, gun swaps and save/load. */
static int MarineStateDuration(int weapon,int state){
 int rate=TemplateWeapon[weapon].TimeOutRateForState[state];
 return rate>0?(int)(((int64_t)ONE_FIXED*ONE_FIXED+rate-1)/rate):0;
}
static int MarineShotInterval(int weapon,int secondary){
 int fire=secondary?WEAPONSTATE_FIRING_SECONDARY:WEAPONSTATE_FIRING_PRIMARY;
 int recoil=secondary?WEAPONSTATE_RECOIL_SECONDARY:WEAPONSTATE_RECOIL_PRIMARY;
 int interval=MarineStateDuration(weapon,fire)+MarineStateDuration(weapon,recoil);
 int rate=TemplateWeapon[weapon].FiringRate;
 int minimum=rate>0?(int)(((int64_t)ONE_FIXED*ONE_FIXED+rate-1)/rate):0;
 return interval>minimum?interval:minimum;
}
static int MarineShotReady(STRATEGYBLOCK*sb,int secondary){
 if(!NPCSupportCanCommand(sb))return 1;
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
 return q->activeSlot>=0 && !q->reloadDelay && !q->shotDelay[q->activeSlot][secondary!=0] &&
  (secondary?q->weapons[q->activeSlot].SecondaryRoundsRemaining>=ONE_FIXED:m->clipammo!=0);
}
static void MarineShotCommitted(STRATEGYBLOCK*sb,int secondary){
 if(!NPCSupportCanCommand(sb))return;
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
 if(q->activeSlot>=0)q->shotDelay[q->activeSlot][secondary!=0]=MarineShotInterval(q->weapons[q->activeSlot].WeaponIDNumber,secondary);
}
static void MarineWeaponTick(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
 for(int i=0;i<MAX_NO_OF_WEAPON_SLOTS;i++)for(int c=0;c<2;c++){int*t=&q->shotDelay[i][c];if(*t>0){*t-=NormalFrameTime;if(*t<0)*t=0;}}
 if(q->reloadDelay>0){q->reloadDelay-=NormalFrameTime;if(q->reloadDelay<0)q->reloadDelay=0;}
 /* Rendering is optional; simulation must consume new animation markers once. */
 if(m->Target || m->behaviourState==MBS_Reloading || m->behaviourState==MBS_PanicReloading || m->behaviourState==MBS_PumpAction)ProveHModel_Far(&m->HModelController,sb);
}
static void MarineReloadClock(STRATEGYBLOCK*sb){
 if(!NPCSupportCanCommand(sb))return;
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
 int weapon=NPCInventoryPlayerWeapon(m->My_Weapon->id);
 q->reloadDelay=MarineStateDuration(weapon,WEAPONSTATE_RELOAD_PRIMARY);
}
static int MarineWeaponRate(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;
 return NPCSupportCanCommand(sb)?TemplateWeapon[NPCInventoryPlayerWeapon(m->My_Weapon->id)].FiringRate:m->My_Weapon->FiringRate;
}
static int MarineFuelSpent(STRATEGYBLOCK*sb){
 if(!NPCSupportCanCommand(sb))return NormalFrameTime;
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;NPC_MARINE_SQUAD*q=&m->squad;
 uint64_t carry=((uint64_t)q->fuelCarryHigh<<32)|q->fuelCarryLow;
 uint64_t denominator=(uint64_t)TemplateAmmo[AMMO_FLAMETHROWER].AmmoPerMagazine*ONE_FIXED;
 uint64_t total=carry+(uint64_t)NormalFrameTime*TemplateWeapon[WEAPON_FLAMETHROWER].FiringRate*m->My_Weapon->clip_size;
 int spent=(int)(total/denominator);carry=total%denominator;q->fuelCarryLow=(unsigned int)carry;q->fuelCarryHigh=(unsigned int)(carry>>32);return spent;
}
static enum AMMO_ID MarineWeaponAmmo(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;
 return NPCSupportCanCommand(sb)?TemplateWeapon[NPCInventoryPlayerWeapon(m->My_Weapon->id)].PrimaryAmmoID:m->My_Weapon->Ammo_Type;
}
