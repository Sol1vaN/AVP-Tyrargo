[English](../../README.md) · [Español](../../README.es.md)

# Guía técnica: código, flujos y ejemplos

Extractos generados del source actual. Los enlaces abren implementación completa. Flujos son esquemáticos; conservar interfaces nativas de simulación.

<a id="input"></a>

## Mouse, teclado y binds

Deltas float de SDL siguen una ruta angular directa con redondeo simétrico, restos fraccionales y escala zoom/FOV. Teclado físico y mouse extendido conservan binds por especie. Corrige errores de conversión ensayados, sin prometer resolver cualquier problema de hardware.

```text
Deltas float SDL → sensibilidad/zoom → ángulos 16.16 → restos pmove → pose fraccional de render
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

**Ejemplo y comprobación:** Comparar cuentas acumuladas iguales con distintos límites y sentidos; revisar movimientos pequeños con zoom. Conservar aparte strafe/movimiento legacy.

<a id="display"></a>

## Modos de pantalla y presentación

Ventana SDL, drawable, framebuffer y lienzo lógico se coordinan con cambios protegidos y fallback. Se restauran texturas al recrear contexto. Mover ventana atiende juego/menú y audio. VSync y límite son opciones; simulación de alto refresco y límite 1000 FPS siguen en auditoría.

```text
modo solicitado → transacción protegida SDL/GL → sincronizar drawable/framebuffer → restaurar texturas/estado o fallback
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

**Ejemplo y comprobación:** Alternar los tres modos, resolución/AA, mover ventana y volver por menús. Comprobar drawable real y no solo modo solicitado.

<a id="geometry"></a>

## Proyección y precisión geométrica

Coordenadas fraccionales sobreviven transformación y clipping. Proyecciones de mundo/viewmodel sincronizan cámara, frustum y visibilidad sin sustituir la representación nativa de gameplay.

```text
pose nativa + pose fraccional render → transformar cámara/modelo → clipping → floats OpenGL
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

**Ejemplo y comprobación:** Observar cañón/corredera en idle y zoom; comparar FOV/aspecto. Precisión visual no debe alterar silenciosamente formas de colisión.

<a id="render"></a>

## Render, iluminación y efectos

Los recortes de textura también limitan la pasada luminosa/profundidad. Relojes UV compartidos corrigen desfases. Orden de vidrio/efectos conserva visibilidad. Buffers ordenados y agrupación de fuego reducen trabajo conservando orden, mezclas y filtros originales.

```text
vértices/índices ordenados → almacenamiento renovado → secuencia original → pasadas luminosas/efectos
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

**Ejemplo y comprobación:** Comparar recortes alpha, mezclas/filtros, vidrio, halos y modos de visión. Reordenar transparencia no es una optimización aceptable.

<a id="water"></a>

## Simulación y render de agua

Propagación y amortiguación por tiempo evitan pérdida por enteros y perturbaciones descontroladas por frame. Óptica moderna incorpora refracción con profundidad, Fresnel, reflejos, especular con luces nativas y cáusticas. Screen-space no refleja geometría fuera de pantalla.

```text
tiempo simulado → integrar ondas → altura/gradiente fraccional → malla → óptica color/profundidad
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

**Ejemplo y comprobación:** Comparar onda aislada en tiempo igual con distintos límites; probar siluetas foreground y límites de reflejo en bordes.

<a id="postfx"></a>

## CRT, ruido y ajustes de imagen

CRT GLSL, ajustes de imagen y ruido de monitores son seleccionables. Trabajo constante o interior evita rutas caras por fragmento cuando es equivalente. Procedencia CRT incluye Mattias/newpixie y CRT-Guest-ReShade; créditos no sustituyen licencias.

```text
framebuffer escena → pasada GLSL imagen/CRT → lienzo seleccionado
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

**Ejemplo y comprobación:** Usar par desactivado/activado para aspecto. Medir misma escena/resolución para coste; GIF no es prueba de precisión.

<a id="ui"></a>

## Menú principal, menú ingame y UX

Menús usan espacios lógicos explícitos, protección de muestreo y composición latina. Perfiles por especie conservan escalas. Escape aplica ajustes ordinarios; confirmación de vídeo es explícita. Slots guardar/cargar permanecen accesibles en carruseles.

```text
perfil → layout lógico/atlas → vista previa → aplicar/guardar con protección
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

**Ejemplo y comprobación:** Comprobar acentos, marcos, estadísticas de muerte y vistas previas en sin bordes y letterboxed.

<a id="media"></a>

## Vídeos y capturas

Decodificadores Smacker/Bink y audio en buffer reproducen vídeos. Viewports físicos soportan modos panorámicos. El jugador suministra medios comerciales. Capturas BMP/PNG/JPEG excluyen sus notificaciones.

```text
stream vídeo → decodificador nativo → reloj audio en buffer → viewport físico → limpieza final/transición
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

**Ejemplo y comprobación:** Probar final, omitir, música de menú solo audio y modos anchos; capturar repetidamente sin incluir avisos.

