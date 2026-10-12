#include "fmv_backend.h"
#include "menu_background.h"

extern "C" {
#include "3dc.h"
#include "fmv.h"
#include "oglfunc.h"
}

#include <SDL3/SDL.h>
#include <AL/al.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <map>
#include <filesystem>
#include <fstream>
#include <cctype>

#include "BinkDecoder.h"
#include "crt_filter.h"

extern "C" {
extern SDL_Surface *surface;
extern SDL_Window *window;
extern int IntroOutroMoviesAreActive;
extern int FMVFilterMode;
extern int SmackerSoundVolume;
extern unsigned char GotAnyKey;
extern unsigned char DebouncedGotAnyKey;
extern void DirectReadKeyboard(void);
extern void CheckForWindowsMessages(void);
extern void FlipBuffers(void);
extern void InitOpenGL(void);
}
extern "C" {
#include "smacker.h"
}

extern "C" {
int FMVScaleMode = FMV_SCALE_ULTRAWIDE_X25;
int MenuBackgroundType = 0;
int MenuMusicMode = 1;
int MenuMusic99Available = 0;
extern int MenuBackgroundMode;
}


struct PCMStream
{
    ALuint source;
    ALuint buffers[4];
    ALenum format;
    ALsizei rate;
    int active;
    unsigned nextBuffer;
    int startAllowed;
    PCMStream *previous, *next;
};

static PCMStream *ActivePCMStreams;

/* Compressed data only: at most 96 MiB, prepared while entering a map.
 * A cold/spinning disk may be slow; neither prefetch nor playback waits on it. */
struct PanelCache {
    std::mutex mutex;
    std::map<int, std::shared_ptr<std::vector<unsigned char>>> files;
    bool started = false;
};
static std::shared_ptr<PanelCache> PanelFileCache = std::make_shared<PanelCache>();
static void PrefetchPanels(std::shared_ptr<PanelCache> cache)
{
    size_t total = 0;
    try {
        for (const auto &entry : std::filesystem::directory_iterator("FMVs")) {
            std::string name = entry.path().filename().string();
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return (char)std::tolower(c); });
            int message = 0, consumed = 0;
            if (std::sscanf(name.c_str(), "message%d.smk%n", &message, &consumed) != 1 || consumed != (int)name.size()) continue;
            if (!entry.is_regular_file()) continue;
            const auto size = entry.file_size();
            if (!size || size > 16u*1024*1024 || total + size > 96u*1024*1024) continue;
            auto bytes = std::make_shared<std::vector<unsigned char>>((size_t)size);
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file.read((char *)bytes->data(), (std::streamsize)size)) continue;
            { std::lock_guard<std::mutex> lock(cache->mutex); cache->files[message] = bytes; }
            total += (size_t)size;
        }
    } catch (...) { /* Missing directory/I/O failure: the demand loader remains available. */ }
}
extern "C" void FMVBackend_PrewarmPanels(void)
{
    auto cache = PanelFileCache;
    std::lock_guard<std::mutex> lock(cache->mutex);
    if (cache->started) return;
    try { std::thread(PrefetchPanels, cache).detach(); cache->started = true; }
    catch (...) { /* No worker: playback still has its independent demand loader. */ }
}

/* File I/O and first-frame decoding never run on the gameplay thread.
 * The worker owns only codec data: OpenAL/texture operations remain on main. */
struct PanelMedia
{
    smk movie = nullptr, audioMovie = nullptr;
    unsigned long width = 0, height = 0, rate = 0;
    double microsecondsPerFrame = 0;
    unsigned char track = 0, channels = 0, bits = 0;
    bool hasAudio = false, videoLast = false, audioLast = false;
    std::atomic<int> ready{0};
    ~PanelMedia() { if (movie) smk_close(movie); if (audioMovie) smk_close(audioMovie); }
};
struct PanelDecoder
{
    std::shared_ptr<PanelMedia> media;
    Uint64 nextFrameAt = 0;
    int message = 0, references = 1;
    bool started = false, audioFinished = false, finished = false;
    PCMStream audio{};
};
static std::vector<PanelDecoder *> ActivePanels;

