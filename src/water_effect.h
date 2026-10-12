#ifndef AVP_WATER_EFFECT_H
#define AVP_WATER_EFFECT_H
#ifdef __cplusplus
extern "C" {
#endif
#define WATER_GRID 257 /* Maximum nodes per axis; active size follows world extent. */
#define WATER_SIM_SPACING 72.0f
#define WATER_MESH_SPACING 96.0f
#define WATER_MESH_MAX_SEGMENTS 255
#define WATER_BODIES 8
extern int WaterEffectMode; /* 0: repaired vanilla, 1: modern */
void WaterReset(void);
void WaterAdvance(int fixedDelta);
void WaterRegister(int minX,int maxX,int minZ,int maxZ,int y);
void WaterDrop(int x,int z,int amplitude);
int WaterObjectRipple(const void *object,int x,int z,int y);
float WaterHeight(float x,float z,int y);
/* Modern geometry only. Vanilla continues to use EffectOfRipples. */
float WaterSurfaceHeight(float x,float z,int y);
int WaterMeshSegments(int extent);
void WaterGradient(float x,float z,int y,float *dx,float *dz);
void WaterGradientField(float x0,float x1,float z0,float z1,int y,int size,float *field);
double WaterTime(void);
unsigned WaterGeneration(void);
/* Render layer owns GL resources and can fall back independently. */
void WaterRenderRelease(void);
void WaterRenderSetLights(const float *positions,const float *colors);
void WaterRenderBeginFrame(void);
typedef struct WATER_VERTEX { float clip[4], uv[2], color[4], world[3]; } WATER_VERTEX;
int WaterRenderDraw(const WATER_VERTEX *v,int count,const unsigned short *indices,int indexCount,
 unsigned int texture,int y,const float *view,float projectionX,float projectionY,float nearClip);

/* Immutable grid; clip.x stores its terrain clearance. All per-frame deformation
 * and camera transforms run on the GPU, with the CPU path as capability fallback. */
typedef struct WATER_GRID_STATE {
 float ripples[100][4], inverseRadius[100][4];
 float uv[4], color[4];
 int count, phase;
 const float *sines;
 unsigned revision;
} WATER_GRID_STATE;
int WaterRippleSnapshot(float (*ripples)[4],float (*inverseRadius)[4],int capacity);
int WaterRenderGridSupported(void);
int WaterRenderDrawGrid(const WATER_VERTEX *v,int count,const unsigned short *indices,int indexCount,
 unsigned int texture,int y,const float *view,float px,float py,float nearClip,const WATER_GRID_STATE *state);

#ifdef __cplusplus
}
#endif
#endif
