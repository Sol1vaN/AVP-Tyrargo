#ifndef OGLFUNC_H
#define OGLFUNC_H

#if defined(_MSC_VER)
#include <windows.h>
#endif

#if defined(USE_OPENGL_ES)
#include <SDL3/SDL_opengles.h>

// OpenGL compatibility
typedef GLclampf GLclampd;
typedef GLfloat GLdouble;

#else
#include <SDL3/SDL_opengl.h>
#endif

#if !defined(GL_CLAMP_TO_EDGE)
// Originally added by GL_SGIS_texture_edge_clamp; part of OpenGL 1.2 core.
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#if !defined(APIENTRY)
#define APIENTRY
#endif

#if !defined(GL_VERTEX_SHADER)
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_TEXTURE0 0x84C0
#endif

typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum);
typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei *, char *);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint *);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei *, char *);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint *);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const char *);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const char * const *, const GLint *);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void (APIENTRY *PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (APIENTRY *PFNGLCLIENTACTIVETEXTUREPROC)(GLenum);

typedef void (APIENTRY *PFNGLALPHAFUNCPROC)(GLenum, GLclampf);
typedef void (APIENTRY *PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void (APIENTRY *PFNGLBLENDFUNCPROC)(GLenum, GLenum);
typedef void (APIENTRY *PFNGLCLEARPROC)(GLbitfield);
typedef void (APIENTRY *PFNGLCLEARCOLORPROC)(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (APIENTRY *PFNGLCOLOR4FPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLCOLORPOINTERPROC)(GLint, GLenum, GLsizei, const GLvoid *);
typedef void (APIENTRY *PFNGLCOPYTEXSUBIMAGE2DPROC)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
typedef void (APIENTRY *PFNGLCULLFACEPROC)(GLenum);
typedef void (APIENTRY *PFNGLDELETETEXTURESPROC)(GLsizei,const GLuint*);
typedef void (APIENTRY *PFNGLDEPTHFUNCPROC)(GLenum);
typedef void (APIENTRY *PFNGLDEPTHMASKPROC)(GLboolean);
typedef void (APIENTRY *PFNGLDEPTHRANGEPROC)(GLclampd, GLclampd);
typedef void (APIENTRY *PFNGLDISABLEPROC)(GLenum);
typedef void (APIENTRY *PFNGLDISABLECLIENTSTATEPROC)(GLenum);
typedef void (APIENTRY *PFNGLDRAWELEMENTSPROC)(GLenum, GLsizei, GLenum, const GLvoid *);
typedef void (APIENTRY *PFNGLENABLEPROC)(GLenum);
typedef void (APIENTRY *PFNGLENABLECLIENTSTATEPROC)(GLenum);
typedef void (APIENTRY *PFNGLFRONTFACEPROC)(GLenum);
typedef void (APIENTRY *PFNGLGENTEXTURESPROC)(GLsizei,GLuint*);
typedef GLenum (APIENTRY *PFNGLGETERRORPROC)(void);
typedef void (APIENTRY *PFNGLGETFLOATVPROC)(GLenum, GLfloat *);
typedef void (APIENTRY *PFNGLGETINTEGERVPROC)(GLenum, GLint *);
typedef const GLubyte* (APIENTRY *PFNGLGETSTRINGPROC)(GLenum);
typedef void (APIENTRY *PFNGLGETTEXPARAMETERFVPROC)(GLenum, GLenum, GLfloat*);
typedef void (APIENTRY *PFNGLHINTPROC)(GLenum, GLenum);
typedef void (APIENTRY *PFNGLLOADIDENTITYPROC)(void);
typedef void (APIENTRY *PFNGLMATRIXMODEPROC)(GLenum);
typedef void (APIENTRY *PFNGLPIXELSTOREIPROC)(GLenum, GLint);
typedef void (APIENTRY *PFNGLPOPMATRIXPROC)(void);
typedef void (APIENTRY *PFNGLPOPATTRIBPROC)(void);
typedef void (APIENTRY *PFNGLPOPCLIENTATTRIBPROC)(void);
typedef void (APIENTRY *PFNGLPOLYGONOFFSETPROC)(GLfloat, GLfloat);
typedef void (APIENTRY *PFNGLPUSHMATRIXPROC)(void);
typedef void (APIENTRY *PFNGLPUSHATTRIBPROC)(GLbitfield);
typedef void (APIENTRY *PFNGLPUSHCLIENTATTRIBPROC)(GLbitfield);
typedef void (APIENTRY *PFNGLREADPIXELSPROC)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
typedef void (APIENTRY *PFNGLSHADEMODELPROC)(GLenum);
typedef void (APIENTRY *PFNGLTEXCOORDPOINTERPROC)(GLint, GLenum, GLsizei, const GLvoid *);
typedef void (APIENTRY *PFNGLTEXENVFPROC)(GLenum, GLenum, GLfloat);
typedef void (APIENTRY *PFNGLTEXENVFVPROC)(GLenum, GLenum, const GLfloat *);
typedef void (APIENTRY *PFNGLTEXENVIPROC)(GLenum, GLenum, GLint);
typedef void (APIENTRY *PFNGLTEXIMAGE2DPROC)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid*);
typedef void (APIENTRY *PFNGLTEXPARAMETERFPROC)(GLenum, GLenum, GLfloat);
typedef void (APIENTRY *PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void (APIENTRY *PFNGLTEXSUBIMAGE2DPROC)(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const GLvoid*);
typedef void (APIENTRY *PFNGLVERTEXPOINTERPROC)(GLint, GLenum, GLsizei, const GLvoid *);
typedef void (APIENTRY *PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);

extern PFNGLACTIVETEXTUREPROC pglActiveTexture;
extern PFNGLATTACHSHADERPROC pglAttachShader;
extern PFNGLCOMPILESHADERPROC pglCompileShader;
extern PFNGLCREATESHADERPROC pglCreateShader;
extern PFNGLCREATEPROGRAMPROC pglCreateProgram;
extern PFNGLDELETEPROGRAMPROC pglDeleteProgram;
extern PFNGLDELETESHADERPROC pglDeleteShader;
extern PFNGLGETPROGRAMINFOLOGPROC pglGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC pglGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC pglGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC pglGetShaderiv;
extern PFNGLGETUNIFORMLOCATIONPROC pglGetUniformLocation;
extern PFNGLLINKPROGRAMPROC pglLinkProgram;
extern PFNGLSHADERSOURCEPROC pglShaderSource;
extern PFNGLUNIFORM1FPROC pglUniform1f;
extern PFNGLUNIFORM1IPROC pglUniform1i;
extern PFNGLUNIFORM2FPROC pglUniform2f;
extern PFNGLUSEPROGRAMPROC pglUseProgram;
extern PFNGLCLIENTACTIVETEXTUREPROC pglClientActiveTexture;

extern PFNGLALPHAFUNCPROC		pglAlphaFunc;
extern PFNGLBINDTEXTUREPROC		pglBindTexture;
extern PFNGLBLENDFUNCPROC		pglBlendFunc;
extern PFNGLCLEARPROC			pglClear;
extern PFNGLCLEARCOLORPROC		pglClearColor;
extern PFNGLCOLOR4FPROC		pglColor4f;
extern PFNGLCOLORPOINTERPROC		pglColorPointer;
extern PFNGLCOPYTEXSUBIMAGE2DPROC pglCopyTexSubImage2D;
extern PFNGLCULLFACEPROC		pglCullFace;
extern PFNGLDELETETEXTURESPROC		pglDeleteTextures;
extern PFNGLDEPTHFUNCPROC		pglDepthFunc;
extern PFNGLDEPTHMASKPROC		pglDepthMask;
extern PFNGLDEPTHRANGEPROC		pglDepthRange;
extern PFNGLDISABLEPROC		pglDisable;
extern PFNGLDISABLECLIENTSTATEPROC	pglDisableClientState;
extern PFNGLDRAWELEMENTSPROC		pglDrawElements;
extern PFNGLENABLEPROC			pglEnable;
extern PFNGLENABLECLIENTSTATEPROC	pglEnableClientState;
extern PFNGLFRONTFACEPROC		pglFrontFace;
extern PFNGLGENTEXTURESPROC		pglGenTextures;
extern PFNGLGETERRORPROC		pglGetError;
extern PFNGLGETFLOATVPROC		pglGetFloatv;
extern PFNGLGETINTEGERVPROC		pglGetIntegerv;
extern PFNGLGETSTRINGPROC		pglGetString;
extern PFNGLGETTEXPARAMETERFVPROC	pglGetTexParameterfv;
extern PFNGLHINTPROC			pglHint;
extern PFNGLLOADIDENTITYPROC		pglLoadIdentity;
extern PFNGLMATRIXMODEPROC		pglMatrixMode;
extern PFNGLPIXELSTOREIPROC		pglPixelStorei;
extern PFNGLPOPMATRIXPROC		pglPopMatrix;
extern PFNGLPOPATTRIBPROC		pglPopAttrib;
extern PFNGLPOPCLIENTATTRIBPROC	pglPopClientAttrib;
extern PFNGLPOLYGONOFFSETPROC		pglPolygonOffset;
extern PFNGLPUSHMATRIXPROC		pglPushMatrix;
extern PFNGLPUSHATTRIBPROC		pglPushAttrib;
extern PFNGLPUSHCLIENTATTRIBPROC	pglPushClientAttrib;
extern PFNGLREADPIXELSPROC		pglReadPixels;
extern PFNGLSHADEMODELPROC		pglShadeModel;
extern PFNGLTEXCOORDPOINTERPROC	pglTexCoordPointer;
extern PFNGLTEXENVFPROC		pglTexEnvf;
extern PFNGLTEXENVFVPROC		pglTexEnvfv;
extern PFNGLTEXENVIPROC		pglTexEnvi;
extern PFNGLTEXIMAGE2DPROC		pglTexImage2D;
extern PFNGLTEXPARAMETERFPROC		pglTexParameterf;
extern PFNGLTEXPARAMETERIPROC		pglTexParameteri;
extern PFNGLTEXSUBIMAGE2DPROC		pglTexSubImage2D;
extern PFNGLVERTEXPOINTERPROC		pglVertexPointer;
extern PFNGLVIEWPORTPROC		pglViewport;

extern int ogl_have_multisample_filter_hint;
extern int ogl_have_texture_filter_anisotropic;

extern int ogl_use_multisample_filter_hint;
extern int ogl_use_texture_filter_anisotropic;

extern void load_ogl_functions(int mode);

extern int check_for_errors_(const char *file, int line);
#define check_for_errors() check_for_errors_(__FILE__, __LINE__)

#endif
