#ifndef AVP_VERTEX_LIGHTS_H
#define AVP_VERTEX_LIGHTS_H
#include "3dc.h"
#define VERTEX_LIGHT_CAPACITY 100
/* Distances use the exact legacy max + (other axes)/4 metric. */
typedef struct VERTEX_LIGHT_VECTORS {
    int x[VERTEX_LIGHT_CAPACITY], y[VERTEX_LIGHT_CAPACITY], z[VERTEX_LIGHT_CAPACITY];
    int distance[VERTEX_LIGHT_CAPACITY];
} VERTEX_LIGHT_VECTORS;
void PrepareVertexLights(LIGHTBLOCK *const *lights, int count);
void BuildVertexLightVectors(const VECTORCH *point, VERTEX_LIGHT_VECTORS *out);
#endif
