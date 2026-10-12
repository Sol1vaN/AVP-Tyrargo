/* Ordered flame/smoke batching. Dual-source blending expresses both original
 * equations in one draw: additive fire, inverse-colour smoke and alpha particles.
 * It does not sort particles, lower counts, or alter their simulation/lights. */
#include "fire_batch.h"
#include "oglfunc.h"
#include <SDL3/SDL.h>
#include <string.h>
#if !defined(USE_OPENGL_ES)
#include <SDL3/SDL_opengl_glext.h>
#define FIRE_BATCH_VERTICES 12288
static FireBatchVertex batch[FIRE_BATCH_VERTICES];
static int count, unavailable;
static GLuint program, vshader, fshader, textureID;
static GLint materialLocation;
static PFNGLBINDFRAGDATALOCATIONINDEXEDPROC bindOutput;
static PFNGLBINDBUFFERPROC bindBuffer;
static void (APIENTRY *drawArrays)(GLenum, GLint, GLsizei);
static const char *vs = "#version 130\n"
 "out vec3 uvMode; out vec4 colour;\n"
 "void main(){gl_Position=gl_Vertex;uvMode=gl_MultiTexCoord0.xyz;colour=gl_Color;}\n";
static const char *fs = "#version 130\n"
 "uniform sampler2D material;in vec3 uvMode;in vec4 colour;\n"
 "out vec4 sourceColour;out vec4 destinationAttenuation;\n"
 "void main(){vec4 c=texture2D(material,uvMode.xy)*colour;\n"
 " if(uvMode.z<0.5){sourceColour=c*c.a;destinationAttenuation=vec4(0.0);}\n"
 " else if(uvMode.z<1.5){sourceColour=vec4(0.0);destinationAttenuation=c;}\n"
 " else{sourceColour=c*c.a;destinationAttenuation=vec4(c.a);}}\n";
static int compile(GLuint shader,const char *source){
 GLint ok=0;pglShaderSource(shader,1,&source,0);pglCompileShader(shader);
 pglGetShaderiv(shader,GL_COMPILE_STATUS,&ok);return ok;
}
int FireBatchAvailable(void){
 if(unavailable)return 0;if(program)return 1;
 if(!pglCreateShader)return 0;
 if(!SDL_GL_ExtensionSupported("GL_ARB_blend_func_extended")){unavailable=1;return 0;}
 bindOutput=(PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)SDL_GL_GetProcAddress("glBindFragDataLocationIndexed");
 bindBuffer=(PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
 drawArrays=(void (APIENTRY *)(GLenum, GLint, GLsizei))SDL_GL_GetProcAddress("glDrawArrays");
 if(!bindOutput||!bindBuffer||!drawArrays)goto fail;
 vshader=pglCreateShader(GL_VERTEX_SHADER);fshader=pglCreateShader(GL_FRAGMENT_SHADER);
 if(!compile(vshader,vs)||!compile(fshader,fs))goto fail;
 program=pglCreateProgram();pglAttachShader(program,vshader);pglAttachShader(program,fshader);
 bindOutput(program,0,0,"sourceColour");bindOutput(program,0,1,"destinationAttenuation");
 pglLinkProgram(program);GLint linked=0;pglGetProgramiv(program,GL_LINK_STATUS,&linked);
 if(!linked)goto fail;
 materialLocation=pglGetUniformLocation(program,"material");
 return 1;
 fail:FireBatchRelease();unavailable=1;return 0;
}
int FireBatchPending(void){return count!=0;}
void FireBatchFlush(void){
 if(!count)return;
 GLint savedProgram=0,active=0,client=0,arrayBuffer=0;
 pglGetIntegerv(GL_CURRENT_PROGRAM,&savedProgram);
 pglGetIntegerv(GL_ACTIVE_TEXTURE,&active);pglGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE,&client);
 pglGetIntegerv(GL_ARRAY_BUFFER_BINDING,&arrayBuffer);
 pglPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_TEXTURE_BIT);
 pglPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
 pglActiveTexture(GL_TEXTURE0);pglClientActiveTexture(GL_TEXTURE0);
 pglBindTexture(GL_TEXTURE_2D,textureID);
 pglUseProgram(program);pglUniform1i(materialLocation,0);
 pglDisable(GL_ALPHA_TEST);pglEnable(GL_BLEND);
 pglBlendFunc(GL_ONE,GL_ONE_MINUS_SRC1_COLOR);
 bindBuffer(GL_ARRAY_BUFFER,0);
 pglEnableClientState(GL_VERTEX_ARRAY);pglEnableClientState(GL_COLOR_ARRAY);pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
 pglVertexPointer(4,GL_FLOAT,sizeof(batch[0]),batch[0].position);
 pglTexCoordPointer(3,GL_FLOAT,sizeof(batch[0]),batch[0].uvMode);
 pglColorPointer(4,GL_UNSIGNED_BYTE,sizeof(batch[0]),batch[0].colour);
 drawArrays(GL_TRIANGLES,0,count);
 pglPopClientAttrib();bindBuffer(GL_ARRAY_BUFFER,arrayBuffer);
 pglPopAttrib();pglUseProgram(savedProgram);
 pglActiveTexture(active);pglClientActiveTexture(client);
 count=0;
}
void FireBatchAppend(const FireBatchVertex *v,int n,unsigned int texture){
 if(n<3)return;
 if(texture!=textureID || count+(n-2)*3>FIRE_BATCH_VERTICES)FireBatchFlush();
 textureID=texture;
 /* Match the engine's clipped-polygon triangulation and winding. */
 #define TRI(a,b,c) do { batch[count++]=v[a];batch[count++]=v[b];batch[count++]=v[c]; } while(0)
 if(n==3){TRI(0,2,1);}
 else if(n==5){TRI(0,1,4);TRI(1,3,4);TRI(1,2,3);}
 else {for(int i=n-2;i>=1;--i){TRI(0,i,i+1);}}
 #undef TRI
}
void FireBatchRelease(void){
 count=0;
 if(program&&pglDeleteProgram)pglDeleteProgram(program);
 if(vshader&&pglDeleteShader)pglDeleteShader(vshader);
 if(fshader&&pglDeleteShader)pglDeleteShader(fshader);
 program=vshader=fshader=0;unavailable=0;
}
#else
int FireBatchAvailable(void){return 0;}
int FireBatchPending(void){return 0;}
void FireBatchAppend(const FireBatchVertex *v,int n,unsigned int t){(void)v;(void)n;(void)t;}
void FireBatchFlush(void){}
void FireBatchRelease(void){}
#endif
