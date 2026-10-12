#ifndef AVP_MOTION_PARAMS_H
#define AVP_MOTION_PARAMS_H
#ifdef __cplusplus
extern "C" {
#endif
enum MOTION_PARAM_ID {
#define MP(id,key,en,fr,es,de,it,sv,unit,lo,hi,step,def,group) MP_##id,
#include "motion_params.def"
#undef MP
 MP_COUNT
};
typedef struct {const char *key,*label[6],*unit;double min,max,step,initial;int group;} MOTION_PARAM;
extern const MOTION_PARAM MotionParams[MP_COUNT];
extern double MotionValues[MP_COUNT];
#define M(id) (MotionValues[MP_##id])
void MotionParamsInitialize(const char *directory);
void MotionParamAdjust(int id,int direction);
void MotionParamDefault(int id);
int MotionParamsExport(void);
int MotionParamMenuIndex(int position);
const char *MotionParamsStatus(int language);
const char *MotionParamHelp(int id,int language);
const char *MotionParamsHelp(int language,int exporting);
#ifdef __cplusplus
}
#endif
#endif
