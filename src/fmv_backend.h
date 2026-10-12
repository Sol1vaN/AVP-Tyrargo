#ifndef FMV_BACKEND_H
#define FMV_BACKEND_H

typedef struct FMVTEXTURE FMVTEXTURE;

#define FMV_SCALE_PANORAMIC_X1 0
#define FMV_SCALE_PANORAMIC_X15 1
#define FMV_SCALE_PANORAMIC_X2 2
#define FMV_SCALE_PANORAMIC_X25 3
#define FMV_SCALE_PANORAMIC_X3 4
#define FMV_SCALE_PANORAMIC_X35 5
#define FMV_SCALE_PANORAMIC_X4 6
#define FMV_SCALE_ULTRAWIDE_X1 7
#define FMV_SCALE_ULTRAWIDE_X15 8
#define FMV_SCALE_ULTRAWIDE_X2 9
#define FMV_SCALE_ULTRAWIDE_X25 10
#define FMV_SCALE_ULTRAWIDE_X3 11
#define FMV_SCALE_ULTRAWIDE_X35 12
#define FMV_SCALE_ULTRAWIDE_X4 13
#define FMV_SCALE_FULLSCREEN_WIDESCREEN 14
#define FMV_SCALE_FULLSCREEN_ULTRAWIDE 15
#ifdef __cplusplus
extern "C" {
#endif

extern int FMVScaleMode;
/* Frontend backdrop and optional original-1999 menu music; not in-game FMVs. */
extern int MenuBackgroundType; /* 0 Starfield.RIM, 1 Menubackground.bik */
extern int MenuMusicMode; /* 0 IntroSound.smk, 1 IntroSound99.smk */
extern int MenuMusic99Available;
void FMVBackend_RefreshMenuAssets(void);
int FMVBackend_DrawMenuBackground(void);
/* 0 nearest, 1 bilinear; video textures have no mip-chain. */
extern int FMVFilterMode;

void FMVBackend_PrewarmPanels(void);
int FMVBackend_OpenPanel(FMVTEXTURE *texture, int messageNumber);
void FMVBackend_ClosePanel(FMVTEXTURE *texture);
int FMVBackend_CopyPanelFrame(FMVTEXTURE *texture, unsigned char *indexedPixels, unsigned char *paletteRGB);
void FMVBackend_CommitPanelAudio(FMVTEXTURE *texture);
void FMVBackend_PlayBinked(const char *filename);
void FMVBackend_StartMenuBackground(void);
int FMVBackend_PlayMenuBackground(void);
void FMVBackend_EndMenuBackground(void);
int FMVBackend_MenuMusicIs99(void);
void FMVBackend_StartMenuMusic(void);
void FMVBackend_EnsureMenuMusic(void);
void FMVBackend_PlayMenuMusic(void);
void FMVBackend_EndMenuMusic(void);
void FMVBackend_ReleaseAudio(void); /* before destroying the shared OpenAL context */

#ifdef __cplusplus
}
#endif

#endif