#include "event_latency.h"
#include "hud_motion.h"
#include "fmv_backend.h"
#include "tyrargo_version.h"
#include "menu_preview.h"
#include "fire_batch.h"
#include "water_effect.h"
#include <math.h>
#include "coplanar_depth.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oglfunc.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl_glext.h>
#include <stddef.h>

#include "fixer.h"

#include "3dc.h"
#include "fmv.h"
#include "projection.h"
#include "viewmodel.h"
#include "platform.h"
#include "inline.h"
#include "module.h"
#include "stratdef.h"
#include "projfont.h"
#include "kshape.h"
#include "prototyp.h"
#include "frustum.h"
#include "lighting.h"
#include "bh_types.h"
#include "showcmds.h"
#include "d3d_hud.h"
#include "hud_layout.h"
#include "avp_userprofile.h"
#include "avp/win95/text_encoding.h"
#include "aw.h"
#include "opengl.h"

int LightIntensityAtPoint(VECTORCH *pointPtr);

extern IMAGEHEADER ImageHeaderArray[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern unsigned char GammaValues[256];
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern int SpecialFXImageNumber;
extern int StaticImageNumber;
extern int PredatorNumbersImageNumber;
extern int BurningImageNumber;
extern int ChromeImageNumber;
extern int WaterShaftImageNumber;
extern int HUDFontsImageNumber;
extern int AAFontImageNumber;

extern int FMVParticleColour;
extern int HUDScaleFactor;
extern int CloakingPhase;

int NoiseOverlayScaleMode = NOISE_OVERLAY_SCALE_1X;
int HUDTextureFilterMode = HUD_TEXTURE_FILTER_BILINEAR;

int OpenGLAntiAliasingMode = 2;
/* Global scene-presentation colour depth. 32-bit is the modern default. */
int OpenGLColorBitDepthMode = OPENGL_COLOR_BIT_DEPTH_32;
int OpenGLAnisotropyMode = 3;
int OpenGLTextureFilterMode = 2;
static D3DTexture *CurrTextureHandle;





static enum TRANSLUCENCY_TYPE CurrentTranslucencyMode = TRANSLUCENCY_OFF;
static enum FILTERING_MODE_ID CurrentFilteringMode = FILTERING_BILINEAR_OFF;
static GLenum TextureMinFilter = GL_LINEAR_MIPMAP_NEAREST;
static D3DTexture *CurrentlyBoundTexture = NULL;
static int CurrentCoplanarTrimBatch=0;

static int LastAnisotropyMode = -1;
static unsigned int TextureQualityGeneration = 1;

static GLenum RequestedTextureMagFilter(void)
{
    return (OpenGLTextureFilterMode == 0) ? GL_NEAREST : GL_LINEAR;
}

static GLenum RequestedTextureMinFilter(void)
{
    switch (OpenGLTextureFilterMode)
    {
    case 0: return GL_NEAREST;
    case 1: return GL_LINEAR_MIPMAP_NEAREST;
    default: return GL_LINEAR_MIPMAP_LINEAR;
    }
}

/* RIM colour-key masks need level-zero sampling. Averaged mip alpha can
 * remove every zero-alpha texel in a small hole; the alpha > 0 cutout then
 * writes depth across it. AvPx also samples these textures at one level.
 * Keep the selected nearest/bilinear filter and opaque-texture mipmaps. */
static GLenum TextureMinFilterForTexture(const D3DTexture *tex)
{
    if (tex->hasAlpha && tex->transparentPixels != 0)
        return RequestedTextureMagFilter();
    return TextureMinFilter;
}

static void ApplyCurrentAnisotropy(void)
{
    GLfloat maximumAnisotropy;
    GLfloat requestedAnisotropy;

    if (!ogl_use_texture_filter_anisotropic)
    {
        return;
    }

    if (OpenGLAnisotropyMode <= 0)
    {
        pglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
        return;
    }

    pglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
    if (maximumAnisotropy <= 1.0f)
    {
        return;
    }

    switch (OpenGLAnisotropyMode)
    {
    case 1: requestedAnisotropy = 2.0f; break;
    case 2: requestedAnisotropy = 4.0f; break;
    case 3: requestedAnisotropy = 8.0f; break;
    default: requestedAnisotropy = 16.0f; break;
    }

    if (requestedAnisotropy > maximumAnisotropy)
    {
        requestedAnisotropy = maximumAnisotropy;
    }
    pglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, requestedAnisotropy);
}

static int LastFMVFilterMode=-1;
static void RefreshTextureQualityState(void)
{
	GLenum requestedMinFilter = RequestedTextureMinFilter();
	if (TextureMinFilter != requestedMinFilter || LastAnisotropyMode != OpenGLAnisotropyMode || LastFMVFilterMode != FMVFilterMode)
	{
		TextureMinFilter = requestedMinFilter;
		LastAnisotropyMode = OpenGLAnisotropyMode;
        LastFMVFilterMode=FMVFilterMode;
        TextureQualityGeneration++;
        if (TextureQualityGeneration == 0) TextureQualityGeneration = 1;
	}
}

#if defined(_MSC_VER)
#define ALIGN16 __declspec(align(16))
#else
#define ALIGN16 __attribute__((__aligned__(16)))
#endif

// need to look into this again at some point
// everything but the hud rendering used an offset
#define TEXCOORD_FIXED(s, r) (((float)((s)+(0<<15))) * (r))

#define TA_MAXVERTICES		2048
#define TA_MAXTRIANGLES		2048

typedef struct VertexArray
{
	GLfloat v[4];
	GLfloat t[2];
	GLubyte c[4];
	GLubyte s[4];
} VertexArray;

typedef struct TriangleArray
{
	unsigned short a;
	unsigned short b;
	unsigned short c;
} TriangleArray;

static ALIGN16 VertexArray varr[TA_MAXVERTICES];
static ALIGN16 TriangleArray tarr[TA_MAXTRIANGLES];
static VertexArray *varrp = varr;
static TriangleArray *tarrp = tarr;
static int varrc, tarrc;

static ALIGN16 TriangleArray starr[TA_MAXTRIANGLES];
static TriangleArray *starrp = starr;
static int starrc;

/* GL 1.5 ordered streaming. Each upload replaces the buffer storage so pending
   GPU draws keep their own data; no waiting for a reused client array. Geometry,
   precision, specular passes and transparency order remain unchanged. */
#if !defined(USE_OPENGL_ES)
static GLuint streamVertex,streamIndex;
static PFNGLGENBUFFERSPROC streamGen;
static PFNGLBINDBUFFERPROC streamBind;
static PFNGLBUFFERDATAPROC streamData;
static PFNGLDELETEBUFFERSPROC streamDelete;
void GeometryStreamRelease(void){
 if(streamDelete){if(streamVertex)streamDelete(1,&streamVertex);if(streamIndex)streamDelete(1,&streamIndex);}
 streamVertex=streamIndex=0;streamGen=NULL;streamBind=NULL;streamData=NULL;streamDelete=NULL;
}
static void GeometryStreamInit(void){
 GeometryStreamRelease();
 streamGen=(PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
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
 pglTexCoordPointer(2,GL_FLOAT,sizeof(varr[0]),varr[0].t);
 pglColorPointer(4,GL_UNSIGNED_BYTE,sizeof(varr[0]),specular?varr[0].s:varr[0].c);
}

#else
void GeometryStreamRelease(void){}
static void GeometryStreamInit(void){}
static void GeometryStreamDraw(TriangleArray*indices,int triangles,int vertices,int specular){
 (void)vertices;(void)specular;pglDrawElements(GL_TRIANGLES,triangles*3,GL_UNSIGNED_SHORT,indices);
}
#endif

/* Do not call this directly! */
static void SetTranslucencyMode(enum TRANSLUCENCY_TYPE mode)
{
	pglDisable(GL_ALPHA_TEST);

	switch(mode) {
		case TRANSLUCENCY_OFF:
			if (TRIPTASTIC_CHEATMODE||MOTIONBLUR_CHEATMODE) {
				pglBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
			} else {
				// alien tail hack
				pglEnable(GL_ALPHA_TEST);
				
				pglBlendFunc(GL_ONE, GL_ZERO);
			}
			break;
		case TRANSLUCENCY_NORMAL:
			pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case TRANSLUCENCY_COLOUR:
			pglBlendFunc(GL_ZERO, GL_SRC_COLOR);
			break;
		case TRANSLUCENCY_INVCOLOUR:
			pglBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
			break;
		case TRANSLUCENCY_GLOWING:
			pglBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case TRANSLUCENCY_DARKENINGCOLOUR:
			pglBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
			break;
		case TRANSLUCENCY_JUSTSETZ:
			pglBlendFunc(GL_ZERO, GL_ONE);
			break;
		default:
			fprintf(stderr, "SetTranslucencyMode: invalid blend mode %d\n", mode);
			break;
	}
}

static void SetSecondPassTranslucencyMode(enum TRANSLUCENCY_TYPE mode)
{
	pglDisable(GL_ALPHA_TEST);

	switch(mode) {
		case TRANSLUCENCY_OFF:
			if (TRIPTASTIC_CHEATMODE||MOTIONBLUR_CHEATMODE) {
				pglBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
			} else {
				/* Match the base pass cutout: zero-alpha texels must not add
				 * specular colour or depth over geometry behind the texture. */
				pglEnable(GL_ALPHA_TEST);
				pglBlendFunc(GL_ONE, GL_ONE);
			}
			break;
		case TRANSLUCENCY_NORMAL:
			pglBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case TRANSLUCENCY_GLOWING:
			pglBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case TRANSLUCENCY_COLOUR:
			//fprintf(stderr, "SetSecondPassTranslucencyMode: unsupported blend mode %d\n", mode);
			// can't easily emulate this one
			pglBlendFunc(GL_DST_COLOR, GL_ONE);
			break;
		case TRANSLUCENCY_INVCOLOUR:
			//fprintf(stderr, "SetSecondPassTranslucencyMode: unsupported blend mode %d\n", mode);
			// can't easily emulate this one
			pglBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
			break;

		case TRANSLUCENCY_DARKENINGCOLOUR:
		case TRANSLUCENCY_JUSTSETZ:
			fprintf(stderr, "SetSecondPassTranslucencyMode: unsupported blend mode %d\n", mode);
			pglBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		default:
			fprintf(stderr, "SetSecondPassTranslucencyMode: invalid blend mode %d\n", mode);
			break;
	}
}

/* 
A few things:
- Vertices with a specular color are done twice.
  Might want to try spitting apart the three arrays and using the same vertex
  array for both passes.
- Fix code for separate color support.
*/

void InitOpenGL()
{
 GeometryStreamInit();
    FireBatchAvailable(); /* Compile once during renderer setup, not the first explosion. */
        pglHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
        pglHint( GL_GENERATE_MIPMAP_HINT, GL_NICEST );

#if GL_NV_multisample_filter_hint
        if ( ogl_use_multisample_filter_hint )
        {
                pglHint( GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST );
        }
#endif

	CurrentTranslucencyMode = TRANSLUCENCY_OFF;
	pglBlendFunc(GL_ONE, GL_ZERO);
	
	pglAlphaFunc(GL_GREATER, 0.0f);
	
	CurrentFilteringMode = FILTERING_BILINEAR_OFF;
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        
	CurrentlyBoundTexture = NULL;
    CurrentCoplanarTrimBatch=0;
	pglBindTexture(GL_TEXTURE_2D, 0);
	
	pglEnableClientState(GL_VERTEX_ARRAY);
	pglVertexPointer(4, GL_FLOAT, sizeof(varr[0]), varr[0].v);
		
	pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	pglTexCoordPointer(2, GL_FLOAT, sizeof(varr[0]), varr[0].t);
		
	pglEnableClientState(GL_COLOR_ARRAY);
	pglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(varr[0]), varr[0].c);

	tarrc = 0;
	tarrp = tarr;
		
	varrc = 0;
	varrp = varr;
	
	starrc = 0;
	starrp = starr;
}

static void FlushTriangleBuffers_MeasuredBody(int backup)
{
    FireBatchFlush();
    int batchVertices=varrc;
    int trimOffset=CurrentCoplanarTrimBatch && (tarrc || starrc);
    if(trimOffset) {
        pglPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
        pglEnable(GL_POLYGON_OFFSET_FILL);
        pglPolygonOffset(COPLANAR_TRIM_OFFSET_FACTOR,COPLANAR_TRIM_OFFSET_UNITS);
    }
	if (tarrc) {
		{ EVENT_TICK stamp=EventLatencyBegin(); EventLatencyCount(EL_BATCHES,1); EventLatencyCount(EL_TRIANGLES,tarrc); GeometryStreamDraw(tarr,tarrc,batchVertices,0); EventLatencyEnd(stamp,"GL draw submission",0,tarrc,0,0,0); }
		
		tarrc = 0;
		tarrp = tarr;
		
		varrc = 0;
		varrp = varr;
	}
	
	if (starrc) {
		//if (CurrentlyBoundTexture != NULL) {
		//	if (!backup) CurrentlyBoundTexture = NULL;
		//	pglBindTexture(GL_TEXTURE_2D, 0);
		//}
		
		//if (CurrentTranslucencyMode != TRANSLUCENCY_GLOWING) {
		//	if (!backup) CurrentTranslucencyMode = TRANSLUCENCY_GLOWING;
		//	SetTranslucencyMode(TRANSLUCENCY_GLOWING);
		//	//if (CurrentTranslucencyMode == TRANSLUCENCY_OFF)
		//	//	pglEnable(GL_BLEND);
		//	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		//}

		SetSecondPassTranslucencyMode(CurrentTranslucencyMode);

		pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
		pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		pglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		pglTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
		pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		pglTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
		pglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

		//pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
		pglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(varr[0]), varr[0].s);

		{ EVENT_TICK stamp=EventLatencyBegin(); EventLatencyCount(EL_BATCHES,1); EventLatencyCount(EL_TRIANGLES,starrc); GeometryStreamDraw(starr,starrc,batchVertices,1); EventLatencyEnd(stamp,"GL draw submission",0,starrc,0,0,0); }

		//pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		pglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(varr[0]), varr[0].c);
		
		//if (backup) {
		//	//if (CurrentlyBoundTexture)
		//	//	pglBindTexture(GL_TEXTURE_2D, CurrentlyBoundTexture->id);
		//	if (CurrentTranslucencyMode != TRANSLUCENCY_GLOWING)
		//		SetTranslucencyMode(CurrentTranslucencyMode);
		//} else {
		//	//CurrentlyBoundTexture = NULL;
		//	CurrentTranslucencyMode = TRANSLUCENCY_GLOWING;
		//}

		SetTranslucencyMode(CurrentTranslucencyMode);
		pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		starrc = 0;
		starrp = starr;
	}
    if(trimOffset)pglPopAttrib();
}
static void FlushTriangleBuffers(int backup)
{
 EVENT_TICK stamp=EventLatencyBegin();
 FlushTriangleBuffers_MeasuredBody(backup);
 EventLatencyEnd(stamp,"GL batch flush and state",0,backup,0,0,0);
}


static void CheckBoundTextureIsCorrect(D3DTexture *tex)
{
	if (tex == CurrentlyBoundTexture && (tex == NULL || tex->qualityGeneration == TextureQualityGeneration))
		return;

	FlushTriangleBuffers(1);
	
	if (tex == NULL) {
		pglBindTexture(GL_TEXTURE_2D, 0);
		
		CurrentlyBoundTexture = NULL;
		
		return;
	} 
	
	{ EVENT_TICK span=EventLatencyBegin(); pglBindTexture(GL_TEXTURE_2D, tex->id); EventLatencyEnd(span,"GL bind texture",0,tex->id,0,0,0); }

	if (tex->filter != CurrentFilteringMode || tex->qualityGeneration != TextureQualityGeneration) {
        EVENT_TICK samplerSpan=EventLatencyBegin();
		if(FMV_IsPanelTexture(tex)){
            GLenum filter=FMVFilterMode==0?GL_NEAREST:GL_LINEAR;
            pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
            pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
            if(ogl_use_texture_filter_anisotropic)pglTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,1.0f);
        } else switch(CurrentFilteringMode) {
			case FILTERING_BILINEAR_OFF:
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;
			case FILTERING_BILINEAR_ON:
    pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, RequestedTextureMagFilter());
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureMinFilterForTexture(tex));
				ApplyCurrentAnisotropy();
				break;
			default:
				break;
		}
		
		tex->filter = CurrentFilteringMode;
        tex->qualityGeneration = TextureQualityGeneration;
        EventLatencyEnd(samplerSpan,"GL texture filter state",0,tex->id,0,0,0);
	}
	
	CurrentlyBoundTexture = tex;
}

static void CheckFilteringModeIsCorrect(enum FILTERING_MODE_ID filter)
{
	CurrentFilteringMode = filter;
	
	if (CurrentlyBoundTexture && (CurrentlyBoundTexture->filter != CurrentFilteringMode || CurrentlyBoundTexture->qualityGeneration != TextureQualityGeneration)) {
		FlushTriangleBuffers(1);
		
		if(FMV_IsPanelTexture(CurrentlyBoundTexture)){
            GLenum videoFilter=FMVFilterMode==0?GL_NEAREST:GL_LINEAR;
            pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,videoFilter);
            pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,videoFilter);
            if(ogl_use_texture_filter_anisotropic)pglTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,1.0f);
        } else switch(CurrentFilteringMode) {
			case FILTERING_BILINEAR_OFF:
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;
			case FILTERING_BILINEAR_ON:
    pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, RequestedTextureMagFilter());
				pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureMinFilterForTexture(CurrentlyBoundTexture));
				ApplyCurrentAnisotropy();
				break;
			default:
				break;
		}
		
		CurrentlyBoundTexture->filter = CurrentFilteringMode;
        CurrentlyBoundTexture->qualityGeneration = TextureQualityGeneration;
	}
}
		
static void CheckTranslucencyModeIsCorrect(enum TRANSLUCENCY_TYPE mode)
{	
	if (CurrentTranslucencyMode == mode)
		return;

	FlushTriangleBuffers(1);
	
	SetTranslucencyMode(mode);
		
	CurrentTranslucencyMode = mode;
}

static void CheckTriangleBuffer(int rver, int sver, int rtri, int stri, D3DTexture *tex, enum TRANSLUCENCY_TYPE mode, enum FILTERING_MODE_ID filter, int coplanarTrim)
{
    FireBatchFlush(); /* Preserve ordering before any ordinary geometry. */
    if(CurrentCoplanarTrimBatch!=coplanarTrim) {
        FlushTriangleBuffers(1);
        CurrentCoplanarTrimBatch=coplanarTrim;
    }
	if ((rver+varrc) >= TA_MAXVERTICES) {
		FlushTriangleBuffers(0);
	} else if (rtri == 0 && ((rver-2+tarrc) >= TA_MAXTRIANGLES)) {
		FlushTriangleBuffers(0);
	} else if (rtri && ((rtri+tarrc) >= TA_MAXTRIANGLES)) {
		FlushTriangleBuffers(0);
	}

	if ((intptr_t)tex != -1)
		CheckBoundTextureIsCorrect(tex);
	if (mode != -1)
		CheckTranslucencyModeIsCorrect(mode);
	if (filter != -1)
		CheckFilteringModeIsCorrect(filter);

#define OUTPUT_TRIANGLE(x, y, z) \
{ \
	tarrp->a = varrc+(x);	\
	tarrp->b = varrc+(y);	\
	tarrp->c = varrc+(z);	\
				\
	tarrp++;		\
	tarrc++; 		\
}
	
	if (rtri == 0) {
		switch(rver) {
			case 0:
				break;
			case 3:
				OUTPUT_TRIANGLE(0, 2, 1);
				break;
			case 5:
				OUTPUT_TRIANGLE(0, 1, 4);
				OUTPUT_TRIANGLE(1, 3, 4);
				OUTPUT_TRIANGLE(1, 2, 3);
				break;
			case 8:
				OUTPUT_TRIANGLE(0, 6, 7);
			case 7:
				OUTPUT_TRIANGLE(0, 5, 6);
			case 6:
				OUTPUT_TRIANGLE(0, 4, 5);
				OUTPUT_TRIANGLE(0, 3, 4);
			case 4:
				OUTPUT_TRIANGLE(0, 2, 3);
				OUTPUT_TRIANGLE(0, 1, 2);
				break;
			default:
				fprintf(stderr, "DrawTriangles_T2F_C4UB_V4F: vertices = %d\n", rver);
		}
	}	
#undef OUTPUT_TRIANGLE
	
#define OUTPUT_TRIANGLE(x, y, z) \
{ \
	starrp->a = varrc+(x);	\
	starrp->b = varrc+(y);	\
	starrp->c = varrc+(z);	\
						\
	starrp++;				\
	starrc++; 				\
}
	if (stri == 0) {
		switch(sver) {
			case 0:
				break;
			case 3:
				OUTPUT_TRIANGLE(0, 2, 1);
				break;
			case 5:
				OUTPUT_TRIANGLE(0, 1, 4);
				OUTPUT_TRIANGLE(1, 3, 4);
				OUTPUT_TRIANGLE(1, 2, 3);
				break;
			case 8:
				OUTPUT_TRIANGLE(0, 6, 7);
			case 7:
				OUTPUT_TRIANGLE(0, 5, 6);
			case 6:
				OUTPUT_TRIANGLE(0, 4, 5);
				OUTPUT_TRIANGLE(0, 3, 4);
			case 4:
				OUTPUT_TRIANGLE(0, 2, 3);
				OUTPUT_TRIANGLE(0, 1, 2);
				break;
			default:
				fprintf(stderr, "DrawTriangles_T2F_C4UB_V4F: vertices = %d\n", sver);
		}
	}
#undef OUTPUT_TRIANGLE
}

