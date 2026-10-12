/* CRT shader adaptation: Inigo Quilez's postprocessing shader, via the
 * MattiasCRT ReShade port and locally expanded MattiasCRT_Unlocked_Ex.fx.
 * The supplied shader's notice is CC BY-NC-SA 3.0 Unported:
 * https://creativecommons.org/licenses/by-nc-sa/3.0/
 * Modifications: GLSL 1.20 translation, profile constants, preview/image controls
 * and rounded-border optimization. See LICENSES/CC-BY-NC-SA-3.0.txt.
 * Mattias Gustavsson/newpixie and HelelSingh/CRT-Guest-ReShade are credited
 * references; the separate Guest shader's GPL code is not included here. */
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "oglfunc.h"
#include "crt_filter.h"
#include "menu_preview.h"

int CRTFilterEnabled = 0;
/* Brightness 0..100 maps to -0.5..+0.5; contrast/saturation 0..200 to 0..2. */
int ImageBrightness=50,ImageContrast=100,ImageSaturation=100;
int ImageAdjustmentsActive(void){return ImageBrightness!=50||ImageContrast!=100||ImageSaturation!=100;}

static GLint PreviewBandUniform;
static GLuint CRTFilterProgram;
static GLuint CRTFilterVertexShader;
static GLuint CRTFilterFragmentShader;
static GLuint CRTFilterTexture;
static GLsizei CRTFilterTextureWidth;
static GLsizei CRTFilterTextureHeight;
static GLint CRTFilterFrameUniform = -1;
static GLint CRTFilterResolutionUniform = -1;
static GLint CRTFilterTimeUniform = -1;
static int CRTFilterUnavailable;
static GLint ImageBrightnessUniform,ImageContrastUniform,ImageSaturationUniform,CRTEnabledUniform;

static const GLshort CRTFilterIndices[6] = { 0, 1, 2, 0, 2, 3 };
static const GLfloat CRTFilterVertices[16] = {
	-1.0f, -1.0f, 0.0f, 0.0f,
	 1.0f, -1.0f, 1.0f, 0.0f,
	 1.0f,  1.0f, 1.0f, 1.0f,
	-1.0f,  1.0f, 0.0f, 1.0f
};

/* GLSL 1.20 port of MattiasCRT_Unlocked_Ex.fx. The constants below are the
 * effective values from the supplied avp.ini profile. Its RGB split, ghosting
 * and crossfade branches are disabled, so wobble deliberately has no sampling
 * path; the external HLSL applies its corrected wobble independently. */
/* A conservative interior of the rounded border is exactly one. Compute its
 * limits per vertex (constant over the full-screen primitive), so the fragment
 * shader only evaluates the expensive superellipse in the narrow edge region.
 * The safety margin keeps floating-point boundary rounding on the slow path. */
static const char *CRTFilterVertexSource =
 "#version 120\n"
 "uniform vec2 uResolution;\n"
 "varying vec2 vUV, vCornerSafe;\n"
 "void main() {\n"
 " gl_Position=gl_Vertex; vUV=gl_MultiTexCoord0.xy;\n"
 " float aspect=uResolution.x/max(uResolution.y,1.0);\n"
 " float csz=mix(400.0,7.0,pow(4.0*0.035,0.10));\n"
 " float bc=0.060*max(1.0,aspect)*0.05;\n"
 " float safe=pow(max(1.0-bc,0.0),2.0)*pow(0.5,1.0/csz)*0.999;\n"
 " vCornerSafe=vec2(safe,pow(safe,aspect));\n"
 "}\n";

