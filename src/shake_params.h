#ifndef TYRARGO_SHAKE_PARAMS_H
#define TYRARGO_SHAKE_PARAMS_H
#ifdef __cplusplus
extern "C" {
#endif
enum SHAKE_PARAM_ID {
#define SP(id,key,en,fr,es,de,it,sv,unit,lo,hi,step,def,kind) HP_##id,
#include "shake_params.def"
#undef SP
 HP_COUNT
};
typedef struct {const char *key,*label[6],*unit;double min,max,step,initial;const char *kind;} SHAKE_PARAM;
/* Calibration retained for future use, disabled in this release. */
#define SHAKE_CALIBRATION_AVAILABLE 0
extern int ShakeEnabled;
extern const SHAKE_PARAM ShakeParams[HP_COUNT];
extern double ShakeValues[HP_COUNT];
/* Normal launches always use source defaults, even if a CSV exists. */
#define H(id) (ShakeEnabled ? ShakeValues[HP_##id] : ShakeParams[HP_##id].initial)
void ShakeParamsInitialize(const char *directory);
void ShakeParamAdjust(int id,int direction);
void ShakeParamDefault(int id);
int ShakeParamsExport(void);
int ShakeParamMenuIndex(int position);
const char *ShakeParamsStatus(int language);
const char *ShakeParamHelp(int id,int language);
const char *ShakeParamsHelp(int language,int exporting);
#ifdef __cplusplus
}
#endif
#endif
