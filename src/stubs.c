#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "psndplat.h"
#include "module.h"
#include "stratdef.h"
#include "avp_userprofile.h"
#include "projfont.h"
#include "savegame.h"
#include "pldnet.h"
#include "kshape.h"
#include "d3d_hud.h"
#include "fmv_backend.h"
#include "awtexld.h"


/* winmain.c */
BOOL KeepMainRifFile = FALSE;
int HWAccel = 1;
int VideoModeNotAvailable=0;

/* bink.c */
void PlayBinkedFMV(char *filenamePtr)
{
	FMVBackend_PlayBinked(filenamePtr);
}

void StartMenuBackgroundBink()
{
	FMVBackend_StartMenuBackground();
}

int PlayMenuBackgroundBink()
{
	return FMVBackend_PlayMenuBackground();
}

void EndMenuBackgroundBink()
{
	FMVBackend_EndMenuBackground();
}

/* alt_tab.cpp */
void ATIncludeSurface(DDSurface * pSurface, AW_BACKUPTEXTUREHANDLE hBackup)
{
	fprintf(stderr, "ATIncludeSurface(%p, %p)\n", pSurface, hBackup);
}

void ATRemoveSurface(DDSurface * pSurface)
{
	fprintf(stderr, "ATRemoveSurface(%p)\n", pSurface);
}

void ATRemoveTexture(D3DTexture * pTexture)
{
	fprintf(stderr, "ATRemoveTexture(%p)\n", pTexture);
}


/* d3_func.cpp */
int GetTextureHandle(IMAGEHEADER *imageHeaderPtr)
{
/*
	fprintf(stderr, "GetTextureHandle(%p)\n", imageHeaderPtr);
*/	
	return 1;
}

void ReleaseDirect3DNotDDOrImages()
{
/*
	fprintf(stderr, "ReleaseDirect3DNotDDOrImages()\n");
*/	
}

void ReleaseDirect3DNotDD()
{
/*
	fprintf(stderr, "ReleaseDirect3DNotDD()\n");
*/	
}

void ReleaseDirect3D()
{
/*
	fprintf(stderr, "ReleaseDirect3D()\n");
*/
}

void ReloadImageIntoD3DImmediateSurface(IMAGEHEADER* iheader)
{
	if (iheader == NULL)
	{
		fprintf(stderr, "OGL_TEXTURE_RESTORE rejected: null image header\n");
		return;
	}

	if (iheader->D3DTexture != NULL)
	{
		fprintf(stderr, "OGL_TEXTURE_RESTORE skipped: texture already present image=%p texture=%p\n", iheader, iheader->D3DTexture);
		return;
	}

	if (iheader->hBackup == NULL)
	{
		fprintf(stderr, "OGL_TEXTURE_RESTORE unavailable: image=%p has no backup\n", iheader);
		return;
	}

	/* The OpenGL backend reuses the legacy loader: hBackup retains the converted
	 * source pixels, while AwCreateTexture creates a new GL texture in the current
	 * context.  The former stub only logged this call and left textures NULL. */
	iheader->D3DTexture = AwCreateTexture("rf", iheader->hBackup,
		AW_TLF_PREVSRC | AW_TLF_COMPRESS);
	iheader->D3DHandle = 0;

	if (iheader->D3DTexture == NULL)
	{
		fprintf(stderr, "OGL_TEXTURE_RESTORE failed: image=%p backup=%p error=%d\n",
			iheader, iheader->hBackup, awTlLastErr);
	}
	else
	{
		fprintf(stderr, "OGL_TEXTURE_RESTORE restored: image=%p texture=%p\n",
			iheader, iheader->D3DTexture);
	}
}/* d3d_render.cpp */
int NumberOfLandscapePolygons;
int FMVParticleColour;
int WireFrameMode;

void InitDrawTest()
{
/*
	fprintf(stderr, "InitDrawTest()\n");
*/
}

void CheckWireFrameMode(int shouldBeOn)
{
	if (shouldBeOn)
		fprintf(stderr, "CheckWireFrameMode(%d)\n", shouldBeOn);
}


/* ddplat.cpp */
void MinimizeAllDDGraphics()
{
/*
	fprintf(stderr, "MinimizeAllDDGraphics()\n");
*/	
}

        
/* dd_func.cpp */
long BackBufferPitch;
int VideoModeColourDepth;

void BlitWin95Char(int x, int y, unsigned char toprint)
{
	fprintf(stderr, "BlitWin95Char(%d, %d, %d)\n", x, y, toprint);
}

void LockSurfaceAndGetBufferPointer()
{
	fprintf(stderr, "LockSurfaceAndGetBufferPointer()\n");
}

void finiObjectsExceptDD()
{
/*
	fprintf(stderr, "finiObjectsExceptDD()\n");
*/	
}

void finiObjects()
{
/*
	fprintf(stderr, "finiObjects()\n");
*/	
}

void UnlockSurface()
{
	fprintf(stderr, "UnlockSurface()\n");
}


BOOL ChangeDirectDrawObject()
{
/*
	fprintf(stderr, "ChangeDirectDrawObject()\n");
*/	
	return FALSE;
}

int SelectDirectDrawObject(void *pGUID)
{
/*
	fprintf(stderr, "SelectDirectDrawObject(%p)\n", pGUID);
*/
	return 0;
}

void GenerateDirectDrawSurface()
{
/*
	fprintf(stderr, "GenerateDirectDrawSurface()\n");
*/	
}


/* dxlog.c */
void dx_str_log(char const * str, int line, char const * file)
{
	FILE *fp;

	fp = OpenGameFile("dx_error.log", FILEMODE_APPEND, FILETYPE_CONFIG);	
	if (fp == NULL)
		fp = stderr;
		
	fprintf(fp, "dx_str_log: %s/%d: %s\n", file, line, str);
	
	if (fp != stderr) fclose(fp);
}

void dx_strf_log(char const * fmt, ... )
{
	va_list ap;
	FILE *fp;
	
	fp = OpenGameFile("dx_error.log", FILEMODE_APPEND, FILETYPE_CONFIG);
	if (fp == NULL)
		fp = stderr;
        
        va_start(ap, fmt);
        fprintf(fp, "dx_strf_log: ");
	vfprintf(fp, fmt,ap);
	fprintf(fp, "\n");
        va_end(ap);
        
        if (fp != stderr) fclose(fp);
}

void dx_line_log(int line, char const * file)
{
	FILE *fp;
	
	fp = OpenGameFile("dx_error.log", FILEMODE_APPEND, FILETYPE_CONFIG);
	if (fp == NULL)
		fp = stderr;	
	
	fprintf(fp, "dx_line_log: %s/%d\n", file, line);
	
	if (fp != stderr) fclose(fp);
}