<a id="audio"></a>

## Audio, música y latencia

Emisores OpenAL siguen posiciones y entorno. Solapamiento de armas y alimentación consideran audio. CD-DA incluye unidad y volumen; bucles incluyen silencio aleatorio. Diagnósticos acotados difieren escrituras fuera de frames.

```text
evento + posición → fuente/entorno OpenAL → reproducir; diagnóstico → buffer acotado → escritura diferida
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

**Ejemplo y comprobación:** Alejarse de pickup NPC, escuchar tras geometría, solapar armas primarias/secundarias y comparar alimentación.

<a id="player"></a>

## Movimiento y armas del jugador

Contacto, trepado y apoyo móvil tienen correcciones conservando fricción de pendientes. Hay correr/agachar y zoom/jetpack inteligentes. Colas de disparo conservan taps; interacción y cooldown evitan ráfagas por frame.

```text
solicitudes entrada → movimiento/contacto nativo → apoyo móvil/trepado → feedback cámara
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

**Ejemplo y comprobación:** Probar pendientes sin quitar fricción, lifts descendentes, giros, taps de disparo y cooldown de bengalas.

<a id="motion"></a>

## Movimiento de cámara, armas y HUD

Capas Marine de cámara/arma/HUD cubren movimiento y equipo con canales configurables. Transiciones incluyen postura, escaleras y aterrizaje. Calibración requiere source de desarrollo; producción conserva valores y controles públicos.

```text
eventos movimiento/equipo → canales independientes acotados → salidas cámara/arma/HUD
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

**Ejemplo y comprobación:** Comparar caminar, correr, agacharse, recarga y aterrizaje con canales desactivados/activados. Movimiento Marine; expansión Predator/Alien sigue planificada.

<a id="campaign"></a>

## Campaña, recursos y Molecular Acid

Acid modifica Hard en singleplayer y no agrega índices a arrays nativos. x1/x1.5/x2 corresponden a enemigos 1.5/2/2.5 y recursos 1.5/2/3. Excepciones son cantidades. Estado de campaña y economía de ammo tienen vidas separadas.

```text
nivel Acid → Hard nativo + población/recursos → capacidades/restos acotados → guardado
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

**Ejemplo y comprobación:** Comparar inicios idénticos por nivel y recargar partidas. 3/4/5 de Pulse Rifle/SADAR/disco son cantidades totales; armadura Predator es 15 × factor recursos.

<a id="aliens"></a>

## Comportamiento y navegación de Alien NPC

Tiempo fraccional, dirección local y recuperación de contacto corrigen casos ensayados de ductos, escotillas y bordes. Salto tiene autorización acotada: no todo salto grande es por FPS. Alimentación incluye animación/audio. Otros mapas requieren pruebas.

```text
objetivo nativo → comprobar avance/apoyo → recuperar ruta/contacto → salto acotado o locomoción
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

**Ejemplo y comprobación:** Usar save de ducto/escotilla y registrar contactos, velocidad solicitada y avance real. Separar saltos altos deliberados de impulsos repetidos inválidos.

<a id="predators"></a>

## Combate y patrulla de Predator NPC

Armas privadas, adquisición y patrulla amplían combate nativo. Adquisición del cañón en movimiento usa locomoción; disparo activo conserva tiempo de finalización. Navegación y camuflaje siguen condicionando persecución.

```text
elegibilidad → adquisición/arma → movimiento + clip locomoción → finalización nativa de disparo
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

**Ejemplo y comprobación:** Registrar adquisición móvil del cañón y nombres de clips; probar melee, taunts y cambios de arma.

<a id="marines"></a>

## Escuadrón, inventario y tácticas Marine NPC

Usar alterna follow/patrol con confirmación. Tareas compartidas, patrulla por etapas e histéresis reducen decisiones en conflicto. Marines recogen armas/recursos, eligen armas, ayudan con torretas y dejan drops. Inventarios/planner guardados tienen protecciones de cargar/reiniciar.

```text
orden/patrulla → tarea/planner compartido → navegación + ritmo → combate/suministro/torreta → estado persistido
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

**Ejemplo y comprobación:** Comprobar confirmaciones usar, dos armas distintas, umbrales, cortesía APC, pendientes y retorno tras combate. Inventario NPC es privado, no compartido con jugador.

<a id="kernel"></a>

## Vida útil, rendimiento y diagnóstico del motor

BVH acotado de mallas inmutables selecciona candidatos conservando matemática/orden nativos. Mallas dinámicas/morph/animadas usan fallback. Caché se limpia antes del pool. IA por etapas y protecciones investigadas conservan efectos/población; resultados dependen del escenario.

```text
bounds inmutables → candidatos BVH → ordenar según polígonos nativos → matemática original; dinámicas → fallback
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

**Ejemplo y comprobación:** Comparar polígonos/normales/orden y vida al cargar/reiniciar. Caché acotada a 32 MiB/64 mallas; se limpia antes de liberar pools.