static unsigned int PowerOfTwo(unsigned int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

static GLuint CreateOGLTextureLatencyBody(D3DTexture *tex, unsigned char *buf);
GLuint CreateOGLTexture(D3DTexture *tex, unsigned char *buf) {
 EVENT_TICK start=EventLatencyBegin();
 EVENT_TICK id=0;
 int eventType=0, eventMessage=0;
 GLuint result=CreateOGLTextureLatencyBody(tex,buf);
 EventLatencyEnd(start,"texture upload",eventType,eventMessage,id,0,1);
 return result;
}
static GLuint CreateOGLTextureLatencyBody(D3DTexture *tex, unsigned char *buf)
{
	if (buf == NULL) {
	    // converting DDSurface to D3DTexture
	    buf = tex->buf;
	}
	if (buf == NULL) {
	    fprintf(stderr, "CreateOGLTexture - null buffer\n");
	    return 0;
	}

	int i;
	tex->transparentPixels = 0;
	tex->opaquePixels = 0;
	int l = tex->w * tex->h;
	for (i = 0; i < l; i++) {
		int o = i*4;
		int r = buf[o+0];
		int g = buf[o+1];
		int b = buf[o+2];
		int a = buf[o+3];

		// kinda pre-multiplied alpha;
		// texels with zero alpha shouldn't
		// be visible.
		if (a == 0) {
			r = 0;
			g = 0;
			b = 0;
			++tex->transparentPixels;
		} else {
			++tex->opaquePixels;
		}

		buf[o+0] = r;
		buf[o+1] = g;
		buf[o+2] = b;
		buf[o+3] = a;
	}

	tex->TexWidth = tex->w;
	tex->TexHeight = tex->h;

	int PotWidth = PowerOfTwo(tex->TexWidth);
	int PotHeight = PowerOfTwo(tex->TexHeight);
	tex->IsNpot = (PotWidth != tex->TexWidth) || (PotHeight != tex->TexHeight);

	GLuint h;

	RefreshTextureQualityState();
	FlushTriangleBuffers(1);

	pglGenTextures(1, &h);

	pglBindTexture(GL_TEXTURE_2D, h);
	pglTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (tex->IsNpot) {
        // OpenGL 1.x compatibility
        tex->TexWidth = PotWidth;
        tex->TexHeight = PotHeight;
    pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, RequestedTextureMagFilter());
        pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, RequestedTextureMagFilter());
		pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->TexWidth, tex->TexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    } else {
    pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, RequestedTextureMagFilter());
		if (tex->IsNpot) {
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureMinFilterForTexture(tex));
		}

		pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	}

	pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    tex->buf = NULL;
	tex->id = h;
	tex->filter = FILTERING_BILINEAR_ON;
    tex->qualityGeneration = TextureQualityGeneration;
	tex->RecipW = 1.0f / (float) tex->TexWidth;
	tex->RecipH = 1.0f / (float) tex->TexHeight;
	ApplyCurrentAnisotropy();
	
	if ( CurrentlyBoundTexture != NULL )
	{
		/* restore the previously-bound texture */
		pglBindTexture(GL_TEXTURE_2D, CurrentlyBoundTexture->id);
	}
	
	free(buf);

	return h;
}

void ReleaseD3DTexture(void *tex)
{
	D3DTexture *TextureHandle = (D3DTexture *)tex;
	
	if (TextureHandle == NULL) {
		return;
	}

	/* A context restart releases every image.  Do not leave renderer cache
	 * pointers referring to an object that is about to be freed. */
	if (CurrentlyBoundTexture == TextureHandle) {
		CurrentlyBoundTexture = NULL;
	}
	if (CurrTextureHandle == TextureHandle) {
		CurrTextureHandle = NULL;
	}


	if (TextureHandle->id != 0) {
		pglDeleteTextures(1, (GLuint*) &(TextureHandle->id));
		TextureHandle->id = 0;
	}

	if (TextureHandle->buf != NULL) {
		free(TextureHandle->buf);
		TextureHandle->buf = NULL;
	}
	
	free(TextureHandle);
}

int CreateIMGSurface(D3DTexture *tex, unsigned char *buf)
{
	tex->buf = buf;
	tex->id = 0;

	return 0;
}

void ReleaseDDSurface(void* DDSurface)
{
	ReleaseD3DTexture(DDSurface);
}

/* ** */

void ThisFramesRenderingHasBegun()
{
#ifdef GL_MULTISAMPLE
 /* Reassert the requested scene state after fullscreen presentation passes. */
 if(OpenGLAntiAliasingMode>0)pglEnable(GL_MULTISAMPLE);else pglDisable(GL_MULTISAMPLE);
#endif

	RefreshTextureQualityState();
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
}

void ThisFramesRenderingHasFinished()
{
	LightBlockDeallocation();
	
	FlushTriangleBuffers(0);
}
        
/* ** */

/* No scene preview: discard any previous scaled UI before drawing the pause menu. */
void ClearPausedMenuBackground(void)
{
                    FlushTriangleBuffers(0);
                    pglPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_SCISSOR_BIT);
                    pglDisable(GL_SCISSOR_TEST);
                    pglDepthMask(GL_TRUE);
                    pglClearColor(0,0,0,0);
                    pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                    pglPopAttrib();
}

void FlushD3DZBuffer()
{
	pglClear(GL_DEPTH_BUFFER_BIT);
}

void SecondFlushD3DZBuffer()
{
	FlushTriangleBuffers(0);
	
	pglClear(GL_DEPTH_BUFFER_BIT);
}

void D3D_DecalSystem_Setup()
{
	FlushTriangleBuffers(0);
	
	pglDepthMask(GL_FALSE);

	/* enable polygon offset to help lessen decal z-fighting... */
	pglEnable(GL_POLYGON_OFFSET_FILL);
	
	static GLfloat factor = 0.0f;
	static GLfloat units = -0.09375f;
	pglPolygonOffset(factor, units);
}

void D3D_DecalSystem_End()
{
	FlushTriangleBuffers(0);
	
	pglDepthMask(GL_TRUE);
	
	pglDisable(GL_POLYGON_OFFSET_FILL);
}

/* ** */

