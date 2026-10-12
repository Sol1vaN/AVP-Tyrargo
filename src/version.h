#ifndef __VERSION_H__
#define __VERSION_H__
#include "tyrargo_version.h"
extern int TyrargoVersionOpacity(unsigned long long elapsedMilliseconds);

extern const char *AvPVersionString;

/* KJL 15:56:24 29/03/98 - this function supplies a text
description of the current build, and the build date. */
extern void GiveVersionDetails(void);

#endif
