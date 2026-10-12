[English](../../README.md) · [Español](../../README.es.md)

# Technical guide: code, flows and examples

Excerpts are generated from the current source snapshot. Links open the complete implementation. Flows are schematic; preserve the native simulation interfaces.

<a id="input"></a>

## Mouse, keyboard and bindings

SDL float deltas follow a direct angular path with signed rounding, fractional residuals and zoom/FOV scaling. Physical keyboard and extended mouse actions preserve species-specific bindings. This corrects tested conversion errors, not every possible hardware issue.

```text
SDL float counts → sensitivity/zoom → 16.16 angles → pmove residuals → fractional render pose
```

[src/avp/win95/usr_io.c:713](../../src/avp/win95/usr_io.c#L713)

```c
 * physical mouse distance, as in Quake 3's direct mouse path. */
static int64_t MouseCountsToAngleFixed(float counts, float sensitivity)
{
	const double result = (double)counts * (double)sensitivity * 2000.0;

	if (result >= (double)INT64_MAX) return INT64_MAX;
	if (result <= (double)INT64_MIN) return INT64_MIN;
	return (result >= 0.0) ? (int64_t)(result + 0.5) : (int64_t)(result - 0.5);
}

/* initialise the player input structure(s) in the player_status block */
void InitPlayerGameInput(STRATEGYBLOCK* sbPtr)
{
	PLAYER_STATUS *playerStatusPtr;

    /* get the player status block ... */
    playerStatusPtr = (PLAYER_STATUS *) (sbPtr->SBdataptr);
    LOCALASSERT(playerStatusPtr);
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Compare equal accumulated counts at several frame caps and in opposite directions; inspect tiny movements under zoom. Preserve legacy strafe/movement handling separately.

<a id="display"></a>

## Display modes and presentation

SDL window, drawable, framebuffer and logical canvas are coordinated through guarded changes and fallback. Texture restoration supports context recreation. Modal window movement services the game/menu and audio paths. VSync and caps are options; high-refresh simulation and the 1000-FPS setting remain audit tasks.

```text
requested mode → guarded SDL/GL transaction → drawable/framebuffer synchronization → texture/state restore or fallback
```

[src/main.c:2279](../../src/main.c#L2279)

```c
		Probe113_RenderTracef("VIDEO_MODE create_window begin flags=0x%x", flags);
		window = SDL_CreateWindow(TYRARGO_WINDOW_TITLE,
								  WindowWidth,
								  WindowHeight,
								  flags);
		if (window == NULL) {
			fprintf(stderr, "(OpenGL) SDL SDL_CreateWindow failed: %s\n", SDL_GetError());
			exit(EXIT_FAILURE);
		}
		Probe113_RenderTracef("VIDEO_MODE create_window ok window=%p", (void *)window);
		if (!ApplySelectedVideoMode()) {
			fprintf(stderr, "Could not apply requested video settings; SDL fallback remains active: %s\n", SDL_GetError());
		}
		
#if defined(_WIN32)
		EnableWindowsKeyHook();
#endif

/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Switch between all three modes, change resolution/AA, drag the window and return through menus. Check the actual drawable, not only the requested mode.

<a id="geometry"></a>

## Projection and geometric precision

Fractional render coordinates survive transforms and clipping. Dedicated world/viewmodel projections synchronize camera pose, frustum and active visibility descriptors without replacing the native gameplay representation.

```text
native gameplay pose + fractional render pose → camera/model transforms → clipping → OpenGL floats
```

[src/playeryaw.c:109](../../src/playeryaw.c#L109)

```c

double PlayerYawRadians(const MATRIXCH *orientation)
{
 if(PlayerYawOwns(orientation))return atan2(YawBasis[2][0],YawBasis[2][2]);
 return atan2((double)orientation->mat31,(double)orientation->mat33);
}
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Observe pistol barrel/slide during idle and zoom; compare FOV/aspect transitions. Render-only precision changes must not silently change collision shapes.

<a id="render"></a>

## Renderer, lighting and effects

Texture cutouts also constrain the additional luminous/depth pass. Shared UV clocks fix luminous-layer drift. Glass/effect ordering preserves visibility. Ordered buffer streaming and fire batching reduce submission work with original draw/blend/filter behavior and fallbacks.

```text
ordered vertices/indices → compatible streamed storage → original draw sequence → luminous/effect passes
```

[src/opengl.c:219](../../src/opengl.c#L219)

```c
 streamBind=(PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
 streamData=(PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
 streamDelete=(PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
 if(!streamGen || !streamBind || !streamData || !streamDelete)return;
 streamGen(1,&streamVertex);streamGen(1,&streamIndex);
}
static void GeometryStreamDraw(TriangleArray*indices,int triangles,int vertices,int specular){
 if(!streamVertex || !streamIndex){pglDrawElements(GL_TRIANGLES,triangles*3,GL_UNSIGNED_SHORT,indices);return;}
 streamBind(GL_ARRAY_BUFFER,streamVertex);
 streamData(GL_ARRAY_BUFFER,(GLsizeiptr)vertices*sizeof(varr[0]),varr,GL_STREAM_DRAW);
 streamBind(GL_ELEMENT_ARRAY_BUFFER,streamIndex);
 streamData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)triangles*sizeof(indices[0]),indices,GL_STREAM_DRAW);
 pglVertexPointer(4,GL_FLOAT,sizeof(varr[0]),(void*)offsetof(VertexArray,v));
 pglTexCoordPointer(2,GL_FLOAT,sizeof(varr[0]),(void*)offsetof(VertexArray,t));
 pglColorPointer(4,GL_UNSIGNED_BYTE,sizeof(varr[0]),(void*)(specular?offsetof(VertexArray,s):offsetof(VertexArray,c)));
 pglDrawElements(GL_TRIANGLES,triangles*3,GL_UNSIGNED_SHORT,NULL);
 streamBind(GL_ARRAY_BUFFER,0);streamBind(GL_ELEMENT_ARRAY_BUFFER,0);
 pglVertexPointer(4,GL_FLOAT,sizeof(varr[0]),varr[0].v);
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Compare alpha cutouts, all blend/filter modes, glass, halos and vision modes. Reordering translucent geometry is not an acceptable optimization.

<a id="water"></a>

## Water simulation and rendering

Time-based wave propagation and damping avoid integer loss and uncontrolled per-frame disturbances. Modern optics add depth-aware refraction, Fresnel, scene reflections, native-light specular and caustics. Screen-space effects cannot reflect unseen geometry.

```text
elapsed simulation time → wave integration → fractional height/gradient → water grid → depth/color optics
```

[src/water_effect.c:60](../../src/water_effect.c#L60)

```c
}
void WaterAdvance(int fixedDelta){
 if(fixedDelta<=0)return;
 clockTime+=fixedDelta/65536.0;
 waveRemainder+=(int64_t)fixedDelta*120;
 /* Engine dt, not render count. Long stalls settle instead of exploding. */
 if(waveRemainder>65536*120){for(int i=0;i<WATER_BODIES;i++){memset(bodies[i].h,0,sizeof bodies[i].h);memset(bodies[i].v,0,sizeof bodies[i].v);memset(bodies[i].old,0,sizeof bodies[i].old);}waveRemainder%=65536;}
 while(waveRemainder>=65536){for(int i=0;i<WATER_BODIES;i++)if(bodies[i].used&&clockTime-bodies[i].seen<10)step(&bodies[i]);waveRemainder-=65536;}
}
static void applyDrop(WaterBody*b,int x,int z,int amplitude){
  float dx=(b->x1-b->x0)/(float)(b->nx-1),dz=(b->z1-b->z0)/(float)(b->nz-1),r=fmaxf(620,4.0f*fmaxf(dx,dz));
  int loX=fmaxf(0,floorf((x-r-b->x0)/dx)),hiX=fminf(b->nx-1,ceilf((x+r-b->x0)/dx));
  int loZ=fmaxf(0,floorf((z-r-b->z0)/dz)),hiZ=fminf(b->nz-1,ceilf((z+r-b->z0)/dz));
  /* Zero-volume impulse: a crest surrounded by a compensating trough.
     Positive-only drops in a closed field would raise its mean indefinitely. */
  double crestSum=0,ringSum=0; /* Preserve zero volume on denser grids. */
  for(int ix=loX;ix<=hiX;ix++)for(int iz=loZ;iz<=hiZ;iz++){
   float px=b->x0+ix*dx-x,pz=b->z0+iz*dz-z,d=sqrtf(px*px+pz*pz)/r;
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Compare an isolated ripple over equal elapsed time at multiple frame caps; test foreground silhouettes and screen-edge reflection fallback.

<a id="postfx"></a>

## CRT, noise and image controls

Native GLSL CRT, image controls and monitor noise are selectable. Constant or interior-region work is kept out of expensive fragment paths where equivalent. CRT provenance includes Mattias/newpixie and CRT-Guest-ReShade; credits do not substitute license notices.

```text
scene framebuffer → GLSL image/CRT pass → selected presentation canvas
```

[src/crt_filter.c:151](../../src/crt_filter.c#L151)

```c
	if (CRTFilterVertexShader == 0 || CRTFilterFragmentShader == 0
		|| !CRTFilterCompileShader(CRTFilterVertexShader, CRTFilterVertexSource, "vertex")
		|| !CRTFilterCompileShader(CRTFilterFragmentShader, CRTFilterFragmentSource, "fragment")) goto failed;

	CRTFilterProgram = pglCreateProgram();
	if (CRTFilterProgram == 0) goto failed;
	pglAttachShader(CRTFilterProgram, CRTFilterVertexShader);
	pglAttachShader(CRTFilterProgram, CRTFilterFragmentShader);
	pglLinkProgram(CRTFilterProgram);
	pglGetProgramiv(CRTFilterProgram, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE) {
		pglGetProgramiv(CRTFilterProgram, GL_INFO_LOG_LENGTH, &length);
		if (length > 1) {
			if (length >= (GLint)sizeof(log)) length = (GLint)sizeof(log) - 1;
			pglGetProgramInfoLog(CRTFilterProgram, length, NULL, log);
			log[length] = '\0';
		} else strcpy(log, "no linker log");
		fprintf(stderr, "CRT GLSL program link failed: %s\n", log);
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Use the supplied off/on pair for appearance. Measure the same scene at the same resolution for cost; a GIF is not a precision benchmark.

<a id="ui"></a>

## Main menu, in-game menu and UX

Menu presentation uses explicit logical spaces, atlas sampling guards and supported Latin composition. Species profiles retain scales. Ordinary settings apply with Escape; video-mode confirmation is deliberately separate. Supported save/load slots remain discoverable in carousels.

```text
profile settings → logical layout/font atlas → live preview → guarded apply/persist
```

[src/avp/win95/frontend/avp_menus.c:2007](../../src/avp/win95/frontend/avp_menus.c#L2007)

```c

static void RenderMenu(void)
{
	AVPMENU_ELEMENT *elementPtr;
	int e;
	int y;
	int firstElement = 0;
	int lastElement = AvPMenus.NumberOfElementsInMenu - 1;
	const int inGameMenuText = (AvPMenus.MenusState == MENUSSTATE_INGAMEMENUS);
	const int textScaleFixed = inGameMenuText ? D3D_GetConsoleTextScaleFixed() : ONE_FIXED;
	int renderedMenuHeight = AvPMenus.MenuHeight;
    if(AvPMenus.NumberOfElementsInMenu>0 && AvPMenus.CurrentlySelectedElement>=0 &&
       AvPMenus.CurrentlySelectedElement<AvPMenus.NumberOfElementsInMenu &&
       !MenuElementIsVisible(&AvPMenus.MenuElements[AvPMenus.CurrentlySelectedElement])){
        int count=AvPMenus.NumberOfElementsInMenu;
        for(int q=1;q<count;++q){
            int next=(AvPMenus.CurrentlySelectedElement+q)%count;
            if(MenuElementIsVisible(&AvPMenus.MenuElements[next])){AvPMenus.CurrentlySelectedElement=next;break;}
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Check accented text, control frame edges, death statistics and live previews in borderless and letterboxed modes.

<a id="media"></a>

## Movies and screenshots

Bundled Smacker/Bink decoders and buffered audio drive playback. Physical movie viewports support wide modes. Original commercial media is supplied by the player. BMP/PNG/JPEG captures exclude their own notifications.

```text
movie stream → native decoder → buffered audio clock → physical viewport → end/transition cleanup
```

[src/fmv_backend.cpp:741](../../src/fmv_backend.cpp#L741)

```c
    MenuMusicPlayingMode=-1;
    if(OpenSmackerAudioLoop(&MenuMusic,MenuMusicAttemptedMode ? "FMVs/IntroSound99.smk" : "FMVs/IntroSound.smk"))
        MenuMusicPlayingMode=MenuMusicAttemptedMode;
    else if(MenuMusicAttemptedMode && OpenSmackerAudioLoop(&MenuMusic,"FMVs/IntroSound.smk"))
        MenuMusicPlayingMode=0;
}
extern "C" void FMVBackend_EnsureMenuMusic(void)
{
    if(MenuMusicAttemptedMode != SelectedMenuMusicMode()) FMVBackend_StartMenuMusic();
}
extern "C" void FMVBackend_PlayMenuMusic(void)
{
    FMVBackend_EnsureMenuMusic();
    UpdateSmackerAudioLoop(&MenuMusic);
}
extern "C" void FMVBackend_EndMenuMusic(void)
{
    CloseSmackerAudioLoop(&MenuMusic); MenuMusicAttemptedMode=MenuMusicPlayingMode=-1;
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Test movie end, skip, audio-only menu music and wide modes; capture repeatedly without including notification text.

<a id="audio"></a>

## Audio, music and latency

OpenAL emitters follow world positions and environmental routing. Weapon overlap and feeding paths include sound handling. CD-DA support includes drive detection and volume; repeat modes include random silence. Bounded diagnostics defer writes away from frames.

```text
event + emitter position → OpenAL source/environment → playback; diagnostics → bounded buffers → deferred writes
```

[src/openal.c:258](../../src/openal.c#L258)

```c
			alSourcef(p, AL_GAIN, 1.0f);
			alSourcefv(p, AL_POSITION, ActiveSounds[i].PropSetP_pos);
			alSourcefv(p, AL_VELOCITY, ActiveSounds[i].PropSetP_vel);
		
			alSourcef(p, AL_ROLLOFF_FACTOR, 0.01f);
			alSourcef(p, AL_REFERENCE_DISTANCE, 1.0f);
		}
	}
	
	if (AvpSourceCount < SOUND_MAXACTIVE_SW) {
		PlatEndSoundSys();
		return 0;
	}
	SoundActivated = 1;
	
	return 1;
}

/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Move away from an NPC pickup, listen through geometry, overlap primary/secondary weapon sounds and compare feeding emitters.

<a id="player"></a>

## Player movement and weapons

Wall contacts, climbing and moving support have dedicated corrections while slope friction remains. Input options include run/crouch and smart zoom/jetpack. Fire-input queues retain valid taps; interaction/cooldown paths avoid frame-driven bursts.

```text
input requests → native movement/contact → supported platform or climbing transitions → camera feedback
```

[src/avp/pmove.c:106](../../src/avp/pmove.c#L106)

```c
 * C99 integer division truncates toward zero, keeping both directions equal. */
static int ConsumeRawMouseAngle(int64_t input, int64_t *remainder)
{
	int64_t total = input + *remainder;
	int64_t whole = total / ONE_FIXED;

	*remainder = total - whole * ONE_FIXED;
	if (whole > INT_MAX) return INT_MAX;
	if (whole < INT_MIN) return INT_MIN;
	return (int)whole;
}

/* Global Externs */
extern DISPLAYBLOCK* Player;
extern int NormalFrameTime;
extern int predHUDSoundHandle;
extern int predOVision_SoundHandle;
extern int TauntSoundPlayed;
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Test slopes without removing friction, descending lifts, wall turns, repeated fire taps and the flare cooldown.

<a id="motion"></a>

## Camera, weapon and HUD motion

Marine camera/viewmodel/HUD layers cover movement and equipment events with configurable channels. Transitions include stance, stairs and landing. Development calibration is opt-in source code; production retains imported values and public intensity controls.

```text
movement/equipment events → independent bounded motion channels → camera/viewmodel/HUD outputs
```

[src/camera_motion.c:43](../../src/camera_motion.c#L43)

```c
static double soft(double x,double limit){return limit>0?limit*tanh(x/limit):0;}
static void spring(MOTION_SPRING*s,double target,double frequency,double dt){
 double d=s->x-target,c=s->v+frequency*d,e=exp(-frequency*dt);
 s->x=target+(d+c*dt)*e;s->v=(s->v-frequency*c*dt)*e;
}
void CameraMotionReset(CAMERA_MOTION_STATE*s){memset(s,0,sizeof(*s));}
unsigned int CameraMotionPack(unsigned int hud,int mode){return(hud&0xffffffu)|((0xc0u+(unsigned)(mode>=0&&mode<=7?mode:0))<<24);}
int CameraMotionUnpack(unsigned int packed){
 unsigned m=packed>>24;
 if(m>=0xc0u&&m<=0xc7u)return (int)(m-0xc0u);
 if(m>=0xb0u&&m<=0xb5u)return m==0xb5u?7:(int)(m-0xb0u);
 /* Legacy Sway+Bobbing keeps its meaning; Shake is explicitly opt-in. */
 if(m>=0xa4u&&m<=0xa7u){static const int old[4]={0,1,2,4};return old[m-0xa4u];}
 return 0;
}
void CameraMotionUpdate(CAMERA_MOTION_STATE*s,const CAMERA_MOTION_INPUT*in,int mode){
 int n,k,j,bob=mode&2,sway=mode&1;double dt=in->dt,step,speed,attenuation;
 if(!in->active||mode<1||mode>7){CameraMotionReset(s);return;}
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Compare walking, running, crouching, reload and landing with each public intensity channel disabled/enabled. This is Marine motion; Predator/Alien expansion remains planned.

<a id="campaign"></a>

## Campaign, resources and Molecular Acid

Acid is a single-player modifier over Hard, not an extra index into native difficulty arrays. UI labels x1/x1.5/x2 map to enemy factors 1.5/2/2.5 and resource factors 1.5/2/3. Exceptions are quantities, not multipliers. Campaign state and optional ammo economy have separate lifetimes.

```text
selected Acid tier → native Hard behavior + population/resources → bounded capacities/pickup remainders → save state
```

[src/molecular_acid.c:12](../../src/molecular_acid.c#L12)

```c
int AcidPickupRemainders[ACID_PICKUP_CHANNELS];
static const int resourceTenths[4]={10,15,20,30};
static const int spawnTenths[4]={10,15,20,25};
void AcidResetPickups(void){memset(AcidPickupRemainders,0,sizeof AcidPickupRemainders);}
void AcidSetTier(int tier){MolecularAcidTier=(tier>=1&&tier<=3)?tier:0;AcidResetPickups();if(MolecularAcidTier)AvP.Difficulty=I_Hard;}
int AcidActive(void){return MolecularAcidTier>=1&&MolecularAcidTier<=3&&AvP.Network==I_No_Network&&AvP.Difficulty==I_Hard;}
static int bounded(int64_t n){return n>INT_MAX?INT_MAX:(int)n;}
int AcidResourceCapacity(int base){return AcidActive()&&base>0?bounded((int64_t)base*resourceTenths[MolecularAcidTier]/10):base;}
int AcidSpawnLimit(int base){return AcidActive()&&base>0?bounded(((int64_t)base*spawnTenths[MolecularAcidTier]+9)/10):base;}
int AcidSpawnInterval(int base){if(!AcidActive()||base<=0)return base;int n=(int)((int64_t)base*10/spawnTenths[MolecularAcidTier]);return n>0?n:1;}
int AcidPickupBonus(int channel,int amount){
 if(!AcidActive()||amount<=0||channel<0||channel>=ACID_PICKUP_CHANNELS)return 0;
 int64_t n=(int64_t)amount*(resourceTenths[MolecularAcidTier]-10)+AcidPickupRemainders[channel];
 AcidPickupRemainders[channel]=(int)(n%10);return bounded(n/10);
}
int AcidCompletion(void){return AcidActive()?5:AvP.Difficulty+1;}
const char *AcidMultiplierLabel(int tier){static const char *labels[]={"x1","x1","x1.5","x2"};return labels[tier>=1&&tier<=3?tier:1];}

/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Compare identical level starts across tiers and reload saved games. Pulse Rifle/SADAR/disc 3/4/5 are total quantities per pickup; Predator armor is 15 × resource factor.

<a id="aliens"></a>

## Alien NPC behavior and navigation

Fractional timing, local steering and support/contact recovery address tested vents, hatches and edges. Leap authorization is bounded: not every large leap is caused by FPS. Corpse feeding includes its animation and audio paths. Remaining arbitrary-map recovery needs playtesting.

```text
native goal/behavior → progress/support checks → local route/contact recovery → bounded leap or locomotion
```

[src/avp/npc_local_nav.c:20](../../src/avp/npc_local_nav.c#L20)

```c
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
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Use a repeatable vent/hatch save and record contacts, requested velocity and actual displacement. Separate authored high leaps from invalid repeated impulses.

<a id="predators"></a>

## Predator NPC combat and patrol

Private weapon state, target acquisition and patrol extend native combat. Moving shoulder-cannon acquisition uses locomotion while active shots retain native completion timing. Level navigation and cloak visibility still constrain pursuit.

```text
target eligibility → acquisition/weapon state → movement + locomotion clip → native shot completion
```

[src/avp/bh_pred.c:90](../../src/avp/bh_pred.c#L90)

```c
  p->huntTimer=6*ONE_FIXED;p->huntMode=!p->huntMode;
  AIMODULE*aliens=NPCSupportAlienHuntModule(sb);
  p->huntGoal=p->huntMode && aliens?aliens:Player->ObStrategyBlock->containingModule->m_aimodule;
 }
 AIMODULE*next=PredatorHuntRoute(sb,p->huntGoal);
 if(!next){p->huntTimer=0;return FarNPC_GetTargetAIModuleForHunt(sb,0);}return next;
}
static PRED_RETURN_CONDITION Execute_PFS_Hunt(STRATEGYBLOCK *sbPtr);
static PRED_RETURN_CONDITION Execute_PFS_Retreat(STRATEGYBLOCK *sbPtr);
static PRED_RETURN_CONDITION Execute_PFS_Avoidance(STRATEGYBLOCK *sbPtr);
static PRED_RETURN_CONDITION Execute_PFS_Pathfinder(STRATEGYBLOCK *sbPtr);
static PRED_RETURN_CONDITION Execute_PFS_Return(STRATEGYBLOCK *sbPtr);
static PRED_RETURN_CONDITION Execute_PFS_Engage(STRATEGYBLOCK *sbPtr);

#if 0
static PRED_RETURN_CONDITION Execute_PNS_Approach(STRATEGYBLOCK *sbPtr);
static PRED_RETURN_CONDITION Execute_PNS_StandGround(STRATEGYBLOCK *sbPtr);
static PRED_RETURN_CONDITION Execute_PNS_NewDischargePistol(STRATEGYBLOCK *sbPtr);
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Record moving shoulder-cannon acquisition and actual animation names; also test melee, taunts and weapon changes.

<a id="marines"></a>

## Marine NPC squad, inventory and tactics

Use commands toggle follow/patrol with acknowledgment. Shared jobs, staged patrol and pacing hysteresis reduce conflicting decisions. Marines collect supported extra weapons/resources, select weapons, assist with sentries and leave recoverable drops. Saved inventories/planning have reset/load guards.

```text
command/patrol task → shared job/planner → local navigation + pacing → combat/resupply/sentry → persisted state
```

[src/avp/marine_squad_impl.inc:76](../../src/avp/marine_squad_impl.inc#L76)

```c
}
static int MarineSupportControl(STRATEGYBLOCK*sb){
 MARINE_STATUS_BLOCK*m=sb->SBdataptr;
 if(!sb->containingModule || !NPCSupportCanCommand(sb) || !Player || !Player->ObStrategyBlock->DynPtr || !Player->ObStrategyBlock->containingModule)return 0;
 NPCInventoryInit(sb);NPCInventorySync(sb);NPC_MARINE_SQUAD*q=&m->squad;
 MarineWeaponTick(sb);
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
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Check Use acknowledgments, two distinct extra weapons, resource thresholds, APC courtesy, slopes and return from combat to follow/patrol. NPC inventory is private, not shared with the human player.

<a id="kernel"></a>

## Engine lifetime, performance and diagnostics

A bounded immutable-mesh BVH selects collision candidates while retaining native polygon math/order. Dynamic/morph/animated meshes fall back. Cache lifetime precedes level-pool teardown. Staged AI/support work and investigated crash guards retain full effects and NPC populations; results are scenario-specific.

```text
immutable-mesh bounds → BVH candidates → sort in native polygon order → original collision math; dynamic mesh → original fallback
```

[src/avp/particle_mesh_cache.inc:61](../../src/avp/particle_mesh_cache.inc#L61)

```c
}
static int ParticleMeshAppend(DISPLAYBLOCK*object,int visibilityPolicy){
 PARTICLE_MESH_CACHE*c=ParticleMeshGet(object);if(!c)return 0;
 int lo[3]={DBBMinX,DBBMinY,DBBMinZ},hi[3]={DBBMaxX,DBBMaxY,DBBMaxZ},count=0;
 if(c->count)ParticleMeshQuery(c,0,lo,hi,&count);qsort(c->hits,count,sizeof(int),ParticleMeshIndexCompare);
 for(int k=0;k<count;k++){
  PARTICLE_MESH_POLY*q=&c->poly[c->hits[k]];
  if((q->flag & iflag_notvis) && (visibilityPolicy==1 || (visibilityPolicy==2 && !(q->flag & iflag_mirror))))continue;
  struct ColPolyTag*out=CollisionPolysPtr;
  *out=q->poly;out->ParentObject=object;
  for(int v=0;v<out->NumberOfVertices;v++){out->PolyPoint[v].vx+=object->ObWorld.vx;out->PolyPoint[v].vy+=object->ObWorld.vy;out->PolyPoint[v].vz+=object->ObWorld.vz;}
  CollisionPolysPtr++;NumberOfCollisionPolys++;LOCALASSERT(NumberOfCollisionPolys<MAXIMUM_NUMBER_OF_COLLISIONPOLYS);
  if(q->flag & iflag_no_bfc){struct ColPolyTag*reverse=CollisionPolysPtr;*reverse=*out;for(int v=0;v<out->NumberOfVertices;v++)reverse->PolyPoint[v]=out->PolyPoint[out->NumberOfVertices-1-v];reverse->PolyNormal.vx=-out->PolyNormal.vx;reverse->PolyNormal.vy=-out->PolyNormal.vy;reverse->PolyNormal.vz=-out->PolyNormal.vz;CollisionPolysPtr++;NumberOfCollisionPolys++;LOCALASSERT(NumberOfCollisionPolys<MAXIMUM_NUMBER_OF_COLLISIONPOLYS);}
 }
 return 1;
}
/* ... excerpt continues in the linked source ... */
```

**Example and verification:** Compare exact polygon lists/normals/order and lifetime across load/restart. Cache is bounded to 32 MiB/64 meshes; it is cleared before freeing map pools.