void D3D_Rectangle(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
	GLfloat x[4], y[4];
	int i;

	if (y1 <= y0)
		return;

	CheckTriangleBuffer(4, 0, 0, 0, NULL, TRANSLUCENCY_GLOWING, -1, 0);

	x[0] = x0;
	x[0] =  (x[0] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[0] = y0;
	y[0] = -(y[0] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[1] = x1 - 1;
	x[1] =  (x[1] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[1] = y0;
	y[1] = -(y[1] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[2] = x1 - 1;
	x[2] =  (x[2] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[2] = y1 - 1;
	y[2] = -(y[2] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[3] = x0;
	x[3] =  (x[3] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[3] = y1 - 1;
	y[3] = -(y[3] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

	for (i = 0; i < 4; i++) {
		varrp->v[0] = x[i];
		varrp->v[1] = y[i];
		varrp->v[2] = -1.0f;
		varrp->v[3] = 1.0f;

		varrp->t[0] = 0.0f;
		varrp->t[1] = 0.0f;

		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

/* ** */

/* Homogeneous projection without divide-to-NDC/multiply-back round trips.
   The GPU receives one final float conversion per component. */
static void ProjectRenderPosition(GLfloat out[4],const RENDERVERTEX*v,int weapon,double nearZ,double offset,int depthMode){
 double x=weapon?v->ViewX:v->X,y=weapon?v->ViewY:v->Y,z=weapon?v->ViewZ:v->Z;
 out[0]=(GLfloat)(x*AvpProjectionX(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreX);
 out[1]=(GLfloat)(-y*AvpProjectionY(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreY);
 out[3]=(GLfloat)z;
 if(depthMode)out[2]=(GLfloat)(z*(depthMode==2?1.0:depthMode>0?.99999:-.99999));
 else out[2]=(GLfloat)(offset==0?z-2.0*nearZ:z-2.0*nearZ*z/(z+offset));
}

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int texoffset;
	D3DTexture *TextureHandle;
	int i;
	GLfloat ZNear;
	float RecipW, RecipH;
		
	ZNear = (GLfloat) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	if (texoffset) {
		TextureHandle = (void *)ImageHeaderArray[texoffset].D3DTexture;
		
		CurrTextureHandle = TextureHandle;
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
	RecipW = TextureHandle->RecipW / 65536.0f;
	RecipH = TextureHandle->RecipH / 65536.0f;

	CheckTriangleBuffer(RenderPolygon.NumberOfVertices, RenderPolygon.NumberOfVertices, 0, 0, TextureHandle, RenderPolygon.TranslucencyMode, -1, !ViewmodelShapeActive && (inputPolyPtr->PolyFlags&iflag_coplanar_trim) && RenderPolygon.TranslucencyMode==TRANSLUCENCY_OFF);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		GLfloat x, y, z;
		GLfloat s, t;
		GLfloat w = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z);
		GLfloat zvalue;
		
		s = TEXCOORD_FIXED(vertices->U, RecipW);
		t = TEXCOORD_FIXED(vertices->V, RecipH);

		x =  ((ViewmodelShapeActive ? vertices->ViewX : vertices->X)*((float)AvpProjectionX(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreX);
		y = -((ViewmodelShapeActive ? vertices->ViewY : vertices->Y)*((float)AvpProjectionY(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreY);
		
		zvalue = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)+HeadUpDisplayZOffset;
		z = 1.0f - 2.0f*ZNear/zvalue;
		
		ProjectRenderPosition(varrp->v,vertices,ViewmodelShapeActive,ZNear,HeadUpDisplayZOffset,0);
		
		varrp->t[0] = s;
		varrp->t[1] = t;
		
		varrp->c[0] = GammaValues[vertices->R];
		varrp->c[1] = GammaValues[vertices->G];
		varrp->c[2] = GammaValues[vertices->B];
		varrp->c[3] = vertices->A;
		
		varrp->s[0] = GammaValues[vertices->SpecularR];
		varrp->s[1] = GammaValues[vertices->SpecularG];
		varrp->s[2] = GammaValues[vertices->SpecularB];
		varrp->s[3] = vertices->A;
		
		varrp++;
		varrc++;
	}
}

void D3D_SkyPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int texoffset;
	D3DTexture *TextureHandle;
	int i;
	float RecipW, RecipH;

	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	TextureHandle = (void *)ImageHeaderArray[texoffset].D3DTexture;		
	CurrTextureHandle = TextureHandle;
	
	RecipW = TextureHandle->RecipW / 65536.0f;
	RecipH = TextureHandle->RecipH / 65536.0f;
	
	CheckTriangleBuffer(RenderPolygon.NumberOfVertices, 0, 0, 0, TextureHandle, RenderPolygon.TranslucencyMode, -1, 0);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		GLfloat x, y, z;
		GLfloat s, t;
		GLfloat w;
		
		w = (float)vertices->Z;
		
		s = TEXCOORD_FIXED(vertices->U, RecipW);
		t = TEXCOORD_FIXED(vertices->V, RecipH);

		x =  ((float)vertices->X*((float)AvpProjectionX(Global_VDB_Ptr)))/((float)vertices->Z*(float)ScreenDescriptorBlock.SDB_CentreX);
		y = -((float)vertices->Y*((float)AvpProjectionY(Global_VDB_Ptr)))/((float)vertices->Z*(float)ScreenDescriptorBlock.SDB_CentreY);

		z = 1.0f;

		ProjectRenderPosition(varrp->v,vertices,0,0,0,2);
		
		varrp->t[0] = s;
		varrp->t[1] = t;
		
		varrp->c[0] = vertices->R;
		varrp->c[1] = vertices->G;
		varrp->c[2] = vertices->B;
		varrp->c[3] = vertices->A;
		
		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

void D3D_ZBufferedCloakedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int flags;
	int texoffset;
	int i;
	D3DTexture *TextureHandle;
	
	float ZNear;
	float RecipW, RecipH;
	
	ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	flags = inputPolyPtr->PolyFlags;
	texoffset = (inputPolyPtr->PolyColour & ClrTxDefn);
	
	TextureHandle = ImageHeaderArray[texoffset].D3DTexture;
	CurrTextureHandle = TextureHandle;
	
	RecipW = TextureHandle->RecipW / 65536.0f;
	RecipH = TextureHandle->RecipH / 65536.0f;
	
	CheckTriangleBuffer(RenderPolygon.NumberOfVertices, 0, 0, 0, TextureHandle, TRANSLUCENCY_NORMAL, -1, 0);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		GLfloat x, y, z;
		GLfloat s, t;
		GLfloat w;
		GLfloat zvalue;
		
		w = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z);
		
		s = TEXCOORD_FIXED(vertices->U, RecipW);
		t = TEXCOORD_FIXED(vertices->V, RecipH);

		x =  ((ViewmodelShapeActive ? vertices->ViewX : vertices->X)*((float)AvpProjectionX(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreX);
		y = -((ViewmodelShapeActive ? vertices->ViewY : vertices->Y)*((float)AvpProjectionY(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreY);
		
		zvalue = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;

		ProjectRenderPosition(varrp->v,vertices,ViewmodelShapeActive,ZNear,HeadUpDisplayZOffset,0);
		
		varrp->t[0] = s;
		varrp->t[1] = t;
		
		varrp->c[0] = vertices->R;
		varrp->c[1] = vertices->G;
		varrp->c[2] = vertices->B;
		varrp->c[3] = vertices->A;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

void D3D_Decal_Output(DECAL *decalPtr, RENDERVERTEX *renderVerticesPtr)
{
	DECAL_DESC *decalDescPtr = &DecalDescription[decalPtr->DecalID];
	int texoffset;
	D3DTexture *TextureHandle;
	int i;
	
	float ZNear;
	float RecipW, RecipH;
	int r, g, b, a;
	
	ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

		
	if (decalPtr->DecalID == DECAL_FMV) {
		/* not (yet) implemented */
		return;
	} else if (decalPtr->DecalID == DECAL_SHAFTOFLIGHT||decalPtr->DecalID == DECAL_SHAFTOFLIGHT_OUTER) {
		TextureHandle = NULL;
		
		RecipW = 1.0 / 256.0; /* ignored */
		RecipH = 1.0 / 256.0;
	} else {
		texoffset = SpecialFXImageNumber;
		
		TextureHandle = ImageHeaderArray[texoffset].D3DTexture;
		
		RecipW = TextureHandle->RecipW / 65536.0f;
		RecipH = TextureHandle->RecipH / 65536.0f;
	}
	
	if (decalDescPtr->IsLit) {
		int intensity = LightIntensityAtPoint(decalPtr->Vertices);
		
		r = MUL_FIXED(intensity,decalDescPtr->RedScale[CurrentVisionMode]);
		g = MUL_FIXED(intensity,decalDescPtr->GreenScale[CurrentVisionMode]);
		b = MUL_FIXED(intensity,decalDescPtr->BlueScale[CurrentVisionMode]);
		a = decalDescPtr->Alpha;
	} else {
		r = decalDescPtr->RedScale[CurrentVisionMode];
		g = decalDescPtr->GreenScale[CurrentVisionMode];
		b = decalDescPtr->BlueScale[CurrentVisionMode];
		a = decalDescPtr->Alpha;
	}
	
	if (RAINBOWBLOOD_CHEATMODE) {
		r = FastRandom()&255;
		g = FastRandom()&255;
		b = FastRandom()&255;
		a = decalDescPtr->Alpha;
	}
	
	CheckTriangleBuffer(RenderPolygon.NumberOfVertices, 0, 0, 0, TextureHandle, decalDescPtr->TranslucencyType, -1, 0);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		GLfloat x, y, z, zvalue;
		GLfloat s, t;
		GLfloat w;
		
		w = (float)vertices->Z;
		
		x =  ((float)vertices->X*((float)AvpProjectionX(Global_VDB_Ptr)))/((float)vertices->Z*(float)ScreenDescriptorBlock.SDB_CentreX);
		y = -((float)vertices->Y*((float)AvpProjectionY(Global_VDB_Ptr)))/((float)vertices->Z*(float)ScreenDescriptorBlock.SDB_CentreY);
		
		s = TEXCOORD_FIXED(vertices->U, RecipW);
		t = TEXCOORD_FIXED(vertices->V, RecipH);
				
		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0f - 2.0f*ZNear/zvalue;

		ProjectRenderPosition(varrp->v,vertices,0,ZNear,HeadUpDisplayZOffset,0);
		
		varrp->t[0] = s;
		varrp->t[1] = t;
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

void D3D_Particle_Output(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
	PARTICLE_DESC *particleDescPtr = &ParticleDescription[particlePtr->ParticleID];
	int texoffset = SpecialFXImageNumber;
	GLfloat ZNear;
	int i;
	float RecipW, RecipH;
	int r, g, b, a;
    FireBatchVertex fireVertices[16];
    const int batchMode = particleDescPtr->TranslucencyType == TRANSLUCENCY_GLOWING ? 0 :
        particleDescPtr->TranslucencyType == TRANSLUCENCY_INVCOLOUR ? 1 :
        particleDescPtr->TranslucencyType == TRANSLUCENCY_NORMAL ? 2 : -1;
    const int fastFire = batchMode >= 0 && RenderPolygon.NumberOfVertices <= 16 && FireBatchAvailable();
	
	D3DTexture *TextureHandle;


	ZNear = (GLfloat) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);	
	
	TextureHandle = ImageHeaderArray[texoffset].D3DTexture;
	
	RecipW = TextureHandle->RecipW / 65536.0f;
	RecipH = TextureHandle->RecipH / 65536.0f;
	
	if (particleDescPtr->IsLit && !(particlePtr->ParticleID==PARTICLE_ALIEN_BLOOD && CurrentVisionMode==VISION_MODE_PRED_SEEALIENS) )
	{
		int intensity = LightIntensityAtPoint(&particlePtr->Position);
		
		if (particlePtr->ParticleID==PARTICLE_SMOKECLOUD || particlePtr->ParticleID==PARTICLE_ANDROID_BLOOD)
		{
			/* this should be OK. (ColourComponents was RGBA while RGBA_MAKE is BGRA (little endian) */
			r = (particlePtr->Colour >> 0)  & 0xFF;
			g = (particlePtr->Colour >> 8)  & 0xFF;
			b = (particlePtr->Colour >> 16) & 0xFF;
			a = (particlePtr->Colour >> 24) & 0xFF;
		} else {
			r = MUL_FIXED(intensity,particleDescPtr->RedScale[CurrentVisionMode]);
			g = MUL_FIXED(intensity,particleDescPtr->GreenScale[CurrentVisionMode]);
			b = MUL_FIXED(intensity,particleDescPtr->BlueScale[CurrentVisionMode]);
			a = particleDescPtr->Alpha;
		}
	} else {
		b = (particlePtr->Colour >> 0)  & 0xFF;
		g = (particlePtr->Colour >> 8)  & 0xFF;
		r = (particlePtr->Colour >> 16) & 0xFF;
		a = (particlePtr->Colour >> 24) & 0xFF;
	}
	if (RAINBOWBLOOD_CHEATMODE) {
		r = FastRandom()&255;
		g = FastRandom()&255;
		b = FastRandom()&255;
		a = particleDescPtr->Alpha;
	}

	if (fastFire) {
        if (!FireBatchPending()) {
            FlushTriangleBuffers(1);
            CheckBoundTextureIsCorrect(TextureHandle);
        }
    } else CheckTriangleBuffer(RenderPolygon.NumberOfVertices, 0, 0, 0, TextureHandle, particleDescPtr->TranslucencyType, -1, 0);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		GLfloat x, y, z;
		GLfloat s, t;
		GLfloat w = (float)(ViewmodelMuzzleActive ? vertices->ViewZ : vertices->Z);
		
		s = TEXCOORD_FIXED(vertices->U, RecipW);
		t = TEXCOORD_FIXED(vertices->V, RecipH);
		
		x =  ((float)(ViewmodelMuzzleActive ? vertices->ViewX : vertices->X)*((float)AvpProjectionX(Global_VDB_Ptr)))/((float)(ViewmodelMuzzleActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreX);
		y = -((float)(ViewmodelMuzzleActive ? vertices->ViewY : vertices->Y)*((float)AvpProjectionY(Global_VDB_Ptr)))/((float)(ViewmodelMuzzleActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreY);		
		
		if (particleDescPtr->IsDrawnInFront) {
			z = -0.99999f; /* ... */
		} else if (particleDescPtr->IsDrawnAtBack) {
			z = 0.99999f;
		} else {
			z = 1.0 - 2.0*ZNear/((float)(ViewmodelMuzzleActive ? vertices->ViewZ : vertices->Z)); /* currently maps [ZNear, inf) to [-1, 1], probably could be more precise with a ZFar */
		}

        if (fastFire) {
            FireBatchVertex *v=&fireVertices[i];
            ProjectRenderPosition(v->position,vertices,ViewmodelMuzzleActive,ZNear,0,particleDescPtr->IsDrawnInFront?-1:particleDescPtr->IsDrawnAtBack?1:0);
            v->uvMode[0]=s;v->uvMode[1]=t;v->uvMode[2]=(float)batchMode;
            v->colour[0]=r;v->colour[1]=g;v->colour[2]=b;v->colour[3]=a;
            continue;
        }
		ProjectRenderPosition(varrp->v,vertices,ViewmodelMuzzleActive,ZNear,0,particleDescPtr->IsDrawnInFront?-1:particleDescPtr->IsDrawnAtBack?1:0);
		
		varrp->t[0] = s;
		varrp->t[1] = t;
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
    if(fastFire)FireBatchAppend(fireVertices,RenderPolygon.NumberOfVertices,TextureHandle->id);
}

void D3D_PredatorThermalVisionPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	float ZNear;
	int i;
	ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	CheckTriangleBuffer(RenderPolygon.NumberOfVertices, 0, 0, 0, NULL, TRANSLUCENCY_OFF, -1, 0);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		GLfloat x, y, z;
		GLfloat w;
		GLfloat zvalue;
		
		x =  ((ViewmodelShapeActive ? vertices->ViewX : vertices->X)*((float)AvpProjectionX(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreX);
		y = -((ViewmodelShapeActive ? vertices->ViewY : vertices->Y)*((float)AvpProjectionY(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreY);
		
		zvalue = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;
		
		w = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z);

		ProjectRenderPosition(varrp->v,vertices,ViewmodelShapeActive,ZNear,HeadUpDisplayZOffset,0);
		
		varrp->c[0] = vertices->R;
		varrp->c[1] = vertices->G;
		varrp->c[2] = vertices->B;
		varrp->c[3] = vertices->A;
	
		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

void D3D_ZBufferedGouraudPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int flags, i;
	float ZNear;
	
	ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	flags = inputPolyPtr->PolyFlags;
	
	CheckTriangleBuffer(RenderPolygon.NumberOfVertices, 0, 0, 0, NULL, RenderPolygon.TranslucencyMode, -1, 0);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];	
		GLfloat x, y, z;
		GLfloat w;
		GLfloat zvalue;
				
		zvalue = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;
				
		w = (ViewmodelShapeActive ? vertices->ViewZ : vertices->Z);

		x =  ((ViewmodelShapeActive ? vertices->ViewX : vertices->X)*((float)AvpProjectionX(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreX);
		y = -((ViewmodelShapeActive ? vertices->ViewY : vertices->Y)*((float)AvpProjectionY(Global_VDB_Ptr)))/((ViewmodelShapeActive ? vertices->ViewZ : vertices->Z)*(float)ScreenDescriptorBlock.SDB_CentreY);	

		ProjectRenderPosition(varrp->v,vertices,ViewmodelShapeActive,ZNear,HeadUpDisplayZOffset,0);
		
		varrp->c[0] = vertices->R;
		varrp->c[1] = vertices->G;
		varrp->c[2] = vertices->B;
		if (flags & iflag_transparent)
			varrp->c[3] = vertices->A;
		else
			varrp->c[3] = 255;
		
		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

void D3D_PlayerOnFireOverlay()
{
	int c = 128;
	int colour = (FMVParticleColour&0xffffff)+(c<<24);
	GLfloat x[4], y[4], s[4], t[4];
	float u, v;
	int r, g, b, a;
	D3DTexture *TextureHandle;
	int i;

	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;
	
	TextureHandle = ImageHeaderArray[BurningImageNumber].D3DTexture;
	
	u = (FastRandom()&255)/256.0f;
	v = (FastRandom()&255)/256.0f;
	
	x[0] = -1.0f;
	y[0] = -1.0f;
	s[0] = u;
	t[0] = v;
	x[1] =  1.0f;
	y[1] = -1.0f;
	s[1] = u + 1.0f;
	t[1] = v;
	x[2] =  1.0f;
	y[2] =  1.0f;
	s[2] = u + 1.0f;
	t[2] = v + 1.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;
	s[3] = u;
	t[3] = v + 1.0f;
	
	CheckTriangleBuffer(4, 0, 0, 0, TextureHandle, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, 0);
	
	for (i = 0; i < 4; i++) {
		varrp->v[0] = x[i];
		varrp->v[1] = y[i];
		varrp->v[2] = -1.0f;
		varrp->v[3] = 1.0f;
		
		varrp->t[0] = s[i];
		varrp->t[1] = t[i];
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

void D3D_PlayerDamagedOverlay(int intensity)
{
	D3DTexture *TextureHandle;
	int theta[2];
	int colour, baseColour;
	int r, g, b, a;
	int i;
	int j;

	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;
	
	TextureHandle = ImageHeaderArray[SpecialFXImageNumber].D3DTexture;
	switch(AvP.PlayerType) {
		default:
			// LOCALASSERT(0);
		case I_Marine:
			baseColour = 0xff0000;
			break;
		case I_Alien:
			baseColour = 0xffff00;
			break;
		case I_Predator:
			baseColour = 0x00ff00;
			break;
	}

	for (i = 0; i < 2; i++) {
		GLfloat x[4], y[4], s[4], t[4];
		
		if (i == 0) {
			CheckTriangleBuffer(4, 0, 0, 0, TextureHandle, TRANSLUCENCY_INVCOLOUR, FILTERING_BILINEAR_ON, 0);

			colour = 0xffffff - baseColour + (intensity<<24);
			
			b = (colour >> 0)  & 0xFF;
			g = (colour >> 8)  & 0xFF;
			r = (colour >> 16) & 0xFF;
			a = (colour >> 24) & 0xFF;
		} else {
			CheckTriangleBuffer(4, 0, 0, 0, TextureHandle, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, 0);

			colour = baseColour + (intensity<<24);
			
			b = (colour >> 0)  & 0xFF;
			g = (colour >> 8)  & 0xFF;
			r = (colour >> 16) & 0xFF;
			a = (colour >> 24) & 0xFF;
		}

		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;	

		x[0] = -1.0f;
		y[0] = -1.0f;
		s[0] = 0.875f + (cos*(-1) - sin*(-1));
		t[0] = 0.375f + (sin*(-1) + cos*(-1));
		x[1] =  1.0f;
		y[1] = -1.0f;
		s[1] = 0.875f + (cos*(+1) - sin*(-1));
		t[1] = 0.375f + (sin*(+1) + cos*(-1));
		x[2] =  1.0f;
		y[2] =  1.0f;
		s[2] = 0.875f + (cos*(+1) - sin*(+1));
		t[2] = 0.375f + (sin*(+1) + cos*(+1));
		x[3] = -1.0f;
		y[3] =  1.0f;
		s[3] = 0.875f + (cos*(-1) - sin*(+1));
		t[3] = 0.375f + (sin*(-1) + cos*(+1));

		for (j = 0; j < 4; j++) {
			varrp->v[0] = x[j];
			varrp->v[1] = y[j];
			varrp->v[2] = -1.0f;
			varrp->v[3] = 1.0f;
			
			varrp->t[0] = s[j];
			varrp->t[1] = t[j];
			
			varrp->c[0] = r;
			varrp->c[1] = g;
			varrp->c[2] = b;
			varrp->c[3] = a;

			varrp->s[0] = 0;
			varrp->s[1] = 0;
			varrp->s[2] = 0;
			varrp->s[3] = 0;

			varrp++;
			varrc++;
		}
	}
}

void DrawNoiseOverlay(int tr)
{
	GLfloat x[4], y[4], s[4], t[4], u, v;
	float uSpan, vSpan, noiseScale;
	int r, g, b;
	D3DTexture *tex;
	int j;

	r = 255;
	g = 255;
	b = 255;

	tex = ImageHeaderArray[StaticImageNumber].D3DTexture;

	/* The original overlay sampled one 256x256 noise tile over a 640x480
	 * framebuffer.  Preserve that physical grain density, rather than
	 * magnifying its texels as the selected video mode becomes larger. */
	noiseScale = (NoiseOverlayScaleMode == NOISE_OVERLAY_SCALE_HALF) ? 0.5f : 1.0f;
	uSpan = 256.0f * ((float)ScreenDescriptorBlock.SDB_Width / 640.0f) / noiseScale;
	vSpan = 256.0f * ((float)ScreenDescriptorBlock.SDB_Height / 480.0f) / noiseScale;

	u = FastRandom()&255;
	v = FastRandom()&255;

	x[0] = -1.0f;
	y[0] = -1.0f;
	s[0] = u / 256.0f;
	t[0] = v / 256.0f;
	x[1] =  1.0f;
	y[1] = -1.0f;
	s[1] = (u + uSpan) / 256.0f;
	t[1] = v / 256.0f;
	x[2] =  1.0f;
	y[2] =  1.0f;
	s[2] = (u + uSpan) / 256.0f;
	t[2] = (v + vSpan) / 256.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;
	s[3] = u / 256.0f;
	t[3] = (v + vSpan) / 256.0f;

	// changing the depth func manually, so flush now
	FlushTriangleBuffers(0);
	CheckTriangleBuffer(4, 0, 0, 0, tex, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, 0);

	for (j = 0; j < 4; j++) {
		varrp->v[0] = x[j];
		varrp->v[1] = y[j];
		varrp->v[2] = 1.0f;
		varrp->v[3] = 1.0f;

		varrp->t[0] = s[j];
		varrp->t[1] = t[j];

		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = tr;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}

	pglDepthFunc(GL_ALWAYS);
	FlushTriangleBuffers(0);
	pglDepthFunc(GL_LEQUAL);
}

void D3D_ScreenInversionOverlay()
{
	D3DTexture *tex;
	int theta[2];
	int i;
	int j;

	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;
		
	tex = ImageHeaderArray[SpecialFXImageNumber].D3DTexture;

	for (i = 0; i < 2; i++) {
		GLfloat x[4], y[4], s[4], t[4];
		
		if (i == 0) {
			CheckTriangleBuffer(4, 0, 0, 0, tex, TRANSLUCENCY_DARKENINGCOLOUR, FILTERING_BILINEAR_ON, 0);
		} else {
			CheckTriangleBuffer(4, 0, 0, 0, tex, TRANSLUCENCY_COLOUR, FILTERING_BILINEAR_ON, 0);
		}

		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;
		
		x[0] = -1.0f;
		y[0] = -1.0f;
		s[0] = 0.375f + (cos*(-1) - sin*(-1));
		t[0] = 0.375f + (sin*(-1) + cos*(-1));
		x[1] =  1.0f;
		y[1] = -1.0f;
		s[1] = 0.375f + (cos*(+1) - sin*(-1));
		t[1] = 0.375f + (sin*(+1) + cos*(-1));
		x[2] =  1.0f;
		y[2] =  1.0f;
		s[2] = 0.375f + (cos*(+1) - sin*(+1));
		t[2] = 0.375f + (sin*(+1) + cos*(+1));
		x[3] = -1.0f;
		y[3] =  1.0f;
		s[3] = 0.375f + (cos*(-1) - sin*(+1));
		t[3] = 0.375f + (sin*(-1) + cos*(+1));

		for (j = 0; j < 4; j++) {
			varrp->v[0] = x[j];
			varrp->v[1] = y[j];
			varrp->v[2] = -1.0f;
			varrp->v[3] = 1.0f;
			
			varrp->t[0] = s[j];
			varrp->t[1] = t[j];
			
			varrp->c[0] = 255;
			varrp->c[1] = 255;
			varrp->c[2] = 255;
			varrp->c[3] = 255;

			varrp->s[0] = 0;
			varrp->s[1] = 0;
			varrp->s[2] = 0;
			varrp->s[3] = 0;

			varrp++;
			varrc++;
		}
	}
}

void D3D_PredatorScreenInversionOverlay()
{
	int j;

	// changing the depth func manually, so flush now
	FlushTriangleBuffers(0);
	CheckTriangleBuffer(4, 0, 0, 0, NULL, TRANSLUCENCY_DARKENINGCOLOUR, -1, 0);
	
	for (j = 0; j < 4; j++) {

		switch (j) {
			case 0:
				varrp->v[0] = -1.0f;
				varrp->v[1] = -1.0f;
				break;
			case 1:
				varrp->v[0] =  1.0f;
				varrp->v[1] = -1.0f;
				break;
			case 2:
				varrp->v[0] =  1.0f;
				varrp->v[1] =  1.0f;
				break;
			case 3:
				varrp->v[0] = -1.0f;
				varrp->v[1] =  1.0f;
				break;
		}

		varrp->v[2] = 1.0f;
		varrp->v[3] = 1.0f;
		
		varrp->t[0] = 0.0f;
		varrp->t[1] = 0.0f;
		
		varrp->c[0] = 255;
		varrp->c[1] = 255;
		varrp->c[2] = 255;
		varrp->c[3] = 255;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}

	pglDepthFunc(GL_ALWAYS);
	FlushTriangleBuffers(0);
	pglDepthFunc(GL_LEQUAL);
}

void DrawScanlinesOverlay(float level)
{
	D3DTexture *tex;
	GLfloat x[4], y[4], s[4], t[4];
	float v, size, verticalScale, noiseScale;
	int c;
	int a;
	int j;

	tex = ImageHeaderArray[PredatorNumbersImageNumber].D3DTexture;

	c = 255;
	a = 64.0f+level*64.0f;
	
	v = 128.0f;
	size = 128.0f*(1.0f-level*0.8f);

	/* Scanline spacing was authored against a 480-line framebuffer.  Tile the
	 * source band vertically with the real framebuffer height so each line
	 * keeps its intended raster density at modern resolutions. */
	verticalScale = (float)ScreenDescriptorBlock.SDB_Height / 480.0f;
	noiseScale = (NoiseOverlayScaleMode == NOISE_OVERLAY_SCALE_HALF) ? 0.5f : 1.0f;
	size *= verticalScale / noiseScale;

	x[0] = -1.0f;
	y[0] = -1.0f;
	s[0] = (v - size) / 256.0f;
	t[0] = 1.0f;
	x[1] =  1.0f;
	y[1] = -1.0f;
	s[1] = (v - size) / 256.0f;
	t[1] = 1.0f;
	x[2] =  1.0f;
	y[2] =  1.0f;
	s[2] = (v + size) / 256.0f;
	t[2] = 1.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;
	s[3] = (v + size) / 256.0f;
	t[3] = 1.0f;
	
	// changing the depth func manually, so flush now
	FlushTriangleBuffers(0);
	CheckTriangleBuffer(4, 0, 0, 0, tex, TRANSLUCENCY_NORMAL, FILTERING_BILINEAR_ON, 0);

	for (j = 0; j < 4; j++) {
		varrp->v[0] = x[j];
		varrp->v[1] = y[j];
		varrp->v[2] = 1.0f;
		varrp->v[3] = 1.0f;
		
		varrp->t[0] = s[j];
		varrp->t[1] = t[j];
		
		varrp->c[0] = c;
		varrp->c[1] = c;
		varrp->c[2] = c;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}

	pglDepthFunc(GL_ALWAYS);
	FlushTriangleBuffers(0);
	pglDepthFunc(GL_LEQUAL);
}

void D3D_FadeDownScreen(int brightness, int colour)
{
	int t, r, g, b, a;
	GLfloat x[4], y[4];
	int i;

	t = 255 - (brightness>>8);
	if (t<0) t = 0;
	colour = (t<<24)+colour;
	
	CheckTriangleBuffer(4, 0, 0, 0, NULL, TRANSLUCENCY_NORMAL, -1, 0);
	
	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;

	x[0] = -1.0f;
	y[0] = -1.0f;
	x[1] =  1.0f;
	y[1] = -1.0f;
	x[2] =  1.0f;
	y[2] =  1.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;

	for (i = 0; i < 4; i++) {
		varrp->v[0] = x[i];
		varrp->v[1] = y[i];
		varrp->v[2] = -1.0f;
		varrp->v[3] = 1.0f;
		
		varrp->t[0] = 0.0f;
		varrp->t[1] = 0.0f;
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

static enum FILTERING_MODE_ID HUDTextureFilteringMode(void)
{
	return (HUDTextureFilterMode == HUD_TEXTURE_FILTER_NEAREST)
		? FILTERING_BILINEAR_OFF : FILTERING_BILINEAR_ON;
}

void D3D_HUD_Setup()
{
	FlushTriangleBuffers(1);
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	
	pglDepthFunc(GL_LEQUAL);	
}

/* Scoped only to help-text outlines. The luminance of the existing font
 * darkens the destination, keeping its antialiased edge and transparent cell.
 * Black in the additive HUD path would be invisible. */
static int HUDTextOutlinePass;
static int HUDQuickLoadText;
static int HUDTextIsolateCells;
void D3D_HUDPolygonFloat_Output(int imageNumber, const struct HUDVertexF *quadVerticesPtr, int count, unsigned int colour)
{
	if(count<3 || count>5)return;
	float RecipW, RecipH;
	int i;
	D3DTexture *tex = ImageHeaderArray[imageNumber].D3DTexture;
	GLfloat x, y, s, t;
	int r, g, b, a;

	/* Asset restoration failures must be diagnosable, never an access
	 * violation in the HUD path. */
	if (tex == NULL) {
		static int lastMissingHudImage = -1;
		if (lastMissingHudImage != imageNumber)
		{
			fprintf(stderr, "OGL_HUD_TEXTURE_MISSING image=%d name=%s (reported once until recovery)\n", imageNumber, ImageHeaderArray[imageNumber].ImageName);
			lastMissingHudImage = imageNumber;
		}
		return;
	}


/* possibly use polygon offset? (predator hud) */

	/* The HUD owns its sampling state.  This prevents numeric/text paths from
	   inheriting whichever filter the preceding world or HUD draw selected. */
	CheckTriangleBuffer(count, 0, 0, 0, tex, HUDTextOutlinePass ? TRANSLUCENCY_INVCOLOUR : TRANSLUCENCY_GLOWING, HUDTextureFilteringMode(), 0);
	
	RecipW = tex->RecipW / 65536.0f;
	RecipH = tex->RecipH / 65536.0f;
	
	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;

	{
		int hudAnchored = HUD_AnchorIsActive();
		int layoutWidth = 0, layoutHeight = 0, hudScale = ONE_FIXED;
		int anchorX = 0, anchorY = 0;
		if (hudAnchored) {
			float minX = quadVerticesPtr[0].X, maxX = quadVerticesPtr[0].X;
			float minY = quadVerticesPtr[0].Y, maxY = quadVerticesPtr[0].Y;
			layoutWidth = HUD_GetLayoutWidth();
			layoutHeight = HUD_GetLayoutHeight();
			hudScale = HUD_GetUserScaleFixed();
			for (i = 1; i < count; i++) {
				if (quadVerticesPtr[i].X < minX) minX = quadVerticesPtr[i].X;
				if (quadVerticesPtr[i].X > maxX) maxX = quadVerticesPtr[i].X;
				if (quadVerticesPtr[i].Y < minY) minY = quadVerticesPtr[i].Y;
				if (quadVerticesPtr[i].Y > maxY) maxY = quadVerticesPtr[i].Y;
			}
			/* Preserve the nearest local HUD edge.  Middle HUD material remains
			 * centred; corner blocks remain framed by their selected HUD canvas. */
			switch (HUD_GetScaleAnchor()) {
				case HUD_SCALE_ANCHOR_TOP_LEFT: anchorX = 0; anchorY = 0; break;
				case HUD_SCALE_ANCHOR_TOP_RIGHT: anchorX = layoutWidth; anchorY = 0; break;
				case HUD_SCALE_ANCHOR_BOTTOM_LEFT: anchorX = 0; anchorY = layoutHeight; break;
				case HUD_SCALE_ANCHOR_BOTTOM_RIGHT: anchorX = layoutWidth; anchorY = layoutHeight; break;
				case HUD_SCALE_ANCHOR_CENTRE: anchorX = layoutWidth / 2; anchorY = layoutHeight / 2; break;
				case HUD_SCALE_ANCHOR_AUTO:
				default:
					if ((minX + maxX) < layoutWidth / 2) anchorX = 0;
					else if ((minX + maxX) > (layoutWidth * 3) / 2) anchorX = layoutWidth;
					else anchorX = layoutWidth / 2;
					if ((minY + maxY) < layoutHeight / 2) anchorY = 0;
					else if ((minY + maxY) > (layoutHeight * 3) / 2) anchorY = layoutHeight;
					else anchorY = layoutHeight / 2;
					break;
			}
		}
		for (i = 0; i < count; i++) {
            float hudW=1.0f;
			if (hudAnchored) {
				x = anchorX + (hudScale/(float)ONE_FIXED)*(quadVerticesPtr[i].X - anchorX) + HUD_GetLayoutOffsetX();
				y = anchorY + (hudScale/(float)ONE_FIXED)*(quadVerticesPtr[i].Y - anchorY) + HUD_GetLayoutOffsetY();
			}
			else {
				x = quadVerticesPtr[i].X;
				y = quadVerticesPtr[i].Y;
			}
        s = (float)quadVerticesPtr[i].U;
        t = (float)quadVerticesPtr[i].V;
        if ((HUDTextIsolateCells || D3D_IsFPSCounterTextRenderActive() || D3D_IsBriefingOrConsoleTextRenderActive() || D3D_IsInGameMenuTextRenderActive() || D3D_IsStatisticsTextRenderActive() || hudAnchored) && imageNumber == AAFontImageNumber)
        {
            /* The old 17x17 quad straddled 16x16 cells. Bound samples to texel
               centres inside this cell, adjusting geometry by the same amount:
               no neighbouring glyphs, no stretching of the letter itself. */
            float u0=(float)quadVerticesPtr[0].U, v0=(float)quadVerticesPtr[0].V;
            float du=(float)(quadVerticesPtr[1].U-quadVerticesPtr[0].U);
            float dv=(float)(quadVerticesPtr[3].V-quadVerticesPtr[0].V);
            float cleanU=(i==0 || i==3)?u0+0.5f:u0+15.5f;
            float cleanV=(i<2)?v0+0.5f:v0+15.5f;
            if (D3D_IsBriefingOrConsoleTextRenderActive())
            {
                /* A console line can already be clipped at the screen top.
                 * Recover its atlas cell from the bottom edge; never expand
                 * that clipped interval back into a complete glyph. */
                float cellU=floorf(u0/16.0f)*16.0f;
                float cellV=floorf(((float)quadVerticesPtr[3].V-2.0f)/16.0f)*16.0f;
                cleanU=fminf(fmaxf(s,cellU+0.5f),cellU+15.5f);
                cleanV=fminf(fmaxf(t,cellV+0.5f),cellV+15.5f);
            }
            float ux=du?(quadVerticesPtr[1].X-quadVerticesPtr[0].X)/du:0;
            float uy=du?(quadVerticesPtr[1].Y-quadVerticesPtr[0].Y)/du:0;
            float vx=dv?(quadVerticesPtr[3].X-quadVerticesPtr[0].X)/dv:0;
            float vy=dv?(quadVerticesPtr[3].Y-quadVerticesPtr[0].Y)/dv:0;
            float scale=hudAnchored?hudScale/(float)ONE_FIXED:1.0f;
            x+=((cleanU-s)*ux+(cleanV-t)*vx)*scale;
            y+=((cleanU-s)*uy+(cleanV-t)*vy)*scale;
            s=cleanU; t=cleanV;
        }
        if(hudAnchored)hudW=MarineHUDMotionTransform(&x,&y);
        x=(x-ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
        y=-(y-ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
        s*=65536.0f*RecipW;
        t*=65536.0f*RecipH;

		varrp->v[0] = x*hudW;
		varrp->v[1] = y*hudW;
		varrp->v[2] = -hudW;
		varrp->v[3] = hudW;
		
		varrp->t[0] = s;
		varrp->t[1] = t;

		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
		}
	}
}
void D3D_HUDQuad_Output(int imageNumber, struct VertexTag *vertices, unsigned int colour)
{
 struct HUDVertexF v[4];
 for(int i=0;i<4;i++){v[i].X=vertices[i].X;v[i].Y=vertices[i].Y;v[i].U=vertices[i].U;v[i].V=vertices[i].V;}
 D3D_HUDPolygonFloat_Output(imageNumber,v,4,colour);
}


void D3D_RenderHUDNumber_Centred(unsigned int number,int x,int y,int colour)
{
	struct VertexTag quadVertices[4];
	int noOfDigits=3;
	for(unsigned int n=number/1000;n;n/=10)++noOfDigits;
	int h = MUL_FIXED(HUDScaleFactor,HUD_DIGITAL_NUMBERS_HEIGHT);
	int w = MUL_FIXED(HUDScaleFactor,HUD_DIGITAL_NUMBERS_WIDTH);

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + h;
	quadVertices[3].Y = y + h;
	
	x += (noOfDigits*w)/2;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	
	do {
		int topLeftU, topLeftV;
		
		int digit = number%10;
		number/=10;
		
		if (digit<8) {
			topLeftU = 1+(digit)*16;
			topLeftV = 1;
		} else {
			topLeftU = 1+(digit-8)*16;
			topLeftV = 1+24;
		}
		if (AvP.PlayerType == I_Marine) topLeftV+=80;
		
		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + HUD_DIGITAL_NUMBERS_WIDTH;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + HUD_DIGITAL_NUMBERS_WIDTH;
		quadVertices[2].V = topLeftV + HUD_DIGITAL_NUMBERS_HEIGHT;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + HUD_DIGITAL_NUMBERS_HEIGHT;
		
		x -= 1+w;
		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + w;
		quadVertices[2].X = x + w;
		
		D3D_HUDQuad_Output(HUDFontsImageNumber, quadVertices, colour);
		
	} while (--noOfDigits);
}

void D3D_DrawSolidRect(int left,int top,int right,int bottom,unsigned int colour,int additive)
{
 int i;const int xs[4]={left,right,right,left},ys[4]={top,top,bottom,bottom};
 if(right<=left||bottom<=top)return;
 CheckTriangleBuffer(4,0,0,0,NULL,additive?TRANSLUCENCY_GLOWING:TRANSLUCENCY_NORMAL,-1, 0);
 for(i=0;i<4;i++){
  varrp->v[0]=2.0f*xs[i]/ScreenDescriptorBlock.SDB_Width-1.0f;
  varrp->v[1]=1.0f-2.0f*ys[i]/ScreenDescriptorBlock.SDB_Height;
  varrp->v[2]=-1.0f;varrp->v[3]=1.0f;varrp->t[0]=varrp->t[1]=0;
  varrp->c[0]=(colour>>16)&255;varrp->c[1]=(colour>>8)&255;varrp->c[2]=colour&255;varrp->c[3]=colour>>24;
  varrp->s[0]=varrp->s[1]=varrp->s[2]=varrp->s[3]=0;varrp++;varrc++;
 }
}
/* Map the mark through the same UV-to-screen transform as its base glyph.
 * This also preserves vertical HUD text and partially clipped console rows. */
static void D3D_HUDQuad_OutputAccent(int imageNumber, struct VertexTag *quadVertices, unsigned char base, unsigned char accent, int colour)
{
	int i;
	int deltaU;
	int deltaV;

	if (!accent) return;
	deltaU = (((accent - 32) & 15) - ((base - 32) & 15)) * 16;
	deltaV = (((accent - 32) >> 4) - ((base - 32) >> 4)) * 16;
	for (i = 0; i < 4; ++i)
	{
		quadVertices[i].U += deltaU;
		quadVertices[i].V += deltaV;
	}
	D3D_HUDQuad_Output(imageNumber, quadVertices, colour);
}
void D3D_RenderHUDString(char *stringPtr,int x,int y,int colour)
{
	const int textOriginX = x;
	const int textOriginY = y;
	const int textScaleFixed = D3D_GetActiveConsoleTextScaleFixed();
	/* Statistics retain their established columns, but expand rows from the
	   top baseline so scaled lines never overlap. */
	if (D3D_IsStatisticsTextRenderActive())
	{
		y = 20 + MUL_FIXED(y - 20, textScaleFixed);
	}
	struct VertexTag quadVertices[4];

	if (stringPtr == NULL)
	{
		return;
	}

	quadVertices[0].Y = y-MUL_FIXED(1, textScaleFixed);
	quadVertices[1].Y = y-MUL_FIXED(1, textScaleFixed);
	quadVertices[2].Y = y + MUL_FIXED(HUD_FONT_HEIGHT + 1, textScaleFixed);
	quadVertices[3].Y = y + MUL_FIXED(HUD_FONT_HEIGHT + 1, textScaleFixed);
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);

	while( *stringPtr )
	{
		unsigned char accent;
		unsigned char c = AvPTextNextGlyphWithAccent(&stringPtr, &accent);

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[1].V = topLeftV - 1;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;
			
			quadVertices[0].X = x - MUL_FIXED(1, textScaleFixed);
			quadVertices[3].X = x - MUL_FIXED(1, textScaleFixed);
			quadVertices[1].X = x + MUL_FIXED(HUD_FONT_WIDTH + 1, textScaleFixed);
			quadVertices[2].X = x + MUL_FIXED(HUD_FONT_WIDTH + 1, textScaleFixed);
			D3D_HUDQuad_Output(AAFontImageNumber, quadVertices, colour);
			D3D_HUDQuad_OutputAccent(AAFontImageNumber, quadVertices, c, accent, colour);
		}
		x += MUL_FIXED(AAFontWidths[(unsigned char)c], textScaleFixed);
	}
}

static void D3D_RenderQuickLoadString(char *text,int x,int y,unsigned int colour)
{
 int saved=HUDTextOutlinePass,savedCells=HUDTextIsolateCells;
 unsigned int a=colour>>24;
 HUDTextIsolateCells=1;
 /* Same glyph at the same coordinates: attenuate by coverage, then add the foreground colour.
  * This implements over compositing without an outline or a different atlas. */
 HUDTextOutlinePass=1;
 D3D_RenderHUDString(text,x,y,(a<<24)|(a<<16)|(a<<8)|a);
 HUDTextOutlinePass=0;
 D3D_RenderHUDString(text,x,y,colour);
 HUDTextOutlinePass=saved;
 HUDTextIsolateCells=savedCells;
}

void D3D_RenderHUDStringOutlined(char *text,int x,int y,int colour,int radius)
{
 int dx,dy;
 int saved=HUDTextOutlinePass;
 int savedCells=HUDTextIsolateCells;
 HUDTextIsolateCells=1;
 if(radius<1)radius=1;
 if(radius>3)radius=3;
 HUDTextOutlinePass=1;
 /* Complete the contour before the foreground, so adjacent glyphs cannot
  * darken each other. INVCOLOUR multiplies the background by (1-mask). */
 for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++)
  if(dx || dy)D3D_RenderHUDString(text,x+dx*radius,y+dy*radius,0xffffffff);
 HUDTextOutlinePass=saved;
 D3D_RenderHUDString(text,x,y,colour);
 HUDTextIsolateCells=savedCells;
}

void D3D_RenderConsoleVersion(int left,int top,int right,int bottom,int alpha)
{
 char label[]=TYRARGO_CONSOLE_VERSION;
 int scale=D3D_GetActiveConsoleTextScaleFixed();
 int width=MUL_FIXED(AvPTextStringWidth(label,AAFontWidths),scale);
 int pad=MUL_FIXED(3,scale),x=right-pad-width,y=top+pad;
 if(x<left+pad || y+MUL_FIXED(HUD_FONT_HEIGHT+2,scale)>bottom)return;
 /* Subdued blue core and a much lighter halo; both share the opening fade. */
 if(alpha<=0)return;
 unsigned int core=((unsigned int)MUL_FIXED(210,alpha)<<24)|0x58b8e8u;
 unsigned int halo=((unsigned int)MUL_FIXED(20,alpha)<<24)|0x0050c0u;
 D3D_RenderHUDString(label,x-1,y,halo);
 D3D_RenderHUDString(label,x+1,y,halo);
 D3D_RenderHUDString(label,x,y-1,halo);
 D3D_RenderHUDString(label,x,y+1,halo);
 D3D_RenderHUDString(label,x,y,core);
}
void D3D_RenderHUDString_Clipped(char *stringPtr,int x,int y,int colour)
{
	const int textOriginX = x;
	const int textScaleFixed = D3D_GetActiveConsoleTextScaleFixed();
	struct VertexTag quadVertices[4];

// 	LOCALASSERT(y<=0);
	if (stringPtr == NULL)
	{
		return;
	}

	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);

	quadVertices[2].Y = y + MUL_FIXED(HUD_FONT_HEIGHT + 1, textScaleFixed);
	quadVertices[3].Y = y + MUL_FIXED(HUD_FONT_HEIGHT + 1, textScaleFixed);
	
	quadVertices[0].Y = 0;
	quadVertices[1].Y = 0;

	while ( *stringPtr )
	{
		unsigned char accent;
		unsigned char c = AvPTextNextGlyphWithAccent(&stringPtr, &accent);

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - ((y << 16) / textScaleFixed);
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH+1;
			quadVertices[1].V = topLeftV - ((y << 16) / textScaleFixed);
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH+1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT+1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT+1;
			
			quadVertices[0].X = x - MUL_FIXED(1, textScaleFixed);
			quadVertices[3].X = x - MUL_FIXED(1, textScaleFixed);
			quadVertices[1].X = x + MUL_FIXED(HUD_FONT_WIDTH + 1, textScaleFixed);
			quadVertices[2].X = x + MUL_FIXED(HUD_FONT_WIDTH + 1, textScaleFixed);
			D3D_HUDQuad_Output(AAFontImageNumber, quadVertices, colour);
			D3D_HUDQuad_OutputAccent(AAFontImageNumber, quadVertices, c, accent, colour);
		}
		x += MUL_FIXED(AAFontWidths[(unsigned char)c], textScaleFixed);
	}
}

/* Glyph quads overlap because advance widths are smaller than atlas cells.
 * With projective shake, z=-w can round to slightly different window depths
 * across those overlapping quads. Even black additive texels used to write
 * depth and reject the next letter. Draw the label as an overlay, then restore
 * both depth states. Flush on each boundary so queued world/HUD geometry
 * never inherits the label's policy. No change to sampling, motion or MSAA. */
typedef struct HUD_LABEL_DEPTH {
 GLint function,writeMask;
} HUD_LABEL_DEPTH;
static HUD_LABEL_DEPTH HUDLabelDepthBegin(void)
{
 HUD_LABEL_DEPTH previous;
 FlushTriangleBuffers(1);
 pglGetIntegerv(GL_DEPTH_FUNC,&previous.function);
 pglGetIntegerv(GL_DEPTH_WRITEMASK,&previous.writeMask);
 pglDepthFunc(GL_ALWAYS);
 pglDepthMask(GL_FALSE);
 return previous;
}
static void HUDLabelDepthEnd(HUD_LABEL_DEPTH previous)
{
 FlushTriangleBuffers(1);
 pglDepthFunc(previous.function);
 pglDepthMask(previous.writeMask!=0);
}

void D3D_RenderHUDString_Centred(char *stringPtr, int centreX, int y, int colour)
{
	float x, length = 0;
 const float scale=HUDScaleFactor/(float)ONE_FIXED;
	char *ptr = stringPtr;
	struct HUDVertexF quadVertices[4];

	if (stringPtr == NULL)
	{
		return;
	}
	
 HUD_LABEL_DEPTH previousDepth=HUDLabelDepthBegin();
	length = AvPTextStringWidth(ptr, AAFontWidths);
	length = scale*length;

	x = centreX-length/2;

	quadVertices[0].Y = y-(scale*(1));
	quadVertices[1].Y = y-(scale*(1));
	quadVertices[2].Y = y + (scale*(HUD_FONT_HEIGHT + 1));
	quadVertices[3].Y = y + (scale*(HUD_FONT_HEIGHT + 1));
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);

	while( *stringPtr )
	{
		unsigned char accent;
		unsigned char c = AvPTextNextGlyphWithAccent(&stringPtr, &accent);

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[1].V = topLeftV - 1;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;

			quadVertices[0].X = x - (scale*(1));
			quadVertices[3].X = x - (scale*(1));
			quadVertices[1].X = x + (scale*(HUD_FONT_WIDTH + 1));
			quadVertices[2].X = x + (scale*(HUD_FONT_WIDTH + 1));
				
			D3D_HUDPolygonFloat_Output(AAFontImageNumber, quadVertices, 4, colour);
			if(accent){
                float du=(((accent-32)&15)-((c-32)&15))*16;
                float dv=(((accent-32)>>4)-((c-32)>>4))*16;
                for(int i=0;i<4;i++){quadVertices[i].U+=du;quadVertices[i].V+=dv;}
                D3D_HUDPolygonFloat_Output(AAFontImageNumber,quadVertices,4,colour);
            }
		}
		x += scale*AAFontWidths[(unsigned char)c];
	}
 HUDLabelDepthEnd(previousDepth);
}

void RenderString(char *stringPtr, int x, int y, int colour)
{
	D3D_RenderHUDString(stringPtr, x, y, colour);
}

void RenderStringCentred(char *stringPtr, int centreX, int y, int colour)
{
	int length = 0;
	char *ptr = stringPtr;

	length = MUL_FIXED(AvPTextStringWidth(ptr, AAFontWidths), D3D_GetActiveConsoleTextScaleFixed());
	D3D_RenderHUDString(stringPtr, centreX-length/2, y, colour);
}

void RenderStringVertically(char *stringPtr, int centreX, int bottomY, int colour)
{
	struct VertexTag quadVertices[4];
	int y = bottomY;

	quadVertices[0].X = centreX - (HUD_FONT_HEIGHT/2) - 1;
	quadVertices[1].X = quadVertices[0].X;
	quadVertices[2].X = quadVertices[0].X+2+HUD_FONT_HEIGHT*1;
	quadVertices[3].X = quadVertices[2].X;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	while( *stringPtr )
	{
		unsigned char accent;
		unsigned char c = AvPTextNextGlyphWithAccent(&stringPtr, &accent);

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH;
			quadVertices[1].V = topLeftV - 1;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;

			quadVertices[0].Y = y ;
			quadVertices[1].Y = y - HUD_FONT_WIDTH*1 -1;
			quadVertices[2].Y = y - HUD_FONT_WIDTH*1 -1;
			quadVertices[3].Y = y ;
				
			D3D_HUDQuad_Output(AAFontImageNumber, quadVertices, colour);
			D3D_HUDQuad_OutputAccent(AAFontImageNumber, quadVertices, c, accent, colour);
		}
	   	y -= AAFontWidths[(unsigned char)c];
	}
}

int Hardware_RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format) 
{
	int anchorX = x;
	switch(format)
	{
		default:
			fprintf(stderr, "Hardware_RenderSmallMenuText: UNKNOWN TEXT FORMAT\n");
			exit(EXIT_FAILURE);
//		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			/* The HUD renderer scales glyph quads for console, briefing and
			   in-game menu scopes.  Justification must use that same width,
			   otherwise scaled right-aligned labels cross into slider columns. */
			length = MUL_FIXED(AvPTextStringWidth(ptr, AAFontWidths),
			                   D3D_GetActiveConsoleTextScaleFixed());

			x -= length;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			length = MUL_FIXED(AvPTextStringWidth(ptr, AAFontWidths),
			                   D3D_GetActiveConsoleTextScaleFixed());

			x -= length/2;
			break;
		}	
	}

//	LOCALASSERT(x>0);

	{
		unsigned int colour = alpha>>8;
		if (colour>255) colour = 255;
		colour = (colour<<24)+0xffffff;
		D3D_RenderHUDString(textPtr, x, y, colour);
	}
	return x;
}

int Hardware_RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue)
{
	int anchorX = x;
	switch(format)
	{
		default:
//		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			fprintf(stderr, "Hardware_RenderSmallMenuText_Coloured: UNKNOWN TEXT FORMAT\n");
			exit(EXIT_FAILURE);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			/* The HUD renderer scales glyph quads for console, briefing and
			   in-game menu scopes.  Justification must use that same width,
			   otherwise scaled right-aligned labels cross into slider columns. */
			length = MUL_FIXED(AvPTextStringWidth(ptr, AAFontWidths),
			                   D3D_GetActiveConsoleTextScaleFixed());

			x -= length;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			length = MUL_FIXED(AvPTextStringWidth(ptr, AAFontWidths),
			                   D3D_GetActiveConsoleTextScaleFixed());

			x -= length/2;
			break;
		}	
	}

//	LOCALASSERT(x>0);

	{
		unsigned int colour = alpha>>8;
		if (colour>255) colour = 255;
		colour = (colour<<24);
		colour += MUL_FIXED(red,255)<<16;
		colour += MUL_FIXED(green,255)<<8;
		colour += MUL_FIXED(blue,255);
		if(HUDQuickLoadText)D3D_RenderQuickLoadString(textPtr,x,y,colour);
        else D3D_RenderHUDString(textPtr, x, y, colour);
	}
	return x;
}

int Hardware_RenderSmallMenuText_QuickLoad(char *text,int x,int y,int alpha,enum AVPMENUFORMAT_ID format)
{
 int saved=HUDQuickLoadText;HUDQuickLoadText=1;
 int result=Hardware_RenderSmallMenuText_Coloured(text,x,y,alpha,format,178*ONE_FIXED/255,24*ONE_FIXED/255,240*ONE_FIXED/255);
 HUDQuickLoadText=saved;
 return result;
}

/* The caller derives the top edge from the real lower edge of the two
   configuration options, then supplies the remaining safe area. */
void Hardware_RenderKeyConfigRectangle(int alpha, int topY, int bottomY)
{
	extern void D3D_DrawRectangle(int x, int y, int w, int h, int alpha);
	int textScope = D3D_IsInGameMenuTextRenderActive();
	if (bottomY <= topY) return;
	/* The frame shares AAFont but is not a 16x16 glyph cell. */
	D3D_SetInGameMenuTextRenderActive(0);
	D3D_DrawRectangle(10, topY, ScreenDescriptorBlock.SDB_Width-20, bottomY-topY, alpha);
	D3D_SetInGameMenuTextRenderActive(textScope);
}

void Hardware_RenderHighlightRectangle(int x1,int y1,int x2,int y2,int r, int g, int b)
{
	D3D_Rectangle(x1, y1, x2, y2, r, g, b, 255);
}

void D3D_DrawSliderBar(int x, int y, int alpha)
{
	struct VertexTag quadVertices[4];
	int sliderHeight = 11;
	
int sliderScale = D3D_GetActiveConsoleTextScaleFixed();
	
int scaledSliderHeight = MUL_FIXED(sliderHeight, sliderScale);
	
int scaledTwo = MUL_FIXED(2, sliderScale);
	
int scaledNine = MUL_FIXED(9, sliderScale);
	
int scaledBarWidth = MUL_FIXED(215, sliderScale);
	
int sliderCentreX = ScreenDescriptorBlock.SDB_Width/2;
	

	
x = sliderCentreX + MUL_FIXED(x-sliderCentreX, sliderScale);
	unsigned int colour = alpha>>8;

	if (colour>255) colour = 255;
	colour = (colour<<24)+0xffffff;

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + scaledSliderHeight;
	quadVertices[3].Y = y + scaledSliderHeight;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	{
		int topLeftU = 1;
		int topLeftV = 68;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 2;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 2;
		quadVertices[2].V = topLeftV + sliderHeight;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + sliderHeight;
		
		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + scaledTwo;
		quadVertices[2].X = x + scaledTwo;
		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour
		);
	}
	{
		int topLeftU = 7;
		int topLeftV = 68;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 2;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 2;
		quadVertices[2].V = topLeftV + sliderHeight;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + sliderHeight;
		
		quadVertices[0].X = x + scaledBarWidth;
		quadVertices[3].X = x + scaledBarWidth;
		quadVertices[1].X = x + scaledBarWidth + scaledTwo;
		quadVertices[2].X = x + scaledBarWidth + scaledTwo;
		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour
		);
	}
	quadVertices[2].Y = y + scaledTwo;
	quadVertices[3].Y = y + scaledTwo;

	{
		int topLeftU = 5;
		int topLeftV = 77;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU;
		quadVertices[2].V = topLeftV + 2;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 2;
		
		quadVertices[0].X = x + scaledTwo;
		quadVertices[3].X = x + scaledTwo;
		quadVertices[1].X = x + scaledBarWidth;
		quadVertices[2].X = x + scaledBarWidth;
		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour
		);
	}
	quadVertices[0].Y = y + scaledNine;
	quadVertices[1].Y = y + scaledNine;
	quadVertices[2].Y = y + scaledSliderHeight;
	quadVertices[3].Y = y + scaledSliderHeight;

	{
		int topLeftU = 5;
		int topLeftV = 77;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU;
		quadVertices[2].V = topLeftV + 2;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 2;
		
		quadVertices[0].X = x + scaledTwo;
		quadVertices[3].X = x + scaledTwo;
		quadVertices[1].X = x + scaledBarWidth;
		quadVertices[2].X = x + scaledBarWidth;
		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour
		);
	}
}

void D3D_DrawSlider(int x, int y, int alpha)
{
	struct VertexTag quadVertices[4];
	int sliderHeight = 5;
	
int sliderScale = D3D_GetActiveConsoleTextScaleFixed();
	
int scaledSliderHeight = MUL_FIXED(sliderHeight, sliderScale);
	
int scaledSliderWidth = MUL_FIXED(9, sliderScale);
	
int sliderCentreX = ScreenDescriptorBlock.SDB_Width/2;
	

	
x = sliderCentreX + MUL_FIXED(x-sliderCentreX, sliderScale);
	unsigned int colour = alpha>>8;

	if (colour>255) colour = 255;
	colour = (colour<<24)+0xffffff;

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + scaledSliderHeight;
	quadVertices[3].Y = y + scaledSliderHeight;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	{
		int topLeftU = 11;
		int topLeftV = 74;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 9;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 9;
		quadVertices[2].V = topLeftV + sliderHeight;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + sliderHeight;
		
		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + scaledSliderWidth;
		quadVertices[2].X = x + scaledSliderWidth;
		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour
		);
	}
}

void D3D_DrawRectangle(int x, int y, int w, int h, int alpha)
{
	struct VertexTag quadVertices[4];
	unsigned int colour = alpha>>8;

	if (colour>255) colour = 255;
	colour = (colour<<24)+0xffffff;

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + 6;
	quadVertices[3].Y = y + 6;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	/* top left corner */
	{
		int topLeftU = 1;
		int topLeftV = 238;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 6;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 6;
		quadVertices[2].V = topLeftV + 6;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 6;
		
		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + 6;
		quadVertices[2].X = x + 6;
			
		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
	/* top */
	{
		int topLeftU = 9;
		int topLeftV = 238;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU;
		quadVertices[2].V = topLeftV + 6;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 6;
		
		quadVertices[0].X = x+6;
		quadVertices[3].X = x+6;
		quadVertices[1].X = x+6 + w-12;
		quadVertices[2].X = x+6 + w-12;
			
		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
	/* top right corner */
	{
		int topLeftU = 11;
		int topLeftV = 238;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 6;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 6;
		quadVertices[2].V = topLeftV + 6;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 6;
		
		quadVertices[0].X = x + w - 6;
		quadVertices[3].X = x + w - 6;
		quadVertices[1].X = x + w;
		quadVertices[2].X = x + w;
			
		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
	quadVertices[0].Y = y + 6;
	quadVertices[1].Y = y + 6;
	quadVertices[2].Y = y + h - 6;
	quadVertices[3].Y = y + h - 6;
	/* right */
	{
		int topLeftU = 1;
		int topLeftV = 246;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 6;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 6;
		quadVertices[2].V = topLeftV;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV;
		
		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
	/* left */
	{
		int topLeftU = 1;
		int topLeftV = 246;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 6;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 6;
		quadVertices[2].V = topLeftV;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV;
		
		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + 6;
		quadVertices[2].X = x + 6;

		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
	quadVertices[0].Y = y + h - 6;
	quadVertices[1].Y = y + h - 6;
	quadVertices[2].Y = y + h;
	quadVertices[3].Y = y + h;
	/* bottom left corner */
	{
		int topLeftU = 1;
		int topLeftV = 248;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 6;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 6;
		quadVertices[2].V = topLeftV + 6;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 6;
		
		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + 6;
		quadVertices[2].X = x + 6;
			
		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
	/* bottom */
	{
		int topLeftU = 9;
		int topLeftV = 238;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU;
		quadVertices[2].V = topLeftV + 6;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 6;
		
		quadVertices[0].X = x+6;
		quadVertices[3].X = x+6;
		quadVertices[1].X = x+6 + w-12;
		quadVertices[2].X = x+6 + w-12;
			
		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
	/* bottom right corner */
	{
		int topLeftU = 11;
		int topLeftV = 248;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 6;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 6;
		quadVertices[2].V = topLeftV + 6;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 6;
		
		quadVertices[0].X = x + w - 6;
		quadVertices[3].X = x + w - 6;
		quadVertices[1].X = x + w;
		quadVertices[2].X = x + w;
			
		D3D_HUDQuad_Output
		(
			AAFontImageNumber,
			quadVertices,
			colour
		);
	}
}

void D3D_DrawColourBarAlpha(int yTop, int yBottom, int rScale, int gScale, int bScale, int alpha)
{
	extern unsigned char GammaValues[256];
	GLfloat x[2], y[2];
	int i;
	int r;
	int g;
	int b;
	int a;
	int start;

	CheckTriangleBuffer(256*2, 0, 255*2, 0, NULL, TRANSLUCENCY_NORMAL, -1, 0);

	start = varrc;

	for (i = 0; i < 256; i++) {
		unsigned int c;
		
		c = GammaValues[i];
		r = MUL_FIXED(c,rScale);
		g = MUL_FIXED(c,gScale);
		b = MUL_FIXED(c,bScale);
		a = MUL_FIXED(255,alpha);

		x[0] = -1.0f + 2.0f*(float)i/255.0f;
		y[0] = yTop;
		y[0] = -(y[0] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		x[1] = x[0];
		y[1] = yBottom;
		y[1] = -(y[1] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

		varrp->v[0] = x[0];
		varrp->v[1] = y[0];
		varrp->v[2] = -1.0f;
		varrp->v[3] = 1.0f;
		
		varrp->t[0] = 0.0f;
		varrp->t[1] = 0.0f;
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
		
		varrp->v[0] = x[1];
		varrp->v[1] = y[1];
		varrp->v[2] = -1.0f;
		varrp->v[3] = 1.0f;
		
		varrp->t[0] = 0.0f;
		varrp->t[1] = 0.0f;
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}

	for (i = 0; i < 255; i++) {
		tarrp->a = start+(i+0)*2+0;
		tarrp->b = start+(i+0)*2+1;
		tarrp->c = start+(i+1)*2+0;
		tarrp++;
		tarrc++;
		tarrp->a = start+(i+0)*2+1;
		tarrp->b = start+(i+1)*2+1;
		tarrp->c = start+(i+1)*2+0;
		tarrp++;
		tarrc++;
	}
}
void D3D_DrawColourBar(int yTop,int yBottom,int rScale,int gScale,int bScale)
{D3D_DrawColourBarAlpha(yTop,yBottom,rScale,gScale,bScale,65536);}


void ColourFillBackBuffer(int FillColour)
{
	float r, g, b, a;

	FlushTriangleBuffers(1);

	b = ((FillColour >> 0)  & 0xFF) / 255.0f;
	g = ((FillColour >> 8)  & 0xFF) / 255.0f;
	r = ((FillColour >> 16) & 0xFF) / 255.0f;
	a = ((FillColour >> 24) & 0xFF) / 255.0f;

	pglClearColor(r, g, b, a);
	
	pglClear(GL_COLOR_BUFFER_BIT);
}

void ColourFillBackBufferQuad(int FillColour, int x0, int y0, int x1, int y1)
{
	GLfloat x[4], y[4];
	int r, g, b, a;
	int i;

	if (y1 <= y0)
		return;
	
	CheckTriangleBuffer(4, 0, 0, 0, NULL, TRANSLUCENCY_OFF, -1, 0);
	
	b = ((FillColour >> 0)  & 0xFF);
	g = ((FillColour >> 8)  & 0xFF);
	r = ((FillColour >> 16) & 0xFF);
	a = ((FillColour >> 24) & 0xFF);	

	x[0] = x0;
	x[0] =  (x[0] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[0] = y0;
	y[0] = -(y[0] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[1] = x1 - 1;
	x[1] =  (x[1] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[1] = y0;
	y[1] = -(y[1] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[2] = x1 - 1;
	x[2] =  (x[2] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[2] = y1 - 1;
	y[2] = -(y[2] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[3] = x0;
	x[3] =  (x[3] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[3] = y1 - 1;
	y[3] = -(y[3] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

	for (i = 0; i < 4; i++) {
		varrp->v[0] = x[i];
		varrp->v[1] = y[i];
		varrp->v[2] = -1.0f;
		varrp->v[3] = 1.0f;
		
		varrp->t[0] = 0.0f;
		varrp->t[1] = 0.0f;
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

void D3D_DrawBackdrop()
{
	extern int NumActiveBlocks;
	extern DISPLAYBLOCK *ActiveBlockList[];
	extern MODULE *playerPherModule;
	
	PLAYER_STATUS *playerStatusPtr;
	int numOfObjects = NumActiveBlocks;
	int needToDrawBackdrop = 0;
	
	if (TRIPTASTIC_CHEATMODE||MOTIONBLUR_CHEATMODE)
		return;
	
	if (ShowDebuggingText.Tears) {
		ColourFillBackBuffer((63<<5));
		return;
	}
	
	while(numOfObjects--) {
		DISPLAYBLOCK *objectPtr = ActiveBlockList[numOfObjects];
		MODULE *modulePtr = objectPtr->ObMyModule;
		
		if (modulePtr && ModuleIsVisibleForRendering(modulePtr->m_index) && modulePtr->m_flags&MODULEFLAG_SKY) {
			needToDrawBackdrop = 1;
			break;
		}	
	}

	if (needToDrawBackdrop) {
		extern BOOL LevelHasStars;
		extern void RenderSky(void);
		extern void RenderStarfield(void);
		
		ColourFillBackBuffer(0);
		
		if (LevelHasStars) {
			RenderStarfield();
		} else {
			RenderSky();
		}
		
		return;
	}
	
	if (!playerPherModule) {
		ColourFillBackBuffer(0);
		return;
	}
	
	playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	
	if (!playerStatusPtr->IsAlive || FREEFALL_CHEATMODE) {
		ColourFillBackBuffer(0);
		return;
	}
}

void BltImage(RECT *dest, DDSurface *image, RECT *src)
{
    float x[4];
    float y[4];
    float s[4];
    float t[4];
    int i;

	x[0] = dest->left;
	x[0] =  (x[0] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[0] = dest->top;
	y[0] = -(y[0] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

	x[1] = dest->right;
	x[1] =  (x[1] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[1] = dest->top;
	y[1] = -(y[1] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

	x[2] = dest->right;
	x[2] =  (x[2] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[2] = dest->bottom;
	y[2] = -(y[2] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

	x[3] = dest->left;
	x[3] =  (x[3] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[3] = dest->bottom;
	y[3] = -(y[3] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

	s[0] = src->left * image->RecipW;
	t[0] = src->top * image->RecipH;

	s[1] = src->right * image->RecipW;
	t[1] = src->top * image->RecipH;

	s[2] = src->right * image->RecipW;
	t[2] = src->bottom * image->RecipH;

	s[3] = src->left * image->RecipW;
	t[3] = src->bottom * image->RecipH;

    CheckTriangleBuffer(4, 0, 0, 0, image, TRANSLUCENCY_OFF, -1, 0);

	for (i = 0; i < 4; i++) {
		varrp->v[0] = x[i];
		varrp->v[1] = y[i];
		varrp->v[2] = -1.0f;
		varrp->v[3] = 1.0f;

		varrp->t[0] = s[i];
		varrp->t[1] = t[i];

		varrp->c[0] = 255;
		varrp->c[1] = 255;
		varrp->c[2] = 255;
		varrp->c[3] = 255;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
	}
}

/* ** */

/* Hacked in special effects */

extern int NormalFrameTime;

void UpdateForceField(void);
void D3D_DrawForceField(int xOrigin, int yOrigin, int zOrigin, int fieldType);

void UpdateWaterFall(void);
void D3D_DrawWaterFall(int xOrigin, int yOrigin, int zOrigin);
void D3D_DrawPowerFence(int xOrigin, int yOrigin, int zOrigin, int xScale, int yScale, int zScale);
void D3D_DrawExplosion(int xOrigin, int yOrigin, int zOrigin, int size);

void D3D_DrawWaterPatch(int xOrigin, int yOrigin, int zOrigin);

void D3D_DrawWaterOctagonPatch(int xOrigin, int yOrigin, int zOrigin, int xOffset, int zOffset);

int LightSourceWaterPoint(VECTORCH *pointPtr,int offset);
void D3D_DrawWaterMesh_Unclipped(void);
void D3D_DrawWaterMesh_Clipped(void);


void D3D_DrawMoltenMetal(int xOrigin, int yOrigin, int zOrigin);
void D3D_DrawMoltenMetalMesh_Unclipped(void);
void D3D_DrawMoltenMetalMesh_Clipped(void);

int MeshXScale;
int MeshZScale;
int WaterFallBase;

int WaterXOrigin;
int WaterZOrigin;
float WaterUScale;
float WaterVScale;

void D3D_DrawParticle_Rain(PARTICLE *particlePtr,VECTORCH *prevPositionPtr)
{
	VECTORCH vertices[3];
	float ZNear;
	int i;
	
	vertices[0] = *prevPositionPtr;
	
	/* translate second vertex into view space */
	TranslatePointIntoViewspace(&vertices[0]);

	/* is particle within normal view frustrum ? */
	if((-vertices[0].vx <= vertices[0].vz)
	&&(vertices[0].vx <= vertices[0].vz)
	&&(-vertices[0].vy <= vertices[0].vz)
	&&(vertices[0].vy <= vertices[0].vz))
	{													

		vertices[1] = particlePtr->Position;
		vertices[2] = particlePtr->Position;
		vertices[1].vx += particlePtr->Offset.vx;
		vertices[2].vx -= particlePtr->Offset.vx;
		vertices[1].vz += particlePtr->Offset.vz;
		vertices[2].vz -= particlePtr->Offset.vz;

		/* translate particle into view space */
		TranslatePointIntoViewspace(&vertices[1]);
		TranslatePointIntoViewspace(&vertices[2]);

		ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

		CheckTriangleBuffer(3, 0, 0, 0, NULL, TRANSLUCENCY_NORMAL, -1, 0);
				
		for (i = 0; i < 3; i++) {
			GLfloat xf, yf, zf;
			GLfloat w;
			
			xf =  ((float)vertices[i].vx*((float)AvpProjectionX(Global_VDB_Ptr)))/((float)vertices[i].vz*(float)ScreenDescriptorBlock.SDB_CentreX);
			yf = -((float)vertices[i].vy*((float)AvpProjectionY(Global_VDB_Ptr)))/((float)vertices[i].vz*(float)ScreenDescriptorBlock.SDB_CentreY);
			
			zf = 1.0f - 2.0f*ZNear/(float)vertices[i].vz;
			w = (float)vertices[i].vz;

			varrp->v[0] = xf*w;
			varrp->v[1] = yf*w;
			varrp->v[2] = zf*w;
			varrp->v[3] = w;

			if (i == 0) {
				varrp->c[0] = 0;
				varrp->c[1] = 255;
				varrp->c[2] = 255;
				varrp->c[3] = 32;
			} else {
				varrp->c[0] = 255;
				varrp->c[1] = 255;
				varrp->c[2] = 255;
				varrp->c[3] = 32;
			}
			
			varrp->s[0] = 0;
			varrp->s[1] = 0;
			varrp->s[2] = 0;
			varrp->s[3] = 0;

			varrp++;
			varrc++;			
		}
	}
}

void PostLandscapeRendering()
{
 extern char LevelName[];
 { EVENT_TICK span=EventLatencyBegin(); WaterRenderBeginFrame(); EventLatencyEnd(span,"water begin frame",0,0,0,LevelName,0); }
	extern int NumOnScreenBlocks;
	extern DISPLAYBLOCK *OnScreenBlockList[];
	int numOfObjects = NumOnScreenBlocks;

	extern char LevelName[];

	if (!strcmp(LevelName,"fall")||!strcmp(LevelName,"fall_m"))
	{
		char drawWaterFall = 0;
		char drawStream = 0;
		char drawStream2 = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!strcmp(modulePtr->name,"fall01"))
				  ||(!strcmp(modulePtr->name,"well01"))
				  ||(!strcmp(modulePtr->name,"well02"))
				  ||(!strcmp(modulePtr->name,"well03"))
				  ||(!strcmp(modulePtr->name,"well04"))
				  ||(!strcmp(modulePtr->name,"well05"))
				  ||(!strcmp(modulePtr->name,"well06"))
				  ||(!strcmp(modulePtr->name,"well07"))
				  ||(!strcmp(modulePtr->name,"well08"))
				  ||(!strcmp(modulePtr->name,"well")))
				{
					drawWaterFall = 1;
				}
				else if( (!strcmp(modulePtr->name,"stream02"))
				       ||(!strcmp(modulePtr->name,"stream03"))
				       ||(!strcmp(modulePtr->name,"watergate")))
				{
		   			drawStream = 1;
				} 
				else if(  (!strcmp(modulePtr->name,"openwat03"))
					||(!strcmp(modulePtr->name,"openwat04"))
					||(!strcmp(modulePtr->name,"openwat04A"))
					||(!strcmp(modulePtr->name,"openwat02")))
				{
					drawStream2 = 1;
				}
				
			}
		}	

		if (drawWaterFall)
		{
//			CurrTextureHandle = NULL;
//			CheckBoundTextureIsCorrect(NULL);
//			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
			
			FlushTriangleBuffers(1);
			pglDepthMask(GL_FALSE);

			WaterFallBase = 109952;
			
			MeshZScale = (66572-51026)/15;
			MeshXScale = (109952+3039)/45;

	   		D3D_DrawWaterFall(175545,-3039,51026);
//			MeshZScale = -(538490-392169);
//			MeshXScale = 55000;
//			D3D_DrawWaterPatch(-100000, WaterFallBase, 538490);
			
			FlushTriangleBuffers(1);
			pglDepthMask(GL_TRUE);
		}
		if (drawStream)
		{
			int x = 68581;
			int y = 12925; /* probably should lower this a little.. */
			int z = 93696;
			MeshXScale = (87869-68581);
			MeshZScale = (105385-93696);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				{ EVENT_TICK span=EventLatencyBegin(); CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y); EventLatencyEnd(span,"water object interactions",0,0,0,LevelName,0); }
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;
			
			CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckTriangleBuffer(0, 0, 0, 0, CurrTextureHandle, TRANSLUCENCY_NORMAL, -1, 0);
			
		 	D3D_DrawWaterPatch(x, y, z);		 	
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
		if (drawStream2)
		{
#if 0 /* added, but then disabled (too squishy) */
			int x = 217400;
			int y = 20750;
			int z = 54000;
			MeshXScale = (87869-68581);
			MeshZScale = (105385-93696);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				{ EVENT_TICK span=EventLatencyBegin(); CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y); EventLatencyEnd(span,"water object interactions",0,0,0,LevelName,0); }
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;
			
			CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckTriangleBuffer(0, 0, 0, 0, CurrTextureHandle, TRANSLUCENCY_NORMAL, -1, 0);
			
		 	D3D_DrawWaterPatch(x, y, z);		 	
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
#endif		 	
		}	
	}
#if 0
	else if ( (!__stricmp(LevelName,"e3demo")) || (!__stricmp(LevelName,"e3demosp")) )
	{
		int drawOctagonPool = -1;
		int drawFMV = -1;
		int drawPredatorFMV = -1;
		int drawSwirlyFMV = -1;
		int drawSwirlyFMV2 = -1;
		int drawSwirlyFMV3 = -1;
		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if(!__stricmp(modulePtr->name,"water1"))
				{
					drawOctagonPool = modulePtr->m_index;
				}
				else if(!__stricmp(modulePtr->name,"marine01b"))
				{
					drawFMV = modulePtr->m_index;
				}
				else if(!_stricmp(modulePtr->name,"predator01"))
				{
					drawPredatorFMV = modulePtr->m_index;
				}
				else if(!_stricmp(modulePtr->name,"toptopgr01"))
				{
					drawSwirlyFMV = modulePtr->m_index;
				}
				else if(!_stricmp(modulePtr->name,"grille04"))
				{
					drawSwirlyFMV2 = modulePtr->m_index;
				}
				#if 0
				else if(!_stricmp(modulePtr->name,"marine05"))
				{
					drawSwirlyFMV3 = modulePtr->m_index;
				}
				#endif
			}
		}	
		#if FMV_ON
//		UpdateFMVTextures(3);
		

		if (drawFMV!=-1)
		{
			DECAL fmvDecal =
			{
				DECAL_FMV,
			};
			fmvDecal.ModuleIndex = drawFMV;
			fmvDecal.UOffset = 0;

			UpdateFMVTextures(4);
			
			for (int z=0; z<6; z++)
			{
				for (int y=0; y<3; y++)
				{	
					fmvDecal.Vertices[0].vx = -149;
					fmvDecal.Vertices[1].vx = -149;
					fmvDecal.Vertices[2].vx = -149;
					fmvDecal.Vertices[3].vx = -149;

					fmvDecal.Vertices[0].vy = -3254+y*744;
					fmvDecal.Vertices[1].vy = -3254+y*744;
					fmvDecal.Vertices[2].vy = -3254+y*744+744;
					fmvDecal.Vertices[3].vy = -3254+y*744+744;

					fmvDecal.Vertices[0].vz = 49440+z*993;
					fmvDecal.Vertices[1].vz = 49440+z*993+993;
					fmvDecal.Vertices[2].vz = 49440+z*993+993;
					fmvDecal.Vertices[3].vz = 49440+z*993;
					fmvDecal.Centre.vx = ((z+y)%3)+1;
					RenderDecal(&fmvDecal);
				}
			}
		}
		if (drawPredatorFMV!=-1)
		{
			DECAL fmvDecal =
			{
				DECAL_FMV,
			};
			fmvDecal.ModuleIndex = drawPredatorFMV;
			fmvDecal.UOffset = 0;

			UpdateFMVTextures(4);
			
			for (int z=0; z<12; z++)
			{
				for (int y=0; y<7; y++)
				{	
					fmvDecal.Vertices[0].vx = -7164;
					fmvDecal.Vertices[1].vx = -7164;
					fmvDecal.Vertices[2].vx = -7164;
					fmvDecal.Vertices[3].vx = -7164;

					fmvDecal.Vertices[0].vy = -20360+y*362;
					fmvDecal.Vertices[1].vy = -20360+y*362;
					fmvDecal.Vertices[2].vy = -20360+y*362+362;
					fmvDecal.Vertices[3].vy = -20360+y*362+362;

					fmvDecal.Vertices[0].vz = 1271+z*483+483;
					fmvDecal.Vertices[1].vz = 1271+z*483;
					fmvDecal.Vertices[2].vz = 1271+z*483;
					fmvDecal.Vertices[3].vz = 1271+z*483+483;
					fmvDecal.Centre.vx = (z+y)%3;
					RenderDecal(&fmvDecal);
				}
			}
		}
		
		#endif
		
		if (drawSwirlyFMV!=-1)
		{
			UpdateFMVTextures(1);
			D3D_DrawSwirlyFMV(30000,-12500,0);
		}
		if (drawSwirlyFMV2!=-1)
		{
			UpdateFMVTextures(1);
			D3D_DrawSwirlyFMV(2605,-6267-2000,17394-3200);
		}
		
		if (drawSwirlyFMV3!=-1)
		{
//			UpdateFMVTextures(1);
			D3D_DrawSwirlyFMV(5117,3456-3000,52710-2000);
		}
		if (drawOctagonPool!=-1)
		{
			#if FMV_ON
			UpdateFMVTextures(1);
			
			MeshXScale = (3000);
			MeshZScale = (4000);
			D3D_DrawFMVOnWater(-1000,3400,22000);
			{
				DECAL fmvDecal =
				{
					DECAL_FMV,
					{
					{0,-2500,29000},
					{2000,-2500,29000},
					{2000,-2500+750*2,29000},
					{0,-2500+750*2,29000}
					},
					0
				};
				fmvDecal.ModuleIndex = drawOctagonPool;
				fmvDecal.Centre.vx = 0;
				fmvDecal.UOffset = 0;

				RenderDecal(&fmvDecal);
			}
			#endif

			int highDetailRequired = 1;
			int x = 1023;
			int y = 3400;
			int z = 27536;
			
			{
				int dx = Player->ObWorld.vx - x;
				if (dx< -8000 || dx > 8000)
				{
					highDetailRequired = 0;
				}
				else
				{
					int dz = Player->ObWorld.vz - z;
					if (dz< -8000 || dz > 8000)
					{
						highDetailRequired = 0;
					}
				}
			}			
			MeshXScale = 7700;
			MeshZScale = 7700;
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				CheckForObjectsInWater(x-MeshXScale, x+MeshXScale, z-MeshZScale, z+MeshZScale, y);
			}
			
			MeshXScale /=15;
			MeshZScale /=15;
			
			// Turn OFF texturing if it is on...
			D3DTEXTUREHANDLE TextureHandle = NULL;
			if (CurrTextureHandle != TextureHandle)
			{
				OP_STATE_RENDER(1, ExecBufInstPtr);
				STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, TextureHandle, ExecBufInstPtr);
				CurrTextureHandle = TextureHandle;
			}	 
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
			if (NumVertices)
			{
			   WriteEndCodeToExecuteBuffer();
		  	   UnlockExecuteBufferAndPrepareForUse();
			   ExecuteBuffer();
		  	   LockExecuteBuffer();
			}
			if (highDetailRequired)
			{
				MeshXScale /= 2;
				MeshZScale /= 2;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
				MeshZScale = -MeshZScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
			}
			else
			{
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				MeshZScale = -MeshZScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
			}

		}
	}
#endif
	else if (!stricmp(LevelName,"hangar"))
	{
#if 0 /* not yet */
	   	#if FMV_ON
		#if WIBBLY_FMV_ON
		UpdateFMVTextures(1);
	   	D3D_DrawFMV(FmvPosition.vx,FmvPosition.vy,FmvPosition.vz);
		#endif
		#endif
		#if 0
		{
			VECTORCH v = {49937,-4000,-37709};		// hangar
			D3D_DrawCable(&v);
		}
		#endif
#endif		
	}
	else if (!stricmp(LevelName,"invasion_a"))
	{
		char drawWater = 0;
		char drawEndWater = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!strcmp(modulePtr->name,"hivepool"))
				  ||(!strcmp(modulePtr->name,"hivepool04")))
				{
					drawWater = 1;
					break;
				}
				else
				{
					if(!strcmp(modulePtr->name,"shaftbot"))
					{
						drawEndWater = 1;
					}
					if((!stricmp(modulePtr->name,"shaft01"))
					 ||(!stricmp(modulePtr->name,"shaft02"))
					 ||(!stricmp(modulePtr->name,"shaft03"))
					 ||(!stricmp(modulePtr->name,"shaft04"))
					 ||(!stricmp(modulePtr->name,"shaft05"))
					 ||(!stricmp(modulePtr->name,"shaft06")))
					{
						extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
						HandleRainShaft(modulePtr, -11726,-107080,10);
						drawEndWater = 1;
						break;
					}
				}
			}

		}	

		if (drawWater)
		{
			int x = 20767;
			int y = -36000+200;
			int z = 30238;
			MeshXScale = (36353-20767);
			MeshZScale = (41927-30238);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				{ EVENT_TICK span=EventLatencyBegin(); CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y); EventLatencyEnd(span,"water object interactions",0,0,0,LevelName,0); }
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;
			
			CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckTriangleBuffer(0, 0, 0, 0, CurrTextureHandle, TRANSLUCENCY_NORMAL, -1, 0);
		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);		 	
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
		else if (drawEndWater)
		{
			int x = -15471;
			int y = -11720-500;
			int z = -55875;
			MeshXScale = (15471-1800);
			MeshZScale = (55875-36392);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				{ EVENT_TICK span=EventLatencyBegin(); CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y); EventLatencyEnd(span,"water object interactions",0,0,0,LevelName,0); }
			}
			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)(MeshXScale+1800-3782);
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;
			
			CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckTriangleBuffer(0, 0, 0, 0, CurrTextureHandle, TRANSLUCENCY_NORMAL, -1, 0);
		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
	}
	else if (!stricmp(LevelName, "derelict"))
	{
		char drawMirrorSurfaces = 0;
		char drawWater = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
			  	if( (!stricmp(modulePtr->name,"start-en01"))
			  	  ||(!stricmp(modulePtr->name,"start")))
				{
					drawMirrorSurfaces = 1;
				}
				else if (!stricmp(modulePtr->name,"water-01"))
				{
					extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
					drawWater = 1;
					{ EVENT_TICK span=EventLatencyBegin(); HandleRainShaft(modulePtr, 32000, 0, 16); EventLatencyEnd(span,"Derelict rain shaft",0,0,0,LevelName,0); }
				}
			}
		}	

		if (drawMirrorSurfaces)
		{
			extern void RenderMirrorSurface(void);
			extern void RenderMirrorSurface2(void);
			extern void RenderParticlesInMirror(void);
			{ EVENT_TICK span=EventLatencyBegin(); RenderParticlesInMirror(); EventLatencyEnd(span,"Derelict mirror particles",0,0,0,LevelName,0); }
			{ EVENT_TICK span=EventLatencyBegin(); RenderMirrorSurface(); EventLatencyEnd(span,"Derelict mirror surface 1",0,0,0,LevelName,0); }
			{ EVENT_TICK span=EventLatencyBegin(); RenderMirrorSurface2(); EventLatencyEnd(span,"Derelict mirror surface 2",0,0,0,LevelName,0); }
		}
		if (drawWater)
		{
			int x = -102799;
			int y = 32000;
			int z = -200964;
			MeshXScale = (102799-87216);
			MeshZScale = (200964-180986);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				{ EVENT_TICK span=EventLatencyBegin(); CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y); EventLatencyEnd(span,"water object interactions",0,0,0,LevelName,0); }
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=2;
		 	MeshZScale/=2;
			
			CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckTriangleBuffer(0, 0, 0, 0, CurrTextureHandle, TRANSLUCENCY_NORMAL, -1, 0);
            EVENT_TICK waterSpan=EventLatencyBegin();
		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
            EventLatencyEnd(waterSpan,"Derelict water patches",0,0,0,LevelName,0);
		}

	}
	else if (!stricmp(LevelName,"genshd1"))
	{
		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!stricmp(modulePtr->name,"largespace"))
				  ||(!stricmp(modulePtr->name,"proc13"))
				  ||(!stricmp(modulePtr->name,"trench01"))
				  ||(!stricmp(modulePtr->name,"trench02"))
				  ||(!stricmp(modulePtr->name,"trench03"))
				  ||(!stricmp(modulePtr->name,"trench04"))
				  ||(!stricmp(modulePtr->name,"trench05"))
				  ||(!stricmp(modulePtr->name,"trench06"))
				  ||(!stricmp(modulePtr->name,"trench07"))
				  ||(!stricmp(modulePtr->name,"trench08"))
				  ||(!stricmp(modulePtr->name,"trench09")))
				{
					extern void HandleRain(int numberOfRaindrops);
					HandleRain(999);
					break;
				}
			}

		}	
	}
}

void D3D_DrawWaterTest(MODULE *testModulePtr)
{
	extern char LevelName[];
	if (!strcmp(LevelName,"genshd1"))
	{
//		DISPLAYBLOCK *objectPtr = OnScreenBlockList[numOfObjects];
		MODULE *modulePtr = testModulePtr;//objectPtr->ObMyModule;
#if 0
		if (testModulePtr && testModulePtr->name)
		if(!strcmp(testModulePtr->name,"LargeSpace"))
		{
			extern void HandleRain(int numberOfRaindrops);
			HandleRain(999);
		}
#endif
		if (modulePtr && modulePtr->name)
		{
			if (!strcmp(modulePtr->name,"05"))
			{
				int y = modulePtr->m_maxy+modulePtr->m_world.vy-500;
		   		int x = modulePtr->m_minx+modulePtr->m_world.vx;
		   		int z = modulePtr->m_minz+modulePtr->m_world.vz;
				MeshXScale = (7791 - -7794);
				MeshZScale = (23378 - 7793);
				{
					extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
					CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
				}
				
				CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
				CheckBoundTextureIsCorrect(CurrTextureHandle);
				CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);

				WaterXOrigin=x;
				WaterZOrigin=z;
				WaterUScale = 4.0f/(float)(MeshXScale);
				WaterVScale = 4.0f/(float)MeshZScale;
			#if 1
				MeshXScale/=2;
				MeshZScale/=2;
				
				CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
				CheckTriangleBuffer(0, 0, 0, 0, CurrTextureHandle, TRANSLUCENCY_NORMAL, -1, 0);
				D3D_DrawWaterPatch(x, y, z);
				D3D_DrawWaterPatch(x+MeshXScale, y, z);
				D3D_DrawWaterPatch(x, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
				
				{
					extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
					HandleRainShaft(modulePtr, y,-21000,1);
				}
			#else
				MeshXScale/=4;
				MeshZScale/=4;
				D3D_DrawWaterPatch(x, y, z);
				D3D_DrawWaterPatch(x, y, z+MeshZScale);
				D3D_DrawWaterPatch(x, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x, y, z+MeshZScale*3);
				D3D_DrawWaterPatch(x+MeshXScale, y, z);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale*3);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale*3);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale*3);
				HandleRainDrops(modulePtr,2);
			#endif
			}
		}
	}
#if 0
	else if ( (!_stricmp(LevelName,"e3demo")) || (!_stricmp(LevelName,"e3demosp")) )
	{
		if (testModulePtr && testModulePtr->name)
		{
			#if 0
			if(!_stricmp(testModulePtr->name,"watermid"))
			{
				DECAL fmvDecal =
				{
					DECAL_FMV,
					{
					{0,-2500,29000},
					{2000,-2500,29000},
					{2000,-2500+750*2,29000},
					{0,-2500+750*2,29000}
					},
					0
				};
				fmvDecal.ModuleIndex = testModulePtr->m_index;
				fmvDecal.Centre.vx = 0;
				fmvDecal.UOffset = 0;

				RenderDecal(&fmvDecal);
			}
			#endif
			if(!_stricmp(testModulePtr->name,"lowlowlo03"))
			{
				VECTORCH position = {6894,469,-13203};
				VECTORCH disp = position;
				int i,d;

				disp.vx -= Player->ObWorld.vx;
				disp.vy -= Player->ObWorld.vy;
				disp.vz -= Player->ObWorld.vz;
				d = ONE_FIXED - Approximate3dMagnitude(&disp)*2;
				if (d<0) d = 0;

				i = MUL_FIXED(10,d);
				while(i--)
				{
					VECTORCH velocity;
					velocity.vx = ((FastRandom()&1023) - 512);
					velocity.vy = ((FastRandom()&1023) - 512)+2000;
					velocity.vz = (1000+(FastRandom()&255))*2;
					MakeParticle(&(position),&(velocity),PARTICLE_STEAM);
				}
			}
		}
	}
#endif
}


VECTORCH MeshVertex[256];
#define TEXTURE_WATER 0

VECTORCH MeshWorldVertex[256];
unsigned int MeshVertexColour[256];
unsigned int MeshVertexSpecular[256];
char MeshVertexOutcode[256];


extern float ViewMatrix[12];
extern int WaterTerrainDepth(int x,int y,int z);
typedef struct {int valid,x,y,z,depth;double time;unsigned generation;} WATER_FLOOR_SAMPLE;
static WATER_FLOOR_SAMPLE WaterFloorCache[65536];
static float WaterFloorOffset(int x,int y,int z,float offset){
 unsigned hash=((unsigned)x*73856093u^(unsigned)y*19349663u^(unsigned)z*83492791u)&65535u;
 WATER_FLOOR_SAMPLE*c=0;double now=WaterTime();unsigned generation=WaterGeneration();
 /* Open addressing avoids repeating expensive terrain rays for colliding grid
    vertices every frame. Static hits live for this level; misses are retried
    as more landscape modules become active. */
 for(unsigned probe=0;probe<65536;probe++){
  c=&WaterFloorCache[(hash+probe)&65535u];
  if(!c->valid||c->generation!=generation||(c->x==x&&c->y==y&&c->z==z))break;
 }
 if(!c->valid||c->generation!=generation||c->x!=x||c->y!=y||c->z!=z||
    (c->depth==512&&now-c->time>.5)){
  c->valid=1;c->generation=generation;c->x=x;c->y=y;c->z=z;c->time=now;c->depth=WaterTerrainDepth(x,y,z);
 }
 /* Positive Y points down. Do not lift water over dry banks; constrain only
    an actual submerged floor. Keep 8 mm separation to prevent z fighting. */
 if(c->depth>0&&c->depth<512){float limit=fmaxf(0,c->depth-8.0f);if(offset>limit)offset=limit;}
 return offset;
}

/* Strongest existing local lights. Only Modern consumes this optical state. */
static void WaterPatchLights(float x,float y,float z){
 extern int NumActiveBlocks;extern DISPLAYBLOCK*ActiveBlockList[];
 float pos[16]={0},color[16]={0},scores[4]={0};
 for(int i=0;i<NumActiveBlocks;i++){
  DISPLAYBLOCK*d=ActiveBlockList[i];
  for(int j=0;j<d->ObNumLights;j++){
   LIGHTBLOCK*l=d->ObLights[j];if(!l||l->LightRange<=0||l->LightBright<=0||(l->LightFlags&LFlag_NoSpecular))continue;
   float dx=l->LightWorld.vx-x,dy=l->LightWorld.vy-y,dz=l->LightWorld.vz-z;
   float distance=sqrtf(dx*dx+dy*dy+dz*dz),range=l->LightRange;
   float power=l->LightBright/65536.0f;
   float score=power*fmaxf(0,1-distance/(range+5000));int slot=0;
   for(int k=1;k<4;k++)if(scores[k]<scores[slot])slot=k;
   if(score<=scores[slot])continue;scores[slot]=score;
   pos[slot*4]=l->LightWorld.vx;pos[slot*4+1]=l->LightWorld.vy;pos[slot*4+2]=l->LightWorld.vz;pos[slot*4+3]=range;
   color[slot*4]=fminf(4,power*l->RedScale/65536.0f);color[slot*4+1]=fminf(4,power*l->GreenScale/65536.0f);color[slot*4+2]=fminf(4,power*l->BlueScale/65536.0f);
   if(CurrentVisionMode==VISION_MODE_IMAGEINTENSIFIER){color[slot*4]=color[slot*4+2]=0;}
   else if(CurrentVisionMode==VISION_MODE_PRED_THERMAL||CurrentVisionMode==VISION_MODE_PRED_SEEALIENS||CurrentVisionMode==VISION_MODE_PRED_SEEPREDTECH){color[slot*4]=color[slot*4+1]=0;}
  }
 }
 WaterRenderSetLights(pos,color);
}
static int DrawRepairedWaterPatch(int xOrigin,int yOrigin,int zOrigin){
#if !defined(USE_OPENGL_ES)
 enum{N=33};static WATER_VERTEX vertices[N*N];static unsigned short indices[(N-1)*(N-1)*6];
 float px=(float)AvpProjectionX(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreX;
 float py=(float)AvpProjectionY(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreY;
 float nearClip=Global_VDB_Ptr->VDB_ClipZ*GlobalScale;
 float phase=(float)WaterTime()*2048.0f;int n=0;
 /* Start with the original material, blend and texture filtering state. */
 FlushTriangleBuffers(1);
 for(int x=0;x<N;x++)for(int z=0;z<N;z++){
  WATER_VERTEX*v=&vertices[x*N+z];VECTORCH point={xOrigin+x*MeshXScale/(N-1),yOrigin,zOrigin+z*MeshZScale/(N-1)};
  float offset;
  if(WaterEffectMode==1){
   float a=(point.vx+point.vz+phase)*(6.283185307f/4096.0f),b=(point.vx-point.vz*2+phase*.5f)*(6.283185307f/4096.0f);
   offset=32*sinf(a)+16*sinf(b)+WaterHeight(point.vx,point.vz,yOrigin);
   offset=fmaxf(-256,fminf(256,offset));
  }else offset=EffectOfRipples(&point);
  offset=WaterFloorOffset(point.vx,yOrigin,point.vz,offset);
  v->world[0]=point.vx;v->world[1]=yOrigin+offset;v->world[2]=point.vz;
  float p[3];for(int k=0;k<3;k++)p[k]=ViewMatrix[k*4]*v->world[0]+ViewMatrix[k*4+1]*v->world[1]+ViewMatrix[k*4+2]*v->world[2]+ViewMatrix[k*4+3];
  /* Keep signed W: the GPU clips triangles crossing the near plane correctly. */
  v->clip[0]=p[0]*px;v->clip[1]=-p[1]*py;v->clip[2]=p[2]-2*nearClip;v->clip[3]=p[2];
  int vy=(int)lroundf(v->world[1]);
  v->uv[0]=((point.vx-WaterXOrigin)/4+MUL_FIXED(GetSin((vy*16)&4095),128))*WaterUScale+1.0f/256;
  v->uv[1]=((point.vz-WaterZOrigin)/4+MUL_FIXED(GetSin((vy*16+200)&4095),128))*WaterVScale+1.0f/256;
  v->color[0]=v->color[1]=v->color[2]=1;v->color[3]=(128-offset/4)/255.0f;
  if(CurrentVisionMode==VISION_MODE_IMAGEINTENSIFIER){v->color[0]=v->color[2]=0;v->color[1]=51/255.0f;}
  else if(CurrentVisionMode==VISION_MODE_PRED_THERMAL||CurrentVisionMode==VISION_MODE_PRED_SEEALIENS||CurrentVisionMode==VISION_MODE_PRED_SEEPREDTECH){v->color[0]=v->color[1]=0;v->color[2]=28/255.0f;}
  if(x<N-1&&z<N-1){int i=x*N+z;indices[n++]=i;indices[n++]=i+1;indices[n++]=i+N;indices[n++]=i+1;indices[n++]=i+N+1;indices[n++]=i+N;}
 }
 if(WaterEffectMode==1)WaterPatchLights(xOrigin+MeshXScale*.5f,yOrigin,zOrigin+MeshZScale*.5f);
 return WaterRenderDraw(vertices,N*N,indices,n,CurrTextureHandle?CurrTextureHandle->id:0,yOrigin,ViewMatrix,px,py,nearClip);
#else
 return 0;
#endif
}

/* Modern's denser mesh must not fill Vanilla's 65k-entry cache and trigger
 * unbounded linear scans. Separate storage also preserves mode-switch behaviour.
 * Bound each lookup to 16 probes; collisions only cause a fresh terrain query. */
static WATER_FLOOR_SAMPLE WaterModernFloorCache[262144];
static float WaterModernFloorOffset(int x,int y,int z,float offset){
 unsigned hash=((unsigned)x*73856093u^(unsigned)y*19349663u^(unsigned)z*83492791u)&262143u;
 WATER_FLOOR_SAMPLE*c=0;double now=WaterTime();unsigned generation=WaterGeneration();
 for(unsigned probe=0;probe<16;probe++){
  c=&WaterModernFloorCache[(hash+probe)&262143u];
  if(!c->valid||c->generation!=generation||(c->x==x&&c->y==y&&c->z==z))break;
 }
 if(!c->valid||c->generation!=generation||c->x!=x||c->y!=y||c->z!=z||
    (c->depth==512&&now-c->time>.5)){
  c->valid=1;c->generation=generation;c->x=x;c->y=y;c->z=z;c->time=now;c->depth=WaterTerrainDepth(x,y,z);
 }
 if(c->depth>0&&c->depth<512){float limit=fmaxf(0,c->depth-8.0f);if(offset>limit)offset=limit;}
 return offset;
}

#ifndef AVP_WATER_DISABLE_GRID
#include "water_grid_mesh.h"
#endif
static int DrawModernWaterPatch(int xOrigin,int yOrigin,int zOrigin){
#if !defined(USE_OPENGL_ES)
#ifndef AVP_WATER_DISABLE_GRID
 if(DrawGPUWaterPatch(xOrigin,yOrigin,zOrigin))return 1;
#endif
 enum{N=WATER_MESH_MAX_SEGMENTS+1};static WATER_VERTEX vertices[N*N];static unsigned short indices[(N-1)*(N-1)*6];
 int nx=WaterMeshSegments(MeshXScale)+1,nz=WaterMeshSegments(MeshZScale)+1;
 float px=(float)AvpProjectionX(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreX;
 float py=(float)AvpProjectionY(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreY;
 float nearClip=Global_VDB_Ptr->VDB_ClipZ*GlobalScale;
 int n=6*(nx-1)*(nz-1);
 static int indexNX,indexNZ,uvY,uvValid;static float warpU[513],warpV[513];
 float gridX[N],gridZ[N];int intX[N],intZ[N];
 for(int x=0;x<nx;x++){gridX[x]=xOrigin+x*(float)MeshXScale/(nx-1);intX[x]=(int)lroundf(gridX[x]);}
 for(int z=0;z<nz;z++){gridZ[z]=zOrigin+z*(float)MeshZScale/(nz-1);intZ[z]=(int)lroundf(gridZ[z]);}
 /* EffectOfRipples and the terrain clamp return integral mm in [-256,256].
  * Reuse the exact sine results for each height instead of per-vertex trig. */
 if(!uvValid||uvY!=yOrigin){
  for(int i=0;i<513;i++){
   float wy=yOrigin+(float)(i-256),phase=wy*16*(6.283185307f/4096.0f);
   warpU[i]=128*sinf(phase);warpV[i]=128*sinf(phase+200*(6.283185307f/4096.0f));
  }uvY=yOrigin;uvValid=1;
 }
 if(indexNX!=nx||indexNZ!=nz){
  int q=0;for(int x=0;x<nx-1;x++)for(int z=0;z<nz-1;z++){
   int i=x*nz+z;indices[q++]=i;indices[q++]=i+1;indices[q++]=i+nz;
   indices[q++]=i+1;indices[q++]=i+nz+1;indices[q++]=i+nz;
  }indexNX=nx;indexNZ=nz;
 }
 /* Start with the original material, blend and texture filtering state. */
 FlushTriangleBuffers(1);
 for(int x=0;x<nx;x++)for(int z=0;z<nz;z++){
  WATER_VERTEX*v=&vertices[x*nz+z];
  float wx=gridX[x],wz=gridZ[z];
  VECTORCH ripplePoint={intX[x],yOrigin,intZ[z]};
  float offset=EffectOfRipples(&ripplePoint); /* Vanilla geometry + Modern optical waves. */
  offset=WaterModernFloorOffset(intX[x],yOrigin,intZ[z],offset);
  v->world[0]=wx;v->world[1]=yOrigin+offset;v->world[2]=wz;
  float p[3];for(int k=0;k<3;k++)p[k]=ViewMatrix[k*4]*v->world[0]+ViewMatrix[k*4+1]*v->world[1]+ViewMatrix[k*4+2]*v->world[2]+ViewMatrix[k*4+3];
  /* Keep signed W: the GPU clips triangles crossing the near plane correctly. */
  v->clip[0]=p[0]*px;v->clip[1]=-p[1]*py;v->clip[2]=p[2]-2*nearClip;v->clip[3]=p[2];
  /* Float UVs remove integer /4 and fixed-sine steps on the denser mesh.
     Preserve the authored scale and the original 128-unit warp amplitude. */
  int warp=(int)offset+256;
  v->uv[0]=((wx-WaterXOrigin)*.25f+warpU[warp])*WaterUScale+1.0f/256;
  v->uv[1]=((wz-WaterZOrigin)*.25f+warpV[warp])*WaterVScale+1.0f/256;
  v->color[0]=v->color[1]=v->color[2]=1;v->color[3]=(128-offset/4)/255.0f;
  if(CurrentVisionMode==VISION_MODE_IMAGEINTENSIFIER){v->color[0]=v->color[2]=0;v->color[1]=51/255.0f;}
  else if(CurrentVisionMode==VISION_MODE_PRED_THERMAL||CurrentVisionMode==VISION_MODE_PRED_SEEALIENS||CurrentVisionMode==VISION_MODE_PRED_SEEPREDTECH){v->color[0]=v->color[1]=0;v->color[2]=28/255.0f;}

 }
 WaterPatchLights(xOrigin+MeshXScale*.5f,yOrigin,zOrigin+MeshZScale*.5f);
 return WaterRenderDraw(vertices,nx*nz,indices,n,CurrTextureHandle?CurrTextureHandle->id:0,yOrigin,ViewMatrix,px,py,nearClip);
#else
 return 0;
#endif
}

void D3D_DrawWaterPatch(int xOrigin, int yOrigin, int zOrigin)
{
 if(WaterEffectMode==1&&DrawModernWaterPatch(xOrigin,yOrigin,zOrigin))return;
 if(DrawRepairedWaterPatch(xOrigin,yOrigin,zOrigin))return;
	int i=0;
	int x;
	int offset;
	
	for (x=0; x<16; x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			
			point->vx = xOrigin+(x*MeshXScale)/15;
			point->vz = zOrigin+(z*MeshZScale)/15;


			offset=0;

		 #if 1
			/* basic noise ripples */
//		 	offset = MUL_FIXED(32,GetSin(  (point->vx+point->vz+CloakingPhase)&4095 ) );
//		 	offset += MUL_FIXED(16,GetSin(  (point->vx-point->vz*2+CloakingPhase/2)&4095 ) );

			{
 				offset += EffectOfRipples(point);
			}
		#endif
	//		if (offset>450) offset = 450;
	//		if (offset<-450) offset = -450;
			point->vy = yOrigin+offset;

			#if 0
			MeshVertexColour[i] = LightSourceWaterPoint(point,offset);
			#else
			{
				int alpha = 128-offset/4;
		//		if (alpha>255) alpha = 255;
		//		if (alpha<128) alpha = 128;
				switch (CurrentVisionMode)
				{
					default:
					case VISION_MODE_NORMAL:
					{
//						MeshVertexColour[i] = RGBALIGHT_MAKE(10,51,28,alpha);
						MeshVertexColour[i] = RGBA_MAKE(255,255,255,alpha);
						#if 0
						#if 1
						VECTORCH pos = {24087,yOrigin,39165};
						int c = (8191-VectorDistance(&pos,point));
						if (c<0) c=0;
						else
						{
							int s = GetSin((CloakingPhase/2)&4095);
							s = MUL_FIXED(s,s)/64;
							c = MUL_FIXED(s,c);
						}
						MeshVertexSpecular[i] = (c<<16)+(((c/4)<<8)&0xff00) + (c/4);
						#else 
						if (!(FastRandom()&1023))
						{
							MeshVertexSpecular[i] = 0xc04040;
						}
						else
						{
							MeshVertexSpecular[i] = 0;
						}
						#endif
						#endif
						break;
					}
					case VISION_MODE_IMAGEINTENSIFIER:
					{
						MeshVertexColour[i] = RGBA_MAKE(0,51,0,alpha);
						break;
					}
					case VISION_MODE_PRED_THERMAL:
					case VISION_MODE_PRED_SEEALIENS:
					case VISION_MODE_PRED_SEEPREDTECH:
					{
						MeshVertexColour[i] = RGBA_MAKE(0,0,28,alpha);
					  	break;
					}
				}

			}
			#endif

			#if 1
			MeshWorldVertex[i].vx = ((point->vx-WaterXOrigin)/4+MUL_FIXED(GetSin((point->vy*16)&4095),128));
			MeshWorldVertex[i].vy = ((point->vz-WaterZOrigin)/4+MUL_FIXED(GetSin((point->vy*16+200)&4095),128));
			#endif
			
			#if 1
			TranslatePointIntoViewspace(point);
			#else
			point->vx -= Global_VDB_Ptr->VDB_World.vx;
			point->vy -= Global_VDB_Ptr->VDB_World.vy;
			point->vz -= Global_VDB_Ptr->VDB_World.vz;
			RotateVector(point,&(Global_VDB_Ptr->VDB_Mat));
			point->vy = MUL_FIXED(point->vy,87381);

			#endif
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}

	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawMoltenMetalMesh_Unclipped();
//		D3D_DrawWaterMesh_Unclipped();
	} else {
		D3D_DrawMoltenMetalMesh_Clipped();
//		D3D_DrawWaterMesh_Clipped();
	}
		
	
}

#if 0 

void D3D_DrawWaterMesh_Unclipped(void)
{
	float ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	/* OUTPUT VERTICES TO EXECUTE BUFFER */
	{
		D3DTLVERTEX *vertexPtr = &((LPD3DTLVERTEX)ExecuteBufferDataArea)[NumVertices];
		VECTORCH *point = MeshVertex;
		#if TEXTURE_WATER
		VECTORCH *pointWS = MeshWorldVertex;
		#endif
		int i;
		for (i=0; i<256; i++)
		{

			if (point->vz<=1) point->vz = 1;
			int x = (point->vx*(Global_VDB_Ptr->VDB_ProjX))/point->vz+Global_VDB_Ptr->VDB_CentreX;
			int y = (point->vy*(Global_VDB_Ptr->VDB_ProjY))/point->vz+Global_VDB_Ptr->VDB_CentreY;
  //			textprint("%d, %d\n",x,y);
			#if 1
			{
				if (x<Global_VDB_Ptr->VDB_ClipLeft)
				{
					x=Global_VDB_Ptr->VDB_ClipLeft;
				}	
				else if (x>Global_VDB_Ptr->VDB_ClipRight)
				{
					x=Global_VDB_Ptr->VDB_ClipRight;	
				}
				
				vertexPtr->sx=x;
			}
			{
				if (y<Global_VDB_Ptr->VDB_ClipUp)
				{
					y=Global_VDB_Ptr->VDB_ClipUp;
				}
				else if (y>Global_VDB_Ptr->VDB_ClipDown)
				{
					y=Global_VDB_Ptr->VDB_ClipDown;	
				}
				vertexPtr->sy=y;
			}
			#else
			vertexPtr->sx=x;
			vertexPtr->sy=y;
			#endif
			#if FOG_ON
			{
				int fog = (point->vz)/FOG_SCALE;
				if (fog<0) fog=0;
			 	if (fog>254) fog=254;
				fog=255-fog;
			   	vertexPtr->specular=RGBALIGHT_MAKE(0,0,0,fog);
			}
			#endif
			point->vz+=HeadUpDisplayZOffset;
		  	float oneOverZ = ((float)(point->vz)-ZNear)/(float)(point->vz);
		  //vertexPtr->color = RGBALIGHT_MAKE(66,70,0,127+(FastRandom()&63));
			vertexPtr->color = MeshVertexColour[i];
			vertexPtr->sz = oneOverZ;
			#if TEXTURE_WATER
			vertexPtr->tu = pointWS->vx/128.0;
			vertexPtr->tv =	pointWS->vz/128.0;
			#endif


			NumVertices++;
			vertexPtr++;
			point++;
			#if TEXTURE_WATER
			pointWS++;
			#endif
		}
	}
 //	textprint("numvertices %d\n",NumVertices);
    
    
    /*
     * Make sure that the triangle data (not OP) will be QWORD aligned
     */
	if (QWORD_ALIGNED(ExecBufInstPtr))
    {
        OP_NOP(ExecBufInstPtr);
    }

  	OP_TRIANGLE_LIST(450, ExecBufInstPtr);
	/* CONSTRUCT POLYS */
	{
		int x;
		for (x=0; x<15; x++)
		{
			int y;
			for(y=0; y<15; y++)
			{
				OUTPUT_TRIANGLE(0+x+(16*y),1+x+(16*y),16+x+(16*y), 256);
				OUTPUT_TRIANGLE(1+x+(16*y),17+x+(16*y),16+x+(16*y), 256);
			}
		}
	}
	#if 1
	{
	   WriteEndCodeToExecuteBuffer();
  	   UnlockExecuteBufferAndPrepareForUse();
	   ExecuteBuffer();
  	   LockExecuteBuffer();
	}
	#endif
}
void D3D_DrawWaterMesh_Clipped(void)
{
	float ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	/* OUTPUT VERTICES TO EXECUTE BUFFER */
	{
		D3DTLVERTEX *vertexPtr = &((LPD3DTLVERTEX)ExecuteBufferDataArea)[NumVertices];
		VECTORCH *point = MeshVertex;
		#if TEXTURE_WATER
		VECTORCH *pointWS = MeshWorldVertex;
		#endif
		int i;
		for (i=0; i<256; i++)
		{
			{
				if (point->vz<=1) point->vz = 1;
				int x = (point->vx*(Global_VDB_Ptr->VDB_ProjX))/point->vz+Global_VDB_Ptr->VDB_CentreX;
				int y = (point->vy*(Global_VDB_Ptr->VDB_ProjY))/point->vz+Global_VDB_Ptr->VDB_CentreY;
				#if 1
				{
					if (x<Global_VDB_Ptr->VDB_ClipLeft)
					{
						x=Global_VDB_Ptr->VDB_ClipLeft;
					}	
					else if (x>Global_VDB_Ptr->VDB_ClipRight)
					{
						x=Global_VDB_Ptr->VDB_ClipRight;	
					}
					
					vertexPtr->sx=x;
				}
				{
					if (y<Global_VDB_Ptr->VDB_ClipUp)
					{
						y=Global_VDB_Ptr->VDB_ClipUp;
					}
					else if (y>Global_VDB_Ptr->VDB_ClipDown)
					{
						y=Global_VDB_Ptr->VDB_ClipDown;	
					}
					vertexPtr->sy=y;
				}
				#else
				vertexPtr->sx=x;
				vertexPtr->sy=y;
				#endif
				#if FOG_ON
				{
					int fog = ((point->vz)/FOG_SCALE);
					if (fog<0) fog=0;
				 	if (fog>254) fog=254;
					fog=255-fog;
				   	vertexPtr->specular=RGBALIGHT_MAKE(0,0,0,fog);
				}
				#endif
				#if TEXTURE_WATER
				vertexPtr->tu = pointWS->vx/128.0;
				vertexPtr->tv =	pointWS->vz/128.0;
				#endif
				point->vz+=HeadUpDisplayZOffset;
			  	float oneOverZ = ((float)(point->vz)-ZNear)/(float)(point->vz);
			  //	vertexPtr->color = RGBALIGHT_MAKE(66,70,0,127+(FastRandom()&63));
				vertexPtr->color = MeshVertexColour[i];
				vertexPtr->sz = oneOverZ;
			}
			NumVertices++;
			vertexPtr++;
			point++;
			#if TEXTURE_WATER
			pointWS++;
			#endif
		}
	}
//	textprint("numvertices %d\n",NumVertices);
	/* CONSTRUCT POLYS */
	{
		int x;
		for (x=0; x<15; x++)
		{
			int y;
			for(y=0; y<15; y++)
			{
				#if 1
				int p1 = 0+x+(16*y);
				int p2 = 1+x+(16*y);
				int p3 = 16+x+(16*y);
				int p4 = 17+x+(16*y);

				if (MeshVertexOutcode[p1]||MeshVertexOutcode[p2]||MeshVertexOutcode[p3])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p1,p2,p3, 256);
				}
				if (MeshVertexOutcode[p2]||MeshVertexOutcode[p3]||MeshVertexOutcode[p4])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p2,p4,p3, 256);
				}	
				#else
				int p2 = 1+x+(16*y);
				int p3 = 16+x+(16*y);

				if (MeshVertexOutcode[p2]&&MeshVertexOutcode[p3])
				{
					int p1 = 0+x+(16*y);
					int p4 = 17+x+(16*y);
					if (MeshVertexOutcode[p1])
					{
						OP_TRIANGLE_LIST(1, ExecBufInstPtr);
						OUTPUT_TRIANGLE(p1,p2,p3, 256);
					}
					if (MeshVertexOutcode[p4])
					{
						OP_TRIANGLE_LIST(1, ExecBufInstPtr);
						OUTPUT_TRIANGLE(p2,p4,p3, 256);
					}
				}	
				#endif				
			}
		}
	}
	#if 1
	{
	   WriteEndCodeToExecuteBuffer();
  	   UnlockExecuteBufferAndPrepareForUse();
	   ExecuteBuffer();
  	   LockExecuteBuffer();
	}
	#endif
}

#endif

signed int ForceFieldPointDisplacement[15*3+1][16];
signed int ForceFieldPointDisplacement2[15*3+1][16];
signed int ForceFieldPointVelocity[15*3+1][16];
unsigned char ForceFieldPointColour1[15*3+1][16];
unsigned char ForceFieldPointColour2[15*3+1][16];

int Phase=0;
int ForceFieldPhase=0;
void InitForceField(void)
{
	int x, y;
	
	for (x=0; x<15*3+1; x++)
		for (y=0; y<16; y++)
		{
			ForceFieldPointDisplacement[x][y]=0;
			ForceFieldPointDisplacement2[x][y]=0;
			ForceFieldPointVelocity[x][y]=0;
		}
	ForceFieldPhase=0;
}

#if 0 /* not used */

#if 1

extern int NormalFrameTime;

void UpdateForceField(void)
{
	#if 1
	int x, y;
	
	Phase+=NormalFrameTime>>6;
	ForceFieldPhase+=NormalFrameTime>>5;
	for (x=1; x<15*3; x++)
	{
		for (y=1; y<15; y++)
		{
			
			int acceleration =32*(-8*ForceFieldPointDisplacement[x][y]
								+ForceFieldPointDisplacement[x-1][y-1]
								+ForceFieldPointDisplacement[x-1][y]
								+ForceFieldPointDisplacement[x-1][y+1]
								+ForceFieldPointDisplacement[x][y-1]
								+ForceFieldPointDisplacement[x][y+1]
#if 0
								)
#else								

								+ForceFieldPointDisplacement[x+1][y-1]
								+ForceFieldPointDisplacement[x+1][y]
								+ForceFieldPointDisplacement[x+1][y+1])			
#endif
								-(ForceFieldPointVelocity[x][y]*5);

			ForceFieldPointVelocity[x][y] += MUL_FIXED(acceleration,NormalFrameTime);
			ForceFieldPointDisplacement2[x][y] += MUL_FIXED(ForceFieldPointVelocity[x][y],NormalFrameTime);
#if 1
			if(ForceFieldPointDisplacement2[x][y]>200) ForceFieldPointDisplacement2[x][y]=200;
			if(ForceFieldPointDisplacement2[x][y]<-200) ForceFieldPointDisplacement2[x][y]=-200;
#else
			if(ForceFieldPointDisplacement2[x][y]>512) ForceFieldPointDisplacement2[x][y]=512;
			if(ForceFieldPointDisplacement2[x][y]<-512) ForceFieldPointDisplacement2[x][y]=-512;

#endif
			{
				int offset = ForceFieldPointDisplacement2[x][y];
				int colour = ForceFieldPointVelocity[x][y]/4;

				if (offset<0) offset =-offset;
				if (colour<0) colour =-colour;
				colour=(colour+offset)/2;

				if(colour>255) colour=255;
				colour++;
				
				ForceFieldPointColour1[x][y]=FastRandom()%colour;
				ForceFieldPointColour2[x][y]=FastRandom()%colour;
			}
		}

	}
	for (x=1; x<15*3; x++)
	{
		int y;
		for (y=1; y<15; y++)
		{
			ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement2[x][y];
		}
	}
	{
		#if 1
	  	if(ForceFieldPhase>1000)
		{
			ForceFieldPhase=0;
			x = 1+(FastRandom()%(15*3-2));
			y = 1+(FastRandom()%13);
			ForceFieldPointVelocity[x][y] = 10000;
			ForceFieldPointVelocity[x][y+1] = 10000;
			ForceFieldPointVelocity[x+1][y] = 10000;
			ForceFieldPointVelocity[x+1][y+1] = 10000;
		}	
		#else
	   //	if(ForceFieldPhase>1000)
		{
			ForceFieldPhase=0;
			x = 1+(FastRandom()%(15*3-2));
			y = 1+(FastRandom()%13);
			ForceFieldPointVelocity[x][y] = (FastRandom()&16383)+8192;
		}
		#endif				   
	}
	#else
	int x;
	int y;
	for (y=0; y<=15; y++)
	{
		ForceFieldPointDisplacement[0][y] += (FastRandom()&127)-64;
		if(ForceFieldPointDisplacement[0][y]>512) ForceFieldPointDisplacement[0][y]=512;
		if(ForceFieldPointDisplacement[0][y]<-512) ForceFieldPointDisplacement[0][y]=-512;
		ForceFieldPointVelocity[0][y] = (FastRandom()&16383)-8192;
	}
	for (x=15*3-1; x>0; x--)
	{
		for (y=0; y<=15; y++)
		{
			ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
			ForceFieldPointVelocity[x][y] = ForceFieldPointVelocity[x-1][y];
		}

	}
	for (x=15*3-1; x>1; x--)
	{
		y = FastRandom()&15;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
		y = (FastRandom()&15)-1;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
	}
	#endif
}
void UpdateWaterFall(void)
{
	int x;
	int y;
	for (y=0; y<=15; y++)
	{
		ForceFieldPointDisplacement[0][y] += (FastRandom()&127)-64;
		if(ForceFieldPointDisplacement[0][y]>512) ForceFieldPointDisplacement[0][y]=512;
		if(ForceFieldPointDisplacement[0][y]<-512) ForceFieldPointDisplacement[0][y]=-512;
		ForceFieldPointVelocity[0][y] = (FastRandom()&16383)-8192;
	}
	for (x=15*3-1; x>0; x--)
	{
		for (y=0; y<=15; y++)
		{
			ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
			ForceFieldPointVelocity[x][y] = ForceFieldPointVelocity[x-1][y];
		}

	}
	for (x=15*3-1; x>1; x--)
	{
		y = FastRandom()&15;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
		y = (FastRandom()&15)-1;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
	}
}

#endif
#endif /* not used */

#if 0 /* not yet */

void D3D_DrawForceField(int xOrigin, int yOrigin, int zOrigin, int fieldType)
{
	MeshXScale = 4096/16;
	MeshZScale = 4096/16;
	
	for (int field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			int offset = ForceFieldPointDisplacement[x][z];
			
			switch(fieldType)
			{
				case 0:
				{
				 	point->vx = xOrigin+(x*MeshXScale);
				 	point->vy = yOrigin+(z*MeshZScale);
				 	point->vz = zOrigin+offset;
					break;
				}
				case 1:
				{	

					int theta = (z*4095)/15;
					int u = (x*65536)/45;

					int b = MUL_FIXED(2*u,(65536-u));
					int c = MUL_FIXED(u,u);
					int phi = (Phase&4095);
					int x3 = (GetSin(phi))/64;
					int y3 = 5000-(GetCos((phi*3+1000)&4095)/128);
					int z3 = (GetSin((3*phi+1324)&4095))/32;
					int x2 = -x3/2;
					int y2 = 3000;
					int z2 = -z3/4;
					int innerRadius = 100;//GetSin(u/32)/16+offset;

					point->vx = xOrigin+(b*x2+c*x3)/65536+MUL_FIXED(innerRadius,GetSin(theta));
					point->vy = yOrigin-5000+(b*y2+c*y3)/65536;
					point->vz = zOrigin+(b*z2+c*z3)/65536+MUL_FIXED(innerRadius,GetCos(theta));
					break;
				}
				case 2:
				{
					int theta = (z*4095)/15;
					int phi = (x*4095)/45;
					int innerRadius = 1000+offset;
					int outerRadius = 4000;
					

					point->vx = xOrigin+MUL_FIXED(outerRadius-MUL_FIXED(innerRadius,GetSin(theta)),GetCos(phi));
					point->vy = yOrigin+MUL_FIXED(innerRadius,GetCos(theta));
					point->vz = zOrigin+MUL_FIXED(outerRadius-MUL_FIXED(innerRadius,GetSin(theta)),GetSin(phi));
					break;
				}
				case 3:
				{	

					int theta = (x*4095)/45;
					int radius = offset+2000;
					point->vx = xOrigin+MUL_FIXED(radius,GetCos(theta));
					point->vy = yOrigin+(z*MeshZScale);
					point->vz = zOrigin+MUL_FIXED(radius,GetSin(theta));
					break;
				}
			}			

			if (offset<0) offset =-offset;
			offset+=16;

//			offset-=32;
//			if (offset<0) offset = 0;

			if(offset>255) offset=255;
	  
			MeshVertexColour[i] = RGBALIGHT_MAKE(ForceFieldPointColour1[x][z],ForceFieldPointColour2[x][z],255,offset);
			#if TEXTURE_WATER
			MeshWorldVertex[i].vx = point->vx;			
			MeshWorldVertex[i].vz = point->vz;			
			#endif
			
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}	
	}
}


void D3D_DrawPowerFence(int xOrigin, int yOrigin, int zOrigin, int xScale, int yScale, int zScale)
{
	for (int field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			int offset = ForceFieldPointDisplacement[x][z];
			
		 	point->vx = xOrigin+(x*xScale);
		 	point->vy = yOrigin+(z*yScale);
		 	point->vz = zOrigin+(x*zScale);

			if (offset<0) offset =-offset;
			offset+=16;

			if(offset>255) offset=255;
	  
			MeshVertexColour[i] = RGBALIGHT_MAKE(ForceFieldPointColour1[x][z],ForceFieldPointColour2[x][z],255,offset);
			
			/* translate particle into view space */
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}	
	}
}

#endif /* not yet */

void D3D_DrawWaterFall(int xOrigin, int yOrigin, int zOrigin)
{
	int i;
	int noRequired = MUL_FIXED(250,NormalFrameTime);
	for (i=0; i<noRequired; i++)
	{
		VECTORCH velocity;
		VECTORCH position;
		position.vx = xOrigin;
		position.vy = yOrigin-(FastRandom()&511);//+45*MeshXScale;
		position.vz = zOrigin+(FastRandom()%(15*MeshZScale));

		velocity.vy = (FastRandom()&511)+512;//-((FastRandom()&1023)+2048)*8;
		velocity.vx = ((FastRandom()&511)+256)*2;
		velocity.vz = 0;//-((FastRandom()&511))*8;
		MakeParticle(&(position), &velocity, PARTICLE_WATERFALLSPRAY);
	}

#if 0 /* not used */
		#if 0
		noRequired = MUL_FIXED(200,NormalFrameTime);
		for (i=0; i<noRequired; i++)
		{
			VECTORCH velocity;
			VECTORCH position;
			position.vx = xOrigin+(FastRandom()%(15*MeshZScale));
			position.vy = yOrigin+45*MeshXScale;
			position.vz = zOrigin;

			velocity.vy = -((FastRandom()&16383)+4096);
			velocity.vx = ((FastRandom()&4095)-2048);
			velocity.vz = -((FastRandom()&2047)+1048);
			MakeParticle(&(position), &velocity, PARTICLE_WATERFALLSPRAY);
		}
		#endif	
	{
		extern void RenderWaterFall(int xOrigin, int yOrigin, int zOrigin);
		//RenderWaterFall(xOrigin, yOrigin-500, zOrigin+50);
	}
   	return;
	for (int field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			int offset = ForceFieldPointDisplacement[x][z];

		#if 1
			int u = (x*65536)/45;

			int b = MUL_FIXED(2*u,(65536-u));
			int c = MUL_FIXED(u,u);
			int y3 = 45*MeshXScale;
			int x3 = 5000;
			int y2 = 1*MeshXScale;
			int x2 = GetSin(CloakingPhase&4095)+GetCos((CloakingPhase*3+399)&4095);
			x2 = MUL_FIXED(x2,x2)/128;

			if (offset<0) offset =-offset;
			point->vx = xOrigin+MUL_FIXED(b,x2)+MUL_FIXED(c,x3)+offset;
			point->vy = yOrigin+MUL_FIXED(b,y2)+MUL_FIXED(c,y3);
			point->vz = zOrigin+(z*MeshZScale);
			
			if (point->vy>4742)
			{
				if (z<=4)
				{
					point->vy-=MeshXScale; 
					if (point->vy<4742) point->vy=4742;
					if (point->vx<179427) point->vx=179427;
				}
				else if (z<=8)
				{
					point->vx+=(8-z)*1000;
				}
			}

			#else
			if (offset<0) offset =-offset;
		 	point->vx = xOrigin-offset;
		 	point->vy = yOrigin+(x*MeshXScale);
		 	point->vz = zOrigin+(z*MeshZScale);
			#endif


			   	

			offset= (offset/4)+127;

//			offset-=32;
//			if (offset<0) offset = 0;

			if(offset>255) offset=255;
	  
			MeshVertexColour[i] = RGBALIGHT_MAKE(offset,offset,255,offset/2);
			#if TEXTURE_WATER
			MeshWorldVertex[i].vx = point->vx;			
			MeshWorldVertex[i].vz = point->vz;			
			#endif
			
			/* translate particle into view space */
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}	
	}
#endif	
}

#if 0 /* not yet */

void D3D_DrawMoltenMetal(int xOrigin, int yOrigin, int zOrigin)
{
	int i=0;
	int x;
	for (x=0; x<16; x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			
			point->vx = xOrigin+(x*MeshXScale)/15;
			point->vz = zOrigin+(z*MeshZScale)/15;
		 #if 0
			
			int offset=0;

		 	offset = MUL_FIXED(32,GetSin(  (point->vx+point->vz+CloakingPhase)&4095 ) );
		 	offset += MUL_FIXED(16,GetSin(  (point->vx-point->vz*2+CloakingPhase/2)&4095 ) );
			{
				float dx=point->vx-22704;
				float dz=point->vz+20652;
				float a = dx*dx+dz*dz;
				a=sqrt(a);

				offset+= MUL_FIXED(200,GetSin( (((int)a-CloakingPhase)&4095)  ));
			}
		#endif
		 #if 1
			int offset=0;

			/* basic noise ripples */
		 	offset = MUL_FIXED(128,GetSin(  ((point->vx+point->vz)/16+CloakingPhase)&4095 ) );
		 	offset += MUL_FIXED(64,GetSin(  ((point->vx-point->vz*2)/4+CloakingPhase/2)&4095 ) );
		 	offset += MUL_FIXED(64,GetSin(  ((point->vx*5-point->vz)/32+CloakingPhase/5)&4095 ) );

		#endif
			if (offset>450) offset = 450;
			if (offset<-1000) offset = -1000;
			point->vy = yOrigin+offset;

			{
				int shade = 191+(offset+256)/8;
				MeshVertexColour[i] = RGBLIGHT_MAKE(shade,shade,shade);
			}
			
			#if 1
			TranslatePointIntoViewspace(point);
			#else
			point->vx -= Global_VDB_Ptr->VDB_World.vx;
			point->vy -= Global_VDB_Ptr->VDB_World.vy;
			point->vz -= Global_VDB_Ptr->VDB_World.vz;
			MeshWorldVertex[i] = *point;
			RotateVector(point,&(Global_VDB_Ptr->VDB_Mat));
			point->vy = MUL_FIXED(point->vy,87381);

			#endif
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			#if 0
			{
				// v
				MeshWorldVertex[i].vy = (offset+256)*4;
				// u 
				MeshWorldVertex[i].vx = ((MeshWorldVertex[i].vx)&4095);
				
			}
			#else
			{
				Normalise(&MeshWorldVertex[i]);
				// v
				int theta = (MeshWorldVertex[i].vy+offset);
				if (theta<0) theta=0;
				if (theta>ONE_FIXED) theta=ONE_FIXED;

				// u 
				int arctan = ((atan2((double)MeshWorldVertex[i].vx,(double)MeshWorldVertex[i].vz)/ 6.28318530718))*4095;
				MeshWorldVertex[i].vx = (arctan+offset)&4095;

				MeshWorldVertex[i].vy = ArcCos(theta);
				
			}
			#endif


			i++;
		}
	}

	D3DTEXTUREHANDLE TextureHandle = (D3DTEXTUREHANDLE)ImageHeaderArray[StaticImageNumber].D3DHandle;
	if (CurrTextureHandle != TextureHandle)
	{
		OP_STATE_RENDER(1, ExecBufInstPtr);
		STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, TextureHandle, ExecBufInstPtr);
		CurrTextureHandle = TextureHandle;
	}
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_OFF);
	if (NumVertices)
	{
	   WriteEndCodeToExecuteBuffer();
  	   UnlockExecuteBufferAndPrepareForUse();
	   ExecuteBuffer();
  	   LockExecuteBuffer();
	}
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawMoltenMetalMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawMoltenMetalMesh_Clipped();
	}
		
	
}

#endif /* not yet */

void D3D_DrawMoltenMetalMesh_Unclipped(void)
{
	float ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	VECTORCH *point = MeshVertex;
	VECTORCH *pointWS = MeshWorldVertex;

	int i, x, y, z;
	int start;
	
	CheckTriangleBuffer(256, 0, 450, 0, (D3DTexture *)-1, -1, -1, 0);
	
	start = varrc;
	for (i=0; i<256; i++) {
		GLfloat xf, yf, zf;
		GLfloat sf, tf;
		GLfloat w;
		int r, g, b, a;
		
		if (point->vz < 1) point->vz = 1;

		xf =  ((float)point->vx*((float)AvpProjectionX(Global_VDB_Ptr)))/((float)point->vz*(float)ScreenDescriptorBlock.SDB_CentreX);
		yf = -((float)point->vy*((float)AvpProjectionY(Global_VDB_Ptr)))/((float)point->vz*(float)ScreenDescriptorBlock.SDB_CentreY);
		
		z = point->vz + HeadUpDisplayZOffset;
		w = (float)point->vz;
		zf = 1.0f - 2.0f*ZNear/(float)z;
		
		sf = pointWS->vx*WaterUScale+(1.0f/256.0f);
		tf = pointWS->vy*WaterVScale+(1.0f/256.0f);

		b = (MeshVertexColour[i] >> 0)  & 0xFF;
		g = (MeshVertexColour[i] >> 8)  & 0xFF;
		r = (MeshVertexColour[i] >> 16) & 0xFF;
		a = (MeshVertexColour[i] >> 24) & 0xFF;
			
		
		varrp->v[0] = xf*w;
		varrp->v[1] = yf*w;
		varrp->v[2] = zf*w;
		varrp->v[3] = w;
		
		varrp->t[0] = sf;
		varrp->t[1] = tf;
		
		varrp->c[0] = r;
		varrp->c[1] = g;
		varrp->c[2] = b;
		varrp->c[3] = a;

		varrp->s[0] = 0;
		varrp->s[1] = 0;
		varrp->s[2] = 0;
		varrp->s[3] = 0;

		varrp++;
		varrc++;
		
		point++;
		pointWS++;
	}
    
	/* CONSTRUCT POLYS */
	
	for (x = 0; x < 15; x++) {
		for(y = 0; y < 15; y++) {
//			OUTPUT_TRIANGLE(0+x+(16*y),1+x+(16*y),16+x+(16*y), 256);
//			OUTPUT_TRIANGLE(1+x+(16*y),17+x+(16*y),16+x+(16*y), 256);

			tarrp[0].a = start+0+x+(16*y);
			tarrp[0].b = start+1+x+(16*y);
			tarrp[0].c = start+16+x+(16*y);
			
			tarrp[1].a = start+1+x+(16*y);
			tarrp[1].b = start+17+x+(16*y);
			tarrp[1].c = start+16+x+(16*y);
			
			tarrp += 2;
			tarrc += 2;
		}
	}
}

void D3D_DrawMoltenMetalMesh_Clipped(void)
{
	D3D_DrawMoltenMetalMesh_Unclipped();
	return;
#if 0
	int i, x, y, z, c, start;

	float ZNear = (float) (ViewmodelShapeActive && AvP.PlayerType==I_Marine ? ViewmodelNearClip() : Global_VDB_Ptr->VDB_ClipZ * GlobalScale);	

	VECTORCH *point = MeshVertex;
	VECTORCH *pointWS = MeshWorldVertex;

	/* how many triangles drawn the first time, (450-c the second time) */
	c = 0;
	for (x=0; x<15; x++) {
		for(y=0; y<15; y++) {
			int p1 = 0+x+(16*y);
			int p2 = 1+x+(16*y);
			int p3 = 16+x+(16*y);
			int p4 = 17+x+(16*y);
			
			if (MeshVertexOutcode[p1]&&MeshVertexOutcode[p2]&&MeshVertexOutcode[p3]&&MeshVertexOutcode[p4])
				c += 2;
		}
	}
	
	CheckTriangleBuffer(256, 0, c, 0, (D3DTexture *)-1, -1, -1, 0);
	start = varrc;
	
		for (i=0; i<256; i++)
		{
			GLfloat xf, yf, zf;
			GLfloat sf, tf;
			GLfloat w;
			int r, g, b, a;
			
			if (point->vz < 1) point->vz = 1;
			
			x = (point->vx*AvpProjectionX(Global_VDB_Ptr))/point->vz+Global_VDB_Ptr->VDB_CentreX;
			y = (point->vy*AvpProjectionY(Global_VDB_Ptr))/point->vz+Global_VDB_Ptr->VDB_CentreY;

			if (x<Global_VDB_Ptr->VDB_ClipLeft) {
				x=Global_VDB_Ptr->VDB_ClipLeft;
			} else if (x>Global_VDB_Ptr->VDB_ClipRight) {
				x=Global_VDB_Ptr->VDB_ClipRight;	
			}
				
			if (y<Global_VDB_Ptr->VDB_ClipUp) {
				y=Global_VDB_Ptr->VDB_ClipUp;
			} else if (y>Global_VDB_Ptr->VDB_ClipDown) {
				y=Global_VDB_Ptr->VDB_ClipDown;	
			}
			
			sf = pointWS->vx*WaterUScale+(1.0f/256.0f);
			tf = pointWS->vy*WaterVScale+(1.0f/256.0f);
	
			z = point->vz + HeadUpDisplayZOffset;
		  	w = (float)point->vz;
		  	
			b = (MeshVertexColour[i] >> 0)  & 0xFF;
			g = (MeshVertexColour[i] >> 8)  & 0xFF;
			r = (MeshVertexColour[i] >> 16) & 0xFF;
			a = (MeshVertexColour[i] >> 24) & 0xFF;
			
			xf =  ((float)x - (float)ScreenDescriptorBlock.SDB_CentreX - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreX - 0.5f);
			yf = -((float)y - (float)ScreenDescriptorBlock.SDB_CentreY - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreY - 0.5f);
			zf = 1.0f - 2.0f*ZNear/(float)z;

			varrp->v[0] = xf*w;
			varrp->v[1] = yf*w;
			varrp->v[2] = zf*w;
			varrp->v[3] = w;
			
			varrp->t[0] = sf;
			varrp->t[1] = tf;
			
			varrp->c[0] = r;
			varrp->c[1] = g;
			varrp->c[2] = b;
			varrp->c[3] = a;

			varrp->s[0] = 0;
			varrp->s[1] = 0;
			varrp->s[2] = 0;
			varrp->s[3] = 0;

			varrp++;
			varrc++;
			
			point++;
			pointWS++;
		}

	/* CONSTRUCT POLYS */
	{
		for (x=0; x<15; x++)
		{
			for(y=0; y<15; y++)
			{
				int p1 = 0+x+(16*y);
				int p2 = 1+x+(16*y);
				int p3 = 16+x+(16*y);
				int p4 = 17+x+(16*y);
				
#if 0
				#if 0
				if (MeshVertexOutcode[p1]&&MeshVertexOutcode[p2]&&MeshVertexOutcode[p3])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p1,p2,p3, 256);
				}
				if (MeshVertexOutcode[p2]&&MeshVertexOutcode[p3]&&MeshVertexOutcode[p4])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p2,p4,p3, 256);
				}	
				#else
				if (MeshVertexOutcode[p1]&&MeshVertexOutcode[p2]&&MeshVertexOutcode[p3]&&MeshVertexOutcode[p4])
				{
					OP_TRIANGLE_LIST(2, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p1,p2,p3, 256);
					OUTPUT_TRIANGLE(p2,p4,p3, 256);
				}	

				#endif
#endif
				if (MeshVertexOutcode[p1]&&MeshVertexOutcode[p2]&&MeshVertexOutcode[p3]&&MeshVertexOutcode[p4]) {
					tarrp[0].a = start+p1;
					tarrp[0].b = start+p2;
					tarrp[0].c = start+p3;
					tarrp[1].a = start+p2;
					tarrp[1].b = start+p4;
					tarrp[1].c = start+p3;

					tarrp += 2;
					tarrc += 2;
				}
			}
		}
	}
	{
		POLYHEADER fakeHeader;

		fakeHeader.PolyFlags = 0;
		fakeHeader.PolyColour = 0;
		RenderPolygon.TranslucencyMode = TRANSLUCENCY_NORMAL;
		
		for (x=0; x<15; x++)
		{
			for(y=0; y<15; y++)
			{
				int p[4];
				p[0] = 0+x+(16*y);
				p[1] = 1+x+(16*y);
				p[2] = 17+x+(16*y);
				p[3] = 16+x+(16*y);

				if (!(MeshVertexOutcode[p[0]]&&MeshVertexOutcode[p[1]]&&MeshVertexOutcode[p[2]]&&MeshVertexOutcode[p[3]]))
				{
					for (i=0; i<4; i++) 
					{
						VerticesBuffer[i].X	= MeshVertex[p[i]].vx;
						VerticesBuffer[i].Y	= MeshVertex[p[i]].vy;
						VerticesBuffer[i].Z	= MeshVertex[p[i]].vz;
						VerticesBuffer[i].U = MeshWorldVertex[p[i]].vx*(WaterUScale*128.0f*65536.0f);
						VerticesBuffer[i].V = MeshWorldVertex[p[i]].vy*(WaterVScale*128.0f*65536.0f);
															   
						VerticesBuffer[i].A = (MeshVertexColour[p[i]]&0xff000000)>>24;
						VerticesBuffer[i].R = (MeshVertexColour[p[i]]&0x00ff0000)>>16;
						VerticesBuffer[i].G	= (MeshVertexColour[p[i]]&0x0000ff00)>>8;
						VerticesBuffer[i].B = MeshVertexColour[p[i]]&0x000000ff;
						VerticesBuffer[i].SpecularR = 0;
						VerticesBuffer[i].SpecularG = 0;
						VerticesBuffer[i].SpecularB = 0;
						RenderPolygon.NumberOfVertices=4;
						
					}
					if (QuadWithinFrustrum())
					{		 
						GouraudTexturedPolygon_ClipWithZ();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithNegativeX();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithPositiveY();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithNegativeY();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithPositiveX();
						if(RenderPolygon.NumberOfVertices<3) continue;
						
						/* draw polygon */
					}
				}
			}
		}
	}
