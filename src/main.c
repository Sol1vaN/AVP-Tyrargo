#include "optimized_ammo.h"
#include "campaign_state.h"
#include "tyrargo_instrumentation.h"
#include "event_latency.h"
#include "fmv_backend.h"
#include "intro_timing.h"
extern void CheckScreenshotInput(void);
#include "menu_preview.h"
#include "water_effect.h"
#include "fire_batch.h"
#include "motion_params.h"
#include "shake_params.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "screenshot_image.h"

#include <SDL3/SDL.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#ifndef GL_RGB565
#define GL_RGB565 0x8D62
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

#include "oglfunc.h"
#include "crt_filter.h"

#if !defined(_MSC_VER)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#endif

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "inline.h"
#include "gamedef.h"
#include "gameplat.h"
#include "ffstdio.h"
#include "vision.h"
#include "comp_shp.h"
#include "avp_envinfo.h"
#include "stratdef.h"
#include "bh_types.h"
#include "avp_userprofile.h"
#include "pldnet.h"
#include "cdtrackselection.h"
#include "gammacontrol.h"
#include "opengl.h"
#include "avp_menus.h"
#include "player_fov.h"
#include "dev_viewmodel.h"
#include "projection.h"
#include "avp_mp_config.h"
#include "npcsetup.h"
#include "cdplayer.h"
#include "hud.h"
#include "d3d_hud.h"
#include "davehook.h"
#include "player.h"
#include "mempool.h"
#include "avpview.h"
#include "consbind.hpp"
#include "progress_bar.h"
#include "scrshot.hpp"
#include "version.h"
#include "fmv.h"

#if defined(__APPLE__)
#include <strings.h>
#define secure_zero(p, n)  secure_avpzero((p),(n))
#elif defined(__linux__)
#include <string.h>
#define secure_zero(p, n)  secure_avpzero((p),(n))
#else
static inline void secure_avpzero(void* p, size_t n) {
	volatile unsigned char* vp = (volatile unsigned char*)p;
	while (n--) *vp++ = 0;
}
#endif

#if defined(__IPHONEOS__) || defined(__ANDROID__)
#define FIXED_WINDOW_SIZE 1
#endif

#if defined(__IPHONEOS__) || defined(__ANDROID__)
#define USE_OPENGL_ES 1
#endif

void RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(char Ch);
void RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(int wParam);

static void handle_keypress(int key, int unicode, int press);
#if defined(_WIN32)
static void EnableWindowsKeyHook(void);
static void DisableWindowsKeyHook(void);
#endif
static bool SDLCALL SDLEventFilter(void* userData, SDL_Event* event);

char LevelName[] = {"predbit6\0QuiteALongNameActually"}; /* the real way to load levels */

int DebouncedGotAnyKey;
unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
int GotJoystick;
int GotMouse;
int JoystickEnabled;
int MouseVelX;
int MouseVelY;
float MouseRawDeltaX;
float MouseRawDeltaY;

extern int ScanDrawMode;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern unsigned char KeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
extern unsigned char GotAnyKey;
extern int NormalFrameTime;
/*
 * The player code consumes MouseVel* as a velocity in 16.16 fixed point and
 * applies NormalFrameTime later in the frame. Keep that contract, but do the
 * conversion from SDL's floating-point relative deltas without first
 * truncating them to an integer. That truncation created a dead zone: sub-count
 * movements disappeared and implicit conversion was directionally asymmetric.
 *
 * The optional curve follows Raw Accel's useful shape: a linear gain above a
 * threshold with a finite cap. It is disabled by default; competitive-style
 * 1:1 raw input is the precise baseline and adds no filtering or latency.
 */
#define MOUSE_ACCEL_THRESHOLD_COUNTS_PER_SECOND 0.0f
#define MOUSE_ACCEL_GAIN_PER_COUNT_PER_SECOND    0.0f
#define MOUSE_ACCEL_MAX_GAIN                     1.50f

static float ApplyMouseAcceleration(float deltaX, float deltaY)
{
    float frameSeconds;
    float speed;
    float gain;

    if (MOUSE_ACCEL_GAIN_PER_COUNT_PER_SECOND <= 0.0f || NormalFrameTime <= 0) {
        return 1.0f;
    }

    frameSeconds = (float)NormalFrameTime / 65536.0f;
    if (frameSeconds <= 0.0f) {
        return 1.0f;
    }

    speed = sqrtf(deltaX * deltaX + deltaY * deltaY) / frameSeconds;
    gain = 1.0f + (speed - MOUSE_ACCEL_THRESHOLD_COUNTS_PER_SECOND)
        * MOUSE_ACCEL_GAIN_PER_COUNT_PER_SECOND;
    if (gain < 1.0f) gain = 1.0f;
    if (gain > MOUSE_ACCEL_MAX_GAIN) gain = MOUSE_ACCEL_MAX_GAIN;
    return gain;
}

static int MouseDeltaToVelocity(float delta, float *residual)
{
    float fixedVelocity;
    int wholeVelocity;

    if (NormalFrameTime <= 0) {
        *residual = 0.0f;
        return 0;
    }

    /* SDL3 can report sub-count relative motion. Preserve the rounding
     * remainder so a stream of micro-movements is never discarded. This is
     * direct raw accumulation, not smoothing or acceleration. */
    fixedVelocity = delta * 65536.0f / (float)NormalFrameTime + *residual;
    wholeVelocity = (fixedVelocity >= 0.0f)
        ? (int)(fixedVelocity + 0.5f)
        : (int)(fixedVelocity - 0.5f);
    *residual = fixedVelocity - (float)wholeVelocity;
    return wholeVelocity;
}

static float MouseVelocityResidualX;
static float MouseVelocityResidualY;

SDL_Window *window;
SDL_GLContext context;
SDL_Surface *surface;

SDL_Joystick *joy;
JOYINFOEX JoystickData;
JOYCAPS JoystickCaps;

// Window configuration and state
static int WindowWidth;
static int WindowHeight;
static int ViewportWidth;
static int ViewportHeight;
/* The whole SDL drawable may be larger than the active game canvas in
 * borderless fullscreen. Keep that presentation rectangle explicit: the
 * engine's SDB and OpenGL viewport must always describe the same canvas. */
static int PresentationViewportX;
static int PresentationViewportY;
static int PresentationViewportWidth;
static int PresentationViewportHeight;
static int VideoPresentationDirty = 1;

int PlayerFOV = PLAYER_FOV_DEFAULT;
int PlayerFOVMarinePredator = PLAYER_FOV_MARINE_DEFAULT;
int PlayerFOVAlien = PLAYER_FOV_ALIEN_DEFAULT;
static int ActivePlayerFOVSpeciesGroup = -1;
int WeaponFOV = WEAPON_FOV_DEFAULT;
int WeaponFOVMarine = WEAPON_FOV_MARINE_PREDATOR_DEFAULT;
int WeaponFOVPredator = WEAPON_FOV_MARINE_PREDATOR_DEFAULT;
int WeaponFOVAlien = WEAPON_FOV_ALIEN_DEFAULT;
static int ActiveWeaponFOVSpecies = -1;
int WeaponFOVStyle = WEAPON_FOV_STYLE_STRETCHED;

enum RENDERING_MODE {
	RENDERING_MODE_SOFTWARE,
	RENDERING_MODE_OPENGL
};

enum RENDERING_MODE RenderingMode;

#if defined(FIXED_WINDOW_SIZE)
static int WantFullscreen = 1;
static int WantFullscreenToggle = 0;
static int WantResolutionChange = 0;
static int WantMouseGrab = 1;
#else
static int WantFullscreen = 1;
static int WantFullscreenToggle = 1;
static int WantResolutionChange = 1;
static int WantMouseGrab = 1;
#endif

// Additional configuration
int WantSound = 1;
static int WantCDRom = 1;
static int WantJoystick = 0;

static GLuint FullscreenTexture;
static GLsizei FullscreenTextureWidth;
static int FrontendCanvasWidth=640,FrontendCanvasHeight=480;
static int FrontendCanvasExpanded;
static int WindowEventsPumping, ModalFrameActive, ModalFrontendActive;
static unsigned int WindowModalFrameCount;
static int *ModalGameMenus, *ModalGameCompleted;
static bool SDLCALL ContinueWindowModalFrame(void *unused, SDL_Event *event);
extern void AvP_UpdateMenus(void);
extern void DrawMainMenusBackdrop(void);
extern void PlayMenuMusic(void);
extern void NewScreenModeMessage(unsigned char *messagePtr);
static const char *ScreenModeNotice;
static Uint64 ScreenModeNoticeUntil;
extern int RenderSmallMenuText_Coloured(char *,int,int,int,enum AVPMENUFORMAT_ID,int,int,int);
void RenderFrontendScreenModeNotice(void)
{
 if(ScreenModeNotice && SDL_GetTicks()<ScreenModeNoticeUntil)
     RenderSmallMenuText_Coloured((char *)ScreenModeNotice,10,4,ONE_FIXED,AVPMENUFORMAT_LEFTJUSTIFIED,ONE_FIXED,0,0);
}
void ConfigureFrontendCanvas(int expanded);
static GLsizei FullscreenTextureHeight;
/* Borderless uses this as a genuine selected-resolution render target before
 * it is presented into the desktop-sized SDL drawable. */
static GLuint BorderlessPresentationTexture;
static GLsizei BorderlessPresentationTextureWidth;
static GLsizei BorderlessPresentationTextureHeight;
static int BorderlessPresentationTextureColorBitDepth = -1;
static Uint64 DesktopRefreshFrameDeadlineNS;
/* The FPS cap has its own deadline so it composes cleanly with the borderless desktop-rate pacer. */
static Uint64 FPSCapFrameDeadlineNS;

/* The SDL multisample attributes are consumed only while a GL context is
 * created. Retain the effective value so the Detail Levels apply action can
 * recreate that context when its anti-aliasing or colour-depth request changed. */
static int AppliedOpenGLAntiAliasingMode = -1;
static int AppliedOpenGLColorBitDepthMode = -1;

/* Client-array data must remain valid until the driver has consumed the
 * submitted presentation draw. Some drivers defer this work past SwapWindow. */
static GLfloat BorderlessPresentationVertices[4 * 4];
static GLfloat FrontendPresentationVertices[4 * 4];
static const GLshort PresentationIndices[6] = { 0, 1, 2, 0, 2, 3 };

/* Probe 113: compact crash trace for the borderless rendering transition.
 * It writes beside the portable executable in logs/, never to game data. */
static FILE *BorderlessRenderTrace;
static unsigned int BorderlessRenderTraceFrame;
static unsigned int Probe114GameplayLoopFrame;
static unsigned int Probe114FrontendFlipFrame;

/* Probe 135: preserve diagnostics in source, disable trace emission in release. */
#define PROBE113_RENDER_TRACE_ENABLED TYRARGO_RENDER_TRACE

void Probe113_RenderTracef(const char *format, ...)
{
#if !PROBE113_RENDER_TRACE_ENABLED
	(void)format;
	return;
#endif
	va_list args;
	char *basePath;
	char fileName[1024];
	if (BorderlessRenderTrace == NULL) {
		basePath = SDL_GetBasePath();
		if (basePath == NULL) return;
		snprintf(fileName, sizeof(fileName), "%slogs\\probe128_recreate_gl_context_for_native_modes.log", basePath);
		SDL_free(basePath);
		BorderlessRenderTrace = fopen(fileName, "w");
		if (BorderlessRenderTrace == NULL) return;
		setvbuf(BorderlessRenderTrace, NULL, _IONBF, 0);
	}
	fprintf(BorderlessRenderTrace, "[" TYRARGO_DISPLAY_VERSION "] [%llu] ", (unsigned long long)SDL_GetTicksNS());
	va_start(args, format);
	vfprintf(BorderlessRenderTrace, format, args);
	va_end(args);
	fprintf(BorderlessRenderTrace, "\n");
}

void Probe113_BeginMapTrace(const char *phase)
{
	BorderlessRenderTraceFrame = 0;
	Probe114GameplayLoopFrame = 0;
	Probe113_RenderTracef("TRACE_BEGIN phase=%s", phase);
}

static void BorderlessRenderTraceOpenGLError(const char *stage)
{
#if !TYRARGO_RENDER_TRACE
 return;
#endif
	GLenum error;
	if (pglGetError == NULL) return;
	while ((error = pglGetError()) != GL_NO_ERROR) {
		Probe113_RenderTracef("GL_ERROR stage=%s error=0x%04x", stage, (unsigned int)error);
	}
}
#if defined(_WIN32)
static void Probe124TraceAddressModule(const char *tag, const void *address)
{
	HMODULE module = NULL;
	char modulePath[MAX_PATH];
	DWORD length = 0;
	uintptr_t moduleBase = 0;

	if (address != NULL && GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)address, &module))
	{
		length = GetModuleFileNameA(module, modulePath, sizeof(modulePath));
		if (length >= sizeof(modulePath)) length = sizeof(modulePath) - 1;
		modulePath[length] = '\0';
		moduleBase = (uintptr_t)module;
		Probe113_RenderTracef("ADDRESS_MODULE tag=%s address=%p module=%s base=%p offset=0x%llx",
			tag, address, modulePath, module,
			(unsigned long long)((uintptr_t)address - moduleBase));
	}
	else
	{
		Probe113_RenderTracef("ADDRESS_MODULE tag=%s address=%p module=unknown", tag, address);
	}
}

/* SDL may temporarily detach the GL context while its native fullscreen mode
 * changes. Re-attach the existing context before any GL call; recreating it
 * here would invalidate renderer-owned textures and display lists. */
static int Probe124EnsureOpenGLContextCurrentLatencyBody(const char *stage)
{
	SDL_Window *currentWindow;
	SDL_GLContext currentContext;

	if (window == NULL || context == NULL) {
		Probe113_RenderTracef("CONTEXT_GUARD stage=%s missing expectedWindow=%p expectedContext=%p",
			stage, window, context);
		return 0;
	}
	currentWindow = SDL_GL_GetCurrentWindow();
	currentContext = SDL_GL_GetCurrentContext();
	if (currentWindow == window && currentContext == context) return 1;
 EventLatencyNote("GL context recovery",0,0,stage);
	Probe113_RenderTracef("CONTEXT_GUARD stage=%s mismatch expectedWindow=%p expectedContext=%p currentWindow=%p currentContext=%p",
		stage, window, context, currentWindow, currentContext);
	if (!SDL_GL_MakeCurrent(window, context)) {
		Probe113_RenderTracef("CONTEXT_GUARD stage=%s make_current_failed error=%s", stage, SDL_GetError());
		return 0;
	}
	Probe113_RenderTracef("CONTEXT_GUARD stage=%s make_current_ok", stage);
	return 1;
}
static int Probe124EnsureOpenGLContextCurrent(const char *stage)
{
 EVENT_TICK stamp=EventLatencyBegin();
 int result=Probe124EnsureOpenGLContextCurrentLatencyBody(stage);
 EventLatencyEnd(stamp,"GL context guard",result,0,0,stage,0);
 return result;
}

/* Native mode switches in NVIDIA's OpenGL path must not occur with the GL
 * context attached. SDL may rebuild the drawable while processing the mode;
 * detach first, perform the whole SDL transaction, then attach once. */
static int Probe125BeginVideoModeTransaction(const char *stage)
{
	if (context == NULL) {
		Probe113_RenderTracef("VIDEO_TRANSACTION stage=%s no_context", stage);
		return 1;
	}
	if (window == NULL || !SDL_GL_MakeCurrent(window, context)) {
		Probe113_RenderTracef("VIDEO_TRANSACTION stage=%s attach_before_failed error=%s", stage, SDL_GetError());
		return 0;
	}
	if (!SDL_GL_MakeCurrent(window, NULL)) {
		Probe113_RenderTracef("VIDEO_TRANSACTION stage=%s detach_failed error=%s", stage, SDL_GetError());
		return 0;
	}
	Probe113_RenderTracef("VIDEO_TRANSACTION stage=%s detached", stage);
	return 1;
}

