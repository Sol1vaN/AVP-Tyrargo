#include "screen_layout.h"
#include "molecular_acid.h"
#include "hud_motion.h"
/* KJL 14:43:27 10/6/97 - d3d_hud.cpp

	Things just got too messy with ddplat.cpp & d3_func.cpp,
	so this file will hold all Direct 3D hud code.

 */
extern "C" {

// Mysterious definition required by objbase.h 
// (included via one of the include files below)
// to start definition of obscure unique in the
// universe IDs required  by Direct3D before it
// will deign to cough up with anything useful...

#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"

#include "hudgfx.h"
#include "huddefs.h"
//#include "hud_data.h"
#include "kshape.h"
#include "chnktexi.h"


#include "hud_layout.h"
#include "language.h"


extern void D3D_RenderHUDString_Centred(char *stringPtr, int centreX, int y, int colour);
extern void D3D_RenderHUDNumber_Centred(unsigned int number,int x,int y,int colour);

extern "C++" 									  
{
#include "r2base.h"
#include "pcmenus.h"
//#include "projload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
};

#include "text_encoding.h"
#include "d3d_hud.h"


#define UseLocalAssert No
#include "ourasert.h"
											

#include "vision.h"
#define RGBLIGHT_MAKE(rr,gg,bb) \
( \
	LCCM_NORMAL == d3d_light_ctrl.ctrl ? \
		RGB_MAKE(rr,gg,bb) \
	: LCCM_CONSTCOLOUR == d3d_light_ctrl.ctrl ? \
		RGB_MAKE(MUL_FIXED(rr,d3d_light_ctrl.r),MUL_FIXED(gg,d3d_light_ctrl.g),MUL_FIXED(bb,d3d_light_ctrl.b)) \
	: \
		RGB_MAKE(d3d_light_ctrl.GetR(rr),d3d_light_ctrl.GetG(gg),d3d_light_ctrl.GetB(bb)) \
)
#define RGBALIGHT_MAKE(rr,gg,bb,aa) \
( \
		RGBA_MAKE(rr,gg,bb,aa) \
)


/* Probe 137: HUD-owned collision bounds for the independent FPS overlay.
 * Bounds are collected in the same logical canvas and scale path as each HUD
 * element, then resolved after HUD_AnchorEnd(). */
typedef struct
{
	int valid;
	int left;
	int top;
	int right;
	int bottom;
} FPSHUDRect;

static FPSHUDRect FPSAlienHealthRect;
static FPSHUDRect FPSMarineHealthRect;
static FPSHUDRect FPSMarineAmmoRect;
static FPSHUDRect FPSMarineRadarRect;
static FPSHUDRect FPSPredatorHealthRect;
static FPSHUDRect FPSPredatorEnergyRect;
static FPSHUDRect FPSPredatorSpeargunRect;

static void FPSHUD_ResetRect(FPSHUDRect *rect)
{
	rect->valid = 0;
}

static void FPSHUD_ResetProtectedRects(void)
{
	FPSHUD_ResetRect(&FPSAlienHealthRect);
	FPSHUD_ResetRect(&FPSMarineHealthRect);
	FPSHUD_ResetRect(&FPSMarineAmmoRect);
	FPSHUD_ResetRect(&FPSMarineRadarRect);
	FPSHUD_ResetRect(&FPSPredatorHealthRect);
	FPSHUD_ResetRect(&FPSPredatorEnergyRect);
	FPSHUD_ResetRect(&FPSPredatorSpeargunRect);
}

static void FPSHUD_AddLogicalRect(FPSHUDRect *rect, int left, int top, int right, int bottom, enum HUD_SCALE_ANCHOR anchor)
{
	int layoutWidth = HUD_GetLayoutWidth();
	int layoutHeight = HUD_GetLayoutHeight();
	int hudScale = HUD_GetUserScaleFixed();
	int anchorX;
	int anchorY;
	int transformedLeft;
	int transformedTop;
	int transformedRight;
	int transformedBottom;

	if (left > right) { int swap = left; left = right; right = swap; }
	if (top > bottom) { int swap = top; top = bottom; bottom = swap; }

	switch (anchor)
	{
		case HUD_SCALE_ANCHOR_TOP_LEFT: anchorX = 0; anchorY = 0; break;
		case HUD_SCALE_ANCHOR_TOP_RIGHT: anchorX = layoutWidth; anchorY = 0; break;
		case HUD_SCALE_ANCHOR_BOTTOM_LEFT: anchorX = 0; anchorY = layoutHeight; break;
		case HUD_SCALE_ANCHOR_BOTTOM_RIGHT: anchorX = layoutWidth; anchorY = layoutHeight; break;
		case HUD_SCALE_ANCHOR_CENTRE: anchorX = layoutWidth / 2; anchorY = layoutHeight / 2; break;
		case HUD_SCALE_ANCHOR_AUTO:
		default:
			anchorX = ((left + right) < layoutWidth / 2) ? 0 : (((left + right) > (layoutWidth * 3) / 2) ? layoutWidth : layoutWidth / 2);
			anchorY = ((top + bottom) < layoutHeight / 2) ? 0 : (((top + bottom) > (layoutHeight * 3) / 2) ? layoutHeight : layoutHeight / 2);
			break;
	}

	transformedLeft = anchorX + MUL_FIXED(hudScale, left - anchorX) + HUD_GetLayoutOffsetX();
	transformedRight = anchorX + MUL_FIXED(hudScale, right - anchorX) + HUD_GetLayoutOffsetX();
	transformedTop = anchorY + MUL_FIXED(hudScale, top - anchorY) + HUD_GetLayoutOffsetY();
	transformedBottom = anchorY + MUL_FIXED(hudScale, bottom - anchorY) + HUD_GetLayoutOffsetY();
	if (transformedLeft > transformedRight) { int swap = transformedLeft; transformedLeft = transformedRight; transformedRight = swap; }
	if (transformedTop > transformedBottom) { int swap = transformedTop; transformedTop = transformedBottom; transformedBottom = swap; }

    /* Match the same homogeneous transform as glyphs and radar artwork. */
    if(AvP.PlayerType==I_Marine){
        float x[4]={(float)transformedLeft,(float)transformedRight,(float)transformedRight,(float)transformedLeft};
        float y[4]={(float)transformedTop,(float)transformedTop,(float)transformedBottom,(float)transformedBottom};
        for(int i=0;i<4;i++)MarineHUDMotionTransform(&x[i],&y[i]);
        transformedLeft=(int)floor(fmin(fmin(x[0],x[1]),fmin(x[2],x[3])));
        transformedRight=(int)ceil(fmax(fmax(x[0],x[1]),fmax(x[2],x[3])));
        transformedTop=(int)floor(fmin(fmin(y[0],y[1]),fmin(y[2],y[3])));
        transformedBottom=(int)ceil(fmax(fmax(y[0],y[1]),fmax(y[2],y[3])));
    }
	if (!rect->valid)
	{
		rect->valid = 1;
		rect->left = transformedLeft;
		rect->top = transformedTop;
		rect->right = transformedRight;
		rect->bottom = transformedBottom;
	}
	else
	{
		if (transformedLeft < rect->left) rect->left = transformedLeft;
		if (transformedTop < rect->top) rect->top = transformedTop;
		if (transformedRight > rect->right) rect->right = transformedRight;
		if (transformedBottom > rect->bottom) rect->bottom = transformedBottom;
	}
}

static int FPSHUD_Overlaps(const FPSHUDRect *rect, int left, int top, int right, int bottom)
{
	return rect->valid && left < rect->right && right > rect->left && top < rect->bottom && bottom > rect->top;
}

/* Return the actual unlocked HUD canvas ratio as x10000. */
static int FPSHUD_FreeCanvasRatio10000(void)
{
	int layoutHeight;
	if (AvP.PlayerType == I_Alien || HUD_GetEffectiveAnchorMode() != HUD_ANCHOR_UNLOCKED) return 0;
	layoutHeight = HUD_GetLayoutHeight();
	if (layoutHeight <= 0) return 0;
	return (HUD_GetLayoutWidth() * 10000 + layoutHeight / 2) / layoutHeight;
}

/* Speargun digits are bottom-left anchored in DisplayPredatorHealthAndEnergy. */
static int FPSHUD_GetPredatorSpeargunTopY(void)
{
	int layoutHeight = HUD_GetLayoutHeight();
	int digitScale = DIV_FIXED(layoutHeight, 480);
	int digitStep = MUL_FIXED(51, digitScale);
	int logicalTop = layoutHeight - digitStep / 2-(int)(((int64_t)5*ONE_FIXED+HUD_GetUserScaleFixed()-1)/HUD_GetUserScaleFixed());
	return layoutHeight + MUL_FIXED(HUD_GetUserScaleFixed(), logicalTop - layoutHeight) + HUD_GetLayoutOffsetY();
}
void HUD_ResolveFPSCounterPlacement(int position, int textWidth, int textHeight, int *x, int *y)
{
	const FPSHUDRect *protectedRect = 0;
	int left;
	int top;
	int right;
	int bottom;
	int margin = 12;
	int freeCanvasRatio = FPSHUD_FreeCanvasRatio10000();
	int forceRelocation = 0;
	int suppressRelocation = 0;
	const int screenWidth = ScreenDescriptorBlock.SDB_Width;
	const int screenHeight = ScreenDescriptorBlock.SDB_Height;

	if (AvP.PlayerType == I_Marine)
	{
		if (position == 1) protectedRect = &FPSMarineHealthRect;
		else if (position == 3) protectedRect = &FPSMarineAmmoRect;
		else if (position == 4) protectedRect = &FPSMarineRadarRect;

		/* Keep screen corners through 1.60:1; at 1.61:1 use the safe side. */
		if (position == 1 || position == 3 || position == 4)
		{
			if (freeCanvasRatio >= 16100) forceRelocation = 1;
			else if (freeCanvasRatio > 0) suppressRelocation = 1;
		}
	}
	else if (AvP.PlayerType == I_Predator)
	{
		if (position == 1) protectedRect = &FPSPredatorEnergyRect;
		else if (position == 2) protectedRect = &FPSPredatorHealthRect;
		else if (position == 4) protectedRect = &FPSPredatorSpeargunRect;

		/* Preserve the wide-canvas preference, but never suppress a real collision. */
		if (position == 1)
		{
			if (freeCanvasRatio >= 16200) forceRelocation = 1;
            /* Actual overlap remains authoritative on narrow free canvases. */
		}
	}

	else if (AvP.PlayerType == I_Alien && (position == 3 || position == 4))
    {
        protectedRect = &FPSAlienHealthRect;
    }

	if (!protectedRect || !protectedRect->valid) return;

	if (position == 1 || position == 3)
	{
		left = *x - textWidth;
		right = *x;
	}
	else
	{
		left = *x;
		right = *x + textWidth;
	}
	top = *y;
	bottom = *y + textHeight;

	if (!forceRelocation && (suppressRelocation || !FPSHUD_Overlaps(protectedRect, left, top, right, bottom))) return;

	if (position == 1)
	{
		/* Keep a precise 5px logical clearance from the measured health rect. */
		if (AvP.PlayerType == I_Marine) margin = 5;
		*x = protectedRect->left - margin;
		if (*x < textWidth + 12) *x = textWidth + 12;
	}
	else if (position == 2 || (position == 4 && AvP.PlayerType == I_Marine))
	{
		*x = protectedRect->right + margin;
		if (*x + textWidth > screenWidth - margin) *x = screenWidth - textWidth - margin;
	}
	else
	{
		int protectedTop = protectedRect->top;
		if (position == 4 && AvP.PlayerType == I_Predator)
		{
			protectedTop = FPSHUD_GetPredatorSpeargunTopY();
		}
		*y = protectedTop - textHeight - margin;
		if (*y < margin) *y = margin;
	}
}
void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr);
void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr);