static const char *CRTFilterFragmentSource =
	"#version 120\n"
	"uniform sampler2D uFrame;\n"
	"uniform vec2 uResolution;\n"
	"uniform float uTime;\n"
 "uniform vec2 uPreviewBand;\n"
 "uniform float uBrightness, uContrast, uSaturation;\n"
 "uniform int uCRTEnabled;\n"
	"varying vec2 vUV, vCornerSafe;\n"
	"vec2 curve(vec2 uv) {\n"
	"  uv = (uv - 0.5) * 2.0; uv *= 1.093;\n"
	"  uv.x *= 1.0 + 0.011 * pow(abs(uv.y), 1.44);\n"
	"  uv.y *= 1.0 + 0.016 * pow(abs(uv.x), 1.44);\n"
	"  uv = uv * 0.5 + 0.5;\n"
	"  return (uv - 0.5) * 0.920 + 0.5;\n"
	"}\n"
	"float CRTGuestCornerBorderMask(vec2 pos) {\n"
	"  float aspect = uResolution.x / max(uResolution.y, 1.0);\n"
	"  float invAspect = 1.0 / aspect;\n"
	"  vec2 bc = 0.060 * vec2(1.0, aspect) * 0.05;\n"
	"  pos = clamp(pos, 0.0, 1.0); pos = abs(2.0 * (pos - 0.5));\n"
	"  if(pos.x < vCornerSafe.x && pos.y < vCornerSafe.y) return 1.0;\n"
	"  float csz = mix(400.0, 7.0, pow(4.0 * 0.035, 0.10));\n"
	"  float crn = pow(dot(pow(pos, csz * vec2(1.0, invAspect)), vec2(1.0)), 1.0 / csz);\n"
	"  pos = max(pos, vec2(crn));\n"
	"  vec2 t = clamp((sqrt(pos) - vec2(1.0)) / -bc, 0.0, 1.0);\n"
	"  vec2 rs = t * t * (3.0 - 2.0 * t);\n"
	"  rs = pow(rs, vec2(0.75)); return sqrt(rs.x * rs.y);\n"
	"}\n"
 "vec3 grade(vec3 c, vec2 sourceUV){\n"
 " if(sourceUV.y>=uPreviewBand.x && sourceUV.y<=uPreviewBand.y)return c;\n"
 " if(uBrightness==0.0&&uContrast==1.0&&uSaturation==1.0)return c;\n"
 " float y=dot(c,vec3(0.2126,0.7152,0.0722));\n"
 " return clamp((mix(vec3(y),c,uSaturation)-0.5)*uContrast+0.5+uBrightness,0.0,1.0); }\n"
	"void main() {\n"
	"  if(uCRTEnabled==0){gl_FragColor=vec4(grade(texture2D(uFrame,vUV).rgb,vUV),1.0);return;}\n"
	"  vec2 uv = curve(vUV); float t = uTime; /* uTime is already seconds. */\n"
	"  /* Intentional internal profile choice: keep wobble at 0.0 so the CRT\n"
	"     remains temporally stable while preserving the resolved AA image. */\n"
	"  const float wobbleX = 0.0;\n"
	"  vec3 col = texture2D(uFrame, uv + vec2(wobbleX, 0.0)).rgb;\n"
	"  col = clamp(col * 0.60 + 0.40 * col * col, 0.0, 1.0);\n"
	"  col *= vec3(0.950, 1.050, 0.950); col *= 2.80;\n"
	"  float vig = 16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);\n"
	"  col *= pow(max(vig, 0.0), 0.170);\n"
	"  float scans = clamp(0.350 + 0.350 * sin(3.50 * t + uv.y * uResolution.y * 1.50), 0.0, 1.0);\n"
	"  col *= 0.400 + 0.700 * pow(max(scans, 0.0), 1.70);\n"
	"  col *= 1.000 + 0.010 * sin(110.0 * t);\n"
	"  float inside = step(0.0, uv.x) * step(0.0, uv.y) * step(uv.x, 1.0) * step(uv.y, 1.0);\n"
	"  float mask = clamp((mod(gl_FragCoord.x, 2.0) - 1.0) * 2.0, 0.0, 1.0);\n"
	"  col *= 1.392 - 0.650 * mask;\n"
	"  gl_FragColor = vec4(grade(col,uv) * inside * CRTGuestCornerBorderMask(uv), 1.0);\n"
	"}\n";

static int CRTFilterCompileShader(GLuint shader, const char *source, const char *name)
{
	GLint compiled = GL_FALSE;
	GLint length = 0;
	char log[2048];

	pglShaderSource(shader, 1, &source, NULL);
	pglCompileShader(shader);
	pglGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_TRUE) return 1;
	pglGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	if (length > 1) {
		if (length >= (GLint)sizeof(log)) length = (GLint)sizeof(log) - 1;
		pglGetShaderInfoLog(shader, length, NULL, log);
		log[length] = '\0';
	} else strcpy(log, "no compiler log");
	fprintf(stderr, "CRT GLSL %s shader compilation failed: %s\n", name, log);
	return 0;
}