static int Probe125FinishVideoModeTransaction(const char *stage)
{
	if (context == NULL) {
		Probe113_RenderTracef("VIDEO_TRANSACTION stage=%s no_context_finish", stage);
		return 1;
	}
	if (window == NULL || !SDL_GL_MakeCurrent(window, context)) {
		Probe113_RenderTracef("VIDEO_TRANSACTION stage=%s attach_after_failed error=%s", stage, SDL_GetError());
		return 0;
	}
	Probe113_RenderTracef("VIDEO_TRANSACTION stage=%s attached", stage);
	return 1;
}
static void Probe124TraceOpenGLContext(const char *stage)
{
#if !TYRARGO_RENDER_TRACE
 return;
#endif
	Probe113_RenderTracef("CONTEXT_STATE stage=%s expectedWindow=%p expectedContext=%p currentWindow=%p currentContext=%p",
		stage, window, context, SDL_GL_GetCurrentWindow(), SDL_GL_GetCurrentContext());
}
static LONG WINAPI Probe114UnhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo)
{
	void *frames[32];
	USHORT frameCount;
	USHORT index;
	DWORD code = 0;
	void *address = NULL;
	uintptr_t imageBase = (uintptr_t)GetModuleHandleA(NULL);
	if (exceptionInfo != NULL && exceptionInfo->ExceptionRecord != NULL) {
		code = exceptionInfo->ExceptionRecord->ExceptionCode;
		address = exceptionInfo->ExceptionRecord->ExceptionAddress;
	}
	Probe113_RenderTracef("CRASH exception=0x%08lx address=%p", (unsigned long)code, address);
	Probe124TraceAddressModule("exception", address);
	frameCount = CaptureStackBackTrace(0, 32, frames, NULL);
	for (index = 0; index < frameCount; ++index) {
		uintptr_t pc = (uintptr_t)frames[index];
		unsigned long long offset = (pc >= imageBase) ? (unsigned long long)(pc - imageBase) : 0ULL;
		Probe113_RenderTracef("CRASH_STACK index=%u pc=%p imageOffset=0x%llx", (unsigned int)index, frames[index], offset);
		Probe124TraceAddressModule("stack", frames[index]);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

static void Probe114InstallCrashTrace(void)
{
#if !TYRARGO_RENDER_TRACE
 return;
#endif
	SetUnhandledExceptionFilter(Probe114UnhandledExceptionFilter);
	Probe113_RenderTracef("CRASH_TRACE installed image=%p", GetModuleHandleA(NULL));
}
#endif
/* Set by DrawMainMenusBackdrop() for exactly one presentation.  Starfield.RIM
 * contains a 640x360 image within its 640x480 canvas; wide menu frames use
 * that active picture area instead of enlarging the baked black bars. */
int MainMenuBackdropPresentPending;

/* originally was "/usr/lib/libGL.so.1:/usr/lib/tls/libGL.so.1:/usr/X11R6/lib/libGL.so" */
static const char * opengl_library = NULL;

static const char * gamedatapath = NULL;

/* ** */

static void IngameKeyboardInput_ClearBuffer(void)
{
	// clear the keyboard state
	memset((void*) KeyboardInput, 0, MAX_NUMBER_OF_INPUT_KEYS);
	GotAnyKey = 0;
}

void DirectReadKeyboard()
{
}

void DirectReadMouse()
{
}

void ReadJoysticks()
{
	int axes, balls, hats;
	Uint8 hat;
	
	JoystickData.dwXpos = 0;
	JoystickData.dwYpos = 0;
	JoystickData.dwRpos = 0;
	JoystickData.dwUpos = 0;
	JoystickData.dwVpos = 0;
	JoystickData.dwPOV = (DWORD) -1;	
	
	if (joy == NULL || !GotJoystick) {
		return;
	}

	SDL_UpdateJoysticks();
	
	axes = SDL_GetNumJoystickAxes(joy);
	balls = SDL_GetNumJoystickBalls(joy);
	hats = SDL_GetNumJoystickHats(joy);
	
	if (axes > 0) {
		JoystickData.dwXpos = SDL_GetJoystickAxis(joy, 0) + 32768;
	}
	if (axes > 1) {
		JoystickData.dwYpos = SDL_GetJoystickAxis(joy, 1) + 32768;
	}
	
	if (hats > 0) {
		hat = SDL_GetJoystickHat(joy, 0);
		
		switch (hat) {
			default:
			case SDL_HAT_CENTERED:
				JoystickData.dwPOV = (DWORD) -1;
				break;
			case SDL_HAT_UP:
				JoystickData.dwPOV = 0;
				break;
			case SDL_HAT_RIGHT:
				JoystickData.dwPOV = 9000;
				break;
			case SDL_HAT_DOWN:
				JoystickData.dwPOV = 18000;
				break;
			case SDL_HAT_LEFT:
				JoystickData.dwPOV = 27000;
				break;
			case SDL_HAT_RIGHTUP:
				JoystickData.dwPOV = 4500;
				break;
			case SDL_HAT_RIGHTDOWN:
				JoystickData.dwPOV = 13500;
				break;
			case SDL_HAT_LEFTUP:
				JoystickData.dwPOV = 31500;
				break;
			case SDL_HAT_LEFTDOWN:
				JoystickData.dwPOV = 22500;
				break;
		}
	}
}

/* ** */

static int CapturePresentedFrame;
unsigned char *GetScreenShot24(int *width, int *height)
{
	unsigned char *buf;

	if (surface == NULL) {
		return NULL;
	}

	*width = (CapturePresentedFrame || RenderingMode == RENDERING_MODE_OPENGL) ? ViewportWidth : surface->w;
	*height = (CapturePresentedFrame || RenderingMode == RENDERING_MODE_OPENGL) ? ViewportHeight : surface->h;
	if(CapturePresentedFrame)SDL_GetWindowSizeInPixels(window,width,height);
	if (*width <= 0 || *height <= 0 || *width > (INT_MAX-1024)/4 ||
	    *height > (INT_MAX-1024)/(*width*4)) return NULL;
	if ((CapturePresentedFrame || RenderingMode == RENDERING_MODE_OPENGL)) {
		GLint packAlignment;
		buf = (unsigned char *)malloc((size_t)*width * *height * 3);
		if (!buf) return NULL;
		pglGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);



		pglPixelStorei(GL_PACK_ALIGNMENT, 1);
		pglReadPixels(0, 0, *width, *height, GL_RGB, GL_UNSIGNED_BYTE, buf);
		pglPixelStorei(GL_PACK_ALIGNMENT, packAlignment);
	} else {
		buf = (unsigned char *)malloc((size_t)*width * *height * 3);
		if (!buf) return NULL;

		unsigned char *ptrd;
		unsigned short int *ptrs;
		int x, y;
	
		if (SDL_MUSTLOCK(surface)) {
			if (!SDL_LockSurface(surface)) {
				free(buf);
				return NULL; /* ... */
			}
		}
		
		ptrd = buf;
		for (y = 0; y < surface->h; y++) {
			ptrs = (unsigned short *)(((unsigned char *)surface->pixels) + (surface->h-y-1)*surface->pitch);
			for (x = 0; x < surface->w; x++) {
				unsigned int c;
				
				c = *ptrs;
				ptrd[0] = (c & 0xF800)>>8;
				ptrd[1] = (c & 0x07E0)>>3;
				ptrd[2] = (c & 0x001F)<<3;
				
				ptrs++;
				ptrd += 3;
			}
		}
		
		*width = surface->w;
		*height = surface->h;

		if (SDL_MUSTLOCK(surface)) {
			SDL_UnlockSurface(surface);
		}
	}

#if 0
	Uint16 redtable[256], greentable[256], bluetable[256];
	
	if (SDL_GetGammaRamp(redtable, greentable, bluetable) != -1) {
		unsigned char *ptr;
		int i;
		
		ptr = buf;
		for (i = 0; i < surface->w*surface->h; i++) {
			ptr[i*3+0] = redtable[ptr[i*3+0]]>>8;
			ptr[i*3+1] = greentable[ptr[i*3+1]]>>8;
			ptr[i*3+2] = bluetable[ptr[i*3+2]]>>8;
			ptr += 3;
		}
	}
#endif

	return buf;
}

/* ** */

PROCESSORTYPES ReadProcessorType()
{
	return PType_PentiumMMX;
}

/* ** */

typedef struct VideoModeStruct
{
	int w;
	int h;
	int available;
} VideoModeStruct;

enum VIDEO_ASPECT_ID
{
	VIDEO_ASPECT_4_3,
	VIDEO_ASPECT_5_4,
	VIDEO_ASPECT_16_9,
	VIDEO_ASPECT_16_10,
	VIDEO_ASPECT_3_2,
	VIDEO_ASPECT_20_13,
	VIDEO_ASPECT_195_139,
	VIDEO_ASPECT_106_75,
	VIDEO_ASPECT_40_27,
	VIDEO_ASPECT_1_85_1,
	VIDEO_ASPECT_225_128,
	VIDEO_ASPECT_DCI,
	VIDEO_ASPECT_2_01_1,
	VIDEO_ASPECT_21_9,
	VIDEO_ASPECT_32_9,
	VIDEO_ASPECT_OTHER,
	VIDEO_ASPECT_COUNT
};

typedef struct VideoAspectDefinition
{
	int id;
	int numerator;
	int denominator;
	int tolerancePPM;
} VideoAspectDefinition;

/* This table categorises only. Availability remains an exact SDL display-mode
 * match, so a tolerated ratio can never create or apply an unsupported mode. */
static const VideoAspectDefinition VideoAspectDefinitions[] = {
	{ VIDEO_ASPECT_4_3,     4,   3,  5000 },
	{ VIDEO_ASPECT_5_4,     5,   4,  5000 },
	{ VIDEO_ASPECT_16_9,   16,   9,  5000 }, /* includes 1366x768 */
	{ VIDEO_ASPECT_16_10,  16,  10,  5000 },
	{ VIDEO_ASPECT_3_2,     3,   2,  3000 },
	{ VIDEO_ASPECT_20_13,  20,  13,  3000 },
	{ VIDEO_ASPECT_195_139,195,139,  3000 },
	{ VIDEO_ASPECT_106_75,106, 75,  3000 },
	{ VIDEO_ASPECT_40_27,  40,  27,  3000 },
	{ VIDEO_ASPECT_1_85_1,185,100,  3000 },
	{ VIDEO_ASPECT_225_128,225,128, 3000 },
	{ VIDEO_ASPECT_DCI,   190,100,  3000 },
	{ VIDEO_ASPECT_2_01_1,201,100,  5000 },
	/* Consumer ultrawides are marketed as 21:9 although 64:27 and 43:18
	 * are their common exact geometries. */
	{ VIDEO_ASPECT_21_9,   21,   9, 30000 },
	{ VIDEO_ASPECT_32_9,   32,   9,  5000 }
};

enum VIDEO_SCREEN_MODE_ID
{
    VIDEO_SCREEN_WINDOWED,
    VIDEO_SCREEN_BORDERLESS,
    VIDEO_SCREEN_EXCLUSIVE
};

enum VIDEO_FPS_CAP_MODE_ID
{
    VIDEO_FPS_CAP_60,
    VIDEO_FPS_CAP_100,
    VIDEO_FPS_CAP_142,
    VIDEO_FPS_CAP_UNCAPPED,
    VIDEO_FPS_CAP_CUSTOM,
    /* Append IDs: modes 0..4 are stored in existing AvP_TempVideo.cfg files. */
    VIDEO_FPS_CAP_65,
    VIDEO_FPS_CAP_75,
    VIDEO_FPS_CAP_85,
    VIDEO_FPS_CAP_120,
    VIDEO_FPS_CAP_MODE_COUNT
};

#define VIDEO_FPS_CAP_CUSTOM_MINIMUM 1
#define VIDEO_FPS_CAP_CUSTOM_MAXIMUM 1000
/* The catalogue is grouped by real geometry, not by a cosmetic UI label.
 * 4096x2034 is deliberately Other: it is not mathematically 16:9. */
static const VideoModeStruct BuiltinVideoModes[] = {
	/* 4:3 */
	{512,384,0}, {640,480,0}, {800,600,0}, {960,720,0}, {1024,768,0},
	{1152,864,0}, {1280,960,0}, {1600,1200,0}, {2048,1536,0},
	/* 5:4 */
	{1280,1024,0}, {2560,2048,0}, {5120,4096,0},
	/* 16:9 */
	{1280,720,0}, {1366,768,0}, {1600,900,0}, {1920,1080,0},
	{2560,1440,0}, {2880,1620,0}, {3200,1800,0}, {3840,2160,0},
	{4096,2304,0}, {4480,2520,0}, {5120,2880,0}, {6016,3384,0},
	{7680,4320,0},
	/* 16:10 */
	{1280,800,0}, {1440,900,0}, {1680,1050,0}, {1920,1200,0},
	{2560,1600,0}, {3456,2160,0},
	/* Exact special ratios, each published under its own mathematical label. */
	{2048,1080,0}, {4096,2160,0}, {8192,4320,0}, /* 256:135 / 1.90:1 DCI */
	{2560,1664,0}, /* 20:13 */
	{3000,2000,0}, /* 3:2 */
	{4096,2034,0}, /* 2048:1017 / 2.01:1 */
	/* 21:9 */
	{2560,1080,0}, {3440,1440,0}, {5120,2160,0},
	/* 32:9 */
	{3840,1080,0}, {5120,1440,0}
};

int CurrentVideoMode;
VideoModeStruct *VideoModeList;
int TotalVideoModes;
static int VideoModeCapacity;
static int LastKnownGoodVideoMode = -1;
static int LastKnownGoodVideoRefreshMilliHz;
static int CurrentVideoAspect = VIDEO_ASPECT_4_3;
static int CurrentVideoOtherNumerator,CurrentVideoOtherDenominator;
static int CurrentVideoScreenMode = VIDEO_SCREEN_EXCLUSIVE;
static int CurrentVideoRefreshMilliHz;
static int CurrentVideoVSync = 1;
static int CurrentVideoFPSCapMode = VIDEO_FPS_CAP_100;
/* This is intentionally exported to the menu table's numeric-entry field. */
int CurrentVideoFPSCapCustom = 100;
/* Current* is the menu's pending transaction. Applied* is the only state
 * consumed by the renderer and changes only through Usa Estos Ajustes. */
static int AppliedVideoMode = -1;
static int AppliedVideoScreenMode = VIDEO_SCREEN_EXCLUSIVE;
static int AppliedVideoRefreshMilliHz;
static int AppliedVideoVSync = 1;
static int AppliedVideoFPSCapMode = VIDEO_FPS_CAP_100;
static int AppliedVideoFPSCapCustom = 100;
static void InitialiseAppliedVideoSettings(void);
static int VideoScreenModeCommandLineOverride;
static int VideoAutoDefaultsPending;
static int *VideoRefreshRatesMilliHz;
static int VideoRefreshCapacity;
static int NumberOfVideoRefreshRates;

static int VideoModeRefreshMilliHz(const SDL_DisplayMode *mode)
{
    int64_t rate;
    if (!mode) return 0;
    if (mode->refresh_rate_numerator > 0 && mode->refresh_rate_denominator > 0) {
        rate = (1000LL * mode->refresh_rate_numerator + mode->refresh_rate_denominator / 2) / mode->refresh_rate_denominator;
        return (rate > 0 && rate <= 2147483647) ? (int)rate : 0;
    }
    /* Reject NaN, infinity and unknown rates before converting to integer. */
    if (!(mode->refresh_rate > 0.0f) || !((double)mode->refresh_rate * 1000.0 < 2147483647.0)) return 0;
    return (int)((double)mode->refresh_rate * 1000.0 + 0.5);
}

/* 100 Hz is the preferred operating point. Treat 98..102 Hz as near-100. */
#define VIDEO_PREFERRED_REFRESH_MILLIHZ 100000
#define VIDEO_NEAR_REFRESH_MILLIHZ 2000

static int FindVideoModeDimensions(int width, int height)
{
    int i;
    for (i = 0; i < TotalVideoModes; ++i)
        if (VideoModeList[i].w == width && VideoModeList[i].h == height) return i;
    return -1;
}

static int RegisterVideoMode(const SDL_DisplayMode *mode)
{
    int index;
    VideoModeStruct *grown;
    if (!mode || mode->w <= 0 || mode->h <= 0) return -1;
    index = FindVideoModeDimensions(mode->w, mode->h);
    if (index < 0) {
        if (TotalVideoModes == VideoModeCapacity) {
            int capacity = VideoModeCapacity ? VideoModeCapacity * 2 : 64;
            grown = SDL_realloc(VideoModeList, (size_t)capacity * sizeof(*grown));
            if (!grown) return -1;
            VideoModeList = grown;
            VideoModeCapacity = capacity;
        }
        index = TotalVideoModes++;
        VideoModeList[index].w = mode->w;
        VideoModeList[index].h = mode->h;
    }
    VideoModeList[index].available = 1;
    return index;
}

static void RegisterVideoRefresh(const SDL_DisplayMode *mode)
{
    int rate, i;
    int *grown;
    if (!mode || CurrentVideoMode < 0 || CurrentVideoMode >= TotalVideoModes) return;
    if (mode->w != VideoModeList[CurrentVideoMode].w || mode->h != VideoModeList[CurrentVideoMode].h) return;
    rate = VideoModeRefreshMilliHz(mode);
    if (rate <= 0) return;
    for (i = 0; i < NumberOfVideoRefreshRates; ++i)
        if (VideoRefreshRatesMilliHz[i] == rate) return;
    if (NumberOfVideoRefreshRates == VideoRefreshCapacity) {
        int capacity = VideoRefreshCapacity ? VideoRefreshCapacity * 2 : 64;
        grown = SDL_realloc(VideoRefreshRatesMilliHz, (size_t)capacity * sizeof(*grown));
        if (!grown) return;
        VideoRefreshRatesMilliHz = grown;
        VideoRefreshCapacity = capacity;
    }
    VideoRefreshRatesMilliHz[NumberOfVideoRefreshRates++] = rate;
}

static int PreferredVideoRefresh(void)
{
    int i, nearestRate = 0, above = 0, below = 0;
    for (i = 0; i < NumberOfVideoRefreshRates; ++i) {
        int rate = VideoRefreshRatesMilliHz[i];
        int distance = abs(rate - VIDEO_PREFERRED_REFRESH_MILLIHZ);
        if (distance <= VIDEO_NEAR_REFRESH_MILLIHZ) {
            if (!nearestRate || distance < abs(nearestRate - VIDEO_PREFERRED_REFRESH_MILLIHZ)
                || (distance == abs(nearestRate - VIDEO_PREFERRED_REFRESH_MILLIHZ) && rate > nearestRate)) nearestRate = rate;
        } else if (rate > VIDEO_PREFERRED_REFRESH_MILLIHZ) {
            if (!above || rate < above) above = rate;
        } else if (rate > below) below = rate;
    }
    return nearestRate ? nearestRate : (above ? above : below);
}

static int LargestAvailableVideoMode(void)
{
    int i, best = -1;
    for (i = 0; i < TotalVideoModes; ++i) {
        int64_t area, bestArea;
        if (!VideoModeList[i].available) continue;
        if (best < 0) { best = i; continue; }
        area = (int64_t)VideoModeList[i].w * VideoModeList[i].h;
        bestArea = (int64_t)VideoModeList[best].w * VideoModeList[best].h;
        if (area > bestArea || (area == bestArea && VideoModeList[i].w > VideoModeList[best].w)) best = i;
    }
    return best;
}

static void SetFirstRunVideoTimingDefaults(void)
{
    int nominalRate = (int)(((int64_t)CurrentVideoRefreshMilliHz + 500) / 1000);
    CurrentVideoVSync = 1;
    CurrentVideoFPSCapMode = VIDEO_FPS_CAP_100;
    CurrentVideoFPSCapCustom = 100;
    /* Remember a high fallback in the custom field, but keep the active cap100. */
    if (nominalRate >= VIDEO_FPS_CAP_CUSTOM_MINIMUM && nominalRate <= VIDEO_FPS_CAP_CUSTOM_MAXIMUM)
        CurrentVideoFPSCapCustom = nominalRate;
    if (CurrentVideoRefreshMilliHz > 0
        && CurrentVideoRefreshMilliHz < VIDEO_PREFERRED_REFRESH_MILLIHZ - VIDEO_NEAR_REFRESH_MILLIHZ) {
        switch (nominalRate) {
            case 60: CurrentVideoFPSCapMode = VIDEO_FPS_CAP_60; break;
            case 65: CurrentVideoFPSCapMode = VIDEO_FPS_CAP_65; break;
            case 75: CurrentVideoFPSCapMode = VIDEO_FPS_CAP_75; break;
            case 85: CurrentVideoFPSCapMode = VIDEO_FPS_CAP_85; break;
            default: CurrentVideoFPSCapMode = VIDEO_FPS_CAP_CUSTOM; break;
        }
    }
}

static int VideoAspectForDimensions(int width, int height)
{
	int i;
	if (width <= 0 || height <= 0) return VIDEO_ASPECT_OTHER;
	for (i = 0; i < (int)(sizeof(VideoAspectDefinitions) / sizeof(VideoAspectDefinitions[0])); i++)
	{
		const VideoAspectDefinition *definition = &VideoAspectDefinitions[i];
		long long actual = (long long)width * definition->denominator;
		long long canonical = (long long)height * definition->numerator;
		long long difference = actual - canonical;
		if (difference < 0) difference = -difference;
		if (difference * 1000000LL <= canonical * definition->tolerancePPM)
			return definition->id;
	}
	return VIDEO_ASPECT_OTHER;
}

static int VideoAspectGreatestCommonDivisor(int a,int b);
static void VideoAspectKeyForMode(int mode,int *numerator,int *denominator)
{
 int id=VideoAspectForDimensions(VideoModeList[mode].w,VideoModeList[mode].h),i;
 for(i=0;i<(int)(sizeof(VideoAspectDefinitions)/sizeof(VideoAspectDefinitions[0]));++i){
  const VideoAspectDefinition *d=&VideoAspectDefinitions[i];
  if(d->id==id){*numerator=d->numerator;*denominator=d->denominator;return;}
 }
 {int divisor=VideoAspectGreatestCommonDivisor(VideoModeList[mode].w,VideoModeList[mode].h);
  *numerator=VideoModeList[mode].w/divisor;*denominator=VideoModeList[mode].h/divisor;}
}
static void SelectVideoAspectForMode(int mode)
{
 CurrentVideoAspect=VideoAspectForDimensions(VideoModeList[mode].w,VideoModeList[mode].h);
 VideoAspectKeyForMode(mode,&CurrentVideoOtherNumerator,&CurrentVideoOtherDenominator);
}
static int VideoModeMatchesCurrentAspect(int mode)
{
 int n,d;
 if(VideoAspectForDimensions(VideoModeList[mode].w,VideoModeList[mode].h)!=CurrentVideoAspect)return 0;
 if(CurrentVideoAspect!=VIDEO_ASPECT_OTHER)return 1;
 VideoAspectKeyForMode(mode,&n,&d);
 return n==CurrentVideoOtherNumerator && d==CurrentVideoOtherDenominator;
}

static SDL_DisplayID VideoMenuDisplay(void)
{
	SDL_DisplayID displayID = 0;
	if (window != NULL) displayID = SDL_GetDisplayForWindow(window);
	if (!displayID) displayID = SDL_GetPrimaryDisplay();
	return displayID;
}

static void RefreshVideoRefreshRates(void)
{
    SDL_DisplayID displayID = VideoMenuDisplay();
    SDL_DisplayMode **modes;
    int count = 0, i, j, desiredRate = CurrentVideoRefreshMilliHz;
    NumberOfVideoRefreshRates = 0;
    if (!displayID) { CurrentVideoRefreshMilliHz = 0; return; }
    modes = SDL_GetFullscreenDisplayModes(displayID, &count);
    if (modes) {
        for (i = 0; i < count; ++i) RegisterVideoRefresh(modes[i]);
        SDL_free(modes);
    }
    if (!NumberOfVideoRefreshRates) {
        RegisterVideoRefresh(SDL_GetDesktopDisplayMode(displayID));
        RegisterVideoRefresh(SDL_GetCurrentDisplayMode(displayID));
    }
    for (i = 0; i < NumberOfVideoRefreshRates; ++i)
        for (j = i + 1; j < NumberOfVideoRefreshRates; ++j)
            if (VideoRefreshRatesMilliHz[j] < VideoRefreshRatesMilliHz[i]) {
                int swap = VideoRefreshRatesMilliHz[i];
                VideoRefreshRatesMilliHz[i] = VideoRefreshRatesMilliHz[j];
                VideoRefreshRatesMilliHz[j] = swap;
            }
    if (!NumberOfVideoRefreshRates) { CurrentVideoRefreshMilliHz = 0; return; }
    for (i = 0; i < NumberOfVideoRefreshRates; ++i)
        if (VideoRefreshRatesMilliHz[i] == desiredRate) return;
    /* Retain the previous carousel behavior for manual resolution changes.
     * First-run selection explicitly calls PreferredVideoRefresh afterwards. */
    CurrentVideoRefreshMilliHz = VideoRefreshRatesMilliHz[0];
}

/* Publish only exact SDL3 exclusive resolutions from the active display. */
static int RefreshVideoModeAvailability(void)
{
    SDL_DisplayID displayID = VideoMenuDisplay();
    SDL_DisplayMode **modes;
    int count = 0, i, found = 0;
    /* Old cfg files store catalogue indices: keep every built-in index stable.
     * SDL-only resolutions are appended and saved by dimensions from now on. */
    if (!TotalVideoModes) {
        VideoModeStruct *initial = SDL_malloc(sizeof(BuiltinVideoModes));
        if (!initial) return 0;
        memcpy(initial, BuiltinVideoModes, sizeof(BuiltinVideoModes));
        VideoModeList = initial;
        TotalVideoModes = VideoModeCapacity = sizeof(BuiltinVideoModes) / sizeof(BuiltinVideoModes[0]);
    }
    for (i = 0; i < TotalVideoModes; ++i) VideoModeList[i].available = 0;
    if (!displayID) return 0;
    modes = SDL_GetFullscreenDisplayModes(displayID, &count);
    if (modes) {
        for (i = 0; i < count; ++i) if (RegisterVideoMode(modes[i]) >= 0) found = 1;
        SDL_free(modes);
    }
    /* If enumeration fails, desktop/current are still SDL-reported valid modes.
     * Never mark an invented 640x480 fallback as supported. */
    if (!found) {
        if (RegisterVideoMode(SDL_GetDesktopDisplayMode(displayID)) >= 0) found = 1;
        if (RegisterVideoMode(SDL_GetCurrentDisplayMode(displayID)) >= 0) found = 1;
    }
    return found;
}

static void SelectFirstAvailableModeForAspect(void)
{
	int i;
	for (i = 0; i < TotalVideoModes; i++)
	{
		if (VideoModeList[i].available && VideoModeMatchesCurrentAspect(i))
		{
			CurrentVideoMode = i;
			RefreshVideoRefreshRates();
			InitialiseAppliedVideoSettings();
			return;
		}
	}
}

static void InitialiseAppliedVideoSettings(void)
{
    AppliedVideoMode = CurrentVideoMode;
    AppliedVideoScreenMode = CurrentVideoScreenMode;
    AppliedVideoRefreshMilliHz = CurrentVideoRefreshMilliHz;
    AppliedVideoVSync = CurrentVideoVSync;
    AppliedVideoFPSCapMode = CurrentVideoFPSCapMode;
    AppliedVideoFPSCapCustom = CurrentVideoFPSCapCustom;
}
extern int MenuBackgroundMode;
void LoadDeviceAndVideoModePreferences()
{
    FILE *fp;
    int mode = -1;
    int count = 0, savedVSync = 0, savedCap = 0;
    int modeWidth = 0, modeHeight = 0, modeSchema = 0;
    int screenMode;
    int refreshMilliHz;
    int antiAliasingMode;
    int anisotropyMode;
    int textureFilterMode;
    int videoVSync = 1;
    int fpsCapMode = VIDEO_FPS_CAP_100;
    int fpsCapCustom = 100;
    int colorBitDepth = OPENGL_COLOR_BIT_DEPTH_32;
    int qualitySchema = 0;
    int brightness=50,contrast=100,saturation=100,imageSchema=0;
    int waterMode=0,waterSchema=0,backgroundMode=1,backgroundSchema=0;
    int screenshotFormat=SCREENSHOT_PNG,screenshotSchema=0;
    int fmvFilter=1,fmvFilterSchema=0;
    int retainState=0,retainSchema=0,optimizedAmmo=0,optimizedAmmoSchema=0;
    int menuType=0,menuMusic=1,menuAssetsSchema=0;
    OptimizedAmmo=0;
    RetainPlayerState=0;
    FMVFilterMode=1;
    ScreenshotFormat=SCREENSHOT_PNG;
    MenuBackgroundMode=1;
    MenuBackgroundType=0;MenuMusicMode=1;
    FMVBackend_RefreshMenuAssets();
    WaterEffectMode=1;
    fp = OpenGameFile("AvP_TempVideo.cfg", FILEMODE_READONLY, FILETYPE_CONFIG);
    if (fp != NULL)
    {
        count = fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &mode, &screenMode, &refreshMilliHz, &antiAliasingMode, &anisotropyMode, &textureFilterMode, &videoVSync, &fpsCapMode, &fpsCapCustom, &colorBitDepth, &qualitySchema, &brightness, &contrast, &saturation, &imageSchema, &waterMode, &waterSchema, &backgroundMode, &backgroundSchema, &modeWidth, &modeHeight, &modeSchema, &screenshotFormat, &screenshotSchema, &fmvFilter, &fmvFilterSchema, &retainState, &retainSchema, &optimizedAmmo, &optimizedAmmoSchema, &menuType, &menuMusic, &menuAssetsSchema);
        fclose(fp);
        ImageBrightness=50;ImageContrast=100;ImageSaturation=100;
        if(count>=33 && menuAssetsSchema==230){
            if(menuType>=0 && menuType<=1)MenuBackgroundType=menuType;
            if(menuMusic>=0 && menuMusic<=1)MenuMusicMode=menuMusic;
        }
        if(count>=17&&waterSchema==158&&waterMode>=0&&waterMode<=1)WaterEffectMode=waterMode;
        if(count>=19){
            if(backgroundSchema==231&&backgroundMode>=0&&backgroundMode<=2)MenuBackgroundMode=backgroundMode;
            else if(backgroundSchema==162&&backgroundMode>=0&&backgroundMode<=1)MenuBackgroundMode=backgroundMode?0:2;
        }
        if(count>=26&&fmvFilterSchema==180&&fmvFilter>=0&&fmvFilter<=1)FMVFilterMode=fmvFilter;
        if(count>=30 && optimizedAmmoSchema==214 && optimizedAmmo>=0 && optimizedAmmo<=1)OptimizedAmmo=optimizedAmmo;
        if(count>=28)RetainPlayerState=CampaignModeFromPreference(retainSchema,retainState);
        if(count>=24&&screenshotSchema==171&&screenshotFormat>=0&&screenshotFormat<SCREENSHOT_FORMAT_COUNT)ScreenshotFormat=screenshotFormat;
        if(count>=15&&imageSchema==157&&brightness>=0&&brightness<=100&&contrast>=0&&contrast<=200&&saturation>=0&&saturation<=200){
            ImageBrightness=brightness;ImageContrast=contrast;ImageSaturation=saturation;
        }
        /* Schema 121 stored its marker in field seven; schema 122 adds VSync;
         * schema 123 adds FPS-cap mode/custom; schema 124 adds colour depth. */
        if (count == 7) { qualitySchema = videoVSync; videoVSync = 1; }
        else if (count == 8) { qualitySchema = fpsCapMode; fpsCapMode = VIDEO_FPS_CAP_100; fpsCapCustom = 100; }
        else if (count == 10) { qualitySchema = colorBitDepth; colorBitDepth = OPENGL_COLOR_BIT_DEPTH_32; }
        if (count >= 7 && (qualitySchema == 121 || qualitySchema == 122 || qualitySchema == 123 || qualitySchema == 124) && antiAliasingMode >= 0 && antiAliasingMode <= 3) OpenGLAntiAliasingMode = antiAliasingMode;
        if (count >= 7 && (qualitySchema == 121 || qualitySchema == 122 || qualitySchema == 123 || qualitySchema == 124) && anisotropyMode >= 0 && anisotropyMode <= 4) OpenGLAnisotropyMode = anisotropyMode;
        if (count >= 7 && (qualitySchema == 121 || qualitySchema == 122 || qualitySchema == 123 || qualitySchema == 124) && textureFilterMode >= 0 && textureFilterMode <= 2) OpenGLTextureFilterMode = textureFilterMode;
        if (count >= 8 && (qualitySchema == 122 || qualitySchema == 123 || qualitySchema == 124) && videoVSync >= 0 && videoVSync <= 1) { CurrentVideoVSync = videoVSync; savedVSync = 1; }
        if (count >= 10 && (qualitySchema == 123 || qualitySchema == 124) && fpsCapMode >= VIDEO_FPS_CAP_60 && fpsCapMode < VIDEO_FPS_CAP_MODE_COUNT)
        {
            savedCap = 1;
            CurrentVideoFPSCapMode = fpsCapMode;
            CurrentVideoFPSCapCustom = (fpsCapCustom >= VIDEO_FPS_CAP_CUSTOM_MINIMUM && fpsCapCustom <= VIDEO_FPS_CAP_CUSTOM_MAXIMUM) ? fpsCapCustom : 100;
        }
        if (count >= 11 && qualitySchema == 124 && (colorBitDepth == OPENGL_COLOR_BIT_DEPTH_16 || colorBitDepth == OPENGL_COLOR_BIT_DEPTH_32)) OpenGLColorBitDepthMode = colorBitDepth;
        /* Geometry survives appended SDL-mode reorderings across launches. */
        if (count >= 22 && modeSchema == 170)
            mode = FindVideoModeDimensions(modeWidth, modeHeight);
        if (!VideoScreenModeCommandLineOverride && count >= 2 && screenMode >= VIDEO_SCREEN_WINDOWED && screenMode <= VIDEO_SCREEN_EXCLUSIVE)
            CurrentVideoScreenMode = screenMode;
        if (count >= 1 && mode >= 0 && mode < TotalVideoModes && VideoModeList[mode].available)
        {
            CurrentVideoMode = mode;
            if (count >= 3 && refreshMilliHz > 0) CurrentVideoRefreshMilliHz = refreshMilliHz;
            SelectVideoAspectForMode(mode);
            RefreshVideoRefreshRates();
            VideoAutoDefaultsPending = 0;
            InitialiseAppliedVideoSettings();
            return;
        }
    }

    /* No usable saved resolution: choose the largest exact mode SDL reported.
     * Valid user timing preferences still win when only the resolution vanished. */
    CurrentVideoMode = LargestAvailableVideoMode();
    if (CurrentVideoMode < 0) return;
    SelectVideoAspectForMode(CurrentVideoMode);
    CurrentVideoRefreshMilliHz = 0;
    RefreshVideoRefreshRates();
    CurrentVideoRefreshMilliHz = PreferredVideoRefresh();
    {
        int userVSync = CurrentVideoVSync;
        int userCapMode = CurrentVideoFPSCapMode;
        int userCustom = CurrentVideoFPSCapCustom;
        SetFirstRunVideoTimingDefaults();
        if (savedVSync) CurrentVideoVSync = userVSync;
        if (savedCap) { CurrentVideoFPSCapMode = userCapMode; CurrentVideoFPSCapCustom = userCustom; }
    }
    /* Persist automatic defaults only after SDL successfully applies them.
     * An existing but unavailable user mode is not overwritten automatically. */
    VideoAutoDefaultsPending = count <= 0;
    InitialiseAppliedVideoSettings();
}
void SaveDeviceAndVideoModePreferencesLatencyBody()
{
    FILE *fp = OpenGameFile("AvP_TempVideo.cfg", FILEMODE_WRITEONLY, FILETYPE_CONFIG);
    if (fp != NULL)
    {
        const int savedMode = (AppliedVideoMode >= 0) ? AppliedVideoMode : CurrentVideoMode;
        const int savedScreenMode = (AppliedVideoMode >= 0) ? AppliedVideoScreenMode : CurrentVideoScreenMode;
        const int savedRefreshMilliHz = (AppliedVideoMode >= 0) ? AppliedVideoRefreshMilliHz : CurrentVideoRefreshMilliHz;
        const int savedVideoVSync = (AppliedVideoMode >= 0) ? AppliedVideoVSync : CurrentVideoVSync;
        const int savedFPSCapMode = (AppliedVideoMode >= 0) ? AppliedVideoFPSCapMode : CurrentVideoFPSCapMode;
        int savedFPSCapCustom = (AppliedVideoMode >= 0) ? AppliedVideoFPSCapCustom : CurrentVideoFPSCapCustom;
        const int savedColorBitDepth = (AppliedOpenGLColorBitDepthMode >= 0) ? AppliedOpenGLColorBitDepthMode : OpenGLColorBitDepthMode;
        if (savedFPSCapCustom < VIDEO_FPS_CAP_CUSTOM_MINIMUM || savedFPSCapCustom > VIDEO_FPS_CAP_CUSTOM_MAXIMUM) savedFPSCapCustom = 100;
        fprintf(fp, "%d %d %d %d %d %d %d %d %d %d %d\n", savedMode, savedScreenMode, savedRefreshMilliHz, OpenGLAntiAliasingMode, OpenGLAnisotropyMode, OpenGLTextureFilterMode, savedVideoVSync, savedFPSCapMode, savedFPSCapCustom, savedColorBitDepth, 124);
        fprintf(fp,"%d %d %d %d\n",ImageBrightness,ImageContrast,ImageSaturation,157);
        fprintf(fp,"%d %d\n",WaterEffectMode==1,158);
        fprintf(fp,"%d %d\n",MenuBackgroundMode>=0&&MenuBackgroundMode<=2?MenuBackgroundMode:1,231);
        if (savedMode >= 0 && savedMode < TotalVideoModes)
            fprintf(fp,"%d %d %d\n",VideoModeList[savedMode].w,VideoModeList[savedMode].h,170);
        else fprintf(fp,"0 0 170\n");
        fprintf(fp,"%d 171\n",ScreenshotFormat>=0&&ScreenshotFormat<SCREENSHOT_FORMAT_COUNT?ScreenshotFormat:SCREENSHOT_PNG);
        fprintf(fp,"%d 180\n",FMVFilterMode==0?0:1);
        fprintf(fp,"%d 207\n",CampaignModeFromPreference(207,RetainPlayerState));
        fprintf(fp,"%d 214\n",OptimizedAmmo==1);
        fprintf(fp,"%d %d 230\n",MenuBackgroundType==1,MenuMusicMode==1);
        fclose(fp);
    }
}
void SaveDeviceAndVideoModePreferences()
{
 EVENT_TICK stamp=EventLatencyBegin();
 SaveDeviceAndVideoModePreferencesLatencyBody();
 EventLatencyEnd(stamp,"save video/config",0,0,0,"AvP_TempVideo.cfg",2);
}
void PreviousVideoMode2(void)
{
	int cur = CurrentVideoMode;
	do { if (cur == 0) cur = TotalVideoModes; cur--; if (cur == CurrentVideoMode) return; } while(!VideoModeList[cur].available || !VideoModeMatchesCurrentAspect(cur));
	CurrentVideoMode = cur;
	RefreshVideoRefreshRates();
}

