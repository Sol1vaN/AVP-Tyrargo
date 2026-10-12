#ifndef TYRARGO_GEOMETRY_ROUNDING_H
#define TYRARGO_GEOMETRY_ROUNDING_H
#include <math.h>
#include <limits.h>
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define GEOMETRY_ROUND_SSE2 1
#endif
/* Keep the compatibility coordinates identical to (int)lrint, including the
 * active rounding mode. Precise double coordinates remain untouched. */
static inline void GeometryRoundCompatibility(VECTORCH *out,const double p[3])
{
#ifdef GEOMETRY_ROUND_SSE2
 if(p[0]>=INT_MIN && p[0]<=INT_MAX && p[1]>=INT_MIN && p[1]<=INT_MAX && p[2]>=INT_MIN && p[2]<=INT_MAX){
  __m128i xy=_mm_cvtpd_epi32(_mm_loadu_pd(p));
  out->vx=_mm_cvtsi128_si32(xy);out->vy=_mm_cvtsi128_si32(_mm_srli_si128(xy,4));
  out->vz=_mm_cvtsd_si32(_mm_load_sd(p+2));return;
 }
#endif
 out->vx=(int)lrint(p[0]);out->vy=(int)lrint(p[1]);out->vz=(int)lrint(p[2]);
}
#endif