extern void YClipMotionTrackerVertices(struct VertexTag *v1, struct VertexTag *v2);
extern void XClipMotionTrackerVertices(struct VertexTag *v1, struct VertexTag *v2);
/* HUD globals */
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern enum HUD_RES_ID HUDResolution;

signed int HUDTranslucencyLevel=64;

static int MotionTrackerHalfWidth;
static int MotionTrackerTextureSize;
static int MotionTrackerCentreY;
static int MotionTrackerCentreX;
static int MT_BlipHeight;
static int MT_BlipWidth;
static HUDImageDesc BlueBar;


int HUDImageNumber;
int SpecialFXImageNumber;
int SmokyImageNumber;
int ChromeImageNumber;
int CloudyImageNumber;
int BurningImageNumber;
int HUDFontsImageNumber;
int RebellionLogoImageNumber;
int FoxLogoImageNumber;
int MotionTrackerScale;
int PredatorVisionChangeImageNumber;
int PredatorNumbersImageNumber;
int StaticImageNumber;
int AlienTongueImageNumber;
int AAFontImageNumber;
int WaterShaftImageNumber;


int HUDScaleFactor;

/* HUD geometry uses its own logical rectangle.  Its height always tracks the
 * real display height, so sprites and glyphs retain their original 1:1 aspect;
 * only the left/right anchors move as the chosen canvas gets wider. */
int HUDAnchorMode = HUD_ANCHOR_16_9;
int HUDAnchorPosition = 0;
int HUDAnchorUnlockedPosition = 0;
int HUDAnchorPositionMarine = 0;
int HUDAnchorPositionPredator = 0;
int HUDAnchorPositionAlien = 0;
int HUDAnchorUnlockedPositionMarine = 0;
int HUDAnchorUnlockedPositionPredator = 0;
int HUDAnchorUnlockedPositionAlien = 0;
int HUDUserScaleTenths = 7;
int HUDUserScaleMarineTenths = 7;
int HUDUserScalePredatorTenths = 7;
int HUDUserScaleAlienTenths = 7;
static int ActiveHUDOptionsSpecies = -1;
int CrosshairScaleTenths = CROSSHAIR_SCALE_TENTHS_DEFAULT;
int CrosshairScaleMarineTenths = CROSSHAIR_SCALE_TENTHS_DEFAULT;
int CrosshairScalePredatorTenths = 20;
/* Marine-only visual thickness; it does not alter aim coordinates or quad scale. */
int MarineCrosshairThicknessMode = MARINE_CROSSHAIR_THICKNESS_X10;
static int ActiveCrosshairScaleSpecies = -1;
static int HUDAnchorRenderingActive = 0;
static enum HUD_SCALE_ANCHOR HUDScaleAnchorOverride = HUD_SCALE_ANCHOR_AUTO;

/* Probe 133: the shared text scaler is opt-in. Console, transition briefings,
 * and standard in-game menu rows enter it explicitly; HUD and main menus stay at 1x. */
int ConsoleTextScaleMode = 2;
static int ConsoleTextRenderActive = 0;
static int BriefingTextRenderActive = 0;
static int InGameMenuTextRenderActive = 0;
static int StatisticsTextRenderActive = 0;
static int FPSCounterTextRenderActive = 0;

int D3D_GetConsoleTextScaleFixed(void)
{
 return ConsoleScaleFixed(ConsoleTextScaleMode);
}

int D3D_ScaleConsoleTextDimension(int value)
{
	return MUL_FIXED(value, D3D_GetConsoleTextScaleFixed());
}

int D3D_GetActiveConsoleTextScaleFixed(void)
{
	return (ConsoleTextRenderActive || BriefingTextRenderActive || InGameMenuTextRenderActive || StatisticsTextRenderActive || FPSCounterTextRenderActive)
		? D3D_GetConsoleTextScaleFixed() : ONE_FIXED;
}

int D3D_ScaleActiveConsoleTextDimension(int value)
{
	return MUL_FIXED(value, D3D_GetActiveConsoleTextScaleFixed());
}

void D3D_SetConsoleTextRenderActive(int active)
{
	ConsoleTextRenderActive = active ? 1 : 0;
}

