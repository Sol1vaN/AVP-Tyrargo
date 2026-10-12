/* OpenLara-inspired water optics; see OpenLara_LICENSE.txt. Scene-space
 * reflection has no information outside the current view: fall back to AvP's
 * authored material, never a fabricated sky. CPU mesh still deforms in 3D. */
#include "water_effect.h"
#include "oglfunc.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#if !defined(USE_OPENGL_ES)
#include <SDL3/SDL_opengl_glext.h>
static GLuint program,vertexShader,fragmentShader,sceneTexture,depthTexture,waveTexture,captureFBO;
static int width,height,depthBits,stencilBits,unavailable,captured,viewportX,viewportY;
static float lightPosition[16],lightColor[16];
static int reportedInheritedError,waveAllocated;
static PFNGLGENFRAMEBUFFERSPROC genFBO;
static PFNGLDELETEFRAMEBUFFERSPROC deleteFBO;
static PFNGLBINDFRAMEBUFFERPROC bindFBO;
static PFNGLFRAMEBUFFERTEXTURE2DPROC attachFBO;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC statusFBO;
static PFNGLBLITFRAMEBUFFERPROC blitFBO;
static PFNGLUNIFORM4FVPROC uniform4fv;
static const char *vertexSource="#version 120\nvarying vec2 vUV;\nvarying vec3 vWorld;\nvarying vec4 vColor;\nvoid main(){gl_Position=gl_Vertex;vUV=gl_MultiTexCoord0.xy;vWorld=gl_MultiTexCoord1.xyz;vColor=gl_Color;}\n";
static const char *fragmentSource="#version 120\nuniform sampler2D uMaterial,uScene,uDepth,uWave;\nuniform vec2 uSize,uProj,uOrigin,uExtent,uViewport;\nuniform vec4 uLightPosition[4],uLightColor[4];\nuniform vec4 uView[3];\nuniform float uNear;\nvarying vec2 vUV;\nvarying vec3 vWorld;\nvarying vec4 vColor;\nvec3 transform(vec3 w){return vec3(dot(uView[0],vec4(w,1.0)),dot(uView[1],vec4(w,1.0)),dot(uView[2],vec4(w,1.0)));}\nvec3 vector(vec3 w){return vec3(dot(uView[0].xyz,w),dot(uView[1].xyz,w),dot(uView[2].xyz,w));}\nvec2 project(vec3 p){return vec2(p.x*uProj.x,-p.y*uProj.y)/p.z*.5+.5;}\nfloat depth(vec2 uv){return uNear/max(0.000001,1.0-texture2D(uDepth,uv).r);}\n// Four bilinear taps reconstruct a bicubic B-spline normal field. Its\n// continuous curvature avoids a visible lattice in the caustic derivatives.\nvec4 weights(float f){return vec4(pow(1.0-f,3.0),3.0*f*f*f-6.0*f*f+4.0,-3.0*f*f*f+3.0*f*f+3.0*f+1.0,f*f*f)/6.0;}\nvec2 waveSlope(vec2 uv){\n vec2 p=uv*192.0-.5,base=floor(p),f=fract(p);\n vec4 wx=weights(f.x),wy=weights(f.y);vec2 gx=vec2(wx.x+wx.y,wx.z+wx.w),gy=vec2(wy.x+wy.y,wy.z+wy.w);\n vec2 tx=(base.x+vec2(-.5,1.5)+vec2(wx.y/gx.x,wx.w/gx.y))/192.0;\n vec2 ty=(base.y+vec2(-.5,1.5)+vec2(wy.y/gy.x,wy.w/gy.y))/192.0;\n return (texture2D(uWave,vec2(tx.x,ty.x)).rg*gx.x+texture2D(uWave,vec2(tx.y,ty.x)).rg*gx.y)*gy.x+\n        (texture2D(uWave,vec2(tx.x,ty.y)).rg*gx.x+texture2D(uWave,vec2(tx.y,ty.y)).rg*gx.y)*gy.y;\n}\nvoid main(){\n vec2 uv=(gl_FragCoord.xy-uViewport)/uSize;\n vec3 P=transform(vWorld);\n vec2 slope=waveSlope((vWorld.xz-uOrigin)/uExtent);\n vec3 N=normalize(cross(vector(vec3(1.0,slope.x,0.0)),vector(vec3(0.0,slope.y,1.0))));\n vec3 V=normalize(-P);if(dot(N,V)<0.0)N=-N;\n float facing=clamp(dot(N,V),0.0,1.0);\n vec4 material=texture2D(uMaterial,vUV)*vColor;\n float alpha=clamp(material.a,0.0,1.0);\n float bottom=depth(uv),thickness=clamp(bottom-P.z,0.0,5000.0);\n // Optical path has an artistic minimum for AvP's thin water film. This\n // changes only the sampled image, never the pool floor or collision depth.\n vec3 flatN=normalize(cross(vector(vec3(1.0,0.0,0.0)),vector(vec3(0.0,0.0,1.0))));\n if(dot(flatN,V)<0.0)flatN=-flatN;\n vec3 disturbance=N-flatN;\n float opticalPath=max(thickness,500.0);\n vec2 bend=vec2(disturbance.x,-disturbance.y)*clamp(opticalPath/max(P.z,1.0),.14,.65)*.19;\n bend=clamp(bend,vec2(-.032),vec2(.032));\n vec2 refractUV=clamp(uv+bend,vec2(.001),vec2(.999));\n float rd=depth(refractUV);\n // Do not drag foreground walls, actors or the water's bank into the pool.\n float valid=smoothstep(-2.0,8.0,rd-P.z);\n refractUV=mix(uv,refractUV,valid);\n vec3 transmission=texture2D(uScene,refractUV).rgb;\n // Area concentration of refracted rays (caustics), not optical diffraction.\n vec2 oldDx=dFdx(uv),oldDy=dFdy(uv),newDx=dFdx(refractUV),newDy=dFdy(refractUV);\n float oldArea=abs(oldDx.x*oldDy.y-oldDx.y*oldDy.x);\n float newArea=abs(newDx.x*newDy.y-newDx.y*newDy.x);\n float caustic=clamp(oldArea/max(newArea,oldArea*.20),.45,3.2);\n transmission*=mix(1.0,caustic,.65*valid*(1.0-smoothstep(3500.0,5000.0,thickness)));\n vec3 result=mix(transmission,material.rgb,alpha);\n // Screen-space reflected ray: visible scene only, material fallback offscreen.\n vec3 R=normalize(reflect(-V,N));\n vec3 reflection=material.rgb;float confidence=0.0;\n float previous=0.0;\n for(int i=0;i<28;i++){\n  float fi=float(i)+1.0;float distance=70.0+fi*fi*32.0;\n  vec3 q=P+N*12.0+R*distance;\n  if(q.z<=uNear)break;\n  vec2 hitUV=project(q);if(hitUV.x<=0.002||hitUV.x>=.998||hitUV.y<=.002||hitUV.y>=.998)break;\n  float sceneZ=depth(hitUV),delta=q.z-sceneZ;\n  if(delta>0.0&&previous<=0.0&&sceneZ<250000.0&&abs(sceneZ-P.z)>40.0){\n   // Refine the crossing to avoid thick, detached reflections at silhouettes.\n   float lo=max(0.0,70.0+(fi-1.0)*(fi-1.0)*32.0),hi=distance;\n   for(int j=0;j<5;j++){float mid=(lo+hi)*.5;vec3 m=P+N*12.0+R*mid;vec2 t=project(m);if(m.z>depth(t))hi=mid;else lo=mid;}\n   hitUV=project(P+N*12.0+R*((lo+hi)*.5));\n   float edge=min(min(hitUV.x,1.0-hitUV.x),min(hitUV.y,1.0-hitUV.y));\n   confidence=smoothstep(0.0,.08,edge)*(1.0-smoothstep(18000.0,25000.0,distance));\n   reflection=texture2D(uScene,hitUV).rgb;break;\n  }\n  previous=delta;\n }\n float fresnel=.035+.965*pow(1.0-facing,5.0);\n // Keep AvP's material, vision colours and depth-dependent transparency.\n result=mix(result,reflection,confidence*fresnel*.92);\n // Specular response to the existing AvP lights; no invented sun or sky.\n vec3 specular=vec3(0.0);\n for(int i=0;i<4;i++){\n  if(uLightPosition[i].w<=0.0)continue;\n  vec3 lightVector=uLightPosition[i].xyz-vWorld;\n  float distance=length(lightVector),range=max(uLightPosition[i].w,1.0);\n  float attenuation=pow(clamp(1.0-distance/range,0.0,1.0),2.0);\n  vec3 L=normalize(vector(lightVector));vec3 halfVector=V+L;vec3 H=halfVector*inversesqrt(max(dot(halfVector,halfVector),1e-8));\n  float highlight=pow(max(dot(N,H),0.0),70.0)+.12*pow(max(dot(N,H),0.0),12.0);\n  specular+=uLightColor[i].rgb*attenuation*highlight*(.16+.84*pow(1.0-facing,2.0));\n }\n result+=specular;\n gl_FragColor=vec4(result,1.0);\n}\n";
static int compile(GLuint shader,const char*source){
 GLint ok=0;pglShaderSource(shader,1,&source,0);pglCompileShader(shader);pglGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[2048];pglGetShaderInfoLog(shader,sizeof(log),0,log);SDL_Log("Water shader: %s",log);}return ok;
}
static int initialise(void){
 if(unavailable||!pglCreateShader)return 0;if(program)return 1;
 genFBO=(PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
 deleteFBO=(PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
 bindFBO=(PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
 attachFBO=(PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
 statusFBO=(PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
 blitFBO=(PFNGLBLITFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBlitFramebuffer");
 uniform4fv=(PFNGLUNIFORM4FVPROC)SDL_GL_GetProcAddress("glUniform4fv");
 if(!genFBO||!deleteFBO||!bindFBO||!attachFBO||!statusFBO||!blitFBO||!uniform4fv)goto fail;
 vertexShader=pglCreateShader(GL_VERTEX_SHADER);fragmentShader=pglCreateShader(GL_FRAGMENT_SHADER);
 if(!compile(vertexShader,vertexSource)||!compile(fragmentShader,fragmentSource))goto fail;
 program=pglCreateProgram();pglAttachShader(program,vertexShader);pglAttachShader(program,fragmentShader);pglLinkProgram(program);
 GLint linked=0;pglGetProgramiv(program,GL_LINK_STATUS,&linked);if(!linked)goto fail;
 pglGenTextures(1,&sceneTexture);pglGenTextures(1,&depthTexture);pglGenTextures(1,&waveTexture);genFBO(1,&captureFBO);
 if(!sceneTexture||!depthTexture||!waveTexture||!captureFBO)goto fail;
 return 1;
 fail:WaterRenderRelease();unavailable=1;SDL_Log("Modern water unavailable; using repaired water.");return 0;
}
static void texture(GLuint id,int filter){pglBindTexture(GL_TEXTURE_2D,id);pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);pglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);}
static int capture(void){
 if(captured)return captured>0;
 GLint viewport[4],savedDraw=0,savedRead=0,db=0,sb=0;
 pglGetIntegerv(GL_VIEWPORT,viewport);pglGetIntegerv(GL_DEPTH_BITS,&db);pglGetIntegerv(GL_STENCIL_BITS,&sb);
 if(viewport[2]<=0||viewport[3]<=0){captured=-1;return 0;}
 viewportX=viewport[0];viewportY=viewport[1];
 /* Isolate errors belonging to earlier engine draws from this capture. */
 GLenum inherited;while((inherited=pglGetError())!=GL_NO_ERROR){if(!reportedInheritedError){SDL_Log("Water: preceding GL error %x did not disable Modern",inherited);reportedInheritedError=1;}}
 pglPixelStorei(GL_UNPACK_ALIGNMENT,1);pglPixelStorei(GL_UNPACK_ROW_LENGTH,0);pglPixelStorei(GL_UNPACK_SKIP_PIXELS,0);pglPixelStorei(GL_UNPACK_SKIP_ROWS,0);
 pglGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&savedDraw);pglGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&savedRead);
 texture(sceneTexture,GL_LINEAR);
 if(width!=viewport[2]||height!=viewport[3]||depthBits!=db||stencilBits!=sb){
  width=viewport[2];height=viewport[3];depthBits=db;stencilBits=sb;
  pglTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
  texture(depthTexture,GL_NEAREST);
  pglTexImage2D(GL_TEXTURE_2D,0,sb?GL_DEPTH24_STENCIL8:(db<=16?GL_DEPTH_COMPONENT16:db<=24?GL_DEPTH_COMPONENT24:GL_DEPTH_COMPONENT32),width,height,0,sb?GL_DEPTH_STENCIL:GL_DEPTH_COMPONENT,sb?GL_UNSIGNED_INT_24_8:GL_UNSIGNED_INT,0);
 }
 bindFBO(GL_DRAW_FRAMEBUFFER,captureFBO);
 attachFBO(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,sceneTexture,0);
 attachFBO(GL_DRAW_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depthTexture,0);
 attachFBO(GL_DRAW_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,GL_TEXTURE_2D,sb?depthTexture:0,0);
 int ok=statusFBO(GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
 if(ok){
  pglPushAttrib(GL_SCISSOR_BIT);pglDisable(GL_SCISSOR_TEST);bindFBO(GL_READ_FRAMEBUFFER,(GLuint)savedDraw);
  blitFBO(viewportX,viewportY,viewportX+width,viewportY+height,0,0,width,height,GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT,GL_NEAREST);
  pglPopAttrib();
  GLenum error=pglGetError();if(error){SDL_Log("Water capture failed (GL %x); repaired water fallback",error);ok=0;unavailable=1;}
 }
 bindFBO(GL_DRAW_FRAMEBUFFER,(GLuint)savedDraw);bindFBO(GL_READ_FRAMEBUFFER,(GLuint)savedRead);
 captured=ok?1:-1;return ok;
}
/* Evaluate the same two ambient waves by angle addition. Their x phase is
 * shared across rows: 768 trig calls replace 32768 per patch without changing
 * normal resolution, bicubic filtering or any optical-response coefficient. */
static void buildWaveField(float x0,float x1,float z0,float z1,int y,float *field){
 WaterGradientField(x0,x1,z0,z1,y,192,field);
 float cs[192],sn[192];
 const double scale=6.283185307/4096.0,phase=WaterTime()*2048.0;
 for(int x=0;x<192;x++){
  float wx=x0+(x+.5f)*(x1-x0)/192;
  cs[x]=(float)cos(wx*scale);sn[x]=(float)sin(wx*scale);
 }
 for(int z=0;z<192;z++){
  float wz=z0+(z+.5f)*(z1-z0)/192;
  float ca=(float)cos((wz+phase)*scale),sa=(float)sin((wz+phase)*scale);
  float cb=(float)cos((-wz*2+phase*.5)*scale),sb=(float)sin((-wz*2+phase*.5)*scale);
  for(int x=0;x<192;x++){
   int i=(z*192+x)*4;float wx=x0+(x+.5f)*(x1-x0)/192;
   field[i]*=1.7f;field[i+1]*=1.7f;field[i+2]=0;field[i+3]=1;
   float a=cs[x]*ca-sn[x]*sa,b=cs[x]*cb-sn[x]*sb;
   field[i]+=32*a*(float)scale+16*b*(float)scale;field[i+1]+=32*a*(float)scale-32*b*(float)scale;
  }
 }
}
#include "water_grid_render.h"
/* Pack the existing RGBA16F samples explicitly. Sending GL_FLOAT made the
 * driver convert every texel on the submission thread (the dominant cost).
 * IEEE round-to-nearest-even preserves the texture's previous precision. */
static unsigned short waterHalf(float value){
 unsigned bits;memcpy(&bits,&value,sizeof bits);
 unsigned sign=(bits>>16)&0x8000u,m=bits&0x7fffffu;int e=(int)((bits>>23)&255)-127+15;
 if(e>=31)return (unsigned short)(sign|0x7c00u|(m?0x200u:0));
 if(e<=0){
  if(e<-10)return (unsigned short)sign;
  m|=0x800000u;unsigned shift=(unsigned)(14-e),q=m>>shift,remainder=m&((1u<<shift)-1),half=1u<<(shift-1);
  return (unsigned short)(sign|q+(remainder>half||(remainder==half&&(q&1))));
 }
 unsigned rounded=m+0xfffu+((m>>13)&1u);
 if(rounded&0x800000u){rounded=0;e++;}
 return (unsigned short)(sign|((unsigned)e<<10)|(rounded>>13));
}
static int halfPixelSupport=-1;
static const void *packWaveField(const float *field,GLenum *type){
 static unsigned short packed[192*192*4];
 if(halfPixelSupport<0){const char *version=(const char*)pglGetString(GL_VERSION);halfPixelSupport=(version&&version[0]>='3')||SDL_GL_ExtensionSupported("GL_ARB_half_float_pixel");}
 if(!halfPixelSupport){*type=GL_FLOAT;return field;}
 for(int i=0;i<192*192;i++){packed[i*4]=waterHalf(field[i*4]);packed[i*4+1]=waterHalf(field[i*4+1]);packed[i*4+2]=0;packed[i*4+3]=0x3c00;}
 *type=GL_HALF_FLOAT;return packed;
}
static void uniform1i(const char*n,int v){pglUniform1i(pglGetUniformLocation(program,n),v);}
static void uniform2f(const char*n,float a,float b){pglUniform2f(pglGetUniformLocation(program,n),a,b);}
#endif
void WaterRenderBeginFrame(void){
#if !defined(USE_OPENGL_ES)
 captured=0;memset(lightPosition,0,sizeof lightPosition);memset(lightColor,0,sizeof lightColor);
#endif
}
void WaterRenderSetLights(const float*positions,const float*colors){
#if !defined(USE_OPENGL_ES)
 memcpy(lightPosition,positions,sizeof lightPosition);memcpy(lightColor,colors,sizeof lightColor);
#endif
}
void WaterRenderRelease(void){
#if !defined(USE_OPENGL_ES)
 halfPixelSupport=-1;
 releaseGrids();

 if(program&&pglDeleteProgram)pglDeleteProgram(program);
 if(vertexShader&&pglDeleteShader)pglDeleteShader(vertexShader);
 if(fragmentShader&&pglDeleteShader)pglDeleteShader(fragmentShader);
 if(sceneTexture)pglDeleteTextures(1,&sceneTexture);if(depthTexture)pglDeleteTextures(1,&depthTexture);if(waveTexture)pglDeleteTextures(1,&waveTexture);
 if(captureFBO&&deleteFBO)deleteFBO(1,&captureFBO);
 program=vertexShader=fragmentShader=sceneTexture=depthTexture=waveTexture=captureFBO=0;
 width=height=depthBits=stencilBits=unavailable=captured=waveAllocated=0;
#endif
}
int WaterRenderDraw(const WATER_VERTEX*v,int count,const unsigned short*indices,int indexCount,unsigned int material,int y,const float*view,float px,float py,float nearClip){
#if !defined(USE_OPENGL_ES)
 GLint savedProgram=0,savedActive=GL_TEXTURE0,savedClient=GL_TEXTURE0,savedArray=0,savedElements=0;
 GLuint cpuProgram=0;
 pglGetIntegerv(GL_ARRAY_BUFFER_BINDING,&savedArray);pglGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&savedElements);
 pglGetIntegerv(GL_CURRENT_PROGRAM,&savedProgram);pglGetIntegerv(GL_ACTIVE_TEXTURE,&savedActive);pglGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE,&savedClient);
 pglPushAttrib(GL_ALL_ATTRIB_BITS);pglPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
 pglActiveTexture(GL_TEXTURE0);pglClientActiveTexture(GL_TEXTURE0);
 int modern=WaterEffectMode==1&&initialise()&&capture();
 if(gridRequest){
  if(!modern||!initialiseGrid()){
   pglPopClientAttrib();pglPopAttrib();pglUseProgram(savedProgram);pglActiveTexture(savedActive);pglClientActiveTexture(savedClient);return 0;
  }
  cpuProgram=program;program=gridProgram;
  bindGrid(v,count,indices,indexCount);
 }
 pglUseProgram(modern?program:0);
 pglActiveTexture(GL_TEXTURE0);pglBindTexture(GL_TEXTURE_2D,material);pglEnable(GL_TEXTURE_2D);
 pglEnable(GL_DEPTH_TEST);pglDepthMask(GL_FALSE);pglDisable(GL_CULL_FACE);
 if(modern){
  float x0=v[0].world[0],z0=v[0].world[2],x1=x0,z1=z0;
  static float field[192*192*4];
  if(gridRequest){x1=v[count-1].world[0];z1=v[count-1].world[2];}
  else for(int i=1;i<count;i++){x0=fminf(x0,v[i].world[0]);x1=fmaxf(x1,v[i].world[0]);z0=fminf(z0,v[i].world[2]);z1=fmaxf(z1,v[i].world[2]);}
  buildWaveField(x0,x1,z0,z1,y,field);
  pglActiveTexture(GL_TEXTURE1);pglBindTexture(GL_TEXTURE_2D,sceneTexture);
  pglActiveTexture(GL_TEXTURE2);pglBindTexture(GL_TEXTURE_2D,depthTexture);
  GLenum fieldType;const void *fieldPixels=packWaveField(field,&fieldType);
  pglActiveTexture(GL_TEXTURE3);texture(waveTexture,GL_LINEAR);pglPixelStorei(GL_UNPACK_ALIGNMENT,1);pglPixelStorei(GL_UNPACK_ROW_LENGTH,0);pglPixelStorei(GL_UNPACK_SKIP_PIXELS,0);pglPixelStorei(GL_UNPACK_SKIP_ROWS,0);if(!waveAllocated){pglTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,192,192,0,GL_RGBA,fieldType,fieldPixels);waveAllocated=1;}else pglTexSubImage2D(GL_TEXTURE_2D,0,0,0,192,192,GL_RGBA,fieldType,fieldPixels);
  uniform1i("uMaterial",0);uniform1i("uScene",1);uniform1i("uDepth",2);uniform1i("uWave",3);
  uniform2f("uSize",width,height);uniform2f("uViewport",viewportX,viewportY);
  uniform4fv(pglGetUniformLocation(program,"uLightPosition"),4,lightPosition);uniform4fv(pglGetUniformLocation(program,"uLightColor"),4,lightColor);uniform2f("uProj",px,py);uniform2f("uOrigin",x0,z0);uniform2f("uExtent",x1-x0,z1-z0);
  pglUniform1f(pglGetUniformLocation(program,"uNear"),nearClip);uniform4fv(pglGetUniformLocation(program,"uView"),3,view);
  if(gridRequest)gridUniforms(y);
  pglDisable(GL_BLEND);pglDisable(GL_ALPHA_TEST);
  pglClientActiveTexture(GL_TEXTURE1);pglEnableClientState(GL_TEXTURE_COORD_ARRAY);pglTexCoordPointer(3,GL_FLOAT,sizeof(*v),gridRequest?(const void*)offsetof(WATER_VERTEX,world):v->world);
 }else{pglEnable(GL_BLEND);pglBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);}
 pglActiveTexture(GL_TEXTURE0);pglClientActiveTexture(GL_TEXTURE0);
 pglEnableClientState(GL_VERTEX_ARRAY);pglVertexPointer(4,GL_FLOAT,sizeof(*v),gridRequest?(const void*)offsetof(WATER_VERTEX,clip):v->clip);
 pglEnableClientState(GL_TEXTURE_COORD_ARRAY);pglTexCoordPointer(2,GL_FLOAT,sizeof(*v),gridRequest?(const void*)offsetof(WATER_VERTEX,uv):v->uv);
 pglEnableClientState(GL_COLOR_ARRAY);pglColorPointer(4,GL_FLOAT,sizeof(*v),gridRequest?(const void*)offsetof(WATER_VERTEX,color):v->color);
 pglDrawElements(GL_TRIANGLES,indexCount,GL_UNSIGNED_SHORT,gridRequest?0:indices);
 if(gridRequest){program=cpuProgram;gridBindBuffer(GL_ARRAY_BUFFER,savedArray);gridBindBuffer(GL_ELEMENT_ARRAY_BUFFER,savedElements);}
 pglPopClientAttrib();pglPopAttrib();pglUseProgram(savedProgram);pglActiveTexture(savedActive);pglClientActiveTexture(savedClient);
 return 1;
#else
 return 0;
#endif
}

int WaterRenderDrawGrid(const WATER_VERTEX *v,int count,const unsigned short *indices,int indexCount,
 unsigned int material,int y,const float *view,float px,float py,float nearClip,const WATER_GRID_STATE *state){
#if !defined(USE_OPENGL_ES)
 gridRequest=state;int ok=WaterRenderDraw(v,count,indices,indexCount,material,y,view,px,py,nearClip);gridRequest=0;return ok;
#else
 return 0;
#endif
}
#if defined(USE_OPENGL_ES)
int WaterRenderGridSupported(void){return 0;}
#endif
