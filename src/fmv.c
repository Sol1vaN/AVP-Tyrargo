#include "menu_preview.h"
/* Native FMV path: Bink cutscenes and Smacker in-world panels. */
#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "stratdef.h"
#include "gamedef.h"
#include "fmv.h"
#include "fmv_backend.h"
#include "avp_menus.h"
#include "avp_userprofile.h"
#include "oglfunc.h"
#include <string.h>

#define UseLocalAssert 1
#include "ourasert.h"

int VolumeOfNearestVideoScreen;
int PanningOfNearestVideoScreen;
extern int GotAnyKey;
extern IMAGEHEADER ImageHeaderArray[];
#if MaxImageGroups>1
extern int NumImagesArray[];
#else
extern int NumImages;
#endif

int SmackerSoundVolume=ONE_FIXED/512;
int MoviesAreActive;
int IntroOutroMoviesAreActive=1;
int FMVFilterMode=1;
int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;

void ReleaseFMVTexture(FMVTEXTURE *ftPtr);
void SetupFMVTexture(FMVTEXTURE *ftPtr);
void UpdateFMVTexture(FMVTEXTURE *ftPtr);
#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
int NumberOfFMVTextures;
/* Panel textures must remain independent of world/HUD texture quality. */
int FMV_IsPanelTexture(const void *texture)
{
 if(!texture)return 0;
 for(int i=0;i<NumberOfFMVTextures;i++)
  if(FMVTexture[i].ImagePtr && FMVTexture[i].ImagePtr->D3DTexture==texture)return 1;
 return 0;
}
/* Default: Probe 092's animated RGB-noise composition. */
int MonitorNoSignalEffectMode=MONITOR_NO_SIGNAL_RGB_NOISE;

void ScanImagesForFMVs(void)
{
	if (MoviesAreActive) FMVBackend_PrewarmPanels();	int i;
	IMAGEHEADER *ihPtr;
	NumberOfFMVTextures=0;
#if MaxImageGroups>1
	for (int j=0; j<MaxImageGroups; j++)
	{
		if (!NumImagesArray[j]) continue;
		ihPtr = &ImageHeaderArray[j*MaxImages];
		for (i=0; i<NumImagesArray[j]; i++, ihPtr++)
#else
	if (NumImages)
	{
		ihPtr = &ImageHeaderArray[0];
		for (i=0; i<NumImages; i++, ihPtr++)
#endif
		{
			if (strstr(ihPtr->ImageName,"FMVs"))
			{
				FMVTEXTURE *texture = &FMVTexture[NumberOfFMVTextures];
				memset(texture,0,sizeof(*texture));
				texture->IsTriggeredPlotFMV=1;
				texture->ImagePtr=ihPtr;
				SetupFMVTexture(texture);
				if (++NumberOfFMVTextures == MAX_NO_FMVTEXTURES) return;
			}
		}
#if MaxImageGroups>1
	}
#else
	}
#endif
}

void UpdateAllFMVTextures(void)
{
	for (int i=0; i<NumberOfFMVTextures; ++i) UpdateFMVTexture(&FMVTexture[i]);
}

void ReleaseAllFMVTextures(void)
{
	for (int i=0; i<NumberOfFMVTextures; ++i) ReleaseFMVTexture(&FMVTexture[i]);
	NumberOfFMVTextures=0;
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr)
{
	unsigned char palette[256*3];
	int frameStatus=0;

	/* A completed movie must not leave its terminal logo frozen on the panel.
	   Closing it here restores native static/no-signal and prevents a completed
	   panel from being stored as an active FMV in a save-game. */
	if (ftPtr->PanelFinishPending)
	{
		FMVBackend_ClosePanel(ftPtr);
		ftPtr->MessageNumber=0;
		ftPtr->StaticImageDrawn=0;
		ftPtr->PanelFinishPending=0;
	}

	if (MoviesAreActive && ftPtr->DecoderHandle)
	{
		frameStatus=FMVBackend_CopyPanelFrame(ftPtr,(unsigned char*)bufferPtr,palette);
	}
	if (frameStatus)
	{
		for (int i=0; i<256; ++i)
		{
			ftPtr->SrcPalette[i].peRed=palette[i*3+0];
			ftPtr->SrcPalette[i].peGreen=palette[i*3+1];
			ftPtr->SrcPalette[i].peBlue=palette[i*3+2];
		}
		/* Present the actual last video image once, then switch to static on the
		   next texture update. */
		if (frameStatus==2) ftPtr->PanelFinishPending=1;
		return 1;
	}
	if (!ftPtr->StaticImageDrawn)
	{
		/* Preserve the animated no-signal source used by Probe 090. The CRT
		   RGB mask is applied later, after this animated signal is composed. */
		int i=128*96/4;
		unsigned int seed=FastRandom();
		int *ptr=(int*)bufferPtr;
		do { seed=(seed*1664525)+1013904223; *ptr++=(int)seed; } while(--i);
		ftPtr->StaticImageDrawn=1;
	}
	return 1;
}