int D3D_IsBriefingOrConsoleTextRenderActive(void)
{ return ConsoleTextRenderActive || BriefingTextRenderActive; }

void D3D_SetBriefingTextRenderActive(int active)
{
	BriefingTextRenderActive = active ? 1 : 0;
}

int D3D_IsInGameMenuTextRenderActive(void) { return InGameMenuTextRenderActive; }

void D3D_SetInGameMenuTextRenderActive(int active)
{
	InGameMenuTextRenderActive = active ? 1 : 0;
}

void D3D_SetStatisticsTextRenderActive(int active)
{
	StatisticsTextRenderActive = active ? 1 : 0;
}

int D3D_IsFPSCounterTextRenderActive(void){return FPSCounterTextRenderActive;}

void D3D_SetFPSCounterTextRenderActive(int active)
{
	FPSCounterTextRenderActive = active ? 1 : 0;
}

int D3D_IsStatisticsTextRenderActive(void)
{
	return StatisticsTextRenderActive;
}

static int HUD_ClampPosition(int value)
{
	if (value < 0) return 0;
	if (value > 100) return 100;
	return value;
}

static int HUDOptionsSpecies(void)
{
	if (AvP.PlayerType == I_Predator) return I_Predator;
	if (AvP.PlayerType == I_Alien) return I_Alien;
	return I_Marine;
}
static int HUD_ClampScale(int value)
{
	if (value < HUD_USER_SCALE_TENTHS_MINIMUM) return HUD_USER_SCALE_TENTHS_MINIMUM;
	if (value > HUD_USER_SCALE_TENTHS_MAXIMUM) return HUD_USER_SCALE_TENTHS_MAXIMUM;
	return value;
}
static void HUD_StoreActivePerSpeciesOptions(void)
{
	if (ActiveHUDOptionsSpecies == I_Predator) {
		HUDAnchorPositionPredator = HUD_ClampPosition(HUDAnchorPosition); HUDAnchorUnlockedPositionPredator = HUD_ClampPosition(HUDAnchorUnlockedPosition); HUDUserScalePredatorTenths = HUD_ClampScale(HUDUserScaleTenths);
	} else if (ActiveHUDOptionsSpecies == I_Alien) {
		HUDAnchorPositionAlien = HUD_ClampPosition(HUDAnchorPosition); HUDAnchorUnlockedPositionAlien = HUD_ClampPosition(HUDAnchorUnlockedPosition); HUDUserScaleAlienTenths = HUD_ClampScale(HUDUserScaleTenths);
	} else if (ActiveHUDOptionsSpecies == I_Marine) {
		HUDAnchorPositionMarine = HUD_ClampPosition(HUDAnchorPosition); HUDAnchorUnlockedPositionMarine = HUD_ClampPosition(HUDAnchorUnlockedPosition); HUDUserScaleMarineTenths = HUD_ClampScale(HUDUserScaleTenths);
	}
}
void HUD_SynchronisePerSpeciesOptions(void)
{
	int species = HUDOptionsSpecies();
	if (ActiveHUDOptionsSpecies != species) {
		HUD_StoreActivePerSpeciesOptions();
		if (species == I_Predator) { HUDAnchorPosition = HUDAnchorPositionPredator; HUDAnchorUnlockedPosition = HUDAnchorUnlockedPositionPredator; HUDUserScaleTenths = HUDUserScalePredatorTenths; }
		else if (species == I_Alien) { HUDAnchorPosition = HUDAnchorPositionAlien; HUDAnchorUnlockedPosition = HUDAnchorUnlockedPositionAlien; HUDUserScaleTenths = HUDUserScaleAlienTenths; }
		else { HUDAnchorPosition = HUDAnchorPositionMarine; HUDAnchorUnlockedPosition = HUDAnchorUnlockedPositionMarine; HUDUserScaleTenths = HUDUserScaleMarineTenths; }
		ActiveHUDOptionsSpecies = species;
	}
	HUDAnchorPosition = HUD_ClampPosition(HUDAnchorPosition);
	HUDAnchorUnlockedPosition = HUD_ClampPosition(HUDAnchorUnlockedPosition);
	HUDUserScaleTenths = HUD_ClampScale(HUDUserScaleTenths);
	HUD_StoreActivePerSpeciesOptions();
}
void HUD_RestorePerSpeciesOptions(void)
{
	ActiveHUDOptionsSpecies = -1;
	HUD_SynchronisePerSpeciesOptions();
}
int HUD_GetEffectiveAnchorMode(void)
{
 if(!VideoHUDUsesWideLayout())return HUD_ANCHOR_4_3;
 return AvP.PlayerType==I_Alien?HUD_ANCHOR_16_9:HUDAnchorMode;
}
int HUD_GetLayoutHeight(void)
{
	return (ScreenDescriptorBlock.SDB_Height > 0) ? ScreenDescriptorBlock.SDB_Height : 480;
}

int HUD_GetLayoutWidth(void)
{
	int physicalWidth;
	HUD_SynchronisePerSpeciesOptions();
	physicalWidth = ScreenDescriptorBlock.SDB_Width;
	int physicalHeight = HUD_GetLayoutHeight();
	int minimumWidth = (physicalHeight * 4) / 3;
	int targetWidth;

	if (physicalWidth <= 0) physicalWidth = 640;
	if (minimumWidth > physicalWidth) minimumWidth = physicalWidth;

	/* Narrow displays use 4:3. Alien uses 16:9 on wide displays.
     * Saved preferences survive temporary display constraints. */
	switch (HUD_GetEffectiveAnchorMode())
	{
		case HUD_ANCHOR_16_9:
			targetWidth = (physicalHeight * 16) / 9;
			if (targetWidth > physicalWidth) targetWidth = physicalWidth;
			if (targetWidth < minimumWidth) targetWidth = minimumWidth;
			return targetWidth;
		case HUD_ANCHOR_UNLOCKED:
			return minimumWidth + ((physicalWidth - minimumWidth) * HUD_ClampPosition(HUDAnchorPosition) + 50) / 100;
		case HUD_ANCHOR_4_3:
		default:
			return minimumWidth;
	}
}

int HUD_GetLayoutOffsetX(void)
{
	return (ScreenDescriptorBlock.SDB_Width - HUD_GetLayoutWidth()) / 2;
}

int HUD_GetLayoutOffsetY(void)
{
	return 0;
}

int HUD_LayoutXFromScreen(int screenX)
{
	return screenX - HUD_GetLayoutOffsetX();
}

int HUD_LayoutYFromScreen(int screenY)
{
	return screenY - HUD_GetLayoutOffsetY();
}

int HUD_GetUserScaleFixed(void)
{
	HUD_SynchronisePerSpeciesOptions();
	return DIV_FIXED(HUDUserScaleTenths, 10);
}

static int CrosshairScaleSpecies(void)
{
	if (AvP.PlayerType == I_Marine) return I_Marine;
	if (AvP.PlayerType == I_Predator) return I_Predator;
	return -1;
}

static int ClampCrosshairScaleValue(int value)
{
	if (value < CROSSHAIR_SCALE_TENTHS_MINIMUM) return CROSSHAIR_SCALE_TENTHS_MINIMUM;
	if (value > CROSSHAIR_SCALE_TENTHS_MAXIMUM) return CROSSHAIR_SCALE_TENTHS_MAXIMUM;
	return value;
}

void HUD_ClampCrosshairScaleForCurrentSpecies(void)
{
	int species = CrosshairScaleSpecies();
	if (species < 0) return;

	if (ActiveCrosshairScaleSpecies != species)
	{
		if (ActiveCrosshairScaleSpecies == I_Marine)
			CrosshairScaleMarineTenths = ClampCrosshairScaleValue(CrosshairScaleTenths);
		else if (ActiveCrosshairScaleSpecies == I_Predator)
			CrosshairScalePredatorTenths = ClampCrosshairScaleValue(CrosshairScaleTenths);

		CrosshairScaleTenths = (species == I_Predator) ? CrosshairScalePredatorTenths : CrosshairScaleMarineTenths;
		ActiveCrosshairScaleSpecies = species;
	}

	CrosshairScaleTenths = ClampCrosshairScaleValue(CrosshairScaleTenths);
	if (species == I_Predator) CrosshairScalePredatorTenths = CrosshairScaleTenths;
	else CrosshairScaleMarineTenths = CrosshairScaleTenths;
}

