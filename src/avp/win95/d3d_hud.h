#ifndef __AVP_WIN95_D3D_HUD_H__
#define __AVP_WIN95_D3D_HUD_H__

#include "langenum.h"

extern int TransparencyLevelOfHUD;
/* takes values of 0 (not drawn) to 255 (opaque) */

struct VertexTag
{
	int X;
	int Y;
	int U;
	int V;
};

struct HUDVertexF { float X,Y,U,V; };
extern void D3D_HUDPolygonFloat_Output(int imageNumber,const struct HUDVertexF *vertices,int count,unsigned int colour);

typedef struct
{
	int ImageNumber;

	int TopLeftX;
	int TopLeftY;
	
	int TopLeftU;
	int TopLeftV;

	int Width;
	int Height;

	int Scale;

	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
	unsigned char Translucency; /* 0 == invisible, 255 == opaque */ 

} HUDImageDesc;


typedef struct
{
	char Character;
	int X;
	int Y;

	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
	unsigned char Alpha;

} HUDCharDesc;

/* Modern displays can be wider than the original 4:3 HUD canvas.  HUD source
 * renders against this independently anchored layout; the OpenGL backend maps
 * it into the physical viewport after the 3D scene is complete. */
enum HUD_ANCHOR_MODE
{
	HUD_ANCHOR_4_3 = 0,
	HUD_ANCHOR_16_9,
	HUD_ANCHOR_UNLOCKED,
};

/* A composed HUD block can request one shared scale origin so every sprite,
 * glyph and overlay within it remains correlated. */
enum HUD_SCALE_ANCHOR
{
	HUD_SCALE_ANCHOR_AUTO = 0,
	HUD_SCALE_ANCHOR_TOP_LEFT,
	HUD_SCALE_ANCHOR_TOP_RIGHT,
	HUD_SCALE_ANCHOR_BOTTOM_LEFT,
	HUD_SCALE_ANCHOR_BOTTOM_RIGHT,
	HUD_SCALE_ANCHOR_CENTRE,
};

extern int HUDAnchorMode;
extern int HUDAnchorPosition;
extern int HUDAnchorUnlockedPosition;
extern int HUDAnchorPositionMarine;
extern int HUDAnchorPositionPredator;
extern int HUDAnchorPositionAlien;
extern int HUDAnchorUnlockedPositionMarine;
extern int HUDAnchorUnlockedPositionPredator;
extern int HUDAnchorUnlockedPositionAlien;

#define HUD_USER_SCALE_TENTHS_MINIMUM 3
#define HUD_USER_SCALE_TENTHS_DEFAULT 7
#define HUD_USER_SCALE_TENTHS_MAXIMUM 13
extern int HUDUserScaleTenths;
extern int HUDUserScaleMarineTenths;
extern int HUDUserScalePredatorTenths;
extern int HUDUserScaleAlienTenths;
extern int HUD_GetUserScaleFixed(void);
extern void HUD_SynchronisePerSpeciesOptions(void);
extern void HUD_RestorePerSpeciesOptions(void);

/* Aim reticles have an independent user scale and are intentionally not part
 * of the anchored HUD composition. Alien has no affected reticle path. */
#define CROSSHAIR_SCALE_TENTHS_MINIMUM 3
#define CROSSHAIR_SCALE_TENTHS_DEFAULT 10
#define CROSSHAIR_SCALE_TENTHS_MAXIMUM 30
/* Marine-only mask dilation. 0 preserves the original atlas blit. */
#define MARINE_CROSSHAIR_THICKNESS_X10 0
#define MARINE_CROSSHAIR_THICKNESS_X15 1
#define MARINE_CROSSHAIR_THICKNESS_X20 2
extern int CrosshairScaleTenths;
extern int MarineCrosshairThicknessMode;

/* Fullscreen interference overlays use 1.0 as the modern physical-density
 * baseline.  0.5 halves the displayed grain size by tiling the same source
 * texture twice as densely; it changes neither timing nor opacity. */
#define NOISE_OVERLAY_SCALE_1X 0
#define NOISE_OVERLAY_SCALE_HALF 1
extern int NoiseOverlayScaleMode;

/* HUD sprites, glyphs and numerics share one explicit sampling policy. */
#define HUD_TEXTURE_FILTER_BILINEAR 0
#define HUD_TEXTURE_FILTER_NEAREST 1
extern int HUDTextureFilterMode;
extern int CrosshairScaleMarineTenths;
extern int CrosshairScalePredatorTenths;
extern int HUD_GetCrosshairScaleFixed(void);
extern void HUD_ClampCrosshairScaleForCurrentSpecies(void);
extern void HUD_RestoreCrosshairScaleForCurrentSpecies(void);
extern void HUD_SetScaleAnchor(enum HUD_SCALE_ANCHOR anchor);
extern enum HUD_SCALE_ANCHOR HUD_GetScaleAnchor(void);
extern void HUD_RefreshLayout(void);
extern void HUD_AnchorBegin(void);
extern void HUD_AnchorEnd(void);
extern int HUD_AnchorIsActive(void);
extern int HUD_GetLayoutWidth(void);
extern int VideoHUDUsesWideLayout(void);
extern int HUD_GetEffectiveAnchorMode(void);
extern int HUD_GetLayoutHeight(void);
extern int HUD_GetLayoutOffsetX(void);
extern int HUD_GetLayoutOffsetY(void);
extern int HUD_LayoutXFromScreen(int screenX);
extern int HUD_LayoutYFromScreen(int screenY);
extern void HUD_ResolveFPSCounterPlacement(int position, int textWidth, int textHeight, int *x, int *y);
extern void D3D_HUD_Setup(void);
extern void D3D_HUDQuad_Output(int imageNumber,struct VertexTag *quadVerticesPtr,unsigned int colour);
 
extern void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr);
extern void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr);
extern int ConsoleTextScaleMode;
extern int D3D_GetConsoleTextScaleFixed(void);
extern int D3D_ScaleConsoleTextDimension(int value);
extern int D3D_GetActiveConsoleTextScaleFixed(void);
extern int D3D_ScaleActiveConsoleTextDimension(int value);
extern void D3D_SetConsoleTextRenderActive(int active);
extern void D3D_SetBriefingTextRenderActive(int active);
extern void D3D_SetInGameMenuTextRenderActive(int active);
extern void D3D_SetStatisticsTextRenderActive(int active);
extern int D3D_IsFPSCounterTextRenderActive(void);
extern int D3D_IsBriefingOrConsoleTextRenderActive(void);
extern int D3D_IsInGameMenuTextRenderActive(void);
extern void D3D_SetFPSCounterTextRenderActive(int active);
extern int D3D_IsStatisticsTextRenderActive(void);


extern void D3D_BlitWhiteChar(int x, int y, unsigned char c);



/* KJL 17:55:48 18/04/98 - called by HUD.c */
extern void Render_HealthAndArmour(unsigned int health, unsigned int armour);
extern void Render_MarineAmmo(enum TEXTSTRING_ID ammoText, enum TEXTSTRING_ID magazinesText, unsigned int magazines, enum TEXTSTRING_ID roundsText, unsigned int rounds, int primaryAmmo);
void D3D_DrawHUDPredatorDigit(HUDCharDesc *charDescPtr, int scale);
void Draw_HUDImage(HUDImageDesc *imageDescPtr);

#endif