void NextVideoMode2(void)
{
	int cur = CurrentVideoMode;
	do { cur++; if (cur == TotalVideoModes) cur = 0; if (cur == CurrentVideoMode) return; } while(!VideoModeList[cur].available || !VideoModeMatchesCurrentAspect(cur));
	CurrentVideoMode = cur;
	RefreshVideoRefreshRates();
}

/* Cycle displayed fraction families, not enum storage order. Other SDL ratios
 * (for example 25:16) participate individually instead of sharing one bucket.
 * Canonical near-matches retain their existing label and persistent mode IDs. */
static int CompareVideoAspectKeys(int n,int d,int otherN,int otherD)
{
 if(n!=otherN)return n<otherN?-1:1;
 return d==otherD?0:(d<otherD?-1:1);
}
static void CycleVideoAspect(int direction)
{
 int n,d,i,best=-1,wrap=-1,bn=0,bd=0,wn=0,wd=0;
 if(CurrentVideoMode<0 || CurrentVideoMode>=TotalVideoModes)return;
 VideoAspectKeyForMode(CurrentVideoMode,&n,&d);
 for(i=0;i<TotalVideoModes;++i){
  int cn,cd,cmp;
  if(!VideoModeList[i].available)continue;
  VideoAspectKeyForMode(i,&cn,&cd);
  cmp=CompareVideoAspectKeys(cn,cd,n,d)*direction;
  if(wrap<0 || CompareVideoAspectKeys(cn,cd,wn,wd)*direction<0){wrap=i;wn=cn;wd=cd;}
  if(cmp>0 && (best<0 || CompareVideoAspectKeys(cn,cd,bn,bd)*direction<0)){best=i;bn=cn;bd=cd;}
 }
 if(best<0)best=wrap;
 if(best<0)return;
 VideoAspectKeyForMode(best,&bn,&bd);
 if(!CompareVideoAspectKeys(bn,bd,n,d))return;
 SelectVideoAspectForMode(best);
 SelectFirstAvailableModeForAspect();
}
void PreviousVideoAspect(void){CycleVideoAspect(-1);}
void NextVideoAspect(void){CycleVideoAspect(1);}

void PreviousVideoRefresh(void)
{
	int i;
	if (NumberOfVideoRefreshRates < 2) return;
	for (i = 0; i < NumberOfVideoRefreshRates; i++) if (VideoRefreshRatesMilliHz[i] == CurrentVideoRefreshMilliHz) break;
	if (i == NumberOfVideoRefreshRates || i == 0) i = NumberOfVideoRefreshRates;
	CurrentVideoRefreshMilliHz = VideoRefreshRatesMilliHz[i-1];
}

void NextVideoRefresh(void)
{
	int i;
	if (NumberOfVideoRefreshRates < 2) return;
	for (i = 0; i < NumberOfVideoRefreshRates; i++) if (VideoRefreshRatesMilliHz[i] == CurrentVideoRefreshMilliHz) break;
	if (i == NumberOfVideoRefreshRates - 1 || i == NumberOfVideoRefreshRates) i = -1;
	CurrentVideoRefreshMilliHz = VideoRefreshRatesMilliHz[i+1];
}

void PreviousVideoScreenMode(void) { CurrentVideoScreenMode = (CurrentVideoScreenMode + 2) % 3; }
void NextVideoScreenMode(void) { CurrentVideoScreenMode = (CurrentVideoScreenMode + 1) % 3; }
void PreviousVideoVSync(void) { CurrentVideoVSync = !CurrentVideoVSync; }
void NextVideoVSync(void) { CurrentVideoVSync = !CurrentVideoVSync; }
/* Display order is independent of persistent mode IDs. */
static const int VideoFPSCapCycle[VIDEO_FPS_CAP_MODE_COUNT] = {
    VIDEO_FPS_CAP_60, VIDEO_FPS_CAP_65, VIDEO_FPS_CAP_75, VIDEO_FPS_CAP_85,
    VIDEO_FPS_CAP_100, VIDEO_FPS_CAP_120, VIDEO_FPS_CAP_142,
    VIDEO_FPS_CAP_UNCAPPED, VIDEO_FPS_CAP_CUSTOM
};
static void StepVideoFPSCap(int direction)
{
    int i;
    for (i = 0; i < VIDEO_FPS_CAP_MODE_COUNT; ++i) {
        if (VideoFPSCapCycle[i] == CurrentVideoFPSCapMode) {
            CurrentVideoFPSCapMode = VideoFPSCapCycle[
                (i + direction + VIDEO_FPS_CAP_MODE_COUNT) % VIDEO_FPS_CAP_MODE_COUNT];
            return;
        }
    }
    CurrentVideoFPSCapMode = VIDEO_FPS_CAP_100;
}
void PreviousVideoFPSCap(void) { StepVideoFPSCap(-1); }
void NextVideoFPSCap(void) { StepVideoFPSCap(1); }

static int VideoFPSCapValue(int mode, int customValue)
{
    switch (mode) {
        case VIDEO_FPS_CAP_60: return 60;
        case VIDEO_FPS_CAP_65: return 65;
        case VIDEO_FPS_CAP_75: return 75;
        case VIDEO_FPS_CAP_85: return 85;
        case VIDEO_FPS_CAP_100: return 100;
        case VIDEO_FPS_CAP_120: return 120;
        case VIDEO_FPS_CAP_142: return 142;
        case VIDEO_FPS_CAP_CUSTOM: return customValue;
        default: return 0;
    }
}
int VideoFPSCapIsCustom(void) { return CurrentVideoFPSCapMode == VIDEO_FPS_CAP_CUSTOM; }
void ResetVideoFPSCapToDefault(void) { CurrentVideoFPSCapMode = VIDEO_FPS_CAP_100; CurrentVideoFPSCapCustom = 100; }

char *GetVideoRendererDescription(void) { return "OpenGL 2.1"; }
static int VideoAspectGreatestCommonDivisor(int a, int b)
{
	while (b != 0)
	{
		int remainder = a % b;
		a = b;
		b = remainder;
	}
	return a;
}

char *GetVideoAspectDescription(void)
{
	static char *names[] = {
		"4:3", "5:4", "16:9", "16:10", "3:2", "20:13",
		"195:139", "106:75", "40:27", "1.85:1", "225:128",
		"1.90:1 (DCI)", "2.01:1", "21:9", "32:9"
	};
	static char exactRatio[32];
	if (CurrentVideoAspect >= 0 && CurrentVideoAspect < VIDEO_ASPECT_OTHER)
		return names[CurrentVideoAspect];
	if (CurrentVideoMode >= 0 && CurrentVideoMode < TotalVideoModes)
	{
		int width = VideoModeList[CurrentVideoMode].w;
		int height = VideoModeList[CurrentVideoMode].h;
		int divisor = VideoAspectGreatestCommonDivisor(width, height);
		_snprintf(exactRatio, sizeof(exactRatio), "%d:%d", width / divisor, height / divisor);
		return exactRatio;
	}
	return "Other";
}
char *GetVideoResolutionDescription(void)
{
	static char buf[64];
	_snprintf(buf, sizeof(buf), "%dx%d", VideoModeList[CurrentVideoMode].w, VideoModeList[CurrentVideoMode].h);
	return buf;
}
char *GetVideoRefreshDescription(void)
{
	static char buf[64];
	if (CurrentVideoRefreshMilliHz <= 0) return "Default";
	if (CurrentVideoRefreshMilliHz % 1000 == 0) _snprintf(buf, sizeof(buf), "%d Hz", CurrentVideoRefreshMilliHz / 1000);
	else _snprintf(buf, sizeof(buf), "%.3f Hz", (float)CurrentVideoRefreshMilliHz / 1000.0f);
	return buf;
}
char *GetVideoScreenModeDescription(void)
{
	static enum TEXTSTRING_ID names[] = { TEXTSTRING_VIDEO_WINDOWED_NEW, TEXTSTRING_VIDEO_BORDERLESS_NEW, TEXTSTRING_VIDEO_EXCLUSIVE_NEW };
	return GetTextString(names[CurrentVideoScreenMode]);
}
char *GetVideoVSyncDescription(void)
{
    return GetTextString(CurrentVideoVSync ? TEXTSTRING_VIDEO_VSYNC_ON_NEW : TEXTSTRING_VIDEO_VSYNC_OFF_NEW);
}

char *GetVideoFPSCapDescription(void)
{
    static char description[64];
    if (CurrentVideoFPSCapMode == VIDEO_FPS_CAP_CUSTOM) {
        sprintf(description, "%s %d", GetTextString(TEXTSTRING_VIDEO_FPSCAP_CUSTOM_NEW), CurrentVideoFPSCapCustom);
    } else {
        int fps = VideoFPSCapValue(CurrentVideoFPSCapMode, CurrentVideoFPSCapCustom);
        if (fps == 0) return GetTextString(TEXTSTRING_VIDEO_FPSCAP_UNCAPPED_NEW);
        sprintf(description, "%d", fps);
    }
    return description;
}
char *GetVideoModeDescription2(void) { return GetVideoRendererDescription(); }
char *GetVideoModeDescription3(void) { return GetVideoResolutionDescription(); }

int InitSDL()
{
	/* Keep SDL relative motion independent of the Windows pointer-speed curve. */
	SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "0");

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL Init failed: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	atexit(SDL_Quit);
#if TYRARGO_MEASURE_LATENCY
 atexit(EventLatencyShutdown);
#endif

	SDL_AddEventWatch(SDLEventFilter, NULL);

#if 0
	SDL_Rect **SDL_AvailableVideoModes;
	SDL_AvailableVideoModes = SDL_ListModes(NULL, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
	if (SDL_AvailableVideoModes == NULL)
		return -1;
	
	if (SDL_AvailableVideoModes != (SDL_Rect **)-1) {
		int i, j, foundit;
		
		foundit = 0;
		for (i = 0; i < TotalVideoModes; i++) {
			SDL_Rect **modes = SDL_AvailableVideoModes;
			
			for (j = 0; modes[j]; j++) {
				if (modes[j]->w >= VideoModeList[i].w &&
				    modes[j]->h >= VideoModeList[i].h) {
					if (SDL_VideoModeOK(VideoModeList[i].w, VideoModeList[i].h, 16, SDL_FULLSCREEN | SDL_OPENGL)) {
						/* assume SDL isn't lying to us */
						VideoModeList[i].available = 1;
						
						foundit = 1;
					}
					break;
				}
			}			
		}		
		if (foundit == 0)
			return -1;
	} else {
		int i, foundit;
		
		foundit = 0;
		for (i = 0; i < TotalVideoModes; i++) {
			if (SDL_VideoModeOK(VideoModeList[i].w, VideoModeList[i].h, 16, SDL_FULLSCREEN | SDL_OPENGL)) {
				/* assume SDL isn't lying to us */
				VideoModeList[i].available = 1;
				
				foundit = 1;
			}
		}
		
		if (foundit == 0)
			return -1;
	}
#endif

	if (!RefreshVideoModeAvailability()) {
		fprintf(stderr, "SDL3 did not report any usable exclusive fullscreen modes: %s\n", SDL_GetError());
		return -1;
	}

	LoadDeviceAndVideoModePreferences();

	if (WantJoystick) {
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
			
		joy = SDL_OpenJoystick(0);
		if (joy) {
			GotJoystick = 1;
			
			JoystickCaps.wCaps = 0; /* no rudder... ? */
			
			JoystickData.dwXpos = 0;
			JoystickData.dwYpos = 0;
			JoystickData.dwRpos = 0;
			JoystickData.dwUpos = 0;
			JoystickData.dwVpos = 0;
			JoystickData.dwPOV = (DWORD) -1;
		}
	}
	
	Uint32 rmask, gmask, bmask, amask;
	
	// pre-create the software surface in OpenGL RGBA order
	// menus.c assumes RGB565; possible to support both?
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x0000f800;
    gmask = 0x000007e0;
    bmask = 0x0000001f;
    amask = 0x00000000;
#endif

	surface = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_RGBA5551);
	if (surface == NULL) {
		return -1;
	}

	return 0;
}

static int PlayerFOVSpeciesGroup(void)
{
	return (AvP.PlayerType == I_Alien) ? 1 : 0;
}

static int ClampPlayerFOVValue(int value, int group)
{
	int minimum = group ? PLAYER_FOV_ALIEN_DEFAULT : PLAYER_FOV_MARINE_PREDATOR_MINIMUM;
	int maximum = group ? PLAYER_FOV_MAXIMUM : PLAYER_FOV_MARINE_PREDATOR_MAXIMUM;
	if (value < minimum) return minimum;
	if (value > maximum) return maximum;
	return value;
}