static void PreparePanel(std::shared_ptr<PanelMedia> media, int message, std::shared_ptr<PanelCache> cache)
{
    char filename[64];
    std::snprintf(filename, sizeof(filename), "FMVs/message%d.smk", message);
    std::shared_ptr<std::vector<unsigned char>> bytes;
    { std::lock_guard<std::mutex> lock(cache->mutex);
      auto found = cache->files.find(message); if (found != cache->files.end()) bytes = found->second; }
    if (bytes) {
        media->movie = smk_open_memory(bytes->data(), (unsigned long)bytes->size());
        media->audioMovie = smk_open_memory_mode(bytes->data(), (unsigned long)bytes->size(), SMK_MODE_AUDIO_ONLY);
    } else {
        /* Full compressed clips in RAM: subsequent frames never seek the disk. */
        media->movie = smk_open_file(filename, SMK_MODE_MEMORY);
        media->audioMovie = smk_open_file(filename, SMK_MODE_MEMORY | SMK_MODE_AUDIO_ONLY);
    }
    if (!media->movie || !media->audioMovie) { media->ready.store(-1); return; }
    unsigned char mask = 0, channels[7] = {}, bits[7] = {}, scale = 0;
    unsigned long rates[7] = {}, frames = 0;
    smk_info_audio(media->audioMovie, &mask, channels, bits, rates);
    smk_enable_all(media->movie, SMK_VIDEO_TRACK);
    smk_enable_all(media->audioMovie, 0);
    for (unsigned char track = 0; track < 7; ++track) {
        if ((mask & (1u << track)) && rates[track] &&
            (channels[track] == 1 || channels[track] == 2) &&
            (bits[track] == 8 || bits[track] == 16)) {
            media->track = track; media->rate = rates[track];
            media->channels = channels[track]; media->bits = bits[track];
            media->hasAudio = true;
            smk_enable_audio(media->audioMovie, track, 1);
            break;
        }
    }
    if (smk_info_all(media->movie, nullptr, &frames, &media->microsecondsPerFrame) < 0 ||
        smk_info_video(media->movie, &media->width, &media->height, &scale) < 0 ||
        !media->width || !media->height || !frames) {
        media->ready.store(-1); return;
    }
    const char videoFirst = smk_first(media->movie);
    const char audioFirst = media->hasAudio ? smk_first(media->audioMovie) : SMK_DONE;
    if (videoFirst == SMK_ERROR || audioFirst == SMK_ERROR) { media->ready.store(-1); return; }
    media->videoLast = videoFirst == SMK_LAST;
    media->audioLast = audioFirst == SMK_LAST;
    media->ready.store(1, std::memory_order_release);
}

static unsigned char ClampToByte(int value)
{
    return (unsigned char)std::max(0, std::min(255, value));
}

static ALenum PCMFormat(unsigned char channels, unsigned char bitDepth)
{
    if (channels == 1) return bitDepth == 16 ? AL_FORMAT_MONO16 : (bitDepth == 8 ? AL_FORMAT_MONO8 : 0);
    if (channels == 2) return bitDepth == 16 ? AL_FORMAT_STEREO16 : (bitDepth == 8 ? AL_FORMAT_STEREO8 : 0);
    return 0;
}

static void PCMClose(PCMStream *stream)
{
    if (!stream || !stream->active) return;
    if (stream->previous) stream->previous->next=stream->next;
    else ActivePCMStreams=stream->next;
    if (stream->next) stream->next->previous=stream->previous;
    if (stream->source) {
        alSourceStop(stream->source);
        alDeleteSources(1, &stream->source);
    }
    for (int i=0;i<4;i++) if (stream->buffers[i]) alDeleteBuffers(1,&stream->buffers[i]);
    std::memset(stream, 0, sizeof(*stream));
}

extern "C" void FMVBackend_ReleaseAudio(void)
{
    /* Includes panels, menu music/background and an active full-screen stream.
       Decoder owners remain valid; their later close/update sees inactive PCM. */
    while (ActivePCMStreams) PCMClose(ActivePCMStreams);
}

