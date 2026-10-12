/* Included in water_render.c after the shared GL setup helpers. */
#include <stddef.h>
#include "water_grid_shader.h"
static GLuint gridProgram,gridShader,geometryTexture;
static int gridUnavailable,geometryY,geometryValid;
static PFNGLGENBUFFERSPROC gridGenBuffers;
static PFNGLDELETEBUFFERSPROC gridDeleteBuffers;
static PFNGLBINDBUFFERPROC gridBindBuffer;
static PFNGLBUFFERDATAPROC gridBufferData;
static const WATER_GRID_STATE *gridRequest;
typedef struct GPU_WATER_GRID { const WATER_VERTEX *key;unsigned revision;GLuint vertices,indices;unsigned used; } GPU_WATER_GRID;
static GPU_WATER_GRID gpuGrids[16];
static unsigned gridSerial;
int WaterRenderGridSupported(void){
 GLint units=0,components=0,maxTexture=0;
 if(gridUnavailable)return 0;
 pglGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,&units);
 pglGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS,&components);
 pglGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxTexture);
 return units>=1&&components>=1024&&maxTexture>=4096;
}
static void releaseGrids(void){
 for(int i=0;i<16;i++){
  if(gpuGrids[i].vertices&&gridDeleteBuffers)gridDeleteBuffers(1,&gpuGrids[i].vertices);
  if(gpuGrids[i].indices&&gridDeleteBuffers)gridDeleteBuffers(1,&gpuGrids[i].indices);
 }
 memset(gpuGrids,0,sizeof gpuGrids);
 if(gridProgram)pglDeleteProgram(gridProgram);if(gridShader)pglDeleteShader(gridShader);
 if(geometryTexture)pglDeleteTextures(1,&geometryTexture);
 gridProgram=gridShader=geometryTexture=0;gridUnavailable=geometryValid=0;gridSerial=0;
}
static int initialiseGrid(void){
 if(gridProgram)return 1;
 if(!WaterRenderGridSupported())return 0;
 gridGenBuffers=(PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
 gridDeleteBuffers=(PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
 gridBindBuffer=(PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
 gridBufferData=(PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
 if(!gridGenBuffers||!gridDeleteBuffers||!gridBindBuffer||!gridBufferData)goto fail;
 gridShader=pglCreateShader(GL_VERTEX_SHADER);
 if(!compile(gridShader,gridVertexSource))goto fail;
 gridProgram=pglCreateProgram();pglAttachShader(gridProgram,gridShader);pglAttachShader(gridProgram,fragmentShader);pglLinkProgram(gridProgram);
 GLint ok=0;pglGetProgramiv(gridProgram,GL_LINK_STATUS,&ok);
 if(!ok){char log[2048];pglGetProgramInfoLog(gridProgram,sizeof log,0,log);SDL_Log("Water grid link: %s",log);goto fail;}
 pglGenTextures(1,&geometryTexture);
 if(!geometryTexture)goto fail;
 return 1;
 fail:releaseGrids();gridUnavailable=1;SDL_Log("Water GPU geometry unavailable; using CPU geometry.");return 0;
}
static void bindGrid(const WATER_VERTEX *v,int count,const unsigned short *indices,int indexCount){
 GPU_WATER_GRID *g=0;
 for(int i=0;i<16;i++)if(gpuGrids[i].key==v){g=&gpuGrids[i];break;}
 if(!g){g=&gpuGrids[0];for(int i=1;i<16;i++)if(gpuGrids[i].used<g->used)g=&gpuGrids[i];g->key=v;g->revision=0;}
 g->used=++gridSerial;
 if(!g->vertices)gridGenBuffers(1,&g->vertices);if(!g->indices)gridGenBuffers(1,&g->indices);
 gridBindBuffer(GL_ARRAY_BUFFER,g->vertices);gridBindBuffer(GL_ELEMENT_ARRAY_BUFFER,g->indices);
 if(g->revision!=gridRequest->revision){
  gridBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)count*sizeof(*v),v,GL_STATIC_DRAW);
  gridBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)indexCount*sizeof(*indices),indices,GL_STATIC_DRAW);
  g->revision=gridRequest->revision;
 }
}
static void gridUniforms(int y){
 const WATER_GRID_STATE *s=gridRequest;
 pglActiveTexture(GL_TEXTURE4);texture(geometryTexture,GL_NEAREST);
 if(!geometryValid){
  static float table[4096*2*4];memset(table,0,sizeof table);
  for(int i=0;i<4096;i++)table[i*4]=s->sines[i];
  pglTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,4096,2,0,GL_RGBA,GL_FLOAT,table);
 }
 if(!geometryValid||geometryY!=y){
  float warp[513*4]={0};
  for(int i=0;i<513;i++){
   float wy=y+(float)(i-256),phase=wy*16*(6.283185307f/4096.0f);
   warp[i*4]=128*sinf(phase);warp[i*4+1]=128*sinf(phase+200*(6.283185307f/4096.0f));
  }
  pglTexSubImage2D(GL_TEXTURE_2D,0,0,1,513,1,GL_RGBA,GL_FLOAT,warp);geometryY=y;geometryValid=1;
 }
 pglUniform1i(pglGetUniformLocation(program,"uGeometry"),4);
 pglUniform1i(pglGetUniformLocation(program,"uRippleCount"),s->count);
 pglUniform1f(pglGetUniformLocation(program,"uPhase"),(float)s->phase);
 if(s->count){uniform4fv(pglGetUniformLocation(program,"uRipple"),s->count,&s->ripples[0][0]);uniform4fv(pglGetUniformLocation(program,"uInverse"),s->count,&s->inverseRadius[0][0]);}
 uniform4fv(pglGetUniformLocation(program,"uWaterUV"),1,s->uv);uniform4fv(pglGetUniformLocation(program,"uVision"),1,s->color);
}