static void SynchronisePlayerFOVForCurrentSpecies(void)
{
	int group = PlayerFOVSpeciesGroup();
	if (ActivePlayerFOVSpeciesGroup != group) {
		if (ActivePlayerFOVSpeciesGroup == 0) PlayerFOVMarinePredator = ClampPlayerFOVValue(PlayerFOV, 0);
		else if (ActivePlayerFOVSpeciesGroup == 1) PlayerFOVAlien = ClampPlayerFOVValue(PlayerFOV, 1);
		PlayerFOV = group ? PlayerFOVAlien : PlayerFOVMarinePredator;
		ActivePlayerFOVSpeciesGroup = group;
	}
}

int PlayerFOVMinimumForCurrentSpecies(void)
{
	return (AvP.PlayerType == I_Alien) ? PLAYER_FOV_ALIEN_DEFAULT : PLAYER_FOV_MARINE_PREDATOR_MINIMUM;
}

int PlayerFOVDefaultForCurrentSpecies(void)
{
	return (AvP.PlayerType == I_Alien) ? PLAYER_FOV_ALIEN_DEFAULT : PLAYER_FOV_MARINE_DEFAULT;
}

int PlayerFOVMaximumForCurrentSpecies(void)
{
	return (AvP.PlayerType == I_Alien) ? PLAYER_FOV_MAXIMUM : PLAYER_FOV_MARINE_PREDATOR_MAXIMUM;
}

void ClampPlayerFOVForCurrentSpecies(void)
{
	int group;
	SynchronisePlayerFOVForCurrentSpecies();
	group = PlayerFOVSpeciesGroup();
	PlayerFOV = ClampPlayerFOVValue(PlayerFOV, group);
	if (group) PlayerFOVAlien = PlayerFOV; else PlayerFOVMarinePredator = PlayerFOV;
}

void RestorePlayerFOVForCurrentSpecies(void)
{
	/* A profile load replaces both saved groups, so discard the old active view. */
	ActivePlayerFOVSpeciesGroup = -1;
	ClampPlayerFOVForCurrentSpecies();
}
static int WeaponFOVSpecies(void)
{
	if (AvP.PlayerType == I_Predator) return I_Predator;
	if (AvP.PlayerType == I_Alien) return I_Alien;
	return I_Marine;
}
static int ClampWeaponFOVValue(int value, int species)
{
	if (value < WEAPON_FOV_MINIMUM) return WEAPON_FOV_MINIMUM;
	int maximum = (species == I_Alien) ? WEAPON_FOV_ALIEN_MAXIMUM : WEAPON_FOV_MAXIMUM;
	if (value > maximum) return maximum;
	return value;
}
void SynchroniseWeaponFOVForCurrentSpecies(void)
{
	int species = WeaponFOVSpecies();
	if (ActiveWeaponFOVSpecies != species)
	{
		if (ActiveWeaponFOVSpecies == I_Marine) WeaponFOVMarine = ClampWeaponFOVValue(WeaponFOV, I_Marine);
		else if (ActiveWeaponFOVSpecies == I_Predator) WeaponFOVPredator = ClampWeaponFOVValue(WeaponFOV, I_Predator);
		else if (ActiveWeaponFOVSpecies == I_Alien) WeaponFOVAlien = ClampWeaponFOVValue(WeaponFOV, I_Alien);
		if (species == I_Predator) WeaponFOV = WeaponFOVPredator;
		else if (species == I_Alien) WeaponFOV = WeaponFOVAlien;
		else WeaponFOV = WeaponFOVMarine;
		ActiveWeaponFOVSpecies = species;
	}
	/* Clamp inactive profile values too, without borrowing the active species limit. */
	WeaponFOVMarine = ClampWeaponFOVValue(WeaponFOVMarine, I_Marine);
	WeaponFOVPredator = ClampWeaponFOVValue(WeaponFOVPredator, I_Predator);
	WeaponFOVAlien = ClampWeaponFOVValue(WeaponFOVAlien, I_Alien);
	WeaponFOV = ClampWeaponFOVValue(WeaponFOV, species);
	if (species == I_Predator) WeaponFOVPredator = WeaponFOV;
	else if (species == I_Alien) WeaponFOVAlien = WeaponFOV;
	else WeaponFOVMarine = WeaponFOV;
}
void RestoreWeaponFOVForCurrentSpecies(void)
{
	ActiveWeaponFOVSpecies = -1;
	SynchroniseWeaponFOVForCurrentSpecies();
}
int WeaponFOVMaximumForCurrentSpecies(void)
{
	return WeaponFOVSpecies() == I_Alien ? WEAPON_FOV_ALIEN_MAXIMUM : WEAPON_FOV_MAXIMUM;
}
int WeaponFOVDefaultForCurrentSpecies(void)
{
	return (AvP.PlayerType == I_Alien) ? WEAPON_FOV_ALIEN_DEFAULT : WEAPON_FOV_MARINE_PREDATOR_DEFAULT;
}


void RefreshPlayerFOV(void)
{
    double distance;
    ClampPlayerFOVForCurrentSpecies();
    distance=AvpProjectionDistance(ScreenDescriptorBlock.SDB_Width,ScreenDescriptorBlock.SDB_Height,PlayerFOV);
    ScreenDescriptorBlock.SDB_ProjX=(int)(distance+0.5);
    ScreenDescriptorBlock.SDB_ProjY=(int)(distance*0.75+0.5);
    if(ScreenDescriptorBlock.SDB_ProjX<1) ScreenDescriptorBlock.SDB_ProjX=1;
    if(ScreenDescriptorBlock.SDB_ProjY<1) ScreenDescriptorBlock.SDB_ProjY=1;
}
double WeaponFOVProjectionForViewSize(int viewWidth, int viewHeight)
{
	int fov;

	SynchroniseWeaponFOVForCurrentSpecies();
	fov = WeaponFOV;

	/* Stretched keeps an independent draw but transfers the world-FOV delta
	   around the original 90-degree weapon lens.  Thus FOV weapon still has
	   a visible effect and the model expands/contracts with the world lens. */
	if (WeaponFOVStyle == WEAPON_FOV_STYLE_STRETCHED)
	{
		fov += PlayerFOV - WEAPON_FOV_WORLD_REFERENCE;
	}

	if (fov < PLAYER_FOV_MINIMUM) fov = PLAYER_FOV_MINIMUM;
	if (fov > PLAYER_FOV_MAXIMUM) fov = PLAYER_FOV_MAXIMUM;
    return AvpProjectionDistance(viewWidth,viewHeight,fov);
}

/* Borderless keeps the physical desktop mode intact. The applied game mode is
 * an independent source framebuffer which is scaled only at presentation. */
static int AppliedVideoModeIsValid(void)
{
	return AppliedVideoMode >= 0 && AppliedVideoMode < TotalVideoModes
		&& VideoModeList[AppliedVideoMode].w > 0 && VideoModeList[AppliedVideoMode].h > 0;
}

static void GetBorderlessPresentationSize(int PhysicalWidth, int PhysicalHeight, int *CanvasWidth, int *CanvasHeight)
{
	int targetWidth = PhysicalWidth;
	int targetHeight = PhysicalHeight;
	int fittedWidth;
	int fittedHeight;

	if (AppliedVideoModeIsValid())
	{
		targetWidth = VideoModeList[AppliedVideoMode].w;
		targetHeight = VideoModeList[AppliedVideoMode].h;
	}

	fittedWidth = (PhysicalHeight * targetWidth) / targetHeight;
	fittedHeight = (PhysicalWidth * targetHeight) / targetWidth;
	if (fittedWidth <= PhysicalWidth)
	{
		*CanvasWidth = fittedWidth;
		*CanvasHeight = PhysicalHeight;
	}
	else
	{
		*CanvasWidth = PhysicalWidth;
		*CanvasHeight = fittedHeight;
	}
	if (*CanvasWidth < 1) *CanvasWidth = 1;
	if (*CanvasHeight < 1) *CanvasHeight = 1;
}

static void SetOpenGLPresentationViewport(int PhysicalWidth, int PhysicalHeight, int SoftwareMenu)
{
	int canvasWidth = PhysicalWidth;
	int canvasHeight = PhysicalHeight;

	if (!SoftwareMenu && AppliedVideoScreenMode == VIDEO_SCREEN_BORDERLESS)
	{
		GetBorderlessPresentationSize(PhysicalWidth, PhysicalHeight, &canvasWidth, &canvasHeight);
	}

	PresentationViewportWidth = canvasWidth;
	PresentationViewportHeight = canvasHeight;
	PresentationViewportX = (PhysicalWidth - canvasWidth) / 2;
	PresentationViewportY = (PhysicalHeight - canvasHeight) / 2;
	if (pglViewport != NULL)
	{
		pglViewport(PresentationViewportX, PresentationViewportY,
			PresentationViewportWidth, PresentationViewportHeight);
	}
}

static void GetVirtualFramebufferSize(int PhysicalWidth, int PhysicalHeight, int SoftwareMenu, int *VirtualWidth, int *VirtualHeight)
{
	if (SoftwareMenu)
	{
		*VirtualWidth = FrontendCanvasWidth;
		*VirtualHeight = FrontendCanvasHeight;
	}
	else if (AppliedVideoScreenMode == VIDEO_SCREEN_BORDERLESS && AppliedVideoModeIsValid())
	{
		/* This is deliberately the selected mode, not a fitted desktop canvas. */
		*VirtualWidth = VideoModeList[AppliedVideoMode].w;
		*VirtualHeight = VideoModeList[AppliedVideoMode].h;
	}
	else
	{
		*VirtualWidth = PhysicalWidth;
		*VirtualHeight = PhysicalHeight;
	}
}

int VideoHUDUsesWideLayout(void)
{
 int w,h;
 GetVirtualFramebufferSize(ViewportWidth,ViewportHeight,0,&w,&h);
 return w>0 && h>0 && (int64_t)w*2>(int64_t)h*3;
}

/* MSAA resolves the scene before this copy. Preserve those edge gradients
   when scaling the virtual canvas; nearest magnification turns them into blocks. */
static GLint BorderlessPresentationFilter(void)
{
 return OpenGLAntiAliasingMode>0?GL_LINEAR:GL_NEAREST;
}
static int EnsureBorderlessPresentationTexture(int SourceWidth, int SourceHeight,
	int PhysicalWidth, int PhysicalHeight)
{
	GLint maximumTextureSize = 0;

	/* The default framebuffer cannot be copied outside the desktop drawable.
	 * Modes larger than the desktop use the regular presentation path rather
	 * than silently claiming a false internal resolution. */
	if (SourceWidth < 1 || SourceHeight < 1 || SourceWidth > PhysicalWidth || SourceHeight > PhysicalHeight) {
		Probe113_RenderTracef("TEXTURE_REJECT source=%dx%d physical=%dx%d", SourceWidth, SourceHeight, PhysicalWidth, PhysicalHeight);
		return 0;
	}
	pglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
	BorderlessRenderTraceOpenGLError("texture-max-query");
	if (maximumTextureSize < SourceWidth || maximumTextureSize < SourceHeight) {
		Probe113_RenderTracef("TEXTURE_REJECT source=%dx%d physical=%dx%d max=%d", SourceWidth, SourceHeight, PhysicalWidth, PhysicalHeight, maximumTextureSize);
		return 0;
	}

	if (BorderlessPresentationTexture == 0) {
		pglGenTextures(1, &BorderlessPresentationTexture);
		BorderlessRenderTraceOpenGLError("texture-create");
	}
	if (BorderlessPresentationTexture == 0) {
		Probe113_RenderTracef("TEXTURE_CREATE_FAILED source=%dx%d", SourceWidth, SourceHeight);
		return 0;
	}

	pglBindTexture(GL_TEXTURE_2D, BorderlessPresentationTexture);
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, BorderlessPresentationFilter());
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, BorderlessPresentationFilter());
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (BorderlessPresentationTextureWidth != SourceWidth || BorderlessPresentationTextureHeight != SourceHeight || BorderlessPresentationTextureColorBitDepth != OpenGLColorBitDepthMode)
	{
		if (OpenGLColorBitDepthMode == OPENGL_COLOR_BIT_DEPTH_16) {
			pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, SourceWidth, SourceHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
		} else {
			pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SourceWidth, SourceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
		BorderlessPresentationTextureWidth = SourceWidth;
		BorderlessPresentationTextureHeight = SourceHeight;
		BorderlessPresentationTextureColorBitDepth = OpenGLColorBitDepthMode;
				Probe113_RenderTracef("TEXTURE_ALLOC handle=%u source=%dx%d physical=%dx%d max=%d colourDepth=%d", (unsigned int)BorderlessPresentationTexture, SourceWidth, SourceHeight, PhysicalWidth, PhysicalHeight, maximumTextureSize, OpenGLColorBitDepthMode);
		BorderlessRenderTraceOpenGLError("texture-allocate");
	}
	pglBindTexture(GL_TEXTURE_2D, 0);
	return 1;
}

/* A borderless render target belongs only to its desktop-sized compositor.
 * Release it before entering a native SDL mode so no stale virtual surface
 * or scissor state survives the fullscreen transaction. */
static void ReleaseBorderlessPresentationTexture(void)
{
	if (BorderlessPresentationTexture != 0 && pglDeleteTextures != NULL) {
		pglDeleteTextures(1, &BorderlessPresentationTexture);
	}
	BorderlessPresentationTexture = 0;
	BorderlessPresentationTextureWidth = 0;
	BorderlessPresentationTextureHeight = 0;
	BorderlessPresentationTextureColorBitDepth = -1;
	DesktopRefreshFrameDeadlineNS = 0;
}

static void SetOpenGLRenderViewport(int PhysicalWidth, int PhysicalHeight, int SoftwareMenu,
	int VirtualWidth, int VirtualHeight)
{
	if (!SoftwareMenu && (AppliedVideoScreenMode == VIDEO_SCREEN_BORDERLESS
		|| (AppliedVideoScreenMode == VIDEO_SCREEN_EXCLUSIVE
			&& OpenGLColorBitDepthMode == OPENGL_COLOR_BIT_DEPTH_16))
		&& EnsureBorderlessPresentationTexture(VirtualWidth, VirtualHeight, PhysicalWidth, PhysicalHeight))
	{
		/* The off-screen presentation texture centres borderless rendering and,
		 * in exclusive 16-bit mode, explicitly quantizes the completed scene to
		 * RGB565 before the final swap. */
		pglViewport(0, 0, VirtualWidth, VirtualHeight);
		if (BorderlessRenderTraceFrame < 8) {
			Probe113_RenderTracef("VIEWPORT_RENDER quantized physical=%dx%d virtual=%dx%d screen=%d depth=%d", PhysicalWidth, PhysicalHeight, VirtualWidth, VirtualHeight, AppliedVideoScreenMode, OpenGLColorBitDepthMode);
			BorderlessRenderTraceOpenGLError("viewport-render");
		}
	}
	else
	{
		if (BorderlessRenderTraceFrame < 8)
			Probe113_RenderTracef("VIEWPORT_PRESENT physical=%dx%d virtual=%dx%d softwareMenu=%d appliedScreen=%d", PhysicalWidth, PhysicalHeight, VirtualWidth, VirtualHeight, SoftwareMenu, AppliedVideoScreenMode);
		SetOpenGLPresentationViewport(PhysicalWidth, PhysicalHeight, SoftwareMenu);
	}
}static void SetWindowSize(int PhysicalWidth, int PhysicalHeight, int VirtualWidth, int VirtualHeight)
{
#if !defined(NDEBUG)
	fprintf(stderr, "SetWindowSize(%d,%d,%d,%d); %d\n", PhysicalWidth, PhysicalHeight, VirtualWidth, VirtualHeight, CurrentVideoMode);
#endif

	ViewportWidth = PhysicalWidth;
	ViewportHeight = PhysicalHeight;

	ScreenDescriptorBlock.SDB_Width     = VirtualWidth;
	ScreenDescriptorBlock.SDB_Height    = VirtualHeight;
	ScreenDescriptorBlock.SDB_CentreX   = VirtualWidth/2;
	ScreenDescriptorBlock.SDB_CentreY   = VirtualHeight/2;
	RefreshPlayerFOV();
	ScreenDescriptorBlock.SDB_ClipLeft  = 0;
	ScreenDescriptorBlock.SDB_ClipRight = VirtualWidth;
	ScreenDescriptorBlock.SDB_ClipUp    = 0;
	ScreenDescriptorBlock.SDB_ClipDown  = VirtualHeight;
    /* Resize the live coordinate systems without recreating gadgets, replaying
       config.cfg, discarding console history or changing saved preferences. */
    DAVEHOOK_RefreshScreenBounds();
    HUD_RefreshLayout();
	Probe113_RenderTracef("SET_WINDOW_SIZE physical=%dx%d virtual=%dx%d appliedMode=%d appliedScreen=%d refresh=%d", PhysicalWidth, PhysicalHeight, VirtualWidth, VirtualHeight, AppliedVideoMode, AppliedVideoScreenMode, AppliedVideoRefreshMilliHz);

	/* This helper only updates renderer geometry. SDL window mutation belongs
	 * exclusively to the confirmed Apply*VideoMode transaction. */
}

/* Set an exact exclusive display mode, including the user-selected real refresh rate. */
static int ApplyVideoModeIndex(int videoMode)
{
	SDL_DisplayID displayID;
	SDL_DisplayMode **displayModes;
	const SDL_DisplayMode *requestedMode = NULL;
	int displayModeCount = 0;
	int i;
	int actualWidth;
	int actualHeight;
	int selectedRefreshMilliHz = 0;

	if (window == NULL || videoMode < 0 || videoMode >= TotalVideoModes || !VideoModeList[videoMode].available) return 0;
	displayID = VideoMenuDisplay();
	if (!displayID) return 0;
	displayModes = SDL_GetFullscreenDisplayModes(displayID, &displayModeCount);
	for (i = 0; displayModes != NULL && i < displayModeCount; i++)
	{
		if (displayModes[i]->w != VideoModeList[videoMode].w || displayModes[i]->h != VideoModeList[videoMode].h) continue;
		if (VideoModeRefreshMilliHz(displayModes[i]) == CurrentVideoRefreshMilliHz) { requestedMode = displayModes[i]; break; }
		if (requestedMode == NULL) requestedMode = displayModes[i];
	}
    if (!requestedMode) {
        const SDL_DisplayMode *fallbacks[2] = {
            SDL_GetDesktopDisplayMode(displayID), SDL_GetCurrentDisplayMode(displayID)
        };
        for (i = 0; i < 2; ++i) {
            const SDL_DisplayMode *candidate = fallbacks[i];
            if (!candidate || candidate->w != VideoModeList[videoMode].w || candidate->h != VideoModeList[videoMode].h) continue;
            if (!requestedMode) requestedMode = candidate;
            if (VideoModeRefreshMilliHz(candidate) == CurrentVideoRefreshMilliHz) { requestedMode = candidate; break; }
        }
    }
	/* displayModes is owned by SDL and is freed below. Cache its selected refresh
	 * now; retaining requestedMode after SDL_free caused borderless-to-exclusive
	 * transitions to read freed mode memory. */
	if (requestedMode != NULL) selectedRefreshMilliHz = VideoModeRefreshMilliHz(requestedMode);

	/* Changing only the refresh rate while SDL is already in exclusive mode is
	 * driver-sensitive. Make the transition explicit and synchronous: leave the
	 * old exclusive mode, select the exact new SDL_DisplayMode, then re-enter. */
	if (requestedMode == NULL
		|| ((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0
			&& (!SDL_SetWindowFullscreen(window, false) || !SDL_SyncWindow(window)))
		|| !SDL_SetWindowFullscreenMode(window, requestedMode)
		|| !SDL_SetWindowFullscreen(window, true) || !SDL_SyncWindow(window))
	{
		SDL_free(displayModes);
		return 0;
	}
	SDL_free(displayModes);
	SDL_GetWindowSizeInPixels(window, &actualWidth, &actualHeight);
	if (actualWidth != VideoModeList[videoMode].w || actualHeight != VideoModeList[videoMode].h) return 0;
	SetWindowSize(actualWidth, actualHeight, actualWidth, actualHeight);
	LastKnownGoodVideoMode = videoMode;
	LastKnownGoodVideoRefreshMilliHz = selectedRefreshMilliHz;
	return 1;
}

static int ApplyWindowedVideoMode(void)
{
	int logicalWidth;
	int logicalHeight;
	int actualWidth;
	int actualHeight;

 SDL_DisplayID display=VideoMenuDisplay();
 if (window == NULL || !SDL_SetWindowFullscreen(window, false) || !SDL_SyncWindow(window)) return 0;
 if (!SDL_SetWindowFullscreenMode(window,NULL)) return 0;
	if (!SDL_SetWindowSize(window, VideoModeList[CurrentVideoMode].w, VideoModeList[CurrentVideoMode].h) || !SDL_SyncWindow(window)) return 0;
 /* Explicit mode/resolution requests are centred; ordinary user moves are not. */
 if (!SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED_DISPLAY(display),SDL_WINDOWPOS_CENTERED_DISPLAY(display)) || !SDL_SyncWindow(window)) return 0;

	/* SDL_SetWindowSize uses logical units; the OpenGL drawable can be larger
	 * on high-DPI displays. Validate each domain with its matching query. */
	SDL_GetWindowSize(window, &logicalWidth, &logicalHeight);
	if (logicalWidth != VideoModeList[CurrentVideoMode].w || logicalHeight != VideoModeList[CurrentVideoMode].h) return 0;
	SDL_GetWindowSizeInPixels(window, &actualWidth, &actualHeight);
	if (actualWidth < 1 || actualHeight < 1) return 0;
	SetWindowSize(actualWidth, actualHeight, actualWidth, actualHeight);
	return 1;
}

static int ApplyBorderlessVideoMode(void)
{
	SDL_WindowFlags windowFlags;
	const SDL_DisplayMode *fullscreenMode;
	int actualWidth = 0;
	int actualHeight = 0;

	if (window == NULL || CurrentVideoMode < 0 || CurrentVideoMode >= TotalVideoModes) return 0;

	/* Borderless owns the desktop drawable. A game resolution is a virtual
	 * presentation canvas, so changing it must not rebuild desktop fullscreen. */
	windowFlags = SDL_GetWindowFlags(window);
	fullscreenMode = SDL_GetWindowFullscreenMode(window);
	if ((windowFlags & SDL_WINDOW_FULLSCREEN) != 0 && fullscreenMode != NULL)
	{
		if (!SDL_SetWindowFullscreen(window, false) || !SDL_SyncWindow(window)) return 0;
		windowFlags = SDL_GetWindowFlags(window);
	}
	if ((windowFlags & SDL_WINDOW_FULLSCREEN) == 0)
	{
		if (!SDL_SetWindowFullscreenMode(window, NULL)
			|| !SDL_SetWindowFullscreen(window, true) || !SDL_SyncWindow(window)) return 0;
	}

	/* Resize events can report intermediate values during focus/fullscreen.
	 * The frame path queries the settled drawable before changing viewport. */
	SDL_GetWindowSizeInPixels(window, &actualWidth, &actualHeight);
	if (actualWidth > 0 && actualHeight > 0)
	{
		WindowWidth = actualWidth;
		WindowHeight = actualHeight;
	}
	VideoPresentationDirty = 1;
	return 1;
}