static int PCMOpen(PCMStream *stream, unsigned char channels, unsigned char bitDepth, unsigned long rate)
{
    if (!stream || !rate) return 0;
    std::memset(stream, 0, sizeof(*stream));
    stream->format = PCMFormat(channels, bitDepth);
    if (!stream->format) return 0;
    stream->active=1;
    stream->next=ActivePCMStreams;
    if (ActivePCMStreams) ActivePCMStreams->previous=stream;
    ActivePCMStreams=stream;
    alGenSources(1, &stream->source);
    alGenBuffers(4, stream->buffers);
    if (alGetError() != AL_NO_ERROR) {
        PCMClose(stream);
        return 0;
    }
    alSourcei(stream->source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(stream->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSourcef(stream->source, AL_ROLLOFF_FACTOR, 0.0f);
    stream->rate = (ALsizei)rate;
    stream->active = 1;
    return 1;
}

static void PCMPump(PCMStream *stream, const unsigned char *data, unsigned long size, float gain)
{
    ALint processed = 0, queued = 0, state = 0;
    if (!stream || !stream->active) return;
    alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0) { ALuint buffer = 0; alSourceUnqueueBuffers(stream->source, 1, &buffer); }
    alSourcef(stream->source, AL_GAIN, std::max(0.0f, std::min(1.0f, gain)));
    alGetSourcei(stream->source, AL_BUFFERS_QUEUED, &queued);
    if (data && size && queued < 3) {
        ALuint buffer = stream->buffers[stream->nextBuffer];
        alBufferData(buffer, stream->format, data, (ALsizei)size, stream->rate);
        if (alGetError() == AL_NO_ERROR) { alSourceQueueBuffers(stream->source, 1, &buffer); stream->nextBuffer = (stream->nextBuffer + 1u) & 3u; }
    }
    alGetSourcei(stream->source, AL_SOURCE_STATE, &state);
    alGetSourcei(stream->source, AL_BUFFERS_QUEUED, &queued);
    if (stream->startAllowed && queued && state != AL_PLAYING) alSourcePlay(stream->source);
}

static int PCMQueued(PCMStream *stream, float gain)
{
    ALint queued = 0;
    PCMPump(stream, 0, 0, gain);
    if (stream && stream->active) alGetSourcei(stream->source, AL_BUFFERS_QUEUED, &queued);
    return (int)queued;
}

static void PCMStart(PCMStream *stream, float gain)
{
    if (!stream || !stream->active) return;
    stream->startAllowed = 1;
    PCMPump(stream, 0, 0, gain);
}

static float PanelGain(void)
{
    return (float)SmackerSoundVolume / (float)(ONE_FIXED / 512);
}

static void QueuePanelAudio(PanelDecoder *decoder)
{
    /* Bound sparse/malformed streams; never scan the whole film in one tick.
     * audio-only decoding avoids reconstructing a second invisible video. */
    PanelMedia &media = *decoder->media;
    for (int budget = 0; budget < 8 && decoder->audio.active &&
         !decoder->audioFinished && PCMQueued(&decoder->audio, PanelGain()) < 3; ++budget) {
        PCMPump(&decoder->audio, smk_get_audio(media.audioMovie, media.track),
                smk_get_audio_size(media.audioMovie, media.track), PanelGain());
        if (media.audioLast) { decoder->audioFinished = true; break; }
        const char result = smk_next(media.audioMovie);
        media.audioLast = result == SMK_LAST;
        /* SMK_LAST is a decoded frame, including its final audio packet. */
        if (result == SMK_DONE || result == SMK_ERROR) decoder->audioFinished = true;
    }
}

extern "C" int FMVBackend_OpenPanel(FMVTEXTURE *texture, int messageNumber)
{
    if (!texture) return 0;
    FMVBackend_ClosePanel(texture);
    PanelDecoder *decoder = nullptr;
    for (auto *active : ActivePanels) {
        if (active->message == messageNumber && !active->finished) {
            decoder = active; ++decoder->references; break;
        }
    }
    if (!decoder) {
        decoder = new PanelDecoder();
        decoder->message = messageNumber;
        decoder->media = std::make_shared<PanelMedia>();
        try { std::thread(PreparePanel, decoder->media, messageNumber, PanelFileCache).detach(); }
        catch (...) { delete decoder; return 0; }
        ActivePanels.push_back(decoder);
    }
    texture->DecoderHandle = decoder;
    texture->MessageNumber = messageNumber;
    texture->StaticImageDrawn = 0;
    texture->PanelFinishPending = 0;
    return 1;
}

extern "C" void FMVBackend_ClosePanel(FMVTEXTURE *texture)
{
    if (!texture || !texture->DecoderHandle) return;
    auto *decoder = static_cast<PanelDecoder *>(texture->DecoderHandle);
    texture->DecoderHandle = nullptr;
    if (--decoder->references) return;
    ActivePanels.erase(std::remove(ActivePanels.begin(), ActivePanels.end(), decoder), ActivePanels.end());
    PCMClose(&decoder->audio);
    /* A pending worker retains its codec ownership until I/O finishes. Closing
     * a panel/map never joins that worker and therefore cannot stall gameplay. */
    delete decoder;
}

extern "C" int FMVBackend_CopyPanelFrame(FMVTEXTURE *texture, unsigned char *indexedPixels, unsigned char *paletteRGB)
{
    if (!texture || !texture->DecoderHandle || !indexedPixels || !paletteRGB) return 0;
    auto *decoder = static_cast<PanelDecoder *>(texture->DecoderHandle);
    PanelMedia &media = *decoder->media;
    const int ready = media.ready.load(std::memory_order_acquire);
    if (ready <= 0) { if (ready < 0) texture->PanelFinishPending = 1; return 0; }
    const Uint64 now = SDL_GetTicks();
    const Uint64 interval = (Uint64)std::max(1.0, media.microsecondsPerFrame / 1000.0);
    if (!decoder->started) {
        decoder->started = true;
        decoder->nextFrameAt = now + interval;
        if (media.hasAudio) PCMOpen(&decoder->audio, media.channels, media.bits, media.rate);
    } else if (!decoder->finished && now >= decoder->nextFrameAt) {
        const char result = media.videoLast ? SMK_DONE : smk_next(media.movie);
        media.videoLast = result == SMK_LAST;
        if (result == SMK_DONE || result == SMK_ERROR) decoder->finished = true;
        decoder->nextFrameAt += ((now - decoder->nextFrameAt) / interval + 1) * interval;
    }
    const auto *pixels = smk_get_video(media.movie);
    const auto *palette = smk_get_palette(media.movie);
    if (!pixels || !palette) { texture->PanelFinishPending = 1; return 0; }
    std::memcpy(paletteRGB, palette, 256 * 3);
    if (media.width == 128 && media.height == 96) std::memcpy(indexedPixels, pixels, 128 * 96);
    else for (int y = 0; y < 96; ++y) {
        const unsigned long sy = (unsigned long)((uint64_t)y * media.height / 96u);
        for (int x = 0; x < 128; ++x) {
            const unsigned long sx = (unsigned long)((uint64_t)x * media.width / 128u);
            indexedPixels[y * 128 + x] = pixels[sy * media.width + sx];
        }
    }
    QueuePanelAudio(decoder);
    return decoder->finished ? 2 : 1;
}

extern "C" void FMVBackend_CommitPanelAudio(FMVTEXTURE *texture)
{
    if (!texture || !texture->DecoderHandle) return;
    auto *decoder = static_cast<PanelDecoder *>(texture->DecoderHandle);
    PCMStart(&decoder->audio, PanelGain());
}
static void ConvertBinkYuvToRgb(const YUVbuffer yuv, uint32_t width, uint32_t height, std::vector<unsigned char> &rgb)
{
    rgb.resize((size_t)width * height * 3u);
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t *srcY = yuv[0].data + y * yuv[0].pitch;
        const uint8_t *srcU = yuv[1].data + (y / 2u) * yuv[1].pitch;
        const uint8_t *srcV = yuv[2].data + (y / 2u) * yuv[2].pitch;
        unsigned char *dst = &rgb[(size_t)y * width * 3u];
        for (uint32_t x = 0; x < width; ++x) {
            const int c = (int)srcY[x] - 16;
            const int d = (int)srcU[x / 2u] - 128;
            const int e = (int)srcV[x / 2u] - 128;
            dst[x * 3u + 0] = ClampToByte((298 * c + 409 * e + 128) >> 8);
            dst[x * 3u + 1] = ClampToByte((298 * c - 100 * d - 208 * e + 128) >> 8);
            dst[x * 3u + 2] = ClampToByte((298 * c + 516 * d + 128) >> 8);
        }
    }
}