void UpdateFMVTexturePalette(FMVTEXTURE *ftPtr)
{
	if (MoviesAreActive && ftPtr->DecoderHandle) return;
	unsigned int seed=FastRandom();
	for (int i=0; i<256; ++i)
	{
		int l=(seed&(seed>>24)&(seed>>16));
		seed=(seed*1664525)+1013904223;
		ftPtr->SrcPalette[i].peRed=l;
		ftPtr->SrcPalette[i].peGreen=l;
		ftPtr->SrcPalette[i].peBlue=l;
	}
}

void StartTriggerPlotFMV(int number)
{
	if (!MoviesAreActive || CheatMode_Active!=CHEATMODE_NONACTIVE) return;
	/* Close the previous shared message completely before a retrigger. */
	for (int i=0; i<NumberOfFMVTextures; ++i)
		if (FMVTexture[i].IsTriggeredPlotFMV) FMVBackend_ClosePanel(&FMVTexture[i]);
	for (int i=0; i<NumberOfFMVTextures; ++i)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV) FMVBackend_OpenPanel(&FMVTexture[i],number);
	}
}

void StartFMVAtFrame(int number, int frame)
{
	/* The legacy save record has no completed/active flag nor a usable frame
	   position. Replaying it always restarts a finished panel after load, so
	   panel playback is now driven exclusively by the map trigger state. */
	(void)number;
	(void)frame;
}

void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr)
{
	for (int i=0; i<NumberOfFMVTextures; ++i)
	{
		if (FMVTexture[i].DecoderHandle)
		{
			*messageNumberPtr=FMVTexture[i].MessageNumber;
			*frameNumberPtr=0;
			return;
		}
	}
	*messageNumberPtr=0;
	*frameNumberPtr=0;
}

void InitialiseTriggeredFMVs(void)
{
	for (int i=0; i<NumberOfFMVTextures; ++i)
	{
		FMVBackend_ClosePanel(&FMVTexture[i]);
		FMVTexture[i].MessageNumber=0;
		FMVTexture[i].StaticImageDrawn=0;
		FMVTexture[i].PanelFinishPending=0;
	}
}

void FindLightingValuesFromTriggeredFMV(unsigned char *bufferPtr, FMVTEXTURE *ftPtr)
{
	unsigned int totalRed=0,totalBlue=0,totalGreen=0;
	int pixels=128*96;
	do
	{
		unsigned char source=*bufferPtr++;
		totalBlue+=ftPtr->SrcPalette[source].peBlue;
		totalGreen+=ftPtr->SrcPalette[source].peGreen;
		totalRed+=ftPtr->SrcPalette[source].peRed;
	} while(--pixels);
	FmvColourRed=totalRed/48*16;
	FmvColourGreen=totalGreen/48*16;
	FmvColourBlue=totalBlue/48*16;
}

void SetupFMVTexture(FMVTEXTURE *ftPtr)
{
 if(ftPtr->ImagePtr && ftPtr->ImagePtr->D3DTexture){
  ftPtr->ImagePtr->D3DTexture->qualityGeneration=0;
  ftPtr->ImagePtr->D3DTexture->filter=-1;
 }
	if (!ftPtr->PalettedBuf) ftPtr->PalettedBuf=(unsigned char*)calloc(1,128*128+128*128*3);
	if (ftPtr->PalettedBuf) ftPtr->RGBBuf=&ftPtr->PalettedBuf[128*128];
}

/*
 * Probe 093: explicit, safe reconstruction of the pre-061 CRT panel path.
 * The old path populated RGBA samples, then uploaded that memory as RGB. The
 * resulting 4:3 stream phase produced the characteristic vertical RGB mask.
 * Recreate only the visual phase here, using the animated no-signal source;
 * do not reuse a mismatched buffer stride or apply it to decoded movie frames.
 */