static int CRTFilterInitialise(void)
{
	GLint linked = GL_FALSE;
	GLint length = 0;
	char log[2048];

	if (CRTFilterProgram != 0) return 1;
	if (CRTFilterUnavailable || pglCreateShader == NULL || pglCreateProgram == NULL) return 0;

	CRTFilterVertexShader = pglCreateShader(GL_VERTEX_SHADER);
	CRTFilterFragmentShader = pglCreateShader(GL_FRAGMENT_SHADER);
	if (CRTFilterVertexShader == 0 || CRTFilterFragmentShader == 0
		|| !CRTFilterCompileShader(CRTFilterVertexShader, CRTFilterVertexSource, "vertex")
		|| !CRTFilterCompileShader(CRTFilterFragmentShader, CRTFilterFragmentSource, "fragment")) goto failed;

	CRTFilterProgram = pglCreateProgram();
	if (CRTFilterProgram == 0) goto failed;
	pglAttachShader(CRTFilterProgram, CRTFilterVertexShader);
	pglAttachShader(CRTFilterProgram, CRTFilterFragmentShader);
	pglLinkProgram(CRTFilterProgram);
	pglGetProgramiv(CRTFilterProgram, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE) {
		pglGetProgramiv(CRTFilterProgram, GL_INFO_LOG_LENGTH, &length);
		if (length > 1) {
			if (length >= (GLint)sizeof(log)) length = (GLint)sizeof(log) - 1;
			pglGetProgramInfoLog(CRTFilterProgram, length, NULL, log);
			log[length] = '\0';
		} else strcpy(log, "no linker log");
		fprintf(stderr, "CRT GLSL program link failed: %s\n", log);
		goto failed;
	}

	CRTFilterFrameUniform = pglGetUniformLocation(CRTFilterProgram, "uFrame");
	CRTFilterResolutionUniform = pglGetUniformLocation(CRTFilterProgram, "uResolution");
	CRTFilterTimeUniform = pglGetUniformLocation(CRTFilterProgram, "uTime");
 ImageBrightnessUniform=pglGetUniformLocation(CRTFilterProgram,"uBrightness");
 ImageContrastUniform=pglGetUniformLocation(CRTFilterProgram,"uContrast");
 ImageSaturationUniform=pglGetUniformLocation(CRTFilterProgram,"uSaturation");
 CRTEnabledUniform=pglGetUniformLocation(CRTFilterProgram,"uCRTEnabled");
 PreviewBandUniform=pglGetUniformLocation(CRTFilterProgram,"uPreviewBand");
	return 1;

failed:
	CRTFilter_Release();
	CRTFilterUnavailable = 1;
	return 0;
}