static void DrawBinkFramePhysical(GLuint texture, uint32_t width, uint32_t height, const unsigned char *rgb)
{
    int viewportWidth = 0;
    int viewportHeight = 0;
    int drawX;
    int drawY;
    int drawWidth;
    int drawHeight;
    GLfloat vertices[16];
    GLshort indices[6] = {0, 1, 2, 0, 2, 3};
    if (!window || !texture || !width || !height || !rgb) return;
    SDL_GetWindowSizeInPixels(window, &viewportWidth, &viewportHeight);
    if (viewportWidth <= 0 || viewportHeight <= 0) return;

    /* A physical viewport makes the presentation mode independent of the
       engine's preceding 2D/3D transform state. */
    int scaleNumerator = 0;
    int aspectWidth = 16;
    int aspectHeight = 9;
    switch (FMVScaleMode) {
        case FMV_SCALE_PANORAMIC_X1:  scaleNumerator = 2; break;
        case FMV_SCALE_PANORAMIC_X15: scaleNumerator = 3; break;
        case FMV_SCALE_PANORAMIC_X2:  scaleNumerator = 4; break;
        case FMV_SCALE_PANORAMIC_X25: scaleNumerator = 5; break;
        case FMV_SCALE_PANORAMIC_X3:  scaleNumerator = 6; break;
        case FMV_SCALE_PANORAMIC_X35: scaleNumerator = 7; break;
        case FMV_SCALE_PANORAMIC_X4:  scaleNumerator = 8; break;
        case FMV_SCALE_ULTRAWIDE_X1:  scaleNumerator = 2; aspectWidth = 21; break;
        case FMV_SCALE_ULTRAWIDE_X15: scaleNumerator = 3; aspectWidth = 21; break;
        case FMV_SCALE_ULTRAWIDE_X2:  scaleNumerator = 4; aspectWidth = 21; break;
        case FMV_SCALE_ULTRAWIDE_X25: scaleNumerator = 5; aspectWidth = 21; break;
        case FMV_SCALE_ULTRAWIDE_X3:  scaleNumerator = 6; aspectWidth = 21; break;
        case FMV_SCALE_ULTRAWIDE_X35: scaleNumerator = 7; aspectWidth = 21; break;
        case FMV_SCALE_ULTRAWIDE_X4:  scaleNumerator = 8; aspectWidth = 21; break;
        case FMV_SCALE_FULLSCREEN_ULTRAWIDE: aspectWidth = 21; break;
        case FMV_SCALE_FULLSCREEN_WIDESCREEN:
        default: break;
    }    if (scaleNumerator) {
        /* x1, x1.5 .. x4 use the selected display aspect, centred and never cropped. */
        drawHeight = std::min((int)(((long long)height * scaleNumerator + 1) / 2), viewportHeight);
        drawWidth = std::max(1, (int)(((double)drawHeight * aspectWidth / aspectHeight) + 0.5));
        if (drawWidth > viewportWidth) {
            drawWidth = viewportWidth;
            drawHeight = std::max(1, (int)(((double)drawWidth * aspectHeight / aspectWidth) + 0.5));
        }
    } else {
        /* Full-screen profiles fill as much of the physical framebuffer as
           their 16:9 or 21:9 presentation rectangle permits. */
        if ((long long)viewportWidth * aspectHeight > (long long)viewportHeight * aspectWidth) {
            drawHeight = viewportHeight;
            drawWidth = std::max(1, (int)(((double)drawHeight * aspectWidth / aspectHeight) + 0.5));
        } else {
            drawWidth = viewportWidth;
            drawHeight = std::max(1, (int)(((double)drawWidth * aspectHeight / aspectWidth) + 0.5));
        }
    }drawX = (viewportWidth - drawWidth) / 2;
    drawY = (viewportHeight - drawHeight) / 2;
    pglViewport(0, 0, viewportWidth, viewportHeight);
    pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    pglDisable(GL_ALPHA_TEST);
    pglDisable(GL_BLEND);
    pglDisable(GL_DEPTH_TEST);
    pglEnable(GL_TEXTURE_2D);
    pglBindTexture(GL_TEXTURE_2D, texture);
    pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FMVFilterMode==0?GL_NEAREST:GL_LINEAR);
    pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FMVFilterMode==0?GL_NEAREST:GL_LINEAR);
    pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)width, (GLsizei)height, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    pglViewport(drawX, drawY, drawWidth, drawHeight);
    vertices[0] = -1.0f; vertices[1] = -1.0f; vertices[2] = 0.0f; vertices[3] = 1.0f;
    vertices[4] =  1.0f; vertices[5] = -1.0f; vertices[6] = 1.0f; vertices[7] = 1.0f;
    vertices[8] =  1.0f; vertices[9] =  1.0f; vertices[10] = 1.0f; vertices[11] = 0.0f;
    vertices[12] = -1.0f; vertices[13] = 1.0f; vertices[14] = 0.0f; vertices[15] = 0.0f;
    pglEnableClientState(GL_VERTEX_ARRAY);
    pglVertexPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, vertices);
    pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
    pglTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat) * 4, vertices + 2);
    pglDisableClientState(GL_COLOR_ARRAY);
    pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    pglDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
    pglBindTexture(GL_TEXTURE_2D, 0);
    /* Apply to the final physical framebuffer so FMV receives the same CRT
       pass as the 2D frontend and the 3D game image. */
    CRTFilter_Apply(viewportWidth, viewportHeight);
    /* Preserve the selected physical viewport through the swap. */
    SDL_GL_SwapWindow(window);
    pglViewport(0, 0, viewportWidth, viewportHeight);
}