void HUD_RestoreCrosshairScaleForCurrentSpecies(void)
{
	ActiveCrosshairScaleSpecies = -1;
	HUD_ClampCrosshairScaleForCurrentSpecies();
}

int HUD_GetCrosshairScaleFixed(void)
{
	/* Alien has no affected reticle path. */
	if (CrosshairScaleSpecies() < 0) return ONE_FIXED;
	HUD_ClampCrosshairScaleForCurrentSpecies();
	return DIV_FIXED(CrosshairScaleTenths, 10);
}
void HUD_SetScaleAnchor(enum HUD_SCALE_ANCHOR anchor)
{
	HUDScaleAnchorOverride = anchor;
}

enum HUD_SCALE_ANCHOR HUD_GetScaleAnchor(void)
{
	return HUDScaleAnchorOverride;
}
void HUD_RefreshLayout(void)
{
	/* Height-derived scale preserves the artwork's native 4:3 pixel geometry. */
	HUDScaleFactor = DIV_FIXED(HUD_GetLayoutHeight(), 480);
    /* Height is the common HUD unit; aspect controls anchors, not glyph size. */
    MotionTrackerScale = HUDScaleFactor;
}

void HUD_AnchorBegin(void)
{
	HUD_SynchronisePerSpeciesOptions();
	HUD_RefreshLayout();
	HUDScaleAnchorOverride = HUD_SCALE_ANCHOR_AUTO;
	HUDAnchorRenderingActive = 1;
    MarineHUDMotionPrepare(HUD_GetLayoutWidth(),HUD_GetLayoutHeight(),HUD_GetLayoutOffsetX(),HUD_GetLayoutOffsetY());
	FPSHUD_ResetProtectedRects();
}

void HUD_AnchorEnd(void)
{
	HUDScaleAnchorOverride = HUD_SCALE_ANCHOR_AUTO;
	HUDAnchorRenderingActive = 0;
}

int HUD_AnchorIsActive(void)
{
	return HUDAnchorRenderingActive;
}

static struct HUDFontDescTag HUDFontDesc[] =
{
	//MARINE_HUD_FONT_BLUE,
	{
		225,//XOffset
		24,//Height
		16,//Width
	},
	//MARINE_HUD_FONT_RED,
	{
		242,//XOffset
		24,//Height
		14,//Width
	},
	//MARINE_HUD_FONT_MT_SMALL,
	{
		232,//XOffset
		12,//Height
		8,//Width
	},
	//MARINE_HUD_FONT_MT_BIG,
	{
		241,//XOffset
		24,//Height
		14,//Width
	},
};
#define BLUE_BAR_WIDTH ((203-0)+1)
#define BLUE_BAR_HEIGHT ((226-195)+1)

void D3D_BLTDigitToHUD(char digit, int x, int y, int font);


void Draw_HUDImage(HUDImageDesc *imageDescPtr)
{
	struct VertexTag quadVertices[4];
	int scaledWidth;
	int scaledHeight;

	if (imageDescPtr->Scale == ONE_FIXED)
	{
		scaledWidth = imageDescPtr->Width;
		scaledHeight = imageDescPtr->Height;
	}
	else
	{
		scaledWidth = MUL_FIXED(imageDescPtr->Scale,imageDescPtr->Width);
		scaledHeight = MUL_FIXED(imageDescPtr->Scale,imageDescPtr->Height);
	}

	quadVertices[0].U = imageDescPtr->TopLeftU;
	quadVertices[0].V = imageDescPtr->TopLeftV;
	quadVertices[1].U = imageDescPtr->TopLeftU + imageDescPtr->Width;
	quadVertices[1].V = imageDescPtr->TopLeftV;
	quadVertices[2].U = imageDescPtr->TopLeftU + imageDescPtr->Width;
	quadVertices[2].V = imageDescPtr->TopLeftV + imageDescPtr->Height;
	quadVertices[3].U = imageDescPtr->TopLeftU;
	quadVertices[3].V = imageDescPtr->TopLeftV + imageDescPtr->Height;
	
	quadVertices[0].X = imageDescPtr->TopLeftX;
	quadVertices[0].Y = imageDescPtr->TopLeftY;
	quadVertices[1].X = imageDescPtr->TopLeftX + scaledWidth;
	quadVertices[1].Y = imageDescPtr->TopLeftY;
	quadVertices[2].X = imageDescPtr->TopLeftX + scaledWidth;
	quadVertices[2].Y = imageDescPtr->TopLeftY + scaledHeight;
	quadVertices[3].X = imageDescPtr->TopLeftX;
	quadVertices[3].Y = imageDescPtr->TopLeftY + scaledHeight;
		
	D3D_HUDQuad_Output
	(
		imageDescPtr->ImageNumber,
		quadVertices,
		RGBALIGHT_MAKE
		(
			imageDescPtr->Red,
			imageDescPtr->Green,
			imageDescPtr->Blue,
			imageDescPtr->Translucency
		)
	);
}


