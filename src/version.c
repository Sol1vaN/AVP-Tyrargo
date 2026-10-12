#include "version.h"
extern void NewOnScreenMessage(unsigned char *messagePtr);
const char *AvPVersionString = TYRARGO_DISPLAY_VERSION "\n"
    "Direct fork of " TYRARGO_BASE "\n"
    "Based on Rebellion Developments AvP Gold source\n";
void GiveVersionDetails(void)
{
    NewOnScreenMessage((unsigned char *)AvPVersionString);
}
/* Real elapsed time: unaffected by paused gameplay or the selected FPS cap. */
int TyrargoVersionOpacity(unsigned long long elapsedMilliseconds)
{
    double t;
    if (elapsedMilliseconds <= 3000) return 0;
    if (elapsedMilliseconds >= 4000) return 65536;
    t = (elapsedMilliseconds - 3000) / 1000.0;
    return (int)(t*t*(3.0-2.0*t)*65536.0 + 0.5);
}