static unsigned char FMVLegacyCRTStreamByte(FMVTEXTURE *ftPtr, unsigned int streamOffset)
{
	unsigned int sourcePixel=streamOffset>>2;
	if ((streamOffset&3)==3) return 255;
	return ftPtr->SrcPalette[ftPtr->PalettedBuf[sourcePixel]].peRed;
}

void UpdateFMVTexture(FMVTEXTURE *ftPtr)
{

	unsigned char *srcPtr;
	unsigned char *dstPtr;
	int staticSignal;
	if (!ftPtr->PalettedBuf || !ftPtr->ImagePtr || !ftPtr->ImagePtr->D3DTexture) return;
    if(RenderPausedPreview){
        if(MoviesAreActive && ftPtr->DecoderHandle)return;
    } else {
        if (!NextFMVTextureFrame(ftPtr,ftPtr->PalettedBuf)) return;
        UpdateFMVTexturePalette(ftPtr);
        FindLightingValuesFromTriggeredFMV(ftPtr->PalettedBuf,ftPtr);
    }
	srcPtr=ftPtr->PalettedBuf;
	dstPtr=ftPtr->RGBBuf;
	staticSignal=!(MoviesAreActive && ftPtr->DecoderHandle);
	for (int y=0; y<96; ++y)
	{
		for (int x=0; x<128; ++x)
		{
			if (staticSignal)
			{
				if (MonitorNoSignalEffectMode==MONITOR_NO_SIGNAL_RGB_NOISE)
				{
					/* Probe 092: animated random signal colorized as RGB noise. */
					unsigned char source=*srcPtr++;
					int luminance=ftPtr->SrcPalette[source].peRed;
					int dim=luminance/4;
					switch (x%3)
					{
						case 0: *dstPtr++=(unsigned char)luminance; *dstPtr++=(unsigned char)dim; *dstPtr++=(unsigned char)dim; break;
						case 1: *dstPtr++=(unsigned char)dim; *dstPtr++=(unsigned char)luminance; *dstPtr++=(unsigned char)dim; break;
						default: *dstPtr++=(unsigned char)dim; *dstPtr++=(unsigned char)dim; *dstPtr++=(unsigned char)luminance; break;
					}
				}
				else
				{
					/* Probe 093: explicit 4:3 legacy CRT stream phase. */
					unsigned int packedOffset=((unsigned int)y*128u+(unsigned int)x)*3u;
					*dstPtr++=FMVLegacyCRTStreamByte(ftPtr,packedOffset+0u);
					*dstPtr++=FMVLegacyCRTStreamByte(ftPtr,packedOffset+1u);
					*dstPtr++=FMVLegacyCRTStreamByte(ftPtr,packedOffset+2u);
				}
			}
			else
			{
				unsigned char source=*srcPtr++;
				*dstPtr++=ftPtr->SrcPalette[source].peRed;
				*dstPtr++=ftPtr->SrcPalette[source].peGreen;
				*dstPtr++=ftPtr->SrcPalette[source].peBlue;
			}
		}
	}
    /* The 128x96 picture lives in a 128x128 texture. Repeat the last picture
       row into padding and clamp the outer edge: bilinear cannot wrap stale
       green/no-signal texels into the first scanline. */
    for(int y=96;y<128;++y)memcpy(ftPtr->RGBBuf+y*128*3,ftPtr->RGBBuf+95*128*3,128*3);
	pglBindTexture(GL_TEXTURE_2D,ftPtr->ImagePtr->D3DTexture->id);
    /* Maps repeat U across adjacent monitors; clamping S smears the last column.
       Keep vertical clamp + padded rows, which prevent the green top seam. */
    pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	pglTexSubImage2D(GL_TEXTURE_2D,0,0,0,128,128,GL_RGB,GL_UNSIGNED_BYTE,ftPtr->RGBBuf);
	if(!RenderPausedPreview)FMVBackend_CommitPanelAudio(ftPtr);
}
void ReleaseFMVTexture(FMVTEXTURE *ftPtr)
{
	FMVBackend_ClosePanel(ftPtr);
	ftPtr->MessageNumber=0;
	if (ftPtr->PalettedBuf) free(ftPtr->PalettedBuf);
	ftPtr->PalettedBuf=0;
	ftPtr->RGBBuf=0;
	ftPtr->StaticImageDrawn=0;
	ftPtr->PanelFinishPending=0;
}