static void ApplyPresentationSwapInterval(void)
{
	if (context == NULL) return;
	SDL_GL_SetSwapInterval(AppliedVideoVSync ? 1 : 0);
	DesktopRefreshFrameDeadlineNS = 0;
	FPSCapFrameDeadlineNS = 0;
}

static void CommitAppliedVideoSettings(void)
{
    AppliedVideoMode = CurrentVideoMode;
    AppliedVideoScreenMode = CurrentVideoScreenMode;
    AppliedVideoRefreshMilliHz = CurrentVideoRefreshMilliHz;
    AppliedVideoVSync = CurrentVideoVSync;
    AppliedVideoFPSCapMode = CurrentVideoFPSCapMode;
    AppliedVideoFPSCapCustom = CurrentVideoFPSCapCustom;
    VideoPresentationDirty = 1;
    ApplyPresentationSwapInterval();
    if (VideoAutoDefaultsPending) {
        VideoAutoDefaultsPending = 0;
        SaveDeviceAndVideoModePreferences();
    }
}
/* Apply exactly the pending request. This is the sole code path that may
 * mutate SDL window state after startup. */
static int ApplyCurrentVideoModeRequest(void)
{
	if (CurrentVideoScreenMode == VIDEO_SCREEN_WINDOWED) return ApplyWindowedVideoMode();
	if (CurrentVideoScreenMode == VIDEO_SCREEN_BORDERLESS) return ApplyBorderlessVideoMode();
	return ApplyVideoModeIndex(CurrentVideoMode);
}

static int WindowMatchesAppliedVideoSettings(void)
{
	SDL_WindowFlags flags;
	const SDL_DisplayMode *fullscreenMode;
	int logicalWidth;
	int logicalHeight;

	if (window == NULL || !AppliedVideoModeIsValid()) return 0;
	flags = SDL_GetWindowFlags(window);
	fullscreenMode = SDL_GetWindowFullscreenMode(window);
	if (AppliedVideoScreenMode == VIDEO_SCREEN_WINDOWED)
	{
		if ((flags & SDL_WINDOW_FULLSCREEN) != 0) return 0;
		SDL_GetWindowSize(window, &logicalWidth, &logicalHeight);
		return logicalWidth == VideoModeList[AppliedVideoMode].w
			&& logicalHeight == VideoModeList[AppliedVideoMode].h;
	}
	if ((flags & SDL_WINDOW_FULLSCREEN) == 0) return 0;
	if (AppliedVideoScreenMode == VIDEO_SCREEN_BORDERLESS) return fullscreenMode == NULL;
	return fullscreenMode != NULL
		&& fullscreenMode->w == VideoModeList[AppliedVideoMode].w
		&& fullscreenMode->h == VideoModeList[AppliedVideoMode].h
		&& VideoModeRefreshMilliHz(fullscreenMode) == AppliedVideoRefreshMilliHz;
}

static void RestorePendingVideoSettingsFromApplied(void)
{
    CurrentVideoMode = AppliedVideoMode;
    CurrentVideoScreenMode = AppliedVideoScreenMode;
    CurrentVideoRefreshMilliHz = AppliedVideoRefreshMilliHz;
    CurrentVideoVSync = AppliedVideoVSync;
    CurrentVideoFPSCapMode = AppliedVideoFPSCapMode;
    CurrentVideoFPSCapCustom = AppliedVideoFPSCapCustom;if (AppliedVideoModeIsValid())
		SelectVideoAspectForMode(AppliedVideoMode);
	RefreshVideoRefreshRates();
}

/* A native drawable must never be retargeted while a live OpenGL context owns it.
 * The fresh-window path below applies the pending mode before creating the context. */
static int RecreateOpenGLVideoContextForNativeModeChange(void);

/* Every screen-mode change is a single SDL transaction. A rejected request is
 * physically rolled back before pending menu values are restored. */
int ApplySelectedVideoModeLatencyBody(void)
{
	int requestedMode = CurrentVideoMode;
	int requestedScreenMode = CurrentVideoScreenMode;
	int requestedRefreshMilliHz = CurrentVideoRefreshMilliHz;
	int applied = 0;
	int restored = 0;
	int transactionStarted = 0;
	int physicalWidth = 0;
	int physicalHeight = 0;

	Probe113_RenderTracef("MODE_REQUEST mode=%d screen=%d refreshMilliHz=%d appliedMode=%d appliedScreen=%d appliedRefresh=%d", requestedMode, requestedScreenMode, requestedRefreshMilliHz, AppliedVideoMode, AppliedVideoScreenMode, AppliedVideoRefreshMilliHz);
	if (requestedMode == AppliedVideoMode && requestedScreenMode == AppliedVideoScreenMode
		&& requestedRefreshMilliHz == AppliedVideoRefreshMilliHz
		&& WindowMatchesAppliedVideoSettings())
	{
		Probe113_RenderTracef("MODE_NOOP mode=%d screen=%d refreshMilliHz=%d", requestedMode, requestedScreenMode, requestedRefreshMilliHz);
		WantFullscreen = (requestedScreenMode != VIDEO_SCREEN_WINDOWED);
		if (context != NULL && !Probe124EnsureOpenGLContextCurrent("mode-noop")) goto restore_pending;
		CommitAppliedVideoSettings();
		return 1;
	}

	/* Any OpenGL operation before the transaction must have the expected
	 * context; SDL is then free to rebuild its native drawable. */
	/* Exclusive modes and native window geometry can replace the platform drawable.
	 * NVIDIA may defer that replacement until the next GL call, so the old context
	 * must be retired before SDL touches the requested native state. Borderless-to-
	 * borderless changes only alter the virtual presentation canvas and stay live. */
	if (context != NULL
		&& (requestedScreenMode != VIDEO_SCREEN_BORDERLESS
			|| AppliedVideoScreenMode != VIDEO_SCREEN_BORDERLESS))
	{
		Probe113_RenderTracef("NATIVE_MODE_RECREATE begin requestedMode=%d screen=%d refreshMilliHz=%d appliedMode=%d appliedScreen=%d appliedRefresh=%d", requestedMode, requestedScreenMode, requestedRefreshMilliHz, AppliedVideoMode, AppliedVideoScreenMode, AppliedVideoRefreshMilliHz);
		if (RecreateOpenGLVideoContextForNativeModeChange())
		{
			Probe113_RenderTracef("NATIVE_MODE_RECREATE complete mode=%d screen=%d refreshMilliHz=%d", AppliedVideoMode, AppliedVideoScreenMode, AppliedVideoRefreshMilliHz);
			return 1;
		}
		Probe113_RenderTracef("NATIVE_MODE_RECREATE failed requestedMode=%d screen=%d refreshMilliHz=%d", requestedMode, requestedScreenMode, requestedRefreshMilliHz);
		goto restore_pending;
	}

	if (context != NULL && !Probe124EnsureOpenGLContextCurrent("mode-preflight"))
		goto restore_pending;	if (AppliedVideoScreenMode == VIDEO_SCREEN_BORDERLESS && requestedScreenMode != VIDEO_SCREEN_BORDERLESS) {
		ReleaseBorderlessPresentationTexture();
		pglDisable(GL_SCISSOR_TEST);
		Probe113_RenderTracef("BORDERLESS_TARGET_RELEASED nextScreen=%d", requestedScreenMode);
	}

	if (!Probe125BeginVideoModeTransaction("mode-apply")) goto restore_pending;
	transactionStarted = 1;
	WantFullscreen = (requestedScreenMode != VIDEO_SCREEN_WINDOWED);
	applied = ApplyCurrentVideoModeRequest();

	if (!applied)
	{
		Probe113_RenderTracef("MODE_APPLY_FAILED requestedMode=%d screen=%d refreshMilliHz=%d sdl=%s", requestedMode, requestedScreenMode, requestedRefreshMilliHz, SDL_GetError());
		RestorePendingVideoSettingsFromApplied();
		WantFullscreen = (CurrentVideoScreenMode != VIDEO_SCREEN_WINDOWED);
		if (AppliedVideoModeIsValid()) restored = ApplyCurrentVideoModeRequest();
		Probe113_RenderTracef("MODE_ROLLBACK result=%d mode=%d screen=%d refreshMilliHz=%d sdl=%s", restored, CurrentVideoMode, CurrentVideoScreenMode, CurrentVideoRefreshMilliHz, SDL_GetError());
	}

	if (transactionStarted && !Probe125FinishVideoModeTransaction(
		applied ? "mode-apply-complete" : (restored ? "mode-rollback-complete" : "mode-rollback-failed")))
	{
		Probe113_RenderTracef("MODE_TRANSACTION_ATTACH_FAILED requestedMode=%d screen=%d refreshMilliHz=%d", requestedMode, requestedScreenMode, requestedRefreshMilliHz);
		applied = 0;
		restored = 0;
	}

	if (applied)
	{
		if (context != NULL && requestedScreenMode != VIDEO_SCREEN_BORDERLESS && pglViewport != NULL) {
			SDL_GetWindowSizeInPixels(window, &physicalWidth, &physicalHeight);
			if (physicalWidth > 0 && physicalHeight > 0)
				pglViewport(0, 0, physicalWidth, physicalHeight);
		}
		CommitAppliedVideoSettings();
		Probe113_RenderTracef("MODE_APPLIED mode=%d screen=%d refreshMilliHz=%d", AppliedVideoMode, AppliedVideoScreenMode, AppliedVideoRefreshMilliHz);
		return 1;
	}

restore_pending:
	RestorePendingVideoSettingsFromApplied();
	if (restored)
	{
		VideoPresentationDirty = 1;
		ApplyPresentationSwapInterval();
		Probe113_RenderTracef("MODE_RESTORED_PHYSICALLY mode=%d screen=%d refreshMilliHz=%d", AppliedVideoMode, AppliedVideoScreenMode, AppliedVideoRefreshMilliHz);
	}
	else
	{
		Probe113_RenderTracef("MODE_RESTORED_LOGICALLY mode=%d screen=%d refreshMilliHz=%d", AppliedVideoMode, AppliedVideoScreenMode, AppliedVideoRefreshMilliHz);
	}
	return 0;
}
int ApplySelectedVideoMode(void)
{
 EVENT_TICK stamp=EventLatencyBegin();
 int result=ApplySelectedVideoModeLatencyBody();
 if(result && window && context) {
     int w=0,h=0,vw=0,vh=0;
     int software=RenderingMode==RENDERING_MODE_SOFTWARE;
     SDL_GetWindowSizeInPixels(window,&w,&h);
     if(w>0 && h>0) {
         WindowWidth=ViewportWidth=w;WindowHeight=ViewportHeight=h;
         if(software)ConfigureFrontendCanvas(FrontendCanvasExpanded);
         GetVirtualFramebufferSize(w,h,software,&vw,&vh);
         SetWindowSize(w,h,vw,vh);
         SetOpenGLRenderViewport(w,h,software,vw,vh);
         VideoPresentationDirty=0;
     }
 }
 EventLatencyEnd(stamp,"apply video mode",CurrentVideoScreenMode,CurrentVideoRefreshMilliHz,0,0,2);
 return result;
}
#include "menu_viewport.h"
void ConfigureFrontendCanvas(int expanded)
{
 int w,h;FrontendCanvasExpanded=expanded;MenuCanvasSize(ViewportWidth,ViewportHeight,expanded,&w,&h);
 if(!surface || surface->w!=w || surface->h!=h){
  SDL_Surface *next=SDL_CreateSurface(w,h,SDL_PIXELFORMAT_RGB565);
  if(!next)return; /* allocation failure retains the working canvas */
  if(surface)SDL_DestroySurface(surface);surface=next;
 }
 FrontendCanvasWidth=surface->w;FrontendCanvasHeight=surface->h;
 SetWindowSize(ViewportWidth,ViewportHeight,FrontendCanvasWidth,FrontendCanvasHeight);
}
static int SetSoftVideoMode(int Width, int Height, int Depth)
{
	//TODO: clear surface

	ConfigureFrontendCanvas(0);
	RenderingMode = RENDERING_MODE_SOFTWARE;
	ScanDrawMode = ScanDrawD3DHardwareRGB;
	GotMouse = 1;

	// reset input
	IngameKeyboardInput_ClearBuffer();

	SetWindowSize(ViewportWidth, ViewportHeight, Width, Height);

	return 0;
}

/* ** */
static bool SDLCALL SDLEventFilter(void* userData, SDL_Event* event) {
	(void) userData;

	//printf("SDLEventFilter: %d\n", event->type);
	
	switch (event->type) {
		case SDL_EVENT_TERMINATING:
			AvP.MainLoopRunning = 0; /* TODO */
			break;
	}
	
	return true;
}

static int InitSDLVideo(void) {
	return 0;
}

static int SetOGLVideoMode(int Width, int Height)
{
	int oldflags;
	int flags;
	
	Probe113_RenderTracef("VIDEO_MODE enter supplied=%dx%d requested=%dx%d screen=%d refresh=%d aa=%d window=%p context=%p", Width, Height, WindowWidth, WindowHeight, CurrentVideoScreenMode, CurrentVideoRefreshMilliHz, OpenGLAntiAliasingMode, (void *)window, (void *)context);
	RenderingMode = RENDERING_MODE_OPENGL;
	ScanDrawMode = ScanDrawD3DHardwareRGB;
	GotMouse = 1;

#if defined(FIXED_WINDOW_SIZE)
	// force the game to use the full desktop
	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
		Width = dm.w;
		Height = dm.h;
	}
#endif

	if (window == NULL) {
		CRTFilter_Release();
    {extern void GeometryStreamRelease(void);GeometryStreamRelease();}FireBatchRelease();
    WaterRenderRelease();

	load_ogl_functions(0);

		flags = SDL_WINDOW_OPENGL;

#if defined(FIXED_WINDOW_SIZE)
		flags |= SDL_WINDOW_BORDERLESS;
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
		/* SDL3 applies the requested mode immediately after creation. */
		if (CurrentVideoScreenMode != VIDEO_SCREEN_WINDOWED) flags |= SDL_WINDOW_FULLSCREEN;

		// the game doesn't properly support window resizing
		//flags |= SDL_WINDOW_RESIZABLE;
#endif

		// reset input
		IngameKeyboardInput_ClearBuffer();
		
		// force restart the video system
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_InitSubSystem(SDL_INIT_VIDEO);

		// set OpenGL attributes first
#if defined(USE_OPENGL_ES)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
		/* The SDL attributes are requests consumed when this context is created. */
		if (OpenGLColorBitDepthMode == OPENGL_COLOR_BIT_DEPTH_16) {
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
		} else {
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		}
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        /* Explicit off/x2/x4/x8 anti-aliasing request. */
        if (OpenGLAntiAliasingMode <= 0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        }
        else
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            switch (OpenGLAntiAliasingMode)
            {
                case 1: SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2); break;
                case 3: SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8); break;
                default: SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); break;
            }
        }

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

		Probe113_RenderTracef("VIDEO_MODE create_context begin window=%p", (void *)window);
		context = SDL_GL_CreateContext(window);
		if (context == NULL) {
			fprintf(stderr, "(OpenGL) SDL SDL_GL_CreateContext failed: %s\n", SDL_GetError());
			exit(EXIT_FAILURE);
		}
		Probe113_RenderTracef("VIDEO_MODE create_context ok context=%p", (void *)context);
		SDL_GL_MakeCurrent(window, context);

		Probe113_RenderTracef("VIDEO_MODE load_gl begin");
		load_ogl_functions(1);
		Probe113_RenderTracef("VIDEO_MODE load_gl ok");
		ApplyPresentationSwapInterval();

		SDL_GetWindowSizeInPixels(window, &Width, &Height);
		pglViewport(0, 0, Width, Height);

		// create fullscreen window texture
		pglGenTextures(1, &FullscreenTexture);

		pglBindTexture(GL_TEXTURE_2D, FullscreenTexture);

		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		FullscreenTextureWidth = 1024;
		FullscreenTextureHeight = 512;
		pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FullscreenTextureWidth, FullscreenTextureHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
	}

	SDL_GetWindowSizeInPixels(window, &Width, &Height);
	{
		int virtualWidth;
		int virtualHeight;
		GetVirtualFramebufferSize(Width, Height, 0, &virtualWidth, &virtualHeight);
		SetWindowSize(Width, Height, virtualWidth, virtualHeight);
	}
	SetOpenGLRenderViewport(Width, Height, 0, ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height);

	pglEnable(GL_BLEND);	pglBlendFunc(GL_SRC_ALPHA, GL_ONE);
		
	pglEnable(GL_DEPTH_TEST);
	pglDepthFunc(GL_LEQUAL);
	pglDepthMask(GL_TRUE);
	pglDepthRange(0.0, 1.0);
		
	pglEnable(GL_TEXTURE_2D);

	pglDisable(GL_CULL_FACE);
	
	pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	Probe113_RenderTracef("VIDEO_MODE init_gl begin");
	InitOpenGL();
	Probe113_RenderTracef("VIDEO_MODE init_gl ok framebuffer=%dx%d", ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height);
	AppliedOpenGLAntiAliasingMode = OpenGLAntiAliasingMode;
 AppliedOpenGLColorBitDepthMode = OpenGLColorBitDepthMode;
 /* SDL delivers these expose events from its native Windows modal timer. */
 SDL_RemoveEventWatch(ContinueWindowModalFrame,NULL);
 SDL_AddEventWatch(ContinueWindowModalFrame,NULL);
	
	return 0;
}

/* Recreate the complete native OpenGL presentation after a mode that can alter
 * the HWND/HDC. Images are minimized while the old context is valid and are only
 * restored after the replacement context is current in its final SDL mode. */
static int RecreateOpenGLVideoContextForNativeModeChange(void)
{
 int wasSoftware=RenderingMode==RENDERING_MODE_SOFTWARE;
 int wasExpanded=FrontendCanvasExpanded;
 int setModeResult;

	if (window == NULL || context == NULL) return 0;
	if (!Probe124EnsureOpenGLContextCurrent("native-mode-recreate")) return 0;

	Probe113_RenderTracef("NATIVE_MODE_RECREATE minimize_images begin");
	MinimizeAllImages();
	Probe113_RenderTracef("NATIVE_MODE_RECREATE minimize_images end");

	if (FullscreenTexture != 0) pglDeleteTextures(1, &FullscreenTexture);
	FullscreenTexture = 0;
	FullscreenTextureWidth = 0;
	FullscreenTextureHeight = 0;

	if (BorderlessPresentationTexture != 0) pglDeleteTextures(1, &BorderlessPresentationTexture);
	BorderlessPresentationTexture = 0;
	BorderlessPresentationTextureWidth = 0;
	BorderlessPresentationTextureHeight = 0;
	BorderlessPresentationTextureColorBitDepth = -1;
	DesktopRefreshFrameDeadlineNS = 0;

	CRTFilter_Release();
    {extern void GeometryStreamRelease(void);GeometryStreamRelease();}FireBatchRelease();
    WaterRenderRelease();

	load_ogl_functions(0);
	Probe113_RenderTracef("NATIVE_MODE_RECREATE destroy_context context=%p", (void *)context);
	SDL_GL_DestroyContext(context);
	context = NULL;

#if defined(_WIN32)
	DisableWindowsKeyHook();
#endif

	Probe113_RenderTracef("NATIVE_MODE_RECREATE destroy_window window=%p", (void *)window);
	SDL_DestroyWindow(window);
	window = NULL;

	Probe113_RenderTracef("NATIVE_MODE_RECREATE create_requested begin");
	setModeResult = SetOGLVideoMode(0, 0);
	if (window == NULL || context == NULL)
	{
		Probe113_RenderTracef("NATIVE_MODE_RECREATE create_requested failed result=%d sdl=%s", setModeResult, SDL_GetError());
		return 0;
	}

	Probe113_RenderTracef("NATIVE_MODE_RECREATE restore_images begin");
	RestoreAllImages();
 Probe113_RenderTracef("NATIVE_MODE_RECREATE restore_images end result=%d", setModeResult);
 if(wasSoftware) { RenderingMode=RENDERING_MODE_SOFTWARE; ConfigureFrontendCanvas(wasExpanded); }
	return 1;
}

/* Recreate the OpenGL context when a context-only presentation request changes.
 * The texture manager retains image data while the old context is gone and
 * restores it once the replacement context is current. */
int RestartOpenGLVideoContextIfNeeded(void)
{
 int wasSoftware=RenderingMode==RENDERING_MODE_SOFTWARE;
 int wasExpanded=FrontendCanvasExpanded;
 int setModeResult;
		Probe113_RenderTracef("VIDEO_CONTEXT_RESTART enter effectiveAA=%d requestedAA=%d effectiveColour=%d requestedColour=%d window=%p context=%p fullscreenTex=%u borderlessTex=%u", AppliedOpenGLAntiAliasingMode, OpenGLAntiAliasingMode, AppliedOpenGLColorBitDepthMode, OpenGLColorBitDepthMode, (void *)window, (void *)context, (unsigned int)FullscreenTexture, (unsigned int)BorderlessPresentationTexture);
	if (AppliedOpenGLAntiAliasingMode == OpenGLAntiAliasingMode && AppliedOpenGLColorBitDepthMode == OpenGLColorBitDepthMode) {
		Probe113_RenderTracef("VIDEO_CONTEXT_RESTART skip reason=unchanged");
		return 0;
	}

	/* Startup has no presentation context yet; its normal creation path will
	 * consume the requested SDL multisample attributes. */
	if ((window == NULL) || (context == NULL)) {
		Probe113_RenderTracef("AA_RESTART skip reason=no_context");
		return 0;
	}

	Probe113_RenderTracef("AA_RESTART minimize_images begin");
	MinimizeAllImages();
	Probe113_RenderTracef("AA_RESTART minimize_images end");

	if (FullscreenTexture != 0) {
		Probe113_RenderTracef("AA_RESTART delete fullscreen texture=%u", (unsigned int)FullscreenTexture);
		pglDeleteTextures(1, &FullscreenTexture);
	}
	FullscreenTexture = 0;
	FullscreenTextureWidth = 0;
	FullscreenTextureHeight = 0;

	if (BorderlessPresentationTexture != 0) {
		Probe113_RenderTracef("AA_RESTART delete borderless texture=%u", (unsigned int)BorderlessPresentationTexture);
		pglDeleteTextures(1, &BorderlessPresentationTexture);
	}
	BorderlessPresentationTexture = 0;
	BorderlessPresentationTextureWidth = 0;
	BorderlessPresentationTextureHeight = 0;
	BorderlessPresentationTextureColorBitDepth = -1;
	DesktopRefreshFrameDeadlineNS = 0;
	BorderlessRenderTraceOpenGLError("AA_RESTART before_context_destroy");

	Probe113_RenderTracef("AA_RESTART unload_gl begin");
	CRTFilter_Release();
    {extern void GeometryStreamRelease(void);GeometryStreamRelease();}FireBatchRelease();
    WaterRenderRelease();

	load_ogl_functions(0);
	SDL_ClearError();
	Probe113_RenderTracef("AA_RESTART destroy_context begin context=%p", (void *)context);
	SDL_GL_DestroyContext(context);
	context = NULL;
	/* SDL_GL_DestroyContext is void: it has no success/failure result to inspect. */
	Probe113_RenderTracef("AA_RESTART destroy_context end");

#if defined(_WIN32)
	DisableWindowsKeyHook();
#endif

	SDL_ClearError();

	Probe113_RenderTracef("AA_RESTART destroy_window begin window=%p", (void *)window);

	SDL_DestroyWindow(window);

	window = NULL;

	/* SDL_DestroyWindow is void: its old error text must not be reported as failure. */

	Probe113_RenderTracef("AA_RESTART destroy_window end");

	Probe113_RenderTracef("AA_RESTART recreate begin");
	setModeResult = SetOGLVideoMode(0, 0);
	Probe113_RenderTracef("AA_RESTART recreate end result=%d window=%p context=%p effective=%d error=%s", setModeResult, (void *)window, (void *)context, AppliedOpenGLAntiAliasingMode, SDL_GetError());
	Probe113_RenderTracef("AA_RESTART restore_images begin");
	RestoreAllImages();
 Probe113_RenderTracef("AA_RESTART restore_images end");
 if(wasSoftware) { RenderingMode=RENDERING_MODE_SOFTWARE; ConfigureFrontendCanvas(wasExpanded); }

	return 1;
}
int InitialiseWindowsSystem(HANDLE hInstance, int nCmdShow, int WinInitMode)
{
	return 0;
}

