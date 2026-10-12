#include "tyrargo_instrumentation.h"
#ifndef AVP_DEV_VIEWMODEL_H
#define AVP_DEV_VIEWMODEL_H
#define DEV_VIEWMODEL_COUNT 17
#define DEV_FLAME_COUNT 6
#define DEV_VIEWMODEL_Z_MIN (-10000)
#define DEV_VIEWMODEL_Z_MAX 10000
extern int DevViewmodelEnabled;
void DevViewmodelInitialize(const char *executableDirectory);
int DevViewmodelZ(int weaponID, int sourceZ);
int DevViewmodelValue(int row);
const char *DevViewmodelLabel(int row);
const char *DevViewmodelStatus(void);
int DevViewmodelEditing(int row);
void DevViewmodelToggle(int row);
void DevViewmodelAdjust(int row, int delta);
int DevViewmodelHandleKeys(int species, int weaponID, int forward, int backward, int toggleStyle, int frameTime);
int DevViewmodelExport(void);
int DevViewmodelMinimum(int row);
int DevViewmodelMaximum(int row);
void DevViewmodelFormat(int row,char *text,int size);
const char *DevViewmodelHelp(void);
int DevFlameEditing(void);
void DevFlameKeys(int forward,int back,int left,int right,int down,int up,int frameTime);
double DevFlameValue(int axis);
void DevFlameRotate(double v[3]);
int DevViewmodelMenuIndex(int position);
#endif