/* The Bink presenter bypasses the engine batcher. Restore both OpenGL and
 * the renderer cache immediately after a movie so the next menu frame cannot
 * inherit GL_REPLACE, disabled colour arrays, or the FMV viewport. */
static void RestoreEngineGLStateAfterFMV(void)
{
    int width = 0;
    int height = 0;
    if (!window) return;

    SDL_GetWindowSizeInPixels(window, &width, &height);
    if (width > 0 && height > 0) pglViewport(0, 0, width, height);
    pglBindTexture(GL_TEXTURE_2D, 0);
    pglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    pglPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    pglEnable(GL_TEXTURE_2D);
    pglEnable(GL_ALPHA_TEST);
    pglAlphaFunc(GL_GREATER, 0.0f);
    pglEnable(GL_DEPTH_TEST);
    pglDepthFunc(GL_LEQUAL);
    pglDepthMask(GL_TRUE);
    pglDisable(GL_BLEND);
    pglBlendFunc(GL_ONE, GL_ZERO);

    /* Rebind the engine arrays and invalidate its texture/translucency cache. */
    InitOpenGL();

    /* Present one neutral frame after the last Bink RGB image. The frontend
     * then owns the following frame entirely, with no decoder image retained
     * in the backbuffer. */
    pglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    pglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CRTFilter_Apply(width, height);
    SDL_GL_SwapWindow(window);
}
static void QueueBinkAudio(BinkHandle &movie, uint32_t track, std::vector<unsigned char> &samples, PCMStream *audio)
{
    while (audio && audio->active && movie.isValid && Bink_GetCurrentFrameNum(movie) < Bink_GetNumFrames(movie) && PCMQueued(audio, 1.0f) < 3) {
        YUVbuffer discard{};
        Bink_GetNextFrame(movie, discard);
        const uint32_t size = Bink_GetAudioData(movie, track, (int16_t *)samples.data());
        PCMPump(audio, samples.data(), size, 1.0f);
    }
}