int ExitWindowsSystem()
{
	if (joy != NULL) {
		SDL_CloseJoystick(joy);
	}

	if (FullscreenTexture != 0) {
		pglDeleteTextures(1, &FullscreenTexture);
	}
	FullscreenTexture = 0;
	if (BorderlessPresentationTexture != 0) {
		pglDeleteTextures(1, &BorderlessPresentationTexture);
	}
	BorderlessPresentationTexture = 0;
	BorderlessPresentationTextureWidth = 0;
	BorderlessPresentationTextureHeight = 0;
	BorderlessPresentationTextureColorBitDepth = -1;

	CRTFilter_Release();
    {extern void GeometryStreamRelease(void);GeometryStreamRelease();}FireBatchRelease();
    WaterRenderRelease();

	load_ogl_functions(0);
	
	if (surface != NULL) {
		SDL_DestroySurface(surface);
	}
	surface = NULL;

	if (context != NULL) {
		SDL_GL_DestroyContext(context);
	}
	context = NULL;

#if defined(_WIN32)
	DisableWindowsKeyHook();
#endif

	if (window != NULL) {
		SDL_DestroyWindow(window);
	}
	window = NULL;

	return 0;
}

static int GotPrintScn, HavePrintScn;
int ScreenshotCapturePending(void){return GotPrintScn!=0;}

void RequestScreenShot(void)
{
	GotPrintScn=1;
}
/* Capture the completed physical frame, with menus and post-processing included. */
static void CapturePendingScreenshot(void)
{
    if(!GotPrintScn)return;
    GotPrintScn=0;
    CapturePresentedFrame=1;
    ScreenShot();
    CapturePresentedFrame=0;
}


#if defined(_WIN32)
/* SDL reports the key to the game, but Windows consumes the Super keys first.
 * A short-lived low-level hook keeps WinL/WinR bindable only while this window
 * has focus, and consumes the OS shell action. */
static HHOOK WindowsKeyHook;

static LRESULT CALLBACK WindowsLowLevelKeyHook(int code, WPARAM wParam, LPARAM lParam)
{
	const KBDLLHOOKSTRUCT *key;
	int press;
	int inputKey;

	if (code != HC_ACTION) {
		return CallNextHookEx(NULL, code, wParam, lParam);
	}

	switch (wParam) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			press = 1;
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			press = 0;
			break;
		default:
			return CallNextHookEx(NULL, code, wParam, lParam);
	}

	key = (const KBDLLHOOKSTRUCT *)lParam;
	switch (key->vkCode) {
		case VK_LWIN:
			inputKey = KEY_LWIN;
			break;
		case VK_RWIN:
			inputKey = KEY_RWIN;
			break;
		default:
			return CallNextHookEx(NULL, code, wParam, lParam);
	}

	handle_keypress(inputKey, 0, press);
	return 1;
}

static void EnableWindowsKeyHook(void)
{
	if (WindowsKeyHook == NULL) {
		WindowsKeyHook = SetWindowsHookExW(WH_KEYBOARD_LL, WindowsLowLevelKeyHook,
			GetModuleHandleW(NULL), 0);
	}
}

static void DisableWindowsKeyHook(void)
{
	if (WindowsKeyHook != NULL) {
		UnhookWindowsHookEx(WindowsKeyHook);
		WindowsKeyHook = NULL;
	}
}
#endif

/* Bindings use layout-independent physical SDL scancodes; text input remains SDL text events. */
static int ScancodeToKey(SDL_Scancode scancode)
{
	switch(scancode) {
		case SDL_SCANCODE_ESCAPE:
			return KEY_ESCAPE;
			
		case SDL_SCANCODE_0:
			return KEY_0;
		case SDL_SCANCODE_1:
			return KEY_1;
		case SDL_SCANCODE_2:
			return KEY_2;
		case SDL_SCANCODE_3:
			return KEY_3;
		case SDL_SCANCODE_4:
			return KEY_4;
		case SDL_SCANCODE_5:
			return KEY_5;
		case SDL_SCANCODE_6:
			return KEY_6;
		case SDL_SCANCODE_7:
			return KEY_7;
		case SDL_SCANCODE_8:
			return KEY_8;
		case SDL_SCANCODE_9:
			return KEY_9;
		
		case SDL_SCANCODE_A:
			return KEY_A;
		case SDL_SCANCODE_B:
			return KEY_B;
		case SDL_SCANCODE_C:
			return KEY_C;
		case SDL_SCANCODE_D:
			return KEY_D;
		case SDL_SCANCODE_E:
			return KEY_E;
		case SDL_SCANCODE_F:
			return KEY_F;
		case SDL_SCANCODE_G:
			return KEY_G;
		case SDL_SCANCODE_H:
			return KEY_H;
		case SDL_SCANCODE_I:
			return KEY_I;
		case SDL_SCANCODE_J:
			return KEY_J;
		case SDL_SCANCODE_K:
			return KEY_K;
		case SDL_SCANCODE_L:
			return KEY_L;
		case SDL_SCANCODE_M:
			return KEY_M;
		case SDL_SCANCODE_N:
			return KEY_N;
		case SDL_SCANCODE_O:
			return KEY_O;
		case SDL_SCANCODE_P:
			return KEY_P;
		case SDL_SCANCODE_Q:
			return KEY_Q;
		case SDL_SCANCODE_R:
			return KEY_R;
		case SDL_SCANCODE_S:
			return KEY_S;
		case SDL_SCANCODE_T:
			return KEY_T;
		case SDL_SCANCODE_U:
			return KEY_U;
		case SDL_SCANCODE_V:
			return KEY_V;
		case SDL_SCANCODE_W:
			return KEY_W;
		case SDL_SCANCODE_X:
			return KEY_X;
		case SDL_SCANCODE_Y:
			return KEY_Y;
		case SDL_SCANCODE_Z:
			return KEY_Z;
				
		case SDL_SCANCODE_LEFT:
			return KEY_LEFT;
		case SDL_SCANCODE_RIGHT:
			return KEY_RIGHT;
		case SDL_SCANCODE_UP:
			return KEY_UP;
		case SDL_SCANCODE_DOWN:
			return KEY_DOWN;		
		case SDL_SCANCODE_RETURN:
			return KEY_CR;
		case SDL_SCANCODE_TAB:
			return KEY_TAB;
		case SDL_SCANCODE_INSERT:
			return KEY_INS;
		case SDL_SCANCODE_DELETE:
			return KEY_DEL;
		case SDL_SCANCODE_END:
			return KEY_END;
		case SDL_SCANCODE_HOME:
			return KEY_HOME;
		case SDL_SCANCODE_PAGEUP:
			return KEY_PAGEUP;
		case SDL_SCANCODE_PAGEDOWN:
			return KEY_PAGEDOWN;
		case SDL_SCANCODE_BACKSPACE:
			return KEY_BACKSPACE;
		case SDL_SCANCODE_COMMA:
			return KEY_COMMA;
		case SDL_SCANCODE_PERIOD:
			return KEY_FSTOP;
		case SDL_SCANCODE_SPACE:
			return KEY_SPACE;
			
		case SDL_SCANCODE_LSHIFT:
			return KEY_LEFTSHIFT;
		case SDL_SCANCODE_RSHIFT:
			return KEY_RIGHTSHIFT;
		case SDL_SCANCODE_LALT:
			return KEY_LEFTALT;
		case SDL_SCANCODE_RALT:
			return KEY_RIGHTALT;
		case SDL_SCANCODE_LCTRL:
			return KEY_LEFTCTRL;
		case SDL_SCANCODE_RCTRL:
			return KEY_RIGHTCTRL;

		case SDL_SCANCODE_CAPSLOCK:
			return KEY_CAPS;
		case SDL_SCANCODE_NUMLOCKCLEAR:
			return KEY_NUMLOCK;
		case SDL_SCANCODE_SCROLLLOCK:
			return KEY_SCROLLOK;
			
		case SDL_SCANCODE_KP_0:
			return KEY_NUMPAD0;
		case SDL_SCANCODE_KP_1:
			return KEY_NUMPAD1;
		case SDL_SCANCODE_KP_2:
			return KEY_NUMPAD2;
		case SDL_SCANCODE_KP_3:
			return KEY_NUMPAD3;
		case SDL_SCANCODE_KP_4:
			return KEY_NUMPAD4;
		case SDL_SCANCODE_KP_5:
			return KEY_NUMPAD5;
		case SDL_SCANCODE_KP_6:
			return KEY_NUMPAD6;
		case SDL_SCANCODE_KP_7:
			return KEY_NUMPAD7;
		case SDL_SCANCODE_KP_8:
			return KEY_NUMPAD8;
		case SDL_SCANCODE_KP_9:
			return KEY_NUMPAD9;
		case SDL_SCANCODE_KP_MINUS:
			return KEY_NUMPADSUB;
		case SDL_SCANCODE_KP_PLUS:
			return KEY_NUMPADADD;
		case SDL_SCANCODE_KP_PERIOD:
			return KEY_NUMPADDEL;
		case SDL_SCANCODE_KP_ENTER:
			return KEY_NUMPADENTER;
		case SDL_SCANCODE_KP_DIVIDE:
			return KEY_NUMPADDIVIDE;
		case SDL_SCANCODE_KP_MULTIPLY:
			return KEY_NUMPADMULTIPLY;
	
		case SDL_SCANCODE_LEFTBRACKET:
			return KEY_LBRACKET;
		case SDL_SCANCODE_RIGHTBRACKET:
			return KEY_RBRACKET;
		case SDL_SCANCODE_SEMICOLON:
			return KEY_SEMICOLON;
		case SDL_SCANCODE_APOSTROPHE:
			return KEY_APOSTROPHE;
		case SDL_SCANCODE_GRAVE:
			return KEY_GRAVE;
		case SDL_SCANCODE_BACKSLASH:
			return KEY_BACKSLASH;
		case SDL_SCANCODE_SLASH:
			return KEY_SLASH;
/*		case SDL_SCANCODE_
			return KEY_CAPITAL; */
		case SDL_SCANCODE_MINUS:
			return KEY_MINUS;
		case SDL_SCANCODE_EQUALS:
			return KEY_EQUALS;
		case SDL_SCANCODE_LGUI:
			return KEY_LWIN;
		case SDL_SCANCODE_RGUI:
			return KEY_RWIN;
		case SDL_SCANCODE_APPLICATION:
			return KEY_APPS;

		/* ISO physical keys have no reliable layout-independent keycode. */
		case SDL_SCANCODE_NONUSBACKSLASH:
			return KEY_LESSTHAN;
		case SDL_SCANCODE_NONUSHASH:
			return KEY_HASH;
		
		case SDL_SCANCODE_F1:
			return KEY_F1;
		case SDL_SCANCODE_F2:
			return KEY_F2;
		case SDL_SCANCODE_F3:
			return KEY_F3;
		case SDL_SCANCODE_F4:
			return KEY_F4;
		case SDL_SCANCODE_F5:
			return KEY_F5;
		case SDL_SCANCODE_F6:
			return KEY_F6;
		case SDL_SCANCODE_F7:
			return KEY_F7;
		case SDL_SCANCODE_F8:
			return KEY_F8;
		case SDL_SCANCODE_F9:
			return KEY_F9;
		case SDL_SCANCODE_F10:
			return KEY_F10;
		case SDL_SCANCODE_F11:
			return KEY_F11;
		case SDL_SCANCODE_F12:
			return KEY_F12;

/* finish foreign keys */

		default:
			return -1;
	}
}

char ShiftDown = 0;
char CapsLockOn = 0;
const char ShiftAddition[2] = { 32, 0 };

static void handle_keypress(int key, int unicode, int press)
{	
	if (key == -1)
		return;

	// Nasty hack to allow temporary character entry by TCH68k on Github
	
	if ((key == KEY_LEFTSHIFT) || (key == KEY_RIGHTSHIFT))
	{
		ShiftDown = press;
	}
	else if (press) {
		if ((key >= KEY_A) && (key <= KEY_Z))
		{
			RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(65 + (key - KEY_A) + ShiftAddition[ShiftDown ^ CapsLockOn]);
		}
		else if ((key >= KEY_0) && (key <= KEY_9))
		{
			RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(48 + (key - KEY_0)); /* TODO: Shift numbers -> symbols */
		}
		else if ((key >= KEY_NUMPAD0) && (key <= KEY_NUMPAD9))
		{
			RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(48 + (key - KEY_NUMPAD0));
		}
		else if (false) /* TODO: other symbols */
		{
			// BOO!
		}
	else switch (key) {
		case KEY_CAPS:
			CapsLockOn ^= 1;
			break;
			case KEY_CR:
				SDL_StartTextInput(NULL);
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR('\r');
				break;
			case KEY_SPACE:
                RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(' ');
                break;
			case KEY_BACKSPACE:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_BACK);
				break;
			case KEY_END:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_END);
				break;
			case KEY_HOME:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_HOME);
				break;
			case KEY_LEFT:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_LEFT);
				break;
			case KEY_UP:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_UP);
				break;
			case KEY_RIGHT:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_RIGHT);
				break;
			case KEY_DOWN:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_DOWN);
				break;
			case KEY_INS:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_INSERT);
				break;
			case KEY_DEL:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_DELETE);
				break;
			case KEY_TAB:
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(VK_TAB);
				break;
			default:
				SDL_StartTextInput(NULL);
				break;
		}
	}
	
	if (press && !KeyboardInput[key]) {
		DebouncedKeyboardInput[key] = 1;
		DebouncedGotAnyKey = 1;
	}
	
	if (press)
		GotAnyKey = 1;
	KeyboardInput[key] = press;
}

void CheckForWindowsMessages()
{
	SDL_Event event;
	float x, y, wantmouse;
	int buttons;
    int screenModeShortcut=0;
	
	if(window)EventLatencyWindowState((SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0,(SDL_GetWindowFlags(window)&SDL_WINDOW_MINIMIZED)!=0);
	GotAnyKey = 0;
	DebouncedGotAnyKey = 0;
	secure_avpzero(DebouncedKeyboardInput, sizeof DebouncedKeyboardInput);
	
	wantmouse =	(SDL_GetWindowRelativeMouseMode(window) == true);

	// "keyboard" events that don't have an up event
	KeyboardInput[KEY_MOUSEWHEELUP] = 0;
	KeyboardInput[KEY_MOUSEWHEELDOWN] = 0;
	
	WindowEventsPumping=1;
	while (SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				if (wantmouse) {
					if (event.wheel.y < 0) {
						handle_keypress(KEY_MOUSEWHEELDOWN, 0, 1);
					} else if (event.wheel.y > 0) {
						handle_keypress(KEY_MOUSEWHEELUP, 0, 1);
					}
				}
				break;
			case SDL_EVENT_TEXT_INPUT: {
				SDL_StartTextInput(NULL);
					int unicode = event.text.text[0]; //TODO convert to utf-32
					if (unicode && !(unicode & 0xFF80)) {
						/* Space is delivered by keydown to the console, never twice.
                         * The separate menu editor still consumes the SDL text event. */
                        if(unicode!=' ')RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(unicode);
						KeyboardEntryQueue_Add(unicode);
					}
				}
				break;
			case SDL_EVENT_KEY_DOWN:
                if(event.key.scancode==SDL_SCANCODE_RETURN && (event.key.mod&SDL_KMOD_ALT)) {
                    if(!event.key.repeat)screenModeShortcut=1;
                    break;
                }
				SDL_StartTextInput(NULL);
				if (event.key.scancode == SDL_SCANCODE_PRINTSCREEN) {
					if (HavePrintScn == 0)
						GotPrintScn = 1;
					HavePrintScn = 1;
				} else {
					handle_keypress(ScancodeToKey(event.key.scancode), 0, 1);
				}
				break;
			case SDL_EVENT_KEY_UP:
				SDL_StartTextInput(NULL);
				if (event.key.scancode == SDL_SCANCODE_PRINTSCREEN) {
					HavePrintScn = 0;
				} else {
					handle_keypress(ScancodeToKey(event.key.scancode), 0, 0);
				}
				break;
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
 EventLatencyWindowState(1,(SDL_GetWindowFlags(window)&SDL_WINDOW_MINIMIZED)!=0);
 EventLatencyNote("focus gained",0,0,0);
#if defined(_WIN32)
				EnableWindowsKeyHook();
#endif
				VideoPresentationDirty = 1;
				break;
			case SDL_EVENT_WINDOW_FOCUS_LOST:
 EventLatencyWindowState(0,(SDL_GetWindowFlags(window)&SDL_WINDOW_MINIMIZED)!=0);
 EventLatencyNote("focus lost",0,0,0);
#if defined(_WIN32)
				DisableWindowsKeyHook();
#endif
				IngameKeyboardInput_ClearBuffer();
				break;
			case SDL_EVENT_WINDOW_MINIMIZED:
			case SDL_EVENT_WINDOW_RESTORED:
 EventLatencyWindowState((SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)!=0,(SDL_GetWindowFlags(window)&SDL_WINDOW_MINIMIZED)!=0);
 EventLatencyNote(event.type==SDL_EVENT_WINDOW_MINIMIZED?"window minimized":"window restored",0,0,0);
 break;
			case SDL_EVENT_WINDOW_RESIZED:
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				/* Event payloads may describe an intermediate fullscreen transition.
				 * Consume the settled drawable only from the frame synchronisation. */
				VideoPresentationDirty = 1;
				break;
			case SDL_EVENT_QUIT:
 EventLatencyNote("quit event",0,0,0);
				AvP_FlushEpisodeSelection();
				AvP.MainLoopRunning = 0; /* TODO */
				exit(0); //TODO
				SDL_StopTextInput(NULL);
				break;
		}
	}
	
	WindowEventsPumping=0;
	/* Synchronise from the settled SDL drawable, not resize event payloads. */
	if (window != NULL && context != NULL) {
		int w = 0, h = 0, vw = 0, vh = 0;
		const int softwareMenu = AvP_IsSoftwareMenuActive();
		SDL_GetWindowSizeInPixels(window, &w, &h);
        if (w > 0 && h > 0) {
            if(softwareMenu && (ViewportWidth!=w || ViewportHeight!=h)) {
                ViewportWidth=w;ViewportHeight=h;
                ConfigureFrontendCanvas(FrontendCanvasExpanded);
            }
            GetVirtualFramebufferSize(w, h, softwareMenu, &vw, &vh);
			if (VideoPresentationDirty || ViewportWidth != w || ViewportHeight != h || ScreenDescriptorBlock.SDB_Width != vw || ScreenDescriptorBlock.SDB_Height != vh) {
				WindowWidth = w; WindowHeight = h;
				{ EVENT_TICK stamp=EventLatencyBegin();
 EventLatencyNote("drawable/layout refresh",w,h,0);
 SetWindowSize(w,h,vw,vh);
 EventLatencyEnd(stamp,"drawable/layout refresh time",vw,vh,0,0,2); }
				VideoPresentationDirty = 0;
			}
			/* Render passes may modify GL state; restore the selected source
			 * framebuffer (or the menu presentation viewport) every frame. */
#if defined(_WIN32)
			if (Probe124EnsureOpenGLContextCurrent("frame-viewport"))
#endif
			SetOpenGLRenderViewport(w, h, softwareMenu, vw, vh);
		}
	}
buttons = SDL_GetRelativeMouseState(&x, &y);
	
	if (wantmouse) {
		if (buttons & SDL_BUTTON_MASK(1))
			handle_keypress(KEY_LMOUSE, 0, 1);
		else
			handle_keypress(KEY_LMOUSE, 0, 0);
		if (buttons & SDL_BUTTON_MASK(2))
			handle_keypress(KEY_MMOUSE, 0, 1);
		else
			handle_keypress(KEY_MMOUSE, 0, 0);
		if (buttons & SDL_BUTTON_MASK(3))
			handle_keypress(KEY_RMOUSE, 0, 1);
		else
			handle_keypress(KEY_RMOUSE, 0, 0);
		if (buttons & SDL_BUTTON_X1MASK)
			handle_keypress(KEY_MOUSEBUTTON4, 0, 1);
		else
			handle_keypress(KEY_MOUSEBUTTON4, 0, 0);
		if (buttons & SDL_BUTTON_X2MASK)
			handle_keypress(KEY_MOUSEBUTTON5, 0, 1);
		else
			handle_keypress(KEY_MOUSEBUTTON5, 0, 0);

		float acceleration = ApplyMouseAcceleration(x, y);
		MouseRawDeltaX = x;
		MouseRawDeltaY = y;
		MouseVelX = MouseDeltaToVelocity(x * acceleration, &MouseVelocityResidualX);
		MouseVelY = MouseDeltaToVelocity(y * acceleration, &MouseVelocityResidualY);
	} else {
		KeyboardInput[KEY_LMOUSE] = 0;
		KeyboardInput[KEY_MMOUSE] = 0;
		KeyboardInput[KEY_RMOUSE] = 0;
		KeyboardInput[KEY_MOUSEBUTTON4] = 0;
		KeyboardInput[KEY_MOUSEBUTTON5] = 0;
		MouseRawDeltaX = 0.0f;
		MouseRawDeltaY = 0.0f;
		MouseVelX = 0;
		MouseVelY = 0;
	}

	if (GotJoystick) {
		float numbuttons;
		int x;
		
		SDL_UpdateJoysticks();
		
		numbuttons = SDL_GetNumJoystickButtons(joy);
		if (numbuttons > 16) numbuttons = 16;
		
		for (x = 0; x < numbuttons; x++) {
			if (SDL_GetJoystickButton(joy, x)) {
				GotAnyKey = 1;
				if (!KeyboardInput[KEY_JOYSTICK_BUTTON_1+x]) {
					KeyboardInput[KEY_JOYSTICK_BUTTON_1+x] = 1;
					DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_1+x] = 1;
				}
			} else {
				KeyboardInput[KEY_JOYSTICK_BUTTON_1+x] = 0;
			}	
		}
	}

 /* ALT+ENTER is an edge-triggered mode transaction, exactly like the menu. */
 if (screenModeShortcut) {
     const int previous=AppliedVideoScreenMode;
     KeyboardInput[KEY_CR]=DebouncedKeyboardInput[KEY_CR]=0;
     DebouncedGotAnyKey=0;
     CurrentVideoScreenMode=previous==VIDEO_SCREEN_WINDOWED?VIDEO_SCREEN_BORDERLESS:
         previous==VIDEO_SCREEN_BORDERLESS?VIDEO_SCREEN_EXCLUSIVE:VIDEO_SCREEN_WINDOWED;
     if(ApplySelectedVideoMode() && AppliedVideoScreenMode==CurrentVideoScreenMode) {
         SaveDeviceAndVideoModePreferences();
         ScreenModeNotice=CurrentVideoScreenMode==VIDEO_SCREEN_BORDERLESS?"MODO BORDELESS":
             CurrentVideoScreenMode==VIDEO_SCREEN_EXCLUSIVE?"MODO FULLSCREEN EXCLUSIVE":"MODO WINDOWED";
         ScreenModeNoticeUntil=SDL_GetTicks()+4000;
         if(RenderingMode!=RENDERING_MODE_SOFTWARE)NewScreenModeMessage((unsigned char *)ScreenModeNotice);
     } else { CurrentVideoScreenMode=previous; }
 }

	if (KeyboardInput[KEY_LEFTCTRL] && DebouncedKeyboardInput[KEY_G]) {
		int IsWindowed = (SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN|SDL_WINDOW_FULLSCREEN)) == 0;

		if (IsWindowed) {
			WantMouseGrab = WantMouseGrab != 0 ? 0 : 1;
			if (WantMouseGrab != 0) {
				SDL_SetWindowRelativeMouseMode(window, true);
			} else {
				SDL_SetWindowRelativeMouseMode(window, false);
			}
			WantMouseGrab = (SDL_GetWindowRelativeMouseMode(window) == true);
		}
	}

	// a second reset of relative mouse state because
	// enabling relative mouse mode moves the mouse
	SDL_SetWindowRelativeMouseMode(window, AppliedVideoScreenMode!=VIDEO_SCREEN_WINDOWED || WantMouseGrab);
        SDL_GetRelativeMouseState(NULL, NULL);

	CheckScreenshotInput();
}
        
