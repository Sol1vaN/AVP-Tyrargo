#ifndef AVP_AUDIO_ENVIRONMENT_H
#define AVP_AUDIO_ENVIRONMENT_H
#include <AL/al.h>
#include <AL/alc.h>
int AudioEnvironmentInit(ALCdevice *device);
void AudioEnvironmentShutdown(void); /* detach sources before calling */
void AudioEnvironmentRoute(ALuint source, int enabled);
void AudioEnvironmentSet(unsigned index, float mix);
int AudioEnvironmentAvailable(void);
#endif
