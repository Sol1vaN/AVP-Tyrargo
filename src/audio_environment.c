/* EFX implementation of the 26 authored AvP/EAX environments.
 * Direct sound keeps the original attenuation/mix. Only opted-in game sources
 * feed the room effect; music, CD and FMV sources never enter this send.
 */
#include "audio_environment.h"
#include <AL/efx.h>
#include <AL/efx-presets.h>
#include <math.h>
#include <string.h>

static const EFXEAXREVERBPROPERTIES presets[] = {
    EFX_REVERB_PRESET_GENERIC,
    EFX_REVERB_PRESET_PADDEDCELL,
    EFX_REVERB_PRESET_ROOM,
    EFX_REVERB_PRESET_BATHROOM,
    EFX_REVERB_PRESET_LIVINGROOM,
    EFX_REVERB_PRESET_STONEROOM,
    EFX_REVERB_PRESET_AUDITORIUM,
    EFX_REVERB_PRESET_CONCERTHALL,
    EFX_REVERB_PRESET_CAVE,
    EFX_REVERB_PRESET_ARENA,
    EFX_REVERB_PRESET_HANGAR,
    EFX_REVERB_PRESET_CARPETEDHALLWAY,
    EFX_REVERB_PRESET_HALLWAY,
    EFX_REVERB_PRESET_STONECORRIDOR,
    EFX_REVERB_PRESET_ALLEY,
    EFX_REVERB_PRESET_FOREST,
    EFX_REVERB_PRESET_CITY,
    EFX_REVERB_PRESET_MOUNTAINS,
    EFX_REVERB_PRESET_QUARRY,
    EFX_REVERB_PRESET_PLAIN,
    EFX_REVERB_PRESET_PARKINGLOT,
    EFX_REVERB_PRESET_SEWERPIPE,
    EFX_REVERB_PRESET_UNDERWATER,
    EFX_REVERB_PRESET_DRUGGED,
    EFX_REVERB_PRESET_DIZZY,
    EFX_REVERB_PRESET_PSYCHOTIC,
};
#define ENV_COUNT (sizeof(presets)/sizeof(presets[0]))
static ALuint effects[ENV_COUNT], slot;
static unsigned currentEnvironment = ~0u;
static float currentMix = -2.0f;
static int available;
static LPALGENEFFECTS genEffects;
static LPALDELETEEFFECTS deleteEffects;
static LPALEFFECTI effecti;
static LPALEFFECTF effectf;
static LPALEFFECTFV effectfv;
static LPALGENAUXILIARYEFFECTSLOTS genSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS deleteSlots;
static LPALAUXILIARYEFFECTSLOTI sloti;
static LPALAUXILIARYEFFECTSLOTF slotf;

static int CreateEnvironment(ALuint effect, const EFXEAXREVERBPROPERTIES *p, int eax)
{
    alGetError();
    effecti(effect, AL_EFFECT_TYPE, eax ? AL_EFFECT_EAXREVERB : AL_EFFECT_REVERB);
    if (alGetError()!=AL_NO_ERROR) return 0;
    if (eax) {
        effectf(effect, AL_EAXREVERB_DENSITY, p->flDensity);
        effectf(effect, AL_EAXREVERB_DIFFUSION, p->flDiffusion);
        effectf(effect, AL_EAXREVERB_GAIN, p->flGain);
        effectf(effect, AL_EAXREVERB_GAINHF, p->flGainHF);
        effectf(effect, AL_EAXREVERB_GAINLF, p->flGainLF);
        effectf(effect, AL_EAXREVERB_DECAY_TIME, p->flDecayTime);
        effectf(effect, AL_EAXREVERB_DECAY_HFRATIO, p->flDecayHFRatio);
        effectf(effect, AL_EAXREVERB_DECAY_LFRATIO, p->flDecayLFRatio);
        effectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, p->flReflectionsGain);
        effectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, p->flReflectionsDelay);
        effectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, p->flLateReverbGain);
        effectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, p->flLateReverbDelay);
        effectf(effect, AL_EAXREVERB_ECHO_TIME, p->flEchoTime);
        effectf(effect, AL_EAXREVERB_ECHO_DEPTH, p->flEchoDepth);
        effectf(effect, AL_EAXREVERB_MODULATION_TIME, p->flModulationTime);
        effectf(effect, AL_EAXREVERB_MODULATION_DEPTH, p->flModulationDepth);
        effectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, p->flAirAbsorptionGainHF);
        effectf(effect, AL_EAXREVERB_HFREFERENCE, p->flHFReference);
        effectf(effect, AL_EAXREVERB_LFREFERENCE, p->flLFReference);
        effectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, p->flRoomRolloffFactor);
        effectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, p->flReflectionsPan);
        effectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, p->flLateReverbPan);
        effecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, p->iDecayHFLimit);
    } else {
        effectf(effect, AL_REVERB_DENSITY, p->flDensity);
        effectf(effect, AL_REVERB_DIFFUSION, p->flDiffusion);
        effectf(effect, AL_REVERB_GAIN, p->flGain);
        effectf(effect, AL_REVERB_GAINHF, p->flGainHF);
        effectf(effect, AL_REVERB_DECAY_TIME, p->flDecayTime);
        effectf(effect, AL_REVERB_DECAY_HFRATIO, p->flDecayHFRatio);
        effectf(effect, AL_REVERB_REFLECTIONS_GAIN, p->flReflectionsGain);
        effectf(effect, AL_REVERB_REFLECTIONS_DELAY, p->flReflectionsDelay);
        effectf(effect, AL_REVERB_LATE_REVERB_GAIN, p->flLateReverbGain);
        effectf(effect, AL_REVERB_LATE_REVERB_DELAY, p->flLateReverbDelay);
        effectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, p->flAirAbsorptionGainHF);
        effectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, p->flRoomRolloffFactor);
        effecti(effect, AL_REVERB_DECAY_HFLIMIT, p->iDecayHFLimit);
    }
    return alGetError()==AL_NO_ERROR;
}