#endif	
}

#if 0 /* not yet */

void D3D_DrawWaterOctagonPatch(int xOrigin, int yOrigin, int zOrigin, int xOffset, int zOffset)
{
	float grad = 2.414213562373;
	int i=0;
	int x;
	for (x=xOffset; x<16+xOffset; x++)
	{
		int z;
		for(z=zOffset; z<16+zOffset; z++)
		{
			VECTORCH *point = &MeshVertex[i];

		  	if (x>z)
			{
				float m,xs;
				if (x!=0)
				{
					m = (float)(z)/(float)(x);
					xs = grad/(grad+m);
				}
				else
				{
					xs = 0;
				}
				#if 1
				f2i(point->vx , xs*x*MeshXScale);
				f2i(point->vz , (grad-grad*xs)*x*MeshZScale);
				#else
				point->vx = xs*x*MeshXScale;
				point->vz = (grad-grad*xs)*x*MeshZScale;
				#endif
			}
			else
			{
				float m,xs;
				if (z!=0)
				{
					m = (float)(x)/(float)(z);
					xs = grad/(grad+m);
				}
				else
				{
					xs = 0;
				}
				#if 1
				f2i(point->vz ,	xs*z*MeshZScale);
				f2i(point->vx ,	(grad-grad*xs)*z*MeshXScale);
				#else
				point->vz =	xs*z*MeshZScale;
				point->vx =	(grad-grad*xs)*z*MeshXScale;
				#endif
			}

			point->vx += xOrigin;
			point->vz += zOrigin;

			int offset = EffectOfRipples(point);
			
			point->vy = yOrigin+offset;

			#if 0
			MeshVertexColour[i] = LightSourceWaterPoint(point,offset);
			#else
			{
				int alpha = 128-offset/4;
		//		if (alpha>255) alpha = 255;
		//		if (alpha<128) alpha = 128;
				switch (CurrentVisionMode)
				{
					default:
					case VISION_MODE_NORMAL:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(10,51,28,alpha);
						break;
					}
					case VISION_MODE_IMAGEINTENSIFIER:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(0,51,0,alpha);
						break;
					}
					case VISION_MODE_PRED_THERMAL:
					case VISION_MODE_PRED_SEEALIENS:
					case VISION_MODE_PRED_SEEPREDTECH:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(0,0,28,alpha);
					  	break;
					}
				}

			}
			#endif
			TranslatePointIntoViewspace(point);
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}

	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}
		
	
}