void D3D_InitialiseMarineHUD(void)
{
	//SelectGenTexDirectory(ITI_TEXTURE);

	/* set game mode: different though for multiplayer game */
	if(AvP.Network==I_No_Network)
		cl_pszGameMode = "marine";
	else
		cl_pszGameMode = "multip";

	/* load HUD gfx of correct resolution */
	{
		HUDResolution = HUD_RES_MED;
		HUDImageNumber = CL_LoadImageOnce("Huds\\Marine\\MarineHUD.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
		MotionTrackerHalfWidth = 127/2;
		MotionTrackerTextureSize = 128;

		BlueBar.ImageNumber = HUDImageNumber;
		BlueBar.TopLeftX = 0;
		BlueBar.TopLeftY = HUD_GetLayoutHeight()-40;
		BlueBar.TopLeftU = 1;
		BlueBar.TopLeftV = 223;
		BlueBar.Red = 255;
		BlueBar.Green = 255;
		BlueBar.Blue = 255;

		BlueBar.Height = BLUE_BAR_HEIGHT;
		BlueBar.Width = BLUE_BAR_WIDTH;

		/* motion tracker blips */
		MT_BlipHeight = 12;
		MT_BlipWidth = 12;

		/* load in sfx */
		SpecialFXImageNumber = CL_LoadImageOnce("Common\\partclfx.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);

//		SpecialFXImageNumber = CL_LoadImageOnceEx("flame1",IRF_D3D,DDSCAPS_SYSTEMMEMORY,0);;
//		SpecialFXImageNumber = CL_LoadImageOnceEx("star",IRF_D3D,DDSCAPS_SYSTEMMEMORY,0);;
//		SmokyImageNumber = CL_LoadImageOnceEx("smoky",IRF_D3D,DDSCAPS_SYSTEMMEMORY,0);

	}

	/* centre of motion tracker */
	MotionTrackerCentreY = BlueBar.TopLeftY;
	MotionTrackerCentreX = BlueBar.TopLeftX+(BlueBar.Width/2);
	MotionTrackerScale = 65536;

	HUD_RefreshLayout();	

	#if UseGadgets
//	MotionTrackerGadget::SetCentre(r2pos(100,100));
	#endif
}

void LoadCommonTextures(void)
{
//	PredatorVisionChangeImageNumber = CL_LoadImageOnce("HUDs\\Predator\\predvisfx.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	if(AvP.Network==I_No_Network)
	{
		switch(AvP.PlayerType)
		{
			case I_Predator:
			{
				PredatorNumbersImageNumber = CL_LoadImageOnce("HUDs\\Predator\\predNumbers.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				StaticImageNumber = CL_LoadImageOnce("Common\\static.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
				break;
			}
			case I_Alien:
			{
				AlienTongueImageNumber = CL_LoadImageOnce("HUDs\\Alien\\AlienTongue.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
				break;
			}
			case I_Marine:
			{
				StaticImageNumber = CL_LoadImageOnce("Common\\static.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
//			   	ChromeImageNumber = CL_LoadImageOnce("Common\\water2.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
				break;
			}
			default:
				break;
		}
	}
	else
	{
   		PredatorNumbersImageNumber = CL_LoadImageOnce("HUDs\\Predator\\predNumbers.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
   		StaticImageNumber = CL_LoadImageOnce("Common\\static.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		AlienTongueImageNumber = CL_LoadImageOnce("HUDs\\Alien\\AlienTongue.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	  //	ChromeImageNumber = CL_LoadImageOnce("Common\\water2.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
	}
	
	HUDFontsImageNumber = CL_LoadImageOnce("Common\\HUDfonts.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	SpecialFXImageNumber = CL_LoadImageOnce("Common\\partclfx.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE/*|LIO_TRANSPARENT*/);
	CloudyImageNumber = CL_LoadImageOnce("Common\\cloudy.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	BurningImageNumber = CL_LoadImageOnce("Common\\burn.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
//	RebellionLogoImageNumber = CL_LoadImageOnce("Common\\logo.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
//	FoxLogoImageNumber = CL_LoadImageOnce("Common\\foxlogo.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
	
	
	{
		extern char LevelName[];
		if (!strcmp(LevelName,"invasion_a"))
		{
#if !ALIEN_DEMO
		   	ChromeImageNumber = CL_LoadImageOnce("Envrnmts\\Invasion\\water2.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
			WaterShaftImageNumber = CL_LoadImageOnce("Envrnmts\\Invasion\\water-shaft.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
#else /* alien demo has these in common */
		   	ChromeImageNumber = CL_LoadImageOnce("Common\\water2.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
			WaterShaftImageNumber = CL_LoadImageOnce("Common\\water-shaft.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
#endif			
		}
		else if (!strcmp(LevelName,"genshd1"))
		{
			WaterShaftImageNumber = CL_LoadImageOnce("Envrnmts\\GenShd1\\colonywater.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		}
		else if (!strcmp(LevelName,"fall")||!strcmp(LevelName,"fall_m"))
		{
			ChromeImageNumber = CL_LoadImageOnce("Envrnmts\\fall\\stream_water.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		}
		else if (!strcmp(LevelName,"derelict"))
		{
			ChromeImageNumber = CL_LoadImageOnce("Envrnmts\\derelict\\water.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE|LIO_TRANSPARENT);
		}
		
	}

	#if 1
	{
		extern void InitDrawTest(void);
		InitDrawTest();
	}
	#endif

}
extern "C" double PlayerYawRadians(const MATRIXCH *orientation);
static int ClipRadarTop(const HUDVertexF *in,HUDVertexF *out,int count)
{
 int n=0;
 for(int i=0;i<count;i++){
  HUDVertexF a=in[i],b=in[(i+1)%count];
  int ai=a.Y<=0,bi=b.Y<=0;
  if(ai)out[n++]=a;
  if(ai!=bi){
   double u=-a.Y/(double)(b.Y-a.Y);
   out[n++]={(float)(a.X+u*(b.X-a.X)),0,(float)(a.U+u*(b.U-a.U)),(float)(a.V+u*(b.V-a.V))};
  }
 }
 return n;
}
void D3D_BLTMotionTrackerToHUD(int scanLineSize)
{

	HUDVertexF quadVertices[4],clipped[6];
	double widthCos,widthSin;

	/* Narrow displays get 10 pixels of space; wide 16:9 keeps its 15-pixel gutter. */
	{ int gap=!VideoHUDUsesWideLayout()?10:(HUD_GetEffectiveAnchorMode()==HUD_ANCHOR_16_9?15:0);
      BlueBar.TopLeftX=(int)(((int64_t)gap*ONE_FIXED+HUD_GetUserScaleFixed()-1)/HUD_GetUserScaleFixed()); }
	BlueBar.TopLeftY = HUD_GetLayoutHeight()-MUL_FIXED(MotionTrackerScale,40);
	MotionTrackerCentreY = BlueBar.TopLeftY;
	MotionTrackerCentreX = BlueBar.TopLeftX+MUL_FIXED(MotionTrackerScale,(BlueBar.Width/2));
	BlueBar.Scale = MotionTrackerScale;

	int motionTrackerScaledHalfWidth = MUL_FIXED(MotionTrackerScale*3,MotionTrackerHalfWidth/2);

 {
  DYNAMICSBLOCK *dyn=Player->ObStrategyBlock->DynPtr;
  double yaw=dyn?PlayerYawRadians(&dyn->OrientMat):PlayerYawRadians(&Player->ObMat);
  double angle=-yaw-6.283185307179586/4096.0;
  widthCos=motionTrackerScaledHalfWidth*cos(angle);
  widthSin=motionTrackerScaledHalfWidth*sin(angle);
 }
	/* I've put these -1s in here to help clipping 45 degree cases,
	where two vertices can end up around the clipping line of Y=0 */
	quadVertices[0].X = (-widthCos - (-widthSin));
	quadVertices[0].Y = (-widthSin + (-widthCos)) -1;
	quadVertices[0].U = 1;
	quadVertices[0].V = 1;
	quadVertices[1].X = (widthCos - (-widthSin));
	quadVertices[1].Y = (widthSin + (-widthCos)) -1;
	quadVertices[1].U = 1+MotionTrackerTextureSize;
	quadVertices[1].V = 1;
	quadVertices[2].X = (widthCos - widthSin);
	quadVertices[2].Y = (widthSin + widthCos) -1;
	quadVertices[2].U = 1+MotionTrackerTextureSize;
	quadVertices[2].V = 1+MotionTrackerTextureSize;
	quadVertices[3].X = ((-widthCos) - widthSin);
	quadVertices[3].Y = ((-widthSin) + widthCos) -1;
	quadVertices[3].U = 1;							   
	quadVertices[3].V = 1+MotionTrackerTextureSize;

 /* Clip a convex polygon, without mutating shared edges or rounding UVs. */
 int count=ClipRadarTop(quadVertices,clipped,4);
 for(int i=0;i<count;i++){clipped[i].X+=MotionTrackerCentreX;clipped[i].Y+=MotionTrackerCentreY;}
 D3D_HUD_Setup();
 D3D_HUDPolygonFloat_Output(HUDImageNumber,clipped,count,RGBALIGHT_MAKE(255,255,255,HUDTranslucencyLevel));

	#if 1
	{
		HUDImageDesc imageDesc;

		imageDesc.ImageNumber = HUDImageNumber;
		imageDesc.Scale = MUL_FIXED(MotionTrackerScale*3,scanLineSize/2);
		imageDesc.TopLeftX = MotionTrackerCentreX - MUL_FIXED(motionTrackerScaledHalfWidth,scanLineSize);
		imageDesc.TopLeftY = MotionTrackerCentreY - MUL_FIXED(motionTrackerScaledHalfWidth,scanLineSize);
		imageDesc.TopLeftU = 1;
		imageDesc.TopLeftV = 132;
		imageDesc.Height = 64;
		imageDesc.Width = 128;
		imageDesc.Red = 255;
		imageDesc.Green = 255;
		imageDesc.Blue = 255;
		imageDesc.Translucency = HUDTranslucencyLevel;

 		Draw_HUDImage(&imageDesc);
	}
	#endif

	/* KJL 16:14:29 30/01/98 - draw bottom bar of MT */
	{
		BlueBar.Translucency = HUDTranslucencyLevel;
		Draw_HUDImage(&BlueBar);
	}
	
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_UNITS],17, -4, MARINE_HUD_FONT_MT_SMALL);	  
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_TENS],9, -4, MARINE_HUD_FONT_MT_SMALL);	  
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_HUNDREDS],-9, -4, MARINE_HUD_FONT_MT_BIG);
    D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_THOUSANDS],-25, -4,MARINE_HUD_FONT_MT_BIG);	

	FPSHUD_AddLogicalRect(&FPSMarineRadarRect, BlueBar.TopLeftX, HUD_GetLayoutHeight() - MUL_FIXED(MotionTrackerScale, 256), BlueBar.TopLeftX + MUL_FIXED(MotionTrackerScale, BLUE_BAR_WIDTH), HUD_GetLayoutHeight(), HUD_SCALE_ANCHOR_BOTTOM_LEFT);
}


void D3D_BLTMotionTrackerBlipToHUD(int x, int y, int brightness)
{
	HUDImageDesc imageDesc;
	int frame;
	int motionTrackerScaledHalfWidth = MUL_FIXED(MotionTrackerScale*3,MotionTrackerHalfWidth/2);
    
	GLOBALASSERT(brightness<=65536);
	
	frame = (brightness*5)/65537;
	GLOBALASSERT(frame>=0 && frame<5);
	
    frame = 4 - frame; // frames bloody wrong way round
	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.Scale = MUL_FIXED(MotionTrackerScale*3,(brightness+ONE_FIXED)/4);
	imageDesc.TopLeftX = MotionTrackerCentreX - MUL_FIXED(MT_BlipWidth/2,imageDesc.Scale) + MUL_FIXED(x,motionTrackerScaledHalfWidth);
	imageDesc.TopLeftY = MotionTrackerCentreY - MUL_FIXED(MT_BlipHeight/2,imageDesc.Scale) - MUL_FIXED(y,motionTrackerScaledHalfWidth);
	imageDesc.TopLeftU = 227;
	imageDesc.TopLeftV = 187;
	imageDesc.Height = MT_BlipHeight;
	imageDesc.Width = MT_BlipWidth;
	{
		int trans = MUL_FIXED(brightness*2,HUDTranslucencyLevel);
		if (trans>255) trans = 255;
		imageDesc.Translucency = trans;
	}
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;
	if (imageDesc.TopLeftX<0) /* then we need to clip */
	{
		imageDesc.Width += imageDesc.TopLeftX;
		imageDesc.TopLeftU -= imageDesc.TopLeftX;
		imageDesc.TopLeftX = 0;
	}
	Draw_HUDImage(&imageDesc);
}
extern void D3D_BlitWhiteChar(int x, int y, unsigned char c)
{
	HUDImageDesc imageDesc;
	
//	if (c>='a' && c<='z') c-='a'-'A';

//	if (c<' ' || c>'_') return;
	if (c==' ') return;

	#if 0
	imageDesc.ImageNumber = HUDFontsImageNumber;

	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = 1+((c-32)&15)*7;
	imageDesc.TopLeftV = 2+((c-32)>>4)*11;
	imageDesc.Height = 8;
	imageDesc.Width = 5;
	#else
	imageDesc.ImageNumber = AAFontImageNumber;

	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = 1+((c-32)&15)*16;
	imageDesc.TopLeftV = 1+((c-32)>>4)*16;
	imageDesc.Height = 15;
	imageDesc.Width = 15;
	#endif
	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = 255;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;

	Draw_HUDImage(&imageDesc);
} 

void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr)
{
	HUDImageDesc imageDesc;

  //	if (charDescPtr->Character<' ' || charDescPtr->Character>'_') return;
	if (charDescPtr->Character == ' ') return;

	imageDesc.ImageNumber = AAFontImageNumber;

	imageDesc.TopLeftX = charDescPtr->X-1;
	imageDesc.TopLeftY = charDescPtr->Y-1;
	imageDesc.TopLeftU = 0+((charDescPtr->Character-32)&15)*16;
	imageDesc.TopLeftV = 0+((charDescPtr->Character-32)>>4)*16;
	imageDesc.Height = HUD_FONT_HEIGHT+2;
	imageDesc.Width = HUD_FONT_WIDTH+2;

	imageDesc.Scale = D3D_GetActiveConsoleTextScaleFixed();
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;

	Draw_HUDImage(&imageDesc);
	
}
void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr)
{
	HUDImageDesc imageDesc;

	imageDesc.ImageNumber = HUDFontsImageNumber;

	imageDesc.TopLeftX = charDescPtr->X;
	imageDesc.TopLeftY = charDescPtr->Y;

	if (charDescPtr->Character<8)
	{
		imageDesc.TopLeftU = 1+(charDescPtr->Character)*16;
		imageDesc.TopLeftV = 81;
	}
	else
	{
		imageDesc.TopLeftU = 1+(charDescPtr->Character-8)*16;
		imageDesc.TopLeftV = 81+24;
	}


	imageDesc.Height = HUD_DIGITAL_NUMBERS_HEIGHT;
	imageDesc.Width = HUD_DIGITAL_NUMBERS_WIDTH;
	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;

	Draw_HUDImage(&imageDesc);
	
}
void D3D_DrawHUDPredatorDigit(HUDCharDesc *charDescPtr, int scale)
{
	HUDImageDesc imageDesc;
	enum HUD_SCALE_ANCHOR anchor = HUD_GetScaleAnchor();
	FPSHUDRect *protectedRect;

	imageDesc.ImageNumber = PredatorNumbersImageNumber;

	imageDesc.TopLeftX = charDescPtr->X;
	imageDesc.TopLeftY = charDescPtr->Y;

	if (charDescPtr->Character<5)
	{
		imageDesc.TopLeftU = (charDescPtr->Character)*51;
		imageDesc.TopLeftV = 1;
	}
	else
	{
		imageDesc.TopLeftU = (charDescPtr->Character-5)*51;
		imageDesc.TopLeftV = 52;
	}


	imageDesc.Height = 50;
	imageDesc.Width = 50;
	imageDesc.Scale = scale;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;

	Draw_HUDImage(&imageDesc);
	/* Use the same group and origin as drawing. A tall column can cross the
	 * canvas midpoint; those health cells must not become speargun bounds. */
	switch (anchor)
	{
		case HUD_SCALE_ANCHOR_TOP_LEFT: protectedRect = &FPSPredatorHealthRect; break;
		case HUD_SCALE_ANCHOR_TOP_RIGHT: protectedRect = &FPSPredatorEnergyRect; break;
		case HUD_SCALE_ANCHOR_BOTTOM_LEFT: protectedRect = &FPSPredatorSpeargunRect; break;
		default:
			/* Preserve the legacy classification for callers without a group. */
			protectedRect = 0;
			if (charDescPtr->Y < HUD_GetLayoutHeight() / 2)
				protectedRect = charDescPtr->X < HUD_GetLayoutWidth() / 2 ? &FPSPredatorHealthRect : &FPSPredatorEnergyRect;
			else if (charDescPtr->X < HUD_GetLayoutWidth() / 2)
				protectedRect = &FPSPredatorSpeargunRect;
			break;
	}
	if (protectedRect)
		FPSHUD_AddLogicalRect(protectedRect, charDescPtr->X, charDescPtr->Y,
			charDescPtr->X + MUL_FIXED(50, scale), charDescPtr->Y + MUL_FIXED(50, scale), anchor);

}

void D3D_BLTDigitToHUD(char digit, int x, int y, int font)
{
	HUDImageDesc imageDesc;
	struct HUDFontDescTag *FontDescPtr;
	int gfxID;

	switch (font)
	{
		case MARINE_HUD_FONT_MT_SMALL:
	  	case MARINE_HUD_FONT_MT_BIG:
		{
		   	gfxID = MARINE_HUD_GFX_TRACKERFONT;
			imageDesc.Scale = MotionTrackerScale;
			x = MUL_FIXED(x,MotionTrackerScale) + MotionTrackerCentreX;
			y = MUL_FIXED(y,MotionTrackerScale) + MotionTrackerCentreY;
			break;
		}
		case MARINE_HUD_FONT_RED:
		case MARINE_HUD_FONT_BLUE:
		{
			if (x<0) x+=HUD_GetLayoutWidth();
		   	gfxID = MARINE_HUD_GFX_NUMERALS;
			imageDesc.Scale=ONE_FIXED;
			break;
		}
		case ALIEN_HUD_FONT:
		{
			gfxID = ALIEN_HUD_GFX_NUMBERS;
			imageDesc.Scale=ONE_FIXED;
			break;
		}
		default:
			LOCALASSERT(0);
			break;
	}

	
	if (HUDResolution == HUD_RES_LO)
	{
		FontDescPtr = &HUDFontDesc[font];
	}
	else if (HUDResolution == HUD_RES_MED)
	{
		FontDescPtr = &HUDFontDesc[font];
	}
	else
	{
		FontDescPtr = &HUDFontDesc[font];
	}



	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = FontDescPtr->XOffset;
	imageDesc.TopLeftV = digit*(FontDescPtr->Height+1)+1;
	
	imageDesc.Height = FontDescPtr->Height;
	imageDesc.Width = FontDescPtr->Width;
	imageDesc.Translucency = HUDTranslucencyLevel;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;

	Draw_HUDImage(&imageDesc);

}

static void DrawMarineGunsightWithThickness(HUDImageDesc *imageDesc, int thicknessMode)
{
	/* The MarineHUD.RIM crosshair cells stay at their original coordinates and
	 * scale. Extra mask samples are displaced in physical pixels around the
	 * same centre, so they increase stroke coverage without a blurry resize. */
	static const int cardinalOffsets[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
	static const int diagonalOffsets[4][2] = {{-1,-1},{1,-1},{-1,1},{1,1}};
	int baseX = imageDesc->TopLeftX;
	int baseY = imageDesc->TopLeftY;
	int i;

	if (thicknessMode < MARINE_CROSSHAIR_THICKNESS_X10 || thicknessMode > MARINE_CROSSHAIR_THICKNESS_X20)
		thicknessMode = MARINE_CROSSHAIR_THICKNESS_X10;

	if (thicknessMode >= MARINE_CROSSHAIR_THICKNESS_X15)
	{
		for (i = 0; i < 4; ++i)
		{
			imageDesc->TopLeftX = baseX + cardinalOffsets[i][0];
			imageDesc->TopLeftY = baseY + cardinalOffsets[i][1];
			Draw_HUDImage(imageDesc);
		}
	}
	if (thicknessMode >= MARINE_CROSSHAIR_THICKNESS_X20)
	{
		for (i = 0; i < 4; ++i)
		{
			imageDesc->TopLeftX = baseX + diagonalOffsets[i][0];
			imageDesc->TopLeftY = baseY + diagonalOffsets[i][1];
			Draw_HUDImage(imageDesc);
		}
	}

	imageDesc->TopLeftX = baseX;
	imageDesc->TopLeftY = baseY;
	Draw_HUDImage(imageDesc);
}

void D3D_BLTGunSightToHUD(int screenX, int screenY, enum GUNSIGHT_SHAPE gunsightShape)
{
	HUDImageDesc imageDesc;
	int gunsightSize = 13;
	int crosshairScale = HUD_GetCrosshairScaleFixed();
	int scaledGunsightSize = MUL_FIXED(crosshairScale,gunsightSize);

	/* Gameplay sights use physical projection coordinates. Only translate them
	 * when a caller deliberately draws inside the anchored HUD rectangle. */
	if (HUD_AnchorIsActive())
	{
		screenX = HUD_LayoutXFromScreen(screenX);
		screenY = HUD_LayoutYFromScreen(screenY);
	}

	screenX -= scaledGunsightSize/2;
	screenY -= scaledGunsightSize/2;

	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.TopLeftX = screenX;
	imageDesc.TopLeftY = screenY;
	imageDesc.TopLeftU = 227;
	imageDesc.TopLeftV = 131+(gunsightShape*(gunsightSize+1));
	imageDesc.Height = gunsightSize;
	imageDesc.Width = gunsightSize;
	imageDesc.Scale = crosshairScale;
	imageDesc.Translucency = 128;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;

	DrawMarineGunsightWithThickness(&imageDesc, MarineCrosshairThicknessMode);
}
static int MarineHUDVerticalInset(void)
{
    int scale=HUD_GetUserScaleFixed();
    if (AvP.PlayerType!=I_Marine || scale<=0) return 0;
    return (int)(((int64_t)5*ONE_FIXED+scale/2)/scale);
}
void Render_HealthAndArmour(unsigned int health, unsigned int armour)
{
	unsigned int healthColour;
	unsigned int armourColour;

	if (AvP.PlayerType == I_Marine)
	{										  
		enum HUD_SCALE_ANCHOR savedAnchor = HUD_GetScaleAnchor();
		int xCentre = MUL_FIXED(HUDLayout_RightmostTextCentre,HUDScaleFactor)+HUD_GetLayoutWidth();
		HUD_SetScaleAnchor(HUD_SCALE_ANCHOR_TOP_RIGHT);
		healthColour = HUDLayout_Colour_MarineGreen;
		armourColour = HUDLayout_Colour_MarineGreen;
		D3D_RenderHUDString_Centred
		(
			GetTextString(TEXTSTRING_INGAME_HEALTH),
			xCentre,
			MUL_FIXED(HUDLayout_Health_TopY,HUDScaleFactor)+MarineHUDVerticalInset(),
			HUDLayout_Colour_BrightWhite
		);
		D3D_RenderHUDNumber_Centred
		(
			health,
			xCentre,
			MUL_FIXED(HUDLayout_Health_TopY+HUDLayout_Linespacing,HUDScaleFactor)+MarineHUDVerticalInset(),
			healthColour
		);	
		D3D_RenderHUDString_Centred
		(
			GetTextString(TEXTSTRING_INGAME_ARMOUR),
			xCentre,
			MUL_FIXED(HUDLayout_Armour_TopY,HUDScaleFactor)+MarineHUDVerticalInset(),
			HUDLayout_Colour_BrightWhite
		);
		D3D_RenderHUDNumber_Centred
		(
			armour,
			xCentre,
			MUL_FIXED(HUDLayout_Armour_TopY+HUDLayout_Linespacing,HUDScaleFactor)+MarineHUDVerticalInset(),
			armourColour
		);	
		HUD_SetScaleAnchor(savedAnchor);
	}
	else
	{										  
		if (health>100)
		{
			healthColour = HUDLayout_Colour_BrightWhite;
		}
		else
		{
			int r = ((health)*128)/100;
			healthColour = 0xff000000 + ((128-r)<<16) + (r<<8);
		}
		if (armour>100)
		{
			armourColour = HUDLayout_Colour_BrightWhite;
		}
		else
		{
			int r = ((armour)*128)/100;
			armourColour = 0xff000000 + ((128-r)<<16) + (r<<8);
		}

		{
		
   			struct VertexTag quadVertices[4];
			int scaledWidth;
			int scaledHeight;
			int x,y;

			if (health<100)
			{
				scaledWidth = WideMulNarrowDiv(HUD_GetLayoutWidth(),health,100);
				scaledHeight = scaledWidth/32;
			}
			else
			{
				scaledWidth = HUD_GetLayoutWidth();
				scaledHeight = scaledWidth/32;
			}
			x = (HUD_GetLayoutWidth() - scaledWidth)/2;
			y = HUD_GetLayoutHeight() - HUD_GetLayoutWidth()/32 + x/32;

			quadVertices[0].U = 8;
			quadVertices[0].V = 5;
			quadVertices[1].U = 57;//255;
			quadVertices[1].V = 5;
			quadVertices[2].U = 57;//255;
			quadVertices[2].V = 55;//255;
			quadVertices[3].U = 8;
			quadVertices[3].V = 55;//255;
			
			quadVertices[0].X = x;
			quadVertices[0].Y = y;
			quadVertices[1].X = x + scaledWidth;
			quadVertices[1].Y = y;
			quadVertices[2].X = x + scaledWidth;
			quadVertices[2].Y = y + scaledHeight;
			quadVertices[3].X = x;
			quadVertices[3].Y = y + scaledHeight;
            if (AvP.PlayerType == I_Alien && scaledWidth > 0 && scaledHeight > 0)
                FPSHUD_AddLogicalRect(&FPSAlienHealthRect, x, y, x+scaledWidth, y+scaledHeight, HUD_SCALE_ANCHOR_AUTO);
				
			D3D_HUDQuad_Output
			(
				SpecialFXImageNumber,// AlienEnergyBarImageNumber,
				quadVertices,
				0xff003fff
			);
		
			health = (health/2);
			if (health<0) health=0;

			if (health<100)
			{
				scaledWidth = WideMulNarrowDiv(HUD_GetLayoutWidth(),health,100);
				scaledHeight = scaledWidth/32;
			}
			else
			{
				scaledWidth = HUD_GetLayoutWidth();
				scaledHeight = scaledWidth/32;
			}
	
			x = (HUD_GetLayoutWidth() - scaledWidth)/2;
			y = HUD_GetLayoutHeight() - HUD_GetLayoutWidth()/32 + x/32;
	
			quadVertices[0].X = x;
			quadVertices[0].Y = y;
			quadVertices[1].X = x + scaledWidth;
			quadVertices[1].Y = y;
			quadVertices[2].X = x + scaledWidth;
			quadVertices[2].Y = y + scaledHeight;
			quadVertices[3].X = x;
			quadVertices[3].Y = y + scaledHeight;
            if (AvP.PlayerType == I_Alien && scaledWidth > 0 && scaledHeight > 0)
                FPSHUD_AddLogicalRect(&FPSAlienHealthRect, x, y, x+scaledWidth, y+scaledHeight, HUD_SCALE_ANCHOR_AUTO);
	
			D3D_HUDQuad_Output
			(
				SpecialFXImageNumber,// AlienEnergyBarImageNumber,
				quadVertices,
				0xffffffff
			);
			
		}

	}
	

		

	if (AvP.PlayerType == I_Marine)
	{
		int healthCentre = MUL_FIXED(HUDLayout_RightmostTextCentre, HUDScaleFactor) + HUD_GetLayoutWidth();
		int healthTextHalfWidth = MUL_FIXED((AvPTextStringWidth(GetTextString(TEXTSTRING_INGAME_HEALTH), AAFontWidths) + 1) / 2, HUDScaleFactor);
		int healthDigitsHalfWidth = MUL_FIXED(3 * HUD_DIGITAL_NUMBERS_WIDTH, HUDScaleFactor) / 2 + 4;
		int healthHalfWidth = (healthTextHalfWidth > healthDigitsHalfWidth) ? healthTextHalfWidth : healthDigitsHalfWidth;
		int healthGuard = MUL_FIXED(6, HUDScaleFactor);

		/* Match the actual health row rather than the former 160px-wide
		 * conservative group box, so a relocated FPS counter sits beside it. */
		FPSHUD_AddLogicalRect(&FPSMarineHealthRect, healthCentre - healthHalfWidth - healthGuard, MarineHUDVerticalInset(), healthCentre + healthHalfWidth + healthGuard, MUL_FIXED(HUDLayout_Health_TopY + HUDLayout_Linespacing + HUD_DIGITAL_NUMBERS_HEIGHT, HUDScaleFactor)+MarineHUDVerticalInset(), HUD_SCALE_ANCHOR_TOP_RIGHT);
	}
} 
void Render_MarineAmmo(enum TEXTSTRING_ID ammoText, enum TEXTSTRING_ID magazinesText, unsigned int magazines, enum TEXTSTRING_ID roundsText, unsigned int rounds, int primaryAmmo)
{
 enum HUD_SCALE_ANCHOR savedAnchor=HUD_GetScaleAnchor();
 int textWidth=AvPTextStringWidth(GetTextString(ammoText),AAFontWidths);
 int otherWidth=AvPTextStringWidth(GetTextString(magazinesText),AAFontWidths);
 int xCentre;
 if(textWidth<otherWidth)textWidth=otherWidth;
 otherWidth=AvPTextStringWidth(GetTextString(roundsText),AAFontWidths);
 if(textWidth<otherWidth)textWidth=otherWidth;
 if(textWidth<3*HUD_DIGITAL_NUMBERS_WIDTH)textWidth=3*HUD_DIGITAL_NUMBERS_WIDTH;
 xCentre=MarineAmmoColumnCentre(HUD_GetLayoutWidth(),HUDScaleFactor,textWidth,primaryAmmo);
 HUD_SetScaleAnchor(HUD_SCALE_ANCHOR_BOTTOM_RIGHT);

	D3D_RenderHUDString_Centred
	(
		GetTextString(ammoText),
		xCentre,
		(HUD_GetLayoutHeight()-2*MarineHUDVerticalInset()) - MUL_FIXED(HUDScaleFactor,HUDLayout_AmmoDesc_TopY),
		HUDLayout_Colour_BrightWhite
	);
	D3D_RenderHUDString_Centred
	(
		GetTextString(magazinesText),
		xCentre,
		(HUD_GetLayoutHeight()-2*MarineHUDVerticalInset()) -MUL_FIXED(HUDScaleFactor, HUDLayout_Magazines_TopY),
		HUDLayout_Colour_BrightWhite
	);
	D3D_RenderHUDNumber_Centred
	(
		magazines,
		xCentre,
		(HUD_GetLayoutHeight()-2*MarineHUDVerticalInset()) - MUL_FIXED(HUDScaleFactor,HUDLayout_Magazines_TopY - HUDLayout_Linespacing),
		HUDLayout_Colour_MarineRed
	);	
	D3D_RenderHUDString_Centred
	(
		GetTextString(roundsText),
		xCentre,
		(HUD_GetLayoutHeight()-2*MarineHUDVerticalInset()) - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY),
		HUDLayout_Colour_BrightWhite
	);
	D3D_RenderHUDNumber_Centred
	(
		rounds,
		xCentre,
		(HUD_GetLayoutHeight()-2*MarineHUDVerticalInset()) - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY - HUDLayout_Linespacing),
		HUDLayout_Colour_MarineRed
	);	

		

	FPSHUD_AddLogicalRect(&FPSMarineAmmoRect, xCentre - MUL_FIXED(180, HUDScaleFactor), (HUD_GetLayoutHeight()-2*MarineHUDVerticalInset()) - MUL_FIXED(HUDLayout_AmmoDesc_TopY + HUD_FONT_HEIGHT + 8, HUDScaleFactor), xCentre + MUL_FIXED(180, HUDScaleFactor), (HUD_GetLayoutHeight()-2*MarineHUDVerticalInset()), HUD_SCALE_ANCHOR_BOTTOM_RIGHT);
 HUD_SetScaleAnchor(savedAnchor);
} 
void DrawPredatorEnergyBar(void)
{
	PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	int maxHeight = HUD_GetLayoutHeight()*3/4;
	int h;
	{
		h = MUL_FIXED(DIV_FIXED(playerStatusPtr->FieldCharge,AcidResourceCapacity(PLAYERCLOAK_MAXENERGY)),maxHeight);
		
		r2rect rectangle
		(
			HUD_GetLayoutWidth()+HUDLayout_RightmostTextCentre*3/2,
			HUD_GetLayoutHeight()-h,
			HUD_GetLayoutWidth()+HUDLayout_RightmostTextCentre/2,
			HUD_GetLayoutHeight()
			
		);
		rectangle . AlphaFill
		(
			0xff, // unsigned char R,
			0x00,// unsigned char G,
			0x00,// unsigned char B,
		   	128 // unsigned char translucency
		);
	}
	if (weaponPtr->WeaponIDNumber == WEAPON_PRED_SHOULDERCANNON)
	{
		h = MUL_FIXED(playerStatusPtr->PlasmaCasterCharge,maxHeight);
			
		r2rect rectangle
		(
			HUD_GetLayoutWidth()+HUDLayout_RightmostTextCentre*3,
			HUD_GetLayoutHeight()-h,
			HUD_GetLayoutWidth()+HUDLayout_RightmostTextCentre*2,
			HUD_GetLayoutHeight()
			
		);
		rectangle . AlphaFill
		(
			0x00, // unsigned char R,
			0xff,// unsigned char G,
			0xff,// unsigned char B,
		   	128 // unsigned char translucency
		);
	}

}

};