extern "C" void FMVBackend_PlayBinked(const char *filename)
{
    BinkHandle videoMovie;
    BinkHandle audioMovie{};
    YUVbuffer yuv{};
    uint32_t width = 0, height = 0, frameNumber = 0, audioTrack = 0;
    float frameRate;
    Uint64 timelineStart;
    GLuint texture = 0;
    std::vector<unsigned char> rgb;
    PCMStream audio{};
    std::vector<unsigned char> audioSamples;
    if (!filename || !IntroOutroMoviesAreActive || !window) return;
    videoMovie = Bink_Open(filename);
    if (!videoMovie.isValid) return;
    Bink_GetFrameSize(videoMovie, width, height);
    if (!width || !height) { Bink_Close(videoMovie); return; }
    if (Bink_GetNumAudioTracks(videoMovie)) {
        audioMovie = Bink_Open(filename);
        if (audioMovie.isValid) {
            const AudioInfo info = Bink_GetAudioTrackDetails(audioMovie, audioTrack);
            if (PCMOpen(&audio, (unsigned char)info.nChannels, 16, info.sampleRate)) {
                audioSamples.resize(info.idealBufferSize);
                QueueBinkAudio(audioMovie, audioTrack, audioSamples, &audio);
            }
        }
    }
    frameRate = Bink_GetFrameRate(videoMovie);
    if (frameRate <= 0.0f) frameRate = 15.0f;
    pglGenTextures(1, &texture);
    pglBindTexture(GL_TEXTURE_2D, texture);
    pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    timelineStart = SDL_GetTicks();
    while (Bink_GetCurrentFrameNum(videoMovie) < Bink_GetNumFrames(videoMovie)) {
        CheckForWindowsMessages();
        Bink_GetNextFrame(videoMovie, yuv);
        ConvertBinkYuvToRgb(yuv, width, height, rgb);
        DrawBinkFramePhysical(texture, width, height, rgb.data());
        /* At this point the first visual PTS is on screen; start its prebuffer. */
        PCMStart(&audio, 1.0f);
        QueueBinkAudio(audioMovie, audioTrack, audioSamples, &audio);
        DirectReadKeyboard();
        if (GotAnyKey && DebouncedGotAnyKey) break;
        ++frameNumber;
        const Uint64 target = timelineStart + (Uint64)((double)frameNumber * 1000.0 / frameRate);
        const Uint64 now = SDL_GetTicks();
        if (target > now) SDL_Delay((Uint32)(target - now));
    }
    if (texture) pglDeleteTextures(1, &texture);
    PCMClose(&audio);
    if (audioMovie.isValid) Bink_Close(audioMovie);
    Bink_Close(videoMovie);
    RestoreEngineGLStateAfterFMV();
}
struct BinkAudioLoop { BinkHandle movie; PCMStream audio; std::vector<unsigned char> samples; uint32_t track; float rate; Uint64 next; int active; };
struct SmackerAudioLoop { smk movie; PCMStream audio; unsigned char track; double frameUs; Uint64 next; int active; int audioFinished; };
static BinkAudioLoop MenuBackground{};
static SmackerAudioLoop MenuMusic{};
static int MenuMusicAttemptedMode = -1;
static int MenuMusicPlayingMode = -1;
struct MenuVideoLoop {
    BinkHandle movie{};
    std::vector<unsigned char> rgb;
    uint32_t width = 0, height = 0;
    Uint64 start = 0;
    float rate = 15.0f;
    int attempted = 0;
};
static MenuVideoLoop MenuVideo;
static void CloseMenuVideo(void)
{
    if (MenuVideo.movie.isValid) Bink_Close(MenuVideo.movie);
    MenuVideo = MenuVideoLoop{};
}
extern "C" void FMVBackend_RefreshMenuAssets(void)
{
    /* Once per frontend entry/settings load, never filesystem polling per frame. */
    FILE *f = fopen("FMVs/IntroSound99.smk", "rb");
    MenuMusic99Available = f != nullptr;
    if (f) fclose(f);
}
static int SelectedMenuMusicMode(void)
{
    return MenuMusicMode == 1 && MenuMusic99Available ? 1 : 0;
}
extern "C" int FMVBackend_DrawMenuBackground(void)
{
    if (!MenuBackgroundType || !surface) { CloseMenuVideo(); return 0; }
    if (!MenuVideo.attempted) {
        MenuVideo.attempted = 1;
        MenuVideo.movie = Bink_Open("FMVs/Menubackground.bik");
        if (!MenuVideo.movie.isValid) return 0;
        Bink_GetFrameSize(MenuVideo.movie, MenuVideo.width, MenuVideo.height);
        MenuVideo.rate = Bink_GetFrameRate(MenuVideo.movie);
        if (MenuVideo.rate <= 0.0f) MenuVideo.rate = 15.0f;
        MenuVideo.start = SDL_GetTicks();
    }
    if (!MenuVideo.movie.isValid || !MenuVideo.width || !MenuVideo.height) return 0;
    const uint32_t frames = Bink_GetNumFrames(MenuVideo.movie);
    if (!frames) return 0;
    const Uint64 now = SDL_GetTicks();
    uint32_t target = (uint32_t)((now-MenuVideo.start)*MenuVideo.rate/1000.0);
    if (target >= frames) {
        /* Decoder rewind keeps loop timing independent of the menu FPS. */
        Bink_GotoFrame(MenuVideo.movie,0);
        MenuVideo.start = now;
        MenuVideo.rgb.clear();
        target = 0;
    }
    YUVbuffer yuv{};
    for(uint32_t decoded=0;decoded<frames; ++decoded) {
        const uint32_t before=Bink_GetCurrentFrameNum(MenuVideo.movie);
        if(before>target)break;
        Bink_GetNextFrame(MenuVideo.movie, yuv);
        ConvertBinkYuvToRgb(yuv, MenuVideo.width, MenuVideo.height, MenuVideo.rgb);
        if(Bink_GetCurrentFrameNum(MenuVideo.movie)==before)break;
    }
    if (MenuVideo.rgb.empty()) return 0;
    if (SDL_MUSTLOCK(surface) && !SDL_LockSurface(surface)) return 0;
    /* Same authored framing and fill/letterbox/center mapping as static backdrops.
     * Draw into the frontend canvas; labels/rectangles keep their own scaling. */
    const int iw = (int)MenuVideo.width;
    const MenuBackdropMapping map=MenuBackdropMap(surface->w,surface->h,iw,(int)MenuVideo.height,MenuBackgroundMode);
    const int sourceTop=map.top,ih=map.height;
    const double scaleX=map.scaleX,scaleY=map.scaleY,ox=map.ox,oy=map.oy;
    for (int y=0; y<surface->h; ++y) {
        auto dst=(unsigned short*)((unsigned char*)surface->pixels+y*surface->pitch);
        const double sourceY=oy+(y+.5)/scaleY;
        const int sy=(int)sourceY;
        for (int x=0; x<surface->w; ++x) {
            const double sourceX=ox+(x+.5)/scaleX;
            const int sx=(int)sourceX;
            if(sourceX<0 || sourceY<0 || sx>=iw || sy>=ih) { dst[x]=0; continue; }
            const unsigned char *rgb=MenuVideo.rgb.data()+3*((sy+sourceTop)*iw+sx);
            dst[x]=((rgb[0]>>3)<<11)|((rgb[1]>>2)<<5)|(rgb[2]>>3);
        }
    }
    if(SDL_MUSTLOCK(surface))SDL_UnlockSurface(surface);
    return 1;
}