static int CRTFilterEnsureTexture(GLsizei width, GLsizei height)
{
	if (width < 1 || height < 1) return 0;
	if (CRTFilterTexture == 0) {
		pglGenTextures(1, &CRTFilterTexture);
		if (CRTFilterTexture == 0) return 0;
		pglBindTexture(GL_TEXTURE_2D, CRTFilterTexture);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		pglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else pglBindTexture(GL_TEXTURE_2D, CRTFilterTexture);
	if (CRTFilterTextureWidth != width || CRTFilterTextureHeight != height) {
		GLint maxTextureSize = 0;
		pglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
		if (width > maxTextureSize || height > maxTextureSize) return 0;
		/* RGB565 quantised the captured scene to 16 bits and created visible colour
		   banding. Keep an 8-bit channel for every RGBA component, like the ReShade
		   backbuffer pass. */
		pglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		CRTFilterTextureWidth = width;
		CRTFilterTextureHeight = height;
	}
	return 1;
}

/* Geometry is already in clip space; GLSL never reads the fixed-function
 * matrices or current colour. Preserve only state that this pass changes. */
int CRTFilter_Apply(int drawableWidth, int drawableHeight)
{
 GLint savedProgram=0,savedActiveTexture=GL_TEXTURE0,savedTexture=0;
 if ((!CRTFilterEnabled && !ImageAdjustmentsActive()) || !CRTFilterInitialise()) return 0;
 if(drawableWidth < 1 || drawableHeight < 1) return 0;
 pglGetIntegerv(GL_CURRENT_PROGRAM,&savedProgram);
 pglGetIntegerv(GL_ACTIVE_TEXTURE,&savedActiveTexture);
 pglActiveTexture(GL_TEXTURE0);
 pglGetIntegerv(GL_TEXTURE_BINDING_2D,&savedTexture);
 pglPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_MULTISAMPLE_BIT);
 pglPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
 pglClientActiveTexture(GL_TEXTURE0);
 if(!CRTFilterEnsureTexture(drawableWidth,drawableHeight)) {
  pglPopClientAttrib();pglPopAttrib();
  pglBindTexture(GL_TEXTURE_2D,savedTexture);pglActiveTexture(savedActiveTexture);
  return 0;
 }
 CopyResolvedFramebufferToTexture(CRTFilterTexture,drawableWidth,drawableHeight);
 pglDisable(GL_SCISSOR_TEST);pglDisable(GL_ALPHA_TEST);pglDisable(GL_BLEND);
 pglDisable(GL_DEPTH_TEST);pglDisable(GL_CULL_FACE);
#ifdef GL_MULTISAMPLE
 /* Preserve the scene's explicit MSAA resolve and multisample final write. */
 pglEnable(GL_MULTISAMPLE);
#endif
 pglViewport(0,0,drawableWidth,drawableHeight);
 /* The opaque full-screen shader writes every colour pixel, including its
  * black curved border. Only the original depth reset is needed here. */
 pglClear(GL_DEPTH_BUFFER_BIT);
 pglUseProgram(CRTFilterProgram);
 pglUniform1i(CRTFilterFrameUniform,0);
 pglUniform1i(CRTEnabledUniform,!!CRTFilterEnabled);
 pglUniform2f(PreviewBandUniform,MenuPreviewBandLow,MenuPreviewBandHigh);
 pglUniform1f(ImageBrightnessUniform,(ImageBrightness-50)/100.0f);
 pglUniform1f(ImageContrastUniform,ImageContrast/100.0f);
 pglUniform1f(ImageSaturationUniform,ImageSaturation/100.0f);
 pglUniform2f(CRTFilterResolutionUniform,(GLfloat)drawableWidth,(GLfloat)drawableHeight);
 pglUniform1f(CRTFilterTimeUniform,(GLfloat)((double)SDL_GetTicksNS()/1000000000.0));
 pglEnableClientState(GL_VERTEX_ARRAY);pglVertexPointer(2,GL_FLOAT,sizeof(GLfloat)*4,CRTFilterVertices);
 pglEnableClientState(GL_TEXTURE_COORD_ARRAY);pglTexCoordPointer(2,GL_FLOAT,sizeof(GLfloat)*4,CRTFilterVertices+2);
 pglDisableClientState(GL_COLOR_ARRAY);
 pglDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,CRTFilterIndices);
 pglUseProgram(savedProgram);
 pglPopClientAttrib();pglPopAttrib();
 pglBindTexture(GL_TEXTURE_2D,savedTexture);pglActiveTexture(savedActiveTexture);
 return 1;
}

void CRTFilter_Release(void)
{
	if (CRTFilterTexture != 0 && pglDeleteTextures != NULL) pglDeleteTextures(1, &CRTFilterTexture);
	if (CRTFilterProgram != 0 && pglDeleteProgram != NULL) pglDeleteProgram(CRTFilterProgram);
	if (CRTFilterVertexShader != 0 && pglDeleteShader != NULL) pglDeleteShader(CRTFilterVertexShader);
	if (CRTFilterFragmentShader != 0 && pglDeleteShader != NULL) pglDeleteShader(CRTFilterFragmentShader);
	CRTFilterProgram = 0; CRTFilterVertexShader = 0; CRTFilterFragmentShader = 0;
	CRTFilterTexture = 0; CRTFilterTextureWidth = 0; CRTFilterTextureHeight = 0;
	CRTFilterFrameUniform = CRTFilterResolutionUniform = CRTFilterTimeUniform = -1;
	CRTFilterUnavailable = 0;
}

