#include "vertex_lights.h"
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define LIGHT_SSE2 1
#endif
static int lx[VERTEX_LIGHT_CAPACITY],ly[VERTEX_LIGHT_CAPACITY],lz[VERTEX_LIGHT_CAPACITY],lightCount;
void PrepareVertexLights(LIGHTBLOCK *const *lights,int count)
{
    lightCount=count;
    for(int i=0;i<count;i++){lx[i]=lights[i]->LocalLP.vx;ly[i]=lights[i]->LocalLP.vy;lz[i]=lights[i]->LocalLP.vz;}
}
#ifdef LIGHT_SSE2
static __m128i abs32(__m128i v){__m128i sign=_mm_srai_epi32(v,31);return _mm_sub_epi32(_mm_xor_si128(v,sign),sign);}
static __m128i max32(__m128i a,__m128i b){__m128i m=_mm_cmpgt_epi32(a,b);return _mm_or_si128(_mm_and_si128(m,a),_mm_andnot_si128(m,b));}
#endif
void BuildVertexLightVectors(const VECTORCH *point,VERTEX_LIGHT_VECTORS *out)
{
    int i=0;
#ifdef LIGHT_SSE2
    __m128i px=_mm_set1_epi32(point->vx),py=_mm_set1_epi32(point->vy),pz=_mm_set1_epi32(point->vz);
    for(;i+4<=lightCount;i+=4){
        __m128i x=_mm_sub_epi32(_mm_loadu_si128((const __m128i*)(lx+i)),px);
        __m128i y=_mm_sub_epi32(_mm_loadu_si128((const __m128i*)(ly+i)),py);
        __m128i z=_mm_sub_epi32(_mm_loadu_si128((const __m128i*)(lz+i)),pz);
        _mm_storeu_si128((__m128i*)(out->x+i),x);_mm_storeu_si128((__m128i*)(out->y+i),y);_mm_storeu_si128((__m128i*)(out->z+i),z);
        x=abs32(x);y=abs32(y);z=abs32(z);
        __m128i maximum=max32(max32(x,y),z);
        __m128i remainder=_mm_sub_epi32(_mm_add_epi32(_mm_add_epi32(x,y),z),maximum);
        _mm_storeu_si128((__m128i*)(out->distance+i),_mm_add_epi32(maximum,_mm_srai_epi32(remainder,2)));
    }
#endif
    for(;i<lightCount;i++){
        int x=out->x[i]=lx[i]-point->vx,y=out->y[i]=ly[i]-point->vy,z=out->z[i]=lz[i]-point->vz;
        if(x<0)x=-x;if(y<0)y=-y;if(z<0)z=-z;
        int maximum=x>y?x:y;if(z>maximum)maximum=z;
        out->distance[i]=maximum+((x==maximum?y+z:y==maximum?x+z:x+y)>>2);
    }
}