static void WaitForPrecisePresentationDeadline(Uint64 deadline);

static void LimitDesktopRefreshRate(void)
{
	Uint64 intervalNS;
	Uint64 now;

	/* Refresh pacing belongs to VSync. Off + uncapped must never sleep here.
	 * A normal window keeps its explicit FPS cap authoritative; only uncapped
	 * VSync uses the selected refresh. Exclusive uses its real display mode. */
	if (AppliedVideoScreenMode == VIDEO_SCREEN_EXCLUSIVE || !AppliedVideoVSync
		|| AppliedVideoRefreshMilliHz <= 0
		|| (AppliedVideoScreenMode == VIDEO_SCREEN_WINDOWED
			&& AppliedVideoFPSCapMode != VIDEO_FPS_CAP_UNCAPPED))
	{
		DesktopRefreshFrameDeadlineNS = 0;
		return;
	}

	/* Windowed and borderless retain the desktop display mode. Honour the
	 * selected rate with presentation pacing while keeping real VSync on. */
	intervalNS = (1000000000000ULL + (Uint64)AppliedVideoRefreshMilliHz / 2ULL)
		/ (Uint64)AppliedVideoRefreshMilliHz;
	now = SDL_GetTicksNS();
	if (DesktopRefreshFrameDeadlineNS == 0 || now > DesktopRefreshFrameDeadlineNS + intervalNS * 4ULL)
	{
		DesktopRefreshFrameDeadlineNS = now + intervalNS;
		return;
	}
	if (now < DesktopRefreshFrameDeadlineNS) WaitForPrecisePresentationDeadline(DesktopRefreshFrameDeadlineNS);
	now = SDL_GetTicksNS();
	do
	{
		DesktopRefreshFrameDeadlineNS += intervalNS;
	} while (DesktopRefreshFrameDeadlineNS <= now);
}

static void WaitForPrecisePresentationDeadline(Uint64 deadline)
{
    /* Sleep for the coarse portion, then spin only for the final 0.20 ms.
     * This avoids scheduler oversleep from turning a stable presentation
     * deadline into the alternating long/short intervals visible at 100 FPS. */
    for (;;)
    {
        Uint64 now = SDL_GetTicksNS();
        Uint64 remaining;
        if (now >= deadline) return;
        remaining = deadline - now;
        if (remaining > 2000000ULL)
            SDL_DelayNS(remaining - 1000000ULL);
        else if (remaining > 300000ULL)
            SDL_DelayNS(remaining - 200000ULL);
    }
}

static void LimitPresentationFrameRate(void)
{
    int fpsCap;
    Uint64 intervalNS;
    Uint64 now;

    fpsCap = VideoFPSCapValue(AppliedVideoFPSCapMode, AppliedVideoFPSCapCustom);
    if (fpsCap <= 0)
    {
        FPSCapFrameDeadlineNS = 0;
        return;
    }

    if (fpsCap < VIDEO_FPS_CAP_CUSTOM_MINIMUM) fpsCap = VIDEO_FPS_CAP_CUSTOM_MINIMUM;
    if (fpsCap > VIDEO_FPS_CAP_CUSTOM_MAXIMUM) fpsCap = VIDEO_FPS_CAP_CUSTOM_MAXIMUM;

    intervalNS = (1000000000ULL + (Uint64)fpsCap / 2ULL) / (Uint64)fpsCap;
    now = SDL_GetTicksNS();
    if (FPSCapFrameDeadlineNS == 0 || now > FPSCapFrameDeadlineNS + intervalNS * 4ULL)
    {
        FPSCapFrameDeadlineNS = now + intervalNS;
        return;
    }

    /* Do not catch up after a frame that was late because gameplay, streaming
     * or a trigger consumed its budget. Catch-up makes its successor too
     * short (the 9/11 ms sawtooth seen by the frametime graph). RTSS-style
     * pacing instead begins a fresh interval after the completed frame. */
    if (now < FPSCapFrameDeadlineNS)
    {
        WaitForPrecisePresentationDeadline(FPSCapFrameDeadlineNS);
        now = SDL_GetTicksNS();
    }
    FPSCapFrameDeadlineNS = now + intervalNS;
}
/* The legacy renderer owns a substantial fixed-function state machine.  The
 * borderless presenter is therefore transactional: it snapshots server and
 * client state, composes the virtual framebuffer, then restores that state
 * byte-for-byte instead of reinitialising the renderer between 3D frames. */
static void PushBorderlessPresentationState(void)
{
	pglPushAttrib(GL_ALL_ATTRIB_BITS);
	pglPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
	pglMatrixMode(GL_PROJECTION);
	pglPushMatrix();
	pglLoadIdentity();
	pglMatrixMode(GL_MODELVIEW);
	pglPushMatrix();
	pglLoadIdentity();
}

static void PopBorderlessPresentationState(void)
{
	pglMatrixMode(GL_MODELVIEW);
	pglPopMatrix();
	pglMatrixMode(GL_PROJECTION);
	pglPopMatrix();
	pglPopClientAttrib();
	pglPopAttrib();
}
static int PresentBorderlessRenderTarget(void)
{
	int physicalWidth;
	int physicalHeight;
	GLfloat *v = BorderlessPresentationVertices;

	/* Some drivers expose an 8:8:8 backbuffer even when exclusive mode requested
	 * 5:6:5. Route exclusive 16-bit frames through the same RGB565 texture used
	 * by borderless presentation, so the selected scene color depth is explicit. */
	if (!((AppliedVideoScreenMode == VIDEO_SCREEN_BORDERLESS)
		|| (AppliedVideoScreenMode == VIDEO_SCREEN_EXCLUSIVE
			&& AppliedOpenGLColorBitDepthMode == OPENGL_COLOR_BIT_DEPTH_16))
		|| BorderlessPresentationTexture == 0
		|| BorderlessPresentationTextureWidth != ScreenDescriptorBlock.SDB_Width
		|| BorderlessPresentationTextureHeight != ScreenDescriptorBlock.SDB_Height) {
		if (BorderlessRenderTraceFrame < 600)
			Probe113_RenderTracef("PRESENT_SKIP screen=%d texture=%u texture=%dx%d sdb=%dx%d", AppliedVideoScreenMode, (unsigned int)BorderlessPresentationTexture, BorderlessPresentationTextureWidth, BorderlessPresentationTextureHeight, ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height);
		return 0;
	}

	SDL_GetWindowSizeInPixels(window, &physicalWidth, &physicalHeight);
	if (physicalWidth < 1 || physicalHeight < 1) return 0;

 /* Native 32-bit rendering already occupies the exact drawable consumed by
  * CRT. Skip the identity presentation pass: its draw plus a second MSAA
  * resolve only adds bandwidth and rounding. CRTFilter_Apply will resolve
  * the original scene once. Retain legacy presentation for scaled modes
  * and RGB565, whose intermediate conversion is part of their appearance. */
 if ((CRTFilterEnabled || ImageAdjustmentsActive())
  && BorderlessPresentationTextureColorBitDepth == OPENGL_COLOR_BIT_DEPTH_32
  && physicalWidth == BorderlessPresentationTextureWidth
  && physicalHeight == BorderlessPresentationTextureHeight) {
  SetOpenGLPresentationViewport(physicalWidth,physicalHeight,0);
  return 0;
 }
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-enter");

	PushBorderlessPresentationState();
	pglBindTexture(GL_TEXTURE_2D, BorderlessPresentationTexture);
	CopyResolvedFramebufferToTexture(BorderlessPresentationTexture,
		BorderlessPresentationTextureWidth, BorderlessPresentationTextureHeight);
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-copy");
	/* A previous scene may leave scissoring enabled. Clear the whole desktop
	 * drawable before the presentation quad so old menu pixels cannot survive
	 * at a second position. */
	pglDisable(GL_SCISSOR_TEST);
	pglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-clear");
	SetOpenGLPresentationViewport(physicalWidth, physicalHeight, 0);
	pglDisable(GL_ALPHA_TEST);
	pglDisable(GL_BLEND);
	pglDisable(GL_DEPTH_TEST);
	pglDisable(GL_CULL_FACE);
	pglEnable(GL_TEXTURE_2D);
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, BorderlessPresentationFilter());
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, BorderlessPresentationFilter());
	pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-state-setup");

	v[0] = -1.0f; v[1] = -1.0f; v[2] = 0.0f; v[3] = 0.0f;
	v[4] = 1.0f; v[5] = -1.0f; v[6] = 1.0f; v[7] = 0.0f;
	v[8] = 1.0f; v[9] = 1.0f; v[10] = 1.0f; v[11] = 1.0f;
	v[12] = -1.0f; v[13] = 1.0f; v[14] = 0.0f; v[15] = 1.0f;
	pglEnableClientState(GL_VERTEX_ARRAY);
	pglVertexPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, &v[0]);
	pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	pglTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, &v[2]);
	pglDisableClientState(GL_COLOR_ARRAY);
	pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	pglDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, PresentationIndices);
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-draw");
	pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	pglDisableClientState(GL_VERTEX_ARRAY);
	PopBorderlessPresentationState();
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-state-restore");
	return 1;
}
void InGameFlipBuffers()
{
#if defined(_WIN32)
	if (!Probe124EnsureOpenGLContextCurrent("gameplay-present")) return;
#endif
#if !defined(NDEBUG)
	check_for_errors();
#endif

	if (BorderlessRenderTraceFrame < 8)
		Probe113_RenderTracef("FRAME begin index=%u screen=%d sdb=%dx%d", BorderlessRenderTraceFrame, AppliedVideoScreenMode, ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height);
	if (BorderlessRenderTraceFrame < 8) Probe113_RenderTracef("FRAME present-enter index=%u", BorderlessRenderTraceFrame);
	if (BorderlessRenderTraceFrame < 600) BorderlessRenderTraceOpenGLError("frame-pre-present");
	PresentBorderlessRenderTarget();
	if (CRTFilterEnabled || ImageAdjustmentsActive()) {
		int crtWidth = 0, crtHeight = 0;
		SDL_GetWindowSizeInPixels(window, &crtWidth, &crtHeight);
		CRTFilter_Apply(crtWidth, crtHeight);
	}
	if (BorderlessRenderTraceFrame < 8) Probe113_RenderTracef("FRAME present-exit index=%u", BorderlessRenderTraceFrame);
	LimitDesktopRefreshRate();
	LimitPresentationFrameRate();
	if (BorderlessRenderTraceFrame < 8) Probe113_RenderTracef("FRAME swap-enter index=%u", BorderlessRenderTraceFrame);
	CapturePendingScreenshot();
	SDL_GL_SwapWindow(window);
	if (BorderlessRenderTraceFrame < 8) {
		BorderlessRenderTraceOpenGLError("swap-window");
		Probe113_RenderTracef("FRAME swap-exit index=%u", BorderlessRenderTraceFrame);
	}
	BorderlessRenderTraceFrame++;
}

/* The software frontend has no mip chain; honour the general texture choice. */
static GLint FrontendPresentationFilter(void)
{
 return OpenGLTextureFilterMode==0?GL_NEAREST:GL_LINEAR;
}

/* The 99 title is blended AFTER presenting the RGB565 menu canvas. This
 * avoids quantizing its gain to 5/6 bits before scaling and presentation.
 * Only this intro uses the deferred overlay; ordinary menu rendering stays. */
static GLuint Intro99LogoTexture;
static const unsigned char *Intro99CachedRGBA;
static int Intro99TextureWidth,Intro99TextureHeight;
static struct {
 const unsigned char *rgba;int iw,ih,x,y,width,height;double gain;
} Intro99Pending;
void QueueIntro99Logo(const unsigned char *rgba,int iw,int ih,int x,int y,int width,int height,double gain)
{
 Intro99Pending.rgba=rgba;Intro99Pending.iw=iw;Intro99Pending.ih=ih;
 Intro99Pending.x=x;Intro99Pending.y=y;Intro99Pending.width=width;Intro99Pending.height=height;
 Intro99Pending.gain=gain<0?0:(gain>INTRO99_LOGO_PEAK_GAIN?INTRO99_LOGO_PEAK_GAIN:gain);
}
void ReleaseIntro99Logo(void)
{
 Intro99Pending.rgba=NULL;Intro99CachedRGBA=NULL;
 if(Intro99LogoTexture)pglDeleteTextures(1,&Intro99LogoTexture);
 Intro99LogoTexture=0;Intro99TextureWidth=Intro99TextureHeight=0;
}
static void PresentIntro99Logo(GLfloat x0,GLfloat x1,GLfloat y0,GLfloat y1)
{
 const unsigned char *rgba=Intro99Pending.rgba;
 Intro99Pending.rgba=NULL; /* Exactly one presentation; no stale menu overlay. */
 int iw=Intro99Pending.iw,ih=Intro99Pending.ih;
 double gain=Intro99Pending.gain;
 if(!rgba || gain<=0 || iw<1 || ih<1 || !surface)return;
 if(!Intro99LogoTexture || Intro99CachedRGBA!=rgba){
  int tw=1,th=1;while(tw<iw)tw*=2;while(th<ih)th*=2;
  unsigned char *padded=(unsigned char*)calloc((size_t)tw*th,4);
  if(!padded)return;
  for(int y=0;y<ih;y++)memcpy(padded+(size_t)y*tw*4,rgba+(size_t)y*iw*4,(size_t)iw*4);
  if(Intro99LogoTexture)pglDeleteTextures(1,&Intro99LogoTexture);
  pglGenTextures(1,&Intro99LogoTexture);pglBindTexture(GL_TEXTURE_2D,Intro99LogoTexture);
  pglTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,tw,th,0,GL_RGBA,GL_UNSIGNED_BYTE,padded);
  free(padded);Intro99CachedRGBA=rgba;Intro99TextureWidth=tw;Intro99TextureHeight=th;
  pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
 }else pglBindTexture(GL_TEXTURE_2D,Intro99LogoTexture);
 pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,FrontendPresentationFilter());
 pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,FrontendPresentationFilter());
 GLfloat left=x0+(x1-x0)*Intro99Pending.x/surface->w;
 GLfloat right=x0+(x1-x0)*(Intro99Pending.x+Intro99Pending.width)/surface->w;
 GLfloat top=y1-(y1-y0)*Intro99Pending.y/surface->h;
 GLfloat bottom=y1-(y1-y0)*(Intro99Pending.y+Intro99Pending.height)/surface->h;
 GLfloat u=(GLfloat)Intro99Pending.width/Intro99TextureWidth,v=(GLfloat)Intro99Pending.height/Intro99TextureHeight;
 GLfloat quad[]={left,bottom,0,v,right,bottom,u,v,right,top,u,0,left,top,0,0};
 pglVertexPointer(2,GL_FLOAT,sizeof(GLfloat)*4,quad);
 pglTexCoordPointer(2,GL_FLOAT,sizeof(GLfloat)*4,quad+2);
 pglTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
 pglEnable(GL_BLEND);pglBlendFunc(GL_ONE,GL_ONE);
 /* Additive black-key composition matches the original title. A second
  * low-gain pass gives genuine peak brightness without clamped GL colours
  * or the legacy alpha>100% branch replacing the whole title rectangle. */
 GLfloat amount=(GLfloat)(gain>1?1:gain);
 pglColor4f(amount,amount,amount,1);pglDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,PresentationIndices);
 if(gain>1){amount=(GLfloat)(gain-1);pglColor4f(amount,amount,amount,1);pglDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,PresentationIndices);}
 /* Caller restores server/client state, then applies CRT/image adjustments. */
}

void FlipBuffers()
{
#if defined(_WIN32)
	if (!Probe124EnsureOpenGLContextCurrent("frontend-present")) return;
#endif
	if (Probe114FrontendFlipFrame < 8) {
		Probe113_RenderTracef("FRONTEND_FLIP begin index=%u viewport=%dx%d fullscreen=%dx%d", Probe114FrontendFlipFrame, ViewportWidth, ViewportHeight, FullscreenTextureWidth, FullscreenTextureHeight);
	}
	/* Frontend/in-game menus can follow a clipped 3D frame. Their presentation
	 * must start from a complete physical framebuffer, not that clipped rect. */
	PushBorderlessPresentationState();
	pglDisable(GL_SCISSOR_TEST);
	pglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-clear");
	SetOpenGLPresentationViewport(ViewportWidth, ViewportHeight, 1);

	pglDisable(GL_ALPHA_TEST);
	pglDisable(GL_BLEND);
	pglDisable(GL_DEPTH_TEST);

	pglBindTexture(GL_TEXTURE_2D, FullscreenTexture);
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FrontendPresentationFilter());
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FrontendPresentationFilter());
	pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    if(FullscreenTextureWidth<surface->w || FullscreenTextureHeight<surface->h){
        if(FullscreenTextureWidth<1)FullscreenTextureWidth=1;
        if(FullscreenTextureHeight<1)FullscreenTextureHeight=1;
        while(FullscreenTextureWidth<surface->w)FullscreenTextureWidth*=2;
        while(FullscreenTextureHeight<surface->h)FullscreenTextureHeight*=2;
        pglTexImage2D(GL_TEXTURE_2D,0,GL_RGB,FullscreenTextureWidth,FullscreenTextureHeight,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,NULL);
    }
    pglTexSubImage2D(GL_TEXTURE_2D,0,0,0,surface->w,surface->h,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,surface->pixels);
	if (Probe114FrontendFlipFrame < 8) {
		Probe113_RenderTracef("FRONTEND_FLIP upload-exit index=%u texture=%dx%d", Probe114FrontendFlipFrame, FullscreenTextureWidth, FullscreenTextureHeight);
	}

	GLfloat x0;
	GLfloat x1;
	GLfloat y0;
	GLfloat y1;
	GLfloat s0;
	GLfloat s1;
	GLfloat t0;
	GLfloat t1;

	/* Starfield.RIM uses a 640x480 canvas. Its central picture is 640x360,
	 * but the top and bottom bands are authored content: retaining the full
	 * canvas keeps the main-menu help/tip rects visible on wide displays.
	 * Consume the marker so no other frontend surface inherits it. */
	MainMenuBackdropPresentPending = 0;
	const GLfloat sourceWidth = (GLfloat)surface->w;
	const GLfloat sourceHeight = (GLfloat)surface->h;

	/* Fit the complete 640x480 canvas. On a 16:9 display that gives the
	 * intended 853x480 logical composition, centered with clean side pillars;
	 * the full top/bottom bands remain present instead of being UV-cropped. */
	GLfloat a = ViewportHeight * sourceWidth / sourceHeight;
	GLfloat b = ViewportWidth * sourceHeight / sourceWidth;

	if (a <= ViewportWidth) {
		y0 = -1.0f;
		y1 =  1.0f;

		x1 = 1.0 - (ViewportWidth - a) / ViewportWidth;
		x0 = -x1;
	} else {
		x0 = -1.0f;
		x1 =  1.0f;

		y1 = 1.0 - (ViewportHeight - b) / ViewportHeight;
		y0 = -y1;
	}

	s0 = 0.0f;
	s1 = sourceWidth / (float)FullscreenTextureWidth;
	t0 = 0.0f;
	t1 = sourceHeight / (float)FullscreenTextureHeight;

	GLfloat *v = FrontendPresentationVertices;

	v[0+0*4] = x0;
	v[1+0*4] = y0;
	v[2+0*4] = s0;
	v[3+0*4] = t1;
	v[0+1*4] = x1;
	v[1+1*4] = y0;
	v[2+1*4] = s1;
	v[3+1*4] = t1;
	v[0+2*4] = x1;
	v[1+2*4] = y1;
	v[2+2*4] = s1;
	v[3+2*4] = t0;
	v[0+3*4] = x0;
	v[1+3*4] = y1;
	v[2+3*4] = s0;
	v[3+3*4] = t0;


	pglEnableClientState(GL_VERTEX_ARRAY);
	pglVertexPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, &v[0]);

	pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	pglTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, &v[2]);

	pglDisableClientState(GL_COLOR_ARRAY);

	pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	pglDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, PresentationIndices);
    PresentIntro99Logo(x0,x1,y0,y1);
	if (BorderlessRenderTraceFrame < 600)
		BorderlessRenderTraceOpenGLError("present-draw");
	pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	pglDisableClientState(GL_VERTEX_ARRAY);

	pglBindTexture(GL_TEXTURE_2D, 0);
	PopBorderlessPresentationState();

	if (CRTFilterEnabled || ImageAdjustmentsActive()) {
		int crtWidth = 0, crtHeight = 0;
		SDL_GetWindowSizeInPixels(window, &crtWidth, &crtHeight);
		CRTFilter_Apply(crtWidth, crtHeight);
	}

