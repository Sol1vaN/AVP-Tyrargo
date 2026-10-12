#ifndef TYRARGO_MENU_PREVIEW_H
#define TYRARGO_MENU_PREVIEW_H
#ifdef __cplusplus
extern "C" {
#endif
extern int RenderPausedPreview;
extern float MenuPreviewBandLow,MenuPreviewBandHigh;
void AvP_UpdateMenuPreview(void);
int AvP_MenuPreviewScene(void);
int AvP_MenuPreviewHideCrosshair(void);
void AvP_RenderPausedPreview(void);
void RenderPausedWeaponPreview(void);
void RenderMenuCrosshairPreview(int x,int y,int thickness);
void PreserveRenderRandomState(int restore);
void D3D_DrawSolidRect(int left,int top,int right,int bottom,unsigned int colour,int additive);
int ConsoleVersionOpacity(void);
void D3D_RenderConsoleVersion(int left,int top,int right,int bottom,int alpha);
#ifdef __cplusplus
}
#endif
#endif
