#ifndef FMV_H
#define FMV_H

#define MONITOR_NO_SIGNAL_RGB_NOISE 0
#define MONITOR_NO_SIGNAL_RGB_SCANLINES 1
extern int MonitorNoSignalEffectMode;

typedef struct FMVTEXTURE
{
	IMAGEHEADER *ImagePtr;
	int SoundVolume;
	int IsTriggeredPlotFMV;
	int StaticImageDrawn;
	/* The decoder has delivered its final picture. Keep that picture for the
	   current update, then return the panel to native no-signal on the next. */
	int PanelFinishPending;
	int MessageNumber;
	PALETTEENTRY SrcPalette[256];
	unsigned char* PalettedBuf;
	unsigned char* RGBBuf;
	void *DecoderHandle;
	int RedScale;
	int GreenScale;
	int BlueScale;
} FMVTEXTURE;

extern int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr);
extern void UpdateFMVTexturePalette(FMVTEXTURE *ftPtr);
extern void InitialiseTriggeredFMVs(void);
extern void StartTriggerPlotFMV(int number);
extern void StartFMVAtFrame(int number, int frame);
extern void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr);
int FMV_IsPanelTexture(const void *texture);
void UpdateAllFMVTextures(void);
void ScanImagesForFMVs(void);
void ReleaseAllFMVTextures(void);
void PlayBinkedFMV(char *filenamePtr);
#endif