int AudioEnvironmentAvailable(void) { return available; }

void AudioEnvironmentShutdown(void)
{
    unsigned i;
    available=0;
    if (slot) {
        sloti(slot,AL_EFFECTSLOT_EFFECT,AL_EFFECT_NULL);
        deleteSlots(1,&slot); slot=0;
    }
    for (i=0;i<ENV_COUNT;i++) if (effects[i]) {
        deleteEffects(1,&effects[i]); effects[i]=0;
    }
    currentEnvironment=~0u; currentMix=-2.0f;
}

int AudioEnvironmentInit(ALCdevice *device)
{
    unsigned i;
    ALCint sends=0;
    if (available) return 1;
    if (!device || !alcIsExtensionPresent(device,"ALC_EXT_EFX")) return 0;
    alcGetIntegerv(device,ALC_MAX_AUXILIARY_SENDS,1,&sends);
    if (sends<1) return 0;
#define LOAD(name,type,api) name=(type)alGetProcAddress(api); if (!name) return 0
    LOAD(genEffects,LPALGENEFFECTS,"alGenEffects");
    LOAD(deleteEffects,LPALDELETEEFFECTS,"alDeleteEffects");
    LOAD(effecti,LPALEFFECTI,"alEffecti");
    LOAD(effectf,LPALEFFECTF,"alEffectf");
    LOAD(effectfv,LPALEFFECTFV,"alEffectfv");
    LOAD(genSlots,LPALGENAUXILIARYEFFECTSLOTS,"alGenAuxiliaryEffectSlots");
    LOAD(deleteSlots,LPALDELETEAUXILIARYEFFECTSLOTS,"alDeleteAuxiliaryEffectSlots");
    LOAD(sloti,LPALAUXILIARYEFFECTSLOTI,"alAuxiliaryEffectSloti");
    LOAD(slotf,LPALAUXILIARYEFFECTSLOTF,"alAuxiliaryEffectSlotf");
#undef LOAD
    alGetError();
    genSlots(1,&slot);
    if (alGetError()!=AL_NO_ERROR || !slot) goto fail;
    for (i=0;i<ENV_COUNT;i++) {
        genEffects(1,&effects[i]);
        if (alGetError()!=AL_NO_ERROR || !effects[i]) goto fail;
        if (!CreateEnvironment(effects[i],&presets[i],1) &&
            !CreateEnvironment(effects[i],&presets[i],0)) goto fail;
    }
    /* World coordinates are millimetres. Avoid interpreting room distances as metres. */
    alListenerf(AL_METERS_PER_UNIT,0.001f);
    slotf(slot,AL_EFFECTSLOT_GAIN,0.0f);
    if (alGetError()!=AL_NO_ERROR) goto fail;
    available=1;
    return 1;
fail:
    AudioEnvironmentShutdown();
    alGetError(); /* EFX failure must not poison ordinary sound playback. */
    return 0;
}

void AudioEnvironmentRoute(ALuint source, int enabled)
{
    if (!available || !source) return;
    /* Also clear the send on reuse: UI and reverb_off sounds must stay dry. */
    alSource3i(source,AL_AUXILIARY_SEND_FILTER,enabled ? slot : AL_EFFECTSLOT_NULL,0,AL_FILTER_NULL);
}

void AudioEnvironmentSet(unsigned index, float mix)
{
    float gain;
    if (!available) return;
    if (index>=ENV_COUNT) index=0; /* malformed/unknown map environment: generic */
    /* -1 is the authored automatic-distance sentinel. EFX supplies automatic
     * send response; explicit 0..1 is the map's wet gain, not the master volume. */
    gain=isfinite(mix) && mix>=0.0f && mix<=1.0f ? mix : 1.0f;
    if (currentEnvironment!=index) {
        sloti(slot,AL_EFFECTSLOT_EFFECT,effects[index]);
        currentEnvironment=index;
    }
    if (currentMix!=gain) {
        slotf(slot,AL_EFFECTSLOT_GAIN,gain);
        currentMix=gain;
    }
}
