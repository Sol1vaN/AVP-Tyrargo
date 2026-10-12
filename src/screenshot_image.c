#include "screenshot_image.h"
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#define STB_IMAGE_WRITE_STATIC
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"

int ScreenshotFormat = SCREENSHOT_PNG;

const char *ScreenshotExtension(int format)
{
    static const char *const extensions[] = {"bmp", "png", "jpg"};
    return extensions[format >= 0 && format < SCREENSHOT_FORMAT_COUNT ? format : SCREENSHOT_BMP];
}

typedef struct ScreenshotOutput { FILE *stream; int failed; } ScreenshotOutput;
static void WriteImageBytes(void *context, void *data, int size)
{
    ScreenshotOutput *out = (ScreenshotOutput *)context;
    if (!out->failed && size > 0 && fwrite(data, 1, (size_t)size, out->stream) != (size_t)size)
        out->failed = 1;
}
static void Store32(unsigned char *dst, uint32_t value)
{
    dst[0]=(unsigned char)value; dst[1]=(unsigned char)(value>>8);
    dst[2]=(unsigned char)(value>>16); dst[3]=(unsigned char)(value>>24);
}
static int WriteBitmap(FILE *stream, int width, int height, const unsigned char *pixels)
{
    const size_t rowBytes = (size_t)width * 3;
    const size_t stride = (rowBytes + 3) & ~(size_t)3;
    unsigned char header[54] = {0};
    unsigned char *row = (unsigned char *)calloc(stride, 1);
    int x, y, ok;
    if (!row) return 0;
    header[0]='B'; header[1]='M';
    Store32(header+2, (uint32_t)(54 + stride * height));
    Store32(header+10, 54); Store32(header+14, 40);
    Store32(header+18, (uint32_t)width); Store32(header+22, (uint32_t)height);
    header[26]=1; header[28]=24;
    Store32(header+34, (uint32_t)(stride * height));
    /* Equal physical density means square pixels, NOT a square image. */
    Store32(header+38, 3780); Store32(header+42, 3780);
    ok = fwrite(header, 1, sizeof(header), stream) == sizeof(header);
    for (y=0; ok && y<height; ++y) {
        const unsigned char *src = pixels + (size_t)y * rowBytes;
        for (x=0; x<width; ++x) {
            row[x*3]=src[x*3+2]; row[x*3+1]=src[x*3+1]; row[x*3+2]=src[x*3];
        }
        ok = fwrite(row, 1, stride, stream) == stride;
    }
    free(row);
    return ok;
}
int WriteScreenshotImage(FILE *stream, int format, int width, int height, const unsigned char *pixels)
{
    ScreenshotOutput out = {stream, 0};
    int ok;
    /* The bundled encoders use signed int sizes internally. */
    if (!stream || !pixels || format < 0 || format >= SCREENSHOT_FORMAT_COUNT ||
        width <= 0 || height <= 0 || width > (INT_MAX-1024)/4 ||
        height > (INT_MAX-1024)/(width*4)) return 0;
    if (format == SCREENSHOT_BMP) ok = WriteBitmap(stream, width, height, pixels);
    else {
        stbi_flip_vertically_on_write(1);
        if (format == SCREENSHOT_PNG)
            ok = stbi_write_png_to_func(WriteImageBytes, &out, width, height, 3, pixels, width*3);
        else
            /* stb disables chroma subsampling at quality >= 90; 100 is fixed. */
            ok = stbi_write_jpg_to_func(WriteImageBytes, &out, width, height, 3, pixels, 100);
        stbi_flip_vertically_on_write(0);
    }
    if (fflush(stream) != 0) out.failed = 1;
    return ok && !out.failed && !ferror(stream);
}
