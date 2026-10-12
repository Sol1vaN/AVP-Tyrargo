#ifndef AVP_SCREENSHOT_IMAGE_H
#define AVP_SCREENSHOT_IMAGE_H
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { SCREENSHOT_BMP, SCREENSHOT_PNG, SCREENSHOT_JPG, SCREENSHOT_FORMAT_COUNT };
extern int ScreenshotFormat;
int ScreenshotCapturePending(void);
/* Packed RGB, rows ordered bottom to top as returned by glReadPixels.
 * Does not close the stream or take ownership of pixels. Single render thread. */
int WriteScreenshotImage(FILE *stream, int format, int width, int height, const unsigned char *pixels);
const char *ScreenshotExtension(int format);
#ifdef __cplusplus
}
#endif
#endif