#endif /* not yet */

void D3D_DrawCable(VECTORCH *centrePtr, MATRIXCH *orientationPtr)
{
	int field;
	
	CurrTextureHandle = NULL;
	CheckBoundTextureIsCorrect(NULL);
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	pglDepthMask(GL_FALSE);

	MeshXScale = 4096/16;
	MeshZScale = 4096/16;
	
	for (field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			{	
				int innerRadius = 20;
				VECTORCH radius;
				int theta = ((4096*z)/15)&4095;
				int rOffset = GetSin((x*64+theta/32-CloakingPhase)&4095);
				rOffset = MUL_FIXED(rOffset,rOffset)/512;


				radius.vx = MUL_FIXED(innerRadius+rOffset/8,GetSin(theta));
				radius.vy = MUL_FIXED(innerRadius+rOffset/8,GetCos(theta));
				radius.vz = 0;
				
				RotateVector(&radius,orientationPtr);

				point->vx = centrePtr[x].vx+radius.vx;
				point->vy = centrePtr[x].vy+radius.vy;
				point->vz = centrePtr[x].vz+radius.vz;

				MeshVertexColour[i] = RGBA_MAKE(0,rOffset,255,128);

			}
			
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
   	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawMoltenMetalMesh_Unclipped();
	   //	D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawMoltenMetalMesh_Clipped();
  	   //	D3D_DrawWaterMesh_Clipped();
	}	
	}
	
	pglDepthMask(GL_TRUE);
}
