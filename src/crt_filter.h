#ifndef CRT_FILTER_H
#define CRT_FILTER_H

/* Final framebuffer post-process. It deliberately sits after every legacy
 * fixed-function render path, so it affects the same image ReShade sees. */
#ifdef __cplusplus
extern "C" {
#endif

extern int CRTFilterEnabled;
extern int ImageBrightness,ImageContrast,ImageSaturation;
int ImageAdjustmentsActive(void);
int CRTFilter_Apply(int drawableWidth, int drawableHeight);
void CRTFilter_Release(void);

#ifdef __cplusplus
}
#endif

#endif
