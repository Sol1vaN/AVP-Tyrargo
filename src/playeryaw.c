#include "3dc.h"
#include "playeryaw.h"
#include <math.h>
#include <string.h>

/* Keep the physical basis between mouse samples. MATRIXCH is an output for
 * legacy consumers, never feedback for the next yaw while we still own it.
 * No smoothing, minimum step, time scaling or direction-dependent rounding. */
static MATRIXCH *YawOwner;
static MATRIXCH YawPublished;
static double YawBasis[3][3];

void PlayerYawReset(void)
{
    YawOwner = NULL;
}

static int PlayerYawOwns(const MATRIXCH *orientation)
{
    return YawOwner == orientation &&
        memcmp(orientation, &YawPublished, sizeof(*orientation)) == 0;
}

static void PlayerYawImport(const MATRIXCH *m)
{
    const double input[3][3] = {
        {m->mat11, m->mat12, m->mat13},
        {m->mat21, m->mat22, m->mat23},
        {m->mat31, m->mat32, m->mat33}
    };
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j) YawBasis[i][j] = input[i][j] / 65536.0;
}

/* Preserve the body's up axis (including Alien wall/ceiling orientations).
 * Gram-Schmidt on doubles prevents drift without quantising accumulated yaw. */
static int PlayerYawUnitBasis(void)
{
    double length = 0.0, dot = 0.0;
    int j;
    for (j = 0; j < 3; ++j) length += YawBasis[1][j] * YawBasis[1][j];
    if (!(length > 1e-20)) return 0;
    length = sqrt(length);
    for (j = 0; j < 3; ++j) YawBasis[1][j] /= length;
    for (j = 0; j < 3; ++j) dot += YawBasis[2][j] * YawBasis[1][j];
    length = 0.0;
    for (j = 0; j < 3; ++j) {
        YawBasis[2][j] -= dot * YawBasis[1][j];
        length += YawBasis[2][j] * YawBasis[2][j];
    }
    if (!(length > 1e-20)) return 0;
    length = sqrt(length);
    for (j = 0; j < 3; ++j) YawBasis[2][j] /= length;
    for (j = 0; j < 3; ++j) {
        int a = (j + 1) % 3, b = (j + 2) % 3;
        YawBasis[0][j] = YawBasis[1][a] * YawBasis[2][b] -
                         YawBasis[1][b] * YawBasis[2][a];
    }
    return 1;
}

static int PlayerYawFixed(double value)
{
    value *= 65536.0;
    return value >= 0.0 ? (int)(value + 0.5) : (int)(value - 0.5);
}

void PlayerYawApply(MATRIXCH *orientation, int64_t angleFixed)
{
    double radians, c, s;
    int j;
    if (!angleFixed) return;
    /* Physics, teleport/load or another player may supply a new orientation.
     * Adopt it rather than restoring a stale cached pose. */
    if (!PlayerYawOwns(orientation)) {
        PlayerYawImport(orientation);
        if (!PlayerYawUnitBasis()) { PlayerYawReset(); return; }
    }
    radians = (double)(angleFixed % (4096LL * 65536LL)) *
              (6.28318530717958647692 / (4096.0 * 65536.0));
    c = cos(radians);
    s = sin(radians);
    for (j = 0; j < 3; ++j) {
        double x = YawBasis[0][j], z = YawBasis[2][j];
        YawBasis[0][j] = c * x - s * z;
        YawBasis[2][j] = s * x + c * z;
    }
    if (!PlayerYawUnitBasis()) { PlayerYawReset(); return; }
    YawPublished = (MATRIXCH){
        PlayerYawFixed(YawBasis[0][0]), PlayerYawFixed(YawBasis[0][1]), PlayerYawFixed(YawBasis[0][2]),
        PlayerYawFixed(YawBasis[1][0]), PlayerYawFixed(YawBasis[1][1]), PlayerYawFixed(YawBasis[1][2]),
        PlayerYawFixed(YawBasis[2][0]), PlayerYawFixed(YawBasis[2][1]), PlayerYawFixed(YawBasis[2][2])
    };
    *orientation = YawPublished;
    YawOwner = orientation;
}

void PlayerYawNormalise(MATRIXCH *orientation)
{
    /* Our double basis is already orthonormal. Re-running the legacy integer
     * normaliser on its rounded copy would reintroduce quantisation feedback.
     * External orientations continue through the original normaliser. */
    if (PlayerYawOwns(orientation)) return;
    PlayerYawReset();
    MNormalise(orientation);
}

double PlayerYawRadians(const MATRIXCH *orientation)
{
 if(PlayerYawOwns(orientation))return atan2(YawBasis[2][0],YawBasis[2][2]);
 return atan2((double)orientation->mat31,(double)orientation->mat33);
}
