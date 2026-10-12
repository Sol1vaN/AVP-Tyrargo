#ifndef TYRARGO_COPLANAR_TRIM_HPP
#define TYRARGO_COPLANAR_TRIM_HPP
#include <cmath>
/* Conservative classification: exact coplanar rectangles, same facing,
   different objects/materials, fully contained narrow strip sharing a span.
   This is not a general rule that smaller polygons are always foreground. */
struct CoplanarTrimRect {
 int axis, facing, object, material;
 double plane, lo[2], hi[2];
};
static inline bool IsCoplanarTrim(const CoplanarTrimRect&a,const CoplanarTrimRect&b) {
 const double e=0.01;
 if(a.axis!=b.axis || a.facing!=b.facing || a.object==b.object || a.material==b.material || std::fabs(a.plane-b.plane)>e)return false;
 for(int k=0;k<2;k++)if(a.lo[k]<b.lo[k]-e || a.hi[k]>b.hi[k]+e)return false;
 for(int k=0;k<2;k++) {
  int other=1-k;double width=a.hi[k]-a.lo[k],base=b.hi[k]-b.lo[k];
  if(width>e && width<=base*0.25 && std::fabs(a.lo[other]-b.lo[other])<=e && std::fabs(a.hi[other]-b.hi[other])<=e)return true;
 }
 return false;
}
#endif