#if !defined(NDEBUG)
	check_for_errors();
#endif

	LimitDesktopRefreshRate();
	LimitPresentationFrameRate();
	if (Probe114FrontendFlipFrame < 8) {
		Probe113_RenderTracef("FRONTEND_FLIP swap-enter index=%u", Probe114FrontendFlipFrame);
	}
	CapturePendingScreenshot();
	SDL_GL_SwapWindow(window);
	if (Probe114FrontendFlipFrame < 8) {
		BorderlessRenderTraceOpenGLError("frontend-swap-window");
		Probe113_RenderTracef("FRONTEND_FLIP swap-exit index=%u", Probe114FrontendFlipFrame);
	}
	Probe114FrontendFlipFrame++;
}

char *AvpCDPath = 0;

#if !defined(_MSC_VER)
static const struct option getopt_long_options[] = {
{ "help",	0,	NULL,	'h' },
{ "version",	0,	NULL,	'v' },
{ "fullscreen",	0,	NULL,	'f' },
{ "windowed",	0,	NULL,	'w' },
{ "nosound",	0,	NULL,	's' },
{ "nocdrom",	0,	NULL,	'c' },
{ "nojoy",	0,	NULL,	'j' },
{ "dev", 0, NULL, 1001 },
{ "dev2", 0, NULL, 1002 },
{ "debug",	0,	NULL,	'd' },
{ "windebug", 0, NULL, 'D' },
{ "withgl",	1,	NULL,	'g' },
{ "datapath",	1,	NULL,	'p' },
/*
{ "loadrifs",	1,	NULL,	'l' },
{ "server",	0,	someval,	1 },
{ "client",	1,	someval,	2 },
*/
{ NULL,		0,	NULL,	0 },
};
#endif

static const char *usage_string =
TYRARGO_DISPLAY_VERSION "\n"
"Direct fork of " TYRARGO_BASE "\n"
"Based on Rebellion Developments AvP Gold source\n"
"      [-h | --help]           Display this help message\n"
"      [-v | --version]        Display the game version\n"
"      [-f | --fullscreen]     Run the game fullscreen\n"
"      [-w | --windowed]       Run the game in a window\n"
"      [-s | --nosound]        Do not access the soundcard\n"
"      [-c | --nocdrom]        Do not access the CD-ROM\n"
"      [-j | --nojoy]          Do not access the joystick\n"
#if TYRARGO_DEVELOPMENT_TOOLS
"      [-d | -debug | --debug]  Enable debugging commands\n"
"      [-dev | --dev]         Viewmodel Z calibration in Controls\n"
#endif
"      [-p | --datapath] [x]   Look at [x] for game files\n"
"      [-g | --withgl] [x]     Use [x] instead of /usr/lib/libGL.so.1 for OpenGL\n"
;
#if !defined(_MSC_VER)
/*
 * Old Windows shortcuts for AvP commonly use a single dash before a long
 * option (for example, -debug). getopt_long only recognizes --debug, so
 * normalize only names explicitly registered above before parsing.
 */
static void NormalizeLegacyLongOptions(int argc, char *argv[])
{
	int i;
	const struct option *option;

	for (i = 1; i < argc; ++i)
	{
		char *argument = argv[i];

		if ((argument[0] != '-') || (argument[1] == '-') || (argument[1] == '\0'))
		{
			continue;
		}

		for (option = getopt_long_options; option->name != NULL; ++option)
		{
			if (strcmp(argument + 1, option->name) == 0)
			{
				size_t length = strlen(argument);
				char *normalized = (char *)malloc(length + 2);

				if (normalized == NULL)
				{
					fprintf(stderr, "Unable to process command-line option.\n");
					exit(EXIT_FAILURE);
				}

				normalized[0] = '-';
				normalized[1] = '-';
				strcpy(normalized + 2, argument + 1);
				argv[i] = normalized;
				break;
			}
		}
	}
}
#endif
#if defined(_WIN32)
static int HasWindowDebugArgument(int argc, char *argv[])
{
	int index;

	for (index = 1; index < argc; ++index)
	{
		if (strcmp(argv[index], "-windebug") == 0 || strcmp(argv[index], "--windebug") == 0)
		{
			return 1;
		}
	}

	return 0;
}

static void EnableOptionalWindowDebugConsole(int argc, char *argv[])
{
	HWND consoleWindow;

	if (!TYRARGO_DEVELOPMENT_TOOLS || !HasWindowDebugArgument(argc, argv)) return;

	/* Always create a second console, even when the process inherited CMD's one. */
	FreeConsole();
	if (!AllocConsole()) return;

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);
	SetConsoleTitleA(TYRARGO_PROJECT_NAME " - Debug Output");

	consoleWindow = GetConsoleWindow();
	if (consoleWindow != NULL)
	{
		ShowWindow(consoleWindow, SW_SHOWNOACTIVATE);
	}
}
#endif

/* Called at a safe boundary between complete frames, never from a loader. */
static void RunGameFrame(int *gameMenus,int *gameCompleted)
{
		if(ModalFrameActive) EventLatencyFrameBegin(LevelName,Player && Player->ObStrategyBlock && Player->ObStrategyBlock->containingModule?Player->ObStrategyBlock->containingModule->m_index:-1);
        if(Player && Player->ObStrategyBlock && Player->ObStrategyBlock->DynPtr){
            STRATEGYBLOCK *sb=Player->ObStrategyBlock;int weapon=-1;
            PLAYER_STATUS *status=(PLAYER_STATUS *)sb->SBdataptr;
            if(status && status->SelectedWeaponSlot>=0 && status->SelectedWeaponSlot<MAX_NO_OF_WEAPON_SLOTS)
                weapon=status->WeaponSlot[status->SelectedWeaponSlot].WeaponIDNumber;
            EventLatencyPlayer(sb->DynPtr->Position.vx,sb->DynPtr->Position.vy,sb->DynPtr->Position.vz,weapon);
        }
		if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP begin index=%u mode=%d", Probe114GameplayLoopFrame, AvP.GameMode);

		AvP_UpdateMenuPreview();
		switch(AvP.GameMode) {
		case I_GM_Playing:
			if ((!(*gameMenus) || (AvP.Network!=I_No_Network && !netGameData.skirmishMode)) && !AvP.LevelCompleted) {
				/* TODO: print some debugging stuff */
				
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP shape_animations-enter");
				{ EVENT_TICK span=EventLatencyBegin(); DoAllShapeAnimations(); EventLatencyEnd(span,"shape animation",0,0,0,0,0); }
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP shape_animations-exit");
				
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP update-enter");
				{ EVENT_TICK span=EventLatencyBegin(); UpdateGame(); EventLatencyEnd(span,"game update",0,0,0,0,0); }
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP update-exit");
				
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP views-enter");
				{ EVENT_TICK span=EventLatencyBegin(); AvpShowViews(); EventLatencyEnd(span,"view total",0,0,0,0,0); }
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP views-exit");
				
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP hud-enter");
				{ EVENT_TICK span=EventLatencyBegin(); MaintainHUD(); EventLatencyEnd(span,"HUD",0,0,0,0,0); }
				if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP hud-exit");
				
				{ EVENT_TICK span=EventLatencyBegin(); CheckCDAndChooseTrackIfNeeded(); EventLatencyEnd(span,"music check",0,0,0,0,0); }
				
				if(InGameMenusAreRunning() && ( (AvP.Network!=I_No_Network && netGameData.skirmishMode) || (AvP.Network==I_No_Network)) ) {
					SoundSys_StopAll();
				}
			} else {
				ReadUserInput();
				
				SoundSys_Management();
				
                if(AvP_MenuPreviewScene() && !AvP.LevelCompleted)AvP_RenderPausedPreview();
                else {
                    /* A paused menu has no fresh scene. Never reuse the physical
                       presentation as next frame's smaller logical background. */
                    extern void ClearPausedMenuBackground(void);
                    ClearPausedMenuBackground();
                    ThisFramesRenderingHasBegun();
                }
			}

			{ EVENT_TICK stamp=EventLatencyBegin(); if(!ModalFrameActive || *gameMenus) (*gameMenus)=AvP_InGameMenus(); EventLatencyEnd(stamp,"ingame menu total",0,0,0,0,0); }
			if (AvP.RestartLevel) (*gameMenus)=0;
			
			if (AvP.LevelCompleted && !ModalFrameActive) {
				SoundSys_FadeOutFast();
				{ EVENT_TICK stamp=EventLatencyBegin(); DoCompletedLevelStatisticsScreen(); EventLatencyEnd(stamp,"mission complete / statistics",0,0,0,0,2); }
				(*gameCompleted) = 1;
			}

			{ EVENT_TICK span=EventLatencyBegin(); ThisFramesRenderingHasFinished(); EventLatencyEnd(span,"render finish",0,0,0,0,0); }

			if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP flip-enter");
			{ EVENT_TICK span=EventLatencyBegin(); InGameFlipBuffers(); EventLatencyEnd(span,"present / swap",0,0,0,0,0); }
			if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP flip-exit");
			
			{ EVENT_TICK span=EventLatencyBegin(); FrameCounterHandler(); EventLatencyEnd(span,"frame limiter",0,0,0,0,0); }
			{
				PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
				
				if (!(*gameMenus) && playerStatusPtr->IsAlive && !AvP.LevelCompleted) {
					DealWithElapsedTime();
				}
			}
			if (Probe114GameplayLoopFrame < 8) Probe113_RenderTracef("GAME_LOOP end index=%u", Probe114GameplayLoopFrame);
			Probe114GameplayLoopFrame++;
			break;
			
		case I_GM_Menus:
			AvP.GameMode = I_GM_Playing;
			break;
		default:
			fprintf(stderr, "AvP.MainLoopRunning: gamemode = %d\n", AvP.GameMode);
			exit(EXIT_FAILURE);
		}
		
		EventLatencyFrameEnd();
}
static bool SDLCALL ContinueWindowModalFrame(void *unused, SDL_Event *event)
{
 static Uint64 lastFrame;
 Uint64 now;
 (void)unused;
 if(event->type!=SDL_EVENT_WINDOW_EXPOSED || !WindowEventsPumping || ModalFrameActive ||
    !SDL_IsMainThread() || !window || !context || event->window.windowID!=SDL_GetWindowID(window)) return true;
 if(!ModalGameMenus && !ModalFrontendActive)return true;
 if(AvP.RestartLevel || AvP.LevelCompleted || (ModalGameMenus && !AvP.MainLoopRunning))return true;
 now=SDL_GetTicksNS();if(now-lastFrame<8000000)return true;lastFrame=now;
 ModalFrameActive=1;
 /* A native move owns input. Do not repeat shots, shortcuts or menu commits. */
 secure_avpzero(KeyboardInput,sizeof KeyboardInput);
 secure_avpzero(DebouncedKeyboardInput,sizeof DebouncedKeyboardInput);
 GotAnyKey=DebouncedGotAnyKey=0;MouseRawDeltaX=MouseRawDeltaY=0;MouseVelX=MouseVelY=0;
 if(ModalGameMenus)RunGameFrame(ModalGameMenus,ModalGameCompleted);
 else { DrawMainMenusBackdrop();ReadUserInput();AvP_UpdateMenus();RenderFrontendScreenModeNotice();FlipBuffers();FrameCounterHandler();PlayMenuMusic();SoundSys_Management(); }
 WindowModalFrameCount++;
 ModalFrameActive=0;
 return true;
}
/* Only the initialized frontend event pump may authorize modal rendering.
 * Intro, menu loading and shutdown also pump window events, but their menu
 * elements are unavailable or may already have been released. */
void SetFrontendModalFramesActive(int active)
{
 ModalFrontendActive=active;
}

int main(int argc, char *argv[])
{
#if defined(_WIN32)
	EnableOptionalWindowDebugConsole(argc, argv);
#endif			
#if !defined(_MSC_VER)
	int c;

	NormalizeLegacyLongOptions(argc, argv);
	
	opterr = 0;
	while ((c = getopt_long(argc, argv, "hvfwscdg:p:", getopt_long_options, NULL)) != -1) {
		switch(c) {
			case 'h':
				printf("%s", usage_string);
				exit(EXIT_SUCCESS);
			case 'v':
				printf("%s", AvPVersionString);
				exit(EXIT_SUCCESS);
			case 'f':
				WantFullscreen = 1;
				CurrentVideoScreenMode = VIDEO_SCREEN_EXCLUSIVE;
				VideoScreenModeCommandLineOverride = 1;
				break;
			case 'w':
				WantFullscreen = 0;
				CurrentVideoScreenMode = VIDEO_SCREEN_WINDOWED;
				VideoScreenModeCommandLineOverride = 1;
				break;
			case 's':
				WantSound = 0;
				break;
			case 'c':
				WantCDRom = 0;
				break;
			case 'j':
				WantJoystick = 0;
				break;			
			case 'D':
				/* Console already reserved early by WinMain when -windebug is present. */
				break;
			case 'd': {
				extern int DebuggingCommandsActive;
				DebuggingCommandsActive = TYRARGO_DEVELOPMENT_TOOLS;
				}
				break;
			case 1002:
				ShakeEnabled = SHAKE_CALIBRATION_AVAILABLE;
				break;
			case 1001:
				DevViewmodelEnabled = TYRARGO_DEVELOPMENT_TOOLS;
				break;
			case 'g':
				opengl_library = optarg;
				break;
			case 'p':
				gamedatapath = optarg;
				break;
			default:
				printf("%s", usage_string);
				exit(EXIT_FAILURE);	
		}
	}
#endif
	fprintf(stderr, "%s", AvPVersionString);
	InitGameDirectories(argv[0], gamedatapath);
#if defined(_WIN32)
	Probe114InstallCrashTrace();
#endif
	Probe113_RenderTracef("BOOT after_InitGameDirectories");
	
	if (InitSDL() == -1) {
		fprintf(stderr, "Could not find a sutable resolution!\n");
		fprintf(stderr, "At least 512x384 is needed.  Does OpenGL work?\n");
		exit(EXIT_FAILURE);
	}
		
	Probe113_RenderTracef("BOOT after_InitSDL");
	if (DevViewmodelEnabled) DevViewmodelInitialize(SDL_GetBasePath());
	MotionParamsInitialize(SDL_GetBasePath());
	ShakeParamsInitialize(SDL_GetBasePath());
	LoadCDTrackList();
	Probe113_RenderTracef("BOOT after_LoadCDTrackList");
	
	SetFastRandom();
	
#if MARINE_DEMO
	ffInit("fastfile/mffinfo.txt","fastfile/");
#elif ALIEN_DEMO
	ffInit("alienfastfile/ffinfo.txt","alienfastfile/");
#else
	ffInit("fastfile/ffinfo.txt","fastfile/");
#endif
	InitGame();
	Probe113_RenderTracef("BOOT after_InitGame");

	WindowWidth = VideoModeList[CurrentVideoMode].w;
	WindowHeight = VideoModeList[CurrentVideoMode].h;
	SetOGLVideoMode(0, 0);
	SetSoftVideoMode(640, 480, 16);
	
	InitialVideoMode();

	/* Env_List can probably be removed */
	Env_List[0]->main = LevelName;
	
	InitialiseSystem();
	InitialiseRenderer();
	Probe113_RenderTracef("BOOT after_InitialiseRenderer");
	
	LoadKeyConfiguration();
	
	SoundSys_Start();
	if (WantCDRom) CDDA_Start();
	
	InitTextStrings();
	
	BuildMultiplayerLevelNameArray();
	
	ChangeDirectDrawObject();
	AvP.LevelCompleted = 0;
	LoadSounds("PLAYER");

	/* is this still neccessary? */
	AvP.CurrentEnv = AvP.StartingEnv = 0;

#if ALIEN_DEMO
	AvP.PlayerType = I_Alien;
	SetLevelToLoad(AVP_ENVIRONMENT_INVASION_A);
#elif PREDATOR_DEMO
	AvP.PlayerType = I_Predator;
	SetLevelToLoad(AVP_ENVIRONMENT_INVASION_P);
#elif MARINE_DEMO
	AvP.PlayerType = I_Marine;
	SetLevelToLoad(AVP_ENVIRONMENT_INVASION);
#endif

#if !(ALIEN_DEMO|PREDATOR_DEMO|MARINE_DEMO)	
Probe113_RenderTracef("BOOT entering_main_menus");
while (AvP_MainMenus())
#else
if (AvP_MainMenus())
#endif
{
	int menusActive = 0;
	int thisLevelHasBeenCompleted = 0;
	
	/* turn off any special effects */
	d3d_light_ctrl.ctrl = LCCM_NORMAL;

	Probe113_BeginMapTrace("GAME_SESSION");
	Probe113_RenderTracef("MAP phase=SetOGLVideoMode-enter");
	SetOGLVideoMode(0, 0);
	Probe113_RenderTracef("MAP phase=SetOGLVideoMode-exit");

	InitialiseGammaSettings(RequestedGammaSetting);
	Probe113_RenderTracef("MAP phase=gamma-exit");
	
	Probe113_RenderTracef("MAP phase=load_shapes-enter");
	EventLatencyLoadCheckpoint("map load_shapes enter",0);
	start_of_loaded_shapes = load_precompiled_shapes();
	Probe113_RenderTracef("MAP phase=load_shapes-exit");
	EventLatencyLoadCheckpoint("map load_shapes exit",0);
	
	Probe113_RenderTracef("MAP phase=InitCharacter-enter");
	EventLatencyLoadCheckpoint("map InitCharacter enter",0);
	InitCharacter();
	Probe113_RenderTracef("MAP phase=InitCharacter-exit");
	EventLatencyLoadCheckpoint("map InitCharacter exit",0);
	
	Probe113_RenderTracef("MAP phase=LoadRifFile-enter");
	EventLatencyLoadCheckpoint("map LoadRifFile enter",0);
	LoadRifFile(); /* sets up a map */
	Probe113_RenderTracef("MAP phase=LoadRifFile-exit");
	EventLatencyLoadCheckpoint("map LoadRifFile exit",0);
	
	Probe113_RenderTracef("MAP phase=AssignAllSBNames-enter");
	AssignAllSBNames();
	Probe113_RenderTracef("MAP phase=AssignAllSBNames-exit");
	
	Probe113_RenderTracef("MAP phase=StartGame-enter");
	EventLatencyLoadCheckpoint("map StartGame enter",0);
	StartGame();
	Probe113_RenderTracef("MAP phase=StartGame-exit");
	EventLatencyLoadCheckpoint("map StartGame exit",0);
	
	ffcloseall();
	
	AvP.MainLoopRunning = 1;
	
	Probe113_RenderTracef("MAP phase=ScanImagesForFMVs-enter");
	EventLatencyLoadCheckpoint("map ScanImagesForFMVs enter",0);
	ScanImagesForFMVs();
	Probe113_RenderTracef("MAP phase=ScanImagesForFMVs-exit");
	EventLatencyLoadCheckpoint("map ScanImagesForFMVs exit",0);
	
	ResetFrameCounter();

	Game_Has_Loaded();
	
	ResetFrameCounter();
	
	if(AvP.Network!=I_No_Network)
	{
		/*Need to choose a starting position for the player , but first we must look
		through the network messages to find out which generator spots are currently clear*/
		netGameData.myGameState = NGS_Playing;
		MinimalNetCollectMessages();
		TeleportNetPlayerToAStartingPosition(Player->ObStrategyBlock,1);
	}

	IngameKeyboardInput_ClearBuffer();
	
	ModalGameMenus=&menusActive;ModalGameCompleted=&thisLevelHasBeenCompleted;
	while(AvP.MainLoopRunning) {
        EventLatencyFrameBegin(LevelName,Player && Player->ObStrategyBlock && Player->ObStrategyBlock->containingModule?Player->ObStrategyBlock->containingModule->m_index:-1);
        { unsigned int modalBefore=WindowModalFrameCount;EVENT_TICK span=EventLatencyBegin();
          CheckForWindowsMessages();
          if(modalBefore!=WindowModalFrameCount) EventLatencyFrameBegin(LevelName,Player && Player->ObStrategyBlock && Player->ObStrategyBlock->containingModule?Player->ObStrategyBlock->containingModule->m_index:-1);
          EventLatencyEnd(span,"window events",0,0,0,0,0); }
        RunGameFrame(&menusActive,&thisLevelHasBeenCompleted);
		if (AvP.RestartLevel) {
			AvP.RestartLevel = 0;
			AvP.LevelCompleted = 0;

			FixCheatModesInUserProfile(UserProfilePtr);

			EventLatencyFlush();
			RestartLevel();
		}
	}
	
	EventLatencyFlush();

	ModalGameMenus=ModalGameCompleted=NULL;
	AvP.LevelCompleted = thisLevelHasBeenCompleted;

	FixCheatModesInUserProfile(UserProfilePtr);

	ReleaseAllFMVTextures();

	CONSBIND_WriteKeyBindingsToConfigFile();
	
	CampaignCapturePlayer();
	DeInitialisePlayer();
	
	DeallocatePlayersMirrorImage();
	
	KillHUD();
	
	Destroy_CurrentEnvironment();
	
	DeallocateAllImages();
	
	EndNPCs();
	
	ExitGame();
	
	SoundSys_StopAll();
	
	SoundSys_ResetFadeLevel();
	
	CDDA_Stop();
	
	if (AvP.Network != I_No_Network) {
		EndAVPNetGame();
	}
	
	ClearMemoryPool();

/* go back to menu mode */
#if !(ALIEN_DEMO|PREDATOR_DEMO|MARINE_DEMO)
	SetSoftVideoMode(640, 480, 16);
#endif	
}

	SoundSys_StopAll();
	SoundSys_RemoveAll();
	
	ExitSystem();
	
	CDDA_End();
	ClearMemoryPool();
	
	return 0;
}