static void CloseBinkAudioLoop(BinkAudioLoop *loop)
{
    if (!loop) return;
    PCMClose(&loop->audio);
    if (loop->movie.isValid) Bink_Close(loop->movie);
    loop->movie = BinkHandle{};
    loop->samples.clear();
    loop->track = 0;
    loop->rate = 0.0f;
    loop->next = 0;
    loop->active = 0;
}
static int OpenBinkAudioLoop(BinkAudioLoop *loop, const char *filename)
{
    if (!loop || !filename) return 0;
    CloseBinkAudioLoop(loop);
    loop->movie = Bink_Open(filename);
    if (!loop->movie.isValid || !Bink_GetNumAudioTracks(loop->movie)) { CloseBinkAudioLoop(loop); return 0; }
    const AudioInfo info = Bink_GetAudioTrackDetails(loop->movie, 0);
    if (!PCMOpen(&loop->audio, (unsigned char)info.nChannels, 16, info.sampleRate)) { CloseBinkAudioLoop(loop); return 0; }
    loop->samples.resize(info.idealBufferSize);
    loop->rate = Bink_GetFrameRate(loop->movie);
    if (loop->rate <= 0.0f) loop->rate = 15.0f;
    loop->active = 1;
    QueueBinkAudio(loop->movie, loop->track, loop->samples, &loop->audio);
    PCMStart(&loop->audio, 1.0f);
    loop->next = SDL_GetTicks() + (Uint64)(3000.0f / loop->rate);
    return 1;
}
static void UpdateBinkAudioLoop(BinkAudioLoop *loop)
{
    const Uint64 now = SDL_GetTicks();
    if (!loop || !loop->active) return;
    if (Bink_GetCurrentFrameNum(loop->movie) >= Bink_GetNumFrames(loop->movie)) { OpenBinkAudioLoop(loop, "FMVs/Menubackground.bik"); return; }
    if (now >= loop->next) {
        QueueBinkAudio(loop->movie, loop->track, loop->samples, &loop->audio);
        do { loop->next += (Uint64)std::max(1.0f, 1000.0f / loop->rate); } while (loop->next <= now);
    } else PCMQueued(&loop->audio, 1.0f);
}
static void CloseSmackerAudioLoop(SmackerAudioLoop *loop)
{
    if (!loop) return;
    PCMClose(&loop->audio);
    if (loop->movie) smk_close(loop->movie);
    std::memset(loop, 0, sizeof(*loop));
}
static int OpenSmackerAudioLoop(SmackerAudioLoop *loop, const char *filename)
{
    unsigned long frame = 0, frames = 0, rates[7] = {0};
    unsigned char mask = 0, channels[7] = {0}, depth[7] = {0};
    if (!loop || !filename) return 0;
    CloseSmackerAudioLoop(loop);
    loop->movie = smk_open_file(filename, SMK_MODE_DISK | SMK_MODE_AUDIO_ONLY);
    if (!loop->movie) return 0;
    smk_info_audio(loop->movie, &mask, channels, depth, rates);
    smk_enable_all(loop->movie, 0);
    for (unsigned char track = 0; track < 7; ++track) if ((mask & (1u << track)) && PCMOpen(&loop->audio, channels[track], depth[track], rates[track])) { loop->track = track; smk_enable_audio(loop->movie, track, 1); break; }
    if (!loop->audio.active || smk_info_all(loop->movie, &frame, &frames, &loop->frameUs) < 0 || smk_first(loop->movie) == SMK_ERROR) { CloseSmackerAudioLoop(loop); return 0; }
    loop->active = 1;
    /* IntroSound is one-shot. Prebuffer first three frames, then do not loop. */
    while (!loop->audioFinished && PCMQueued(&loop->audio, 1.0f) < 3) {
        PCMPump(&loop->audio, smk_get_audio(loop->movie, loop->track), smk_get_audio_size(loop->movie, loop->track), 1.0f);
        const char result = smk_next(loop->movie);
        if (result == SMK_DONE || result == SMK_LAST) loop->audioFinished = 1;
    }
    PCMStart(&loop->audio, 1.0f);
    return 1;
}
static void UpdateSmackerAudioLoop(SmackerAudioLoop *loop)
{
    if (!loop || !loop->active) return;
    while (!loop->audioFinished && PCMQueued(&loop->audio, 1.0f) < 3) {
        PCMPump(&loop->audio, smk_get_audio(loop->movie, loop->track), smk_get_audio_size(loop->movie, loop->track), 1.0f);
        const char result = smk_next(loop->movie);
        if (result == SMK_DONE || result == SMK_LAST) loop->audioFinished = 1;
    }
    PCMQueued(&loop->audio, 1.0f);
}extern "C" void FMVBackend_StartMenuBackground(void) { OpenBinkAudioLoop(&MenuBackground, "FMVs/Menubackground.bik"); }
extern "C" int FMVBackend_PlayMenuBackground(void) { UpdateBinkAudioLoop(&MenuBackground); return 0; }
extern "C" void FMVBackend_EndMenuBackground(void) { CloseBinkAudioLoop(&MenuBackground); CloseMenuVideo(); }
extern "C" int FMVBackend_MenuMusicIs99(void) { return MenuMusicPlayingMode==1; }
extern "C" void FMVBackend_StartMenuMusic(void)
{
    MenuMusicAttemptedMode=SelectedMenuMusicMode();
    MenuMusicPlayingMode=-1;
    if(OpenSmackerAudioLoop(&MenuMusic,MenuMusicAttemptedMode ? "FMVs/IntroSound99.smk" : "FMVs/IntroSound.smk"))
        MenuMusicPlayingMode=MenuMusicAttemptedMode;
    else if(MenuMusicAttemptedMode && OpenSmackerAudioLoop(&MenuMusic,"FMVs/IntroSound.smk"))
        MenuMusicPlayingMode=0;
}
extern "C" void FMVBackend_EnsureMenuMusic(void)
{
    if(MenuMusicAttemptedMode != SelectedMenuMusicMode()) FMVBackend_StartMenuMusic();
}
extern "C" void FMVBackend_PlayMenuMusic(void)
{
    FMVBackend_EnsureMenuMusic();
    UpdateSmackerAudioLoop(&MenuMusic);
}
extern "C" void FMVBackend_EndMenuMusic(void)
{
    CloseSmackerAudioLoop(&MenuMusic); MenuMusicAttemptedMode=MenuMusicPlayingMode=-1;
}