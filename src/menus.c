#include "tyrargo_version.h"
#include "menu_background.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "fixer.h"

#include "strtab.hpp"

#include "3dc.h"
#include "inline.h"
#include "awtexld.h"
#include "chnktexi.h"
#include "hud_layout.h"

#include "avp_menus.h"
#include "avp/win95/text_encoding.h"

#define UseLocalAssert Yes
#include "ourasert.h"
#include "ffstdio.h"

char AAFontWidths[256];

extern SDL_Surface *surface;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern int CloudTable[128][128];
extern int CloakingPhase;

AVPMENUGFX AvPMenuGfxStorage[MAX_NO_OF_AVPMENUGFXS] =
{
	{"Menus\\fractal.rim"},
	{"Common\\aa_font.rim"},// Warning! Texture from common used

	{"Menus\\copyright.rim"},

	{"Menus\\FIandRD.rim"},
	{"Menus\\presents.rim"},
	{"Menus\\AliensVPredator.rim"},
	
	{"Menus\\sliderbar.rim"},//AVPMENUGFX_SLIDERBAR,
	{"Menus\\slider.rim"},//AVPMENUGFX_SLIDER,

	{"Menus\\starfield.rim"},
	{"Menus\\aliens.rim"},
	{"Menus\\Alien.rim"},
	{"Menus\\Marine.rim"},
	{"Menus\\Predator.rim"},

	{"Menus\\glowy_left.rim"},
	{"Menus\\glowy_middle.rim"},
	{"Menus\\glowy_right.rim"},
	
	// Marine level
	{"Menus\\MarineEpisode1.rim"},
	{"Menus\\MarineEpisode2.rim"},
	{"Menus\\MarineEpisode3.rim"},
	{"Menus\\MarineEpisode4.rim"},
	{"Menus\\MarineEpisode5.rim"},
	{"Menus\\MarineEpisode6.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	
	// Predator level
	{"Menus\\PredatorEpisode1.rim"},
	{"Menus\\PredatorEpisode2.rim"},
	{"Menus\\PredatorEpisode3.rim"},
	{"Menus\\PredatorEpisode4.rim"},
	{"Menus\\PredatorEpisode5.rim"},
	{"Menus\\PredatorEpisode5.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},

	// Alien level
	{"Menus\\AlienEpisode2.rim"},
	{"Menus\\AlienEpisode4.rim"},
	{"Menus\\AlienEpisode1.rim"},
	{"Menus\\AlienEpisode3.rim"},
	{"Menus\\AlienEpisode5.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},

	// Splash screens
#if MARINE_DEMO
	{"MarineSplash\\splash00.rim"},
	{"MarineSplash\\splash01.rim"},
	{"MarineSplash\\splash02.rim"},
	{"MarineSplash\\splash03.rim"},
	{"MarineSplash\\splash04.rim"},
	{"MarineSplash\\splash05.rim"},
#elif ALIEN_DEMO
	{"AlienSplash\\splash00.rim"},
	{"AlienSplash\\splash01.rim"},
	{"AlienSplash\\splash02.rim"},
	{"AlienSplash\\splash03.rim"},
	{"AlienSplash\\splash04.rim"},
	{"AlienSplash\\splash05.rim"},
#else
	{"PredatorSplash\\splash00.rim"},
	{"PredatorSplash\\splash01.rim"},
	{"PredatorSplash\\splash02.rim"},
	{"PredatorSplash\\splash03.rim"},
	{"PredatorSplash\\splash04.rim"},
	{"PredatorSplash\\splash05.rim"},
#endif
};

static void DrawAvPMenuGlowyBar(int topleftX, int topleftY, int alpha, int length)
{
	enum AVPMENUGFX_ID menuGfxID = AVPMENUGFX_GLOWY_MIDDLE;
	unsigned char *srcPtr;
	unsigned short *destPtr;
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	image = gfxPtr->ImagePtr;
	srcPtr = image->buf;
	
	if (ScreenDescriptorBlock.SDB_Width - topleftX < length) {
		length = ScreenDescriptorBlock.SDB_Width - topleftX;
	}
	if (length<0) length = 0;
	
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	if (alpha>ONE_FIXED) {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch) + topleftX;
			
			for (x=0; x<length; x++) {
				*destPtr =	((srcPtr[0]>>3)<<11) |
						((srcPtr[1]>>2)<<5 ) |
						((srcPtr[2]>>3));
				destPtr++;
			}
			
			srcPtr += image->w * 4;
		}
	} else {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch)) + topleftX;
			
			for (x=0; x<length; x++) {
				if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
					unsigned int destR, destG, destB;
						
					destR = (*destPtr & 0xF800)>>8;
					destG = (*destPtr & 0x07E0)>>3;
					destB = (*destPtr & 0x001F)<<3;
						
					destR += MUL_FIXED(alpha, srcPtr[0]);
					destG += MUL_FIXED(alpha, srcPtr[1]);
					destB += MUL_FIXED(alpha, srcPtr[2]);
					if (destR > 0x00FF) destR = 0x00FF;
					if (destG > 0x00FF) destG = 0x00FF;
					if (destB > 0x00FF) destB = 0x00FF;
						
					*destPtr =	((destR>>3)<<11) |
							((destG>>2)<<5 ) |
							((destB>>3));
				}
				
				destPtr++;
			}
			
			srcPtr += image->w * 4;
		}
	}
		
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

static void DrawAvPMenuGlowyBar_Clipped(int topleftX, int topleftY, int alpha, int length, int topY, int bottomY)
{
	enum AVPMENUGFX_ID menuGfxID = AVPMENUGFX_GLOWY_MIDDLE;
	unsigned char *srcPtr;
	unsigned short *destPtr;
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	image = gfxPtr->ImagePtr;
	srcPtr = image->buf;
	
	if (length<0) length = 0;
	
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	if (alpha>ONE_FIXED) {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			if(y>=topY && y<=bottomY) {
				destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch) + topleftX;
			
				for (x=0; x<length; x++) {
					*destPtr =	((srcPtr[0]>>3)<<11) |
							((srcPtr[1]>>2)<<5 ) |
							((srcPtr[2]>>3));
					destPtr++;
				}	
			}
			srcPtr += image->w * 4;
		}
	} else {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			if(y>=topY && y<=bottomY) {
				destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch)) + topleftX;
			
				for (x=0; x<length; x++) {				
					if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
						unsigned int destR, destG, destB;
						
						destR = (*destPtr & 0xF800)>>8;
						destG = (*destPtr & 0x07E0)>>3;
						destB = (*destPtr & 0x001F)<<3;
						
						destR += MUL_FIXED(alpha, srcPtr[0]);
						destG += MUL_FIXED(alpha, srcPtr[1]);
						destB += MUL_FIXED(alpha, srcPtr[2]);
						if (destR > 0x00FF) destR = 0x00FF;
						if (destG > 0x00FF) destG = 0x00FF;
						if (destB > 0x00FF) destB = 0x00FF;
						
						*destPtr =	((destR>>3)<<11) |
								((destG>>2)<<5 ) |
								((destB>>3));
					}
				
					destPtr++;
				}
			}	
			srcPtr += image->w * 4;
		}
	}
		
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

typedef struct AVPIndexedFont
{
	AVPMENUGFX info;	/* graphic info */
	int swidth;		/* width for space */
	int ascii;		/* ascii code for initial character */
	int height;		/* height per character */
	
	int numchars;
	int FontWidth[256];
} AVPIndexedFont;

AVPIndexedFont IntroFont_Light;

static void LoadMenuFont()
{
	AVPMENUGFX *gfxPtr;
	char buffer[100];
	size_t fastFileLength;
	void const *pFastFileData;
	
	IntroFont_Light.height = 33;
	IntroFont_Light.swidth = 5;
	IntroFont_Light.ascii = 32;
		
	gfxPtr = &IntroFont_Light.info;
	
	CL_GetImageFileName(buffer, 100, "Menus\\IntroFont.rim", LIO_RELATIVEPATH);
	
	pFastFileData = ffreadbuf(buffer, &fastFileLength);
	
	if (pFastFileData) {
		gfxPtr->ImagePtr = AwCreateSurface(
			"pxfXY",
			pFastFileData,
			fastFileLength,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	} else {
		gfxPtr->ImagePtr = AwCreateSurface(
			"sfXY",
			buffer,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	}
	
	GLOBALASSERT(gfxPtr->ImagePtr);
	GLOBALASSERT(gfxPtr->Width>0);
	GLOBALASSERT(gfxPtr->Height>0);
	
	gfxPtr->hBackup = 0;
	
{
	D3DTexture *image = gfxPtr->ImagePtr;
	unsigned char *srcPtr = image->buf;
	int c;
	
	if ((image->w != 30) || ((image->h % 33) != 0)) {
		fprintf(stderr, "ERROR: I am going to give up now, because I don't like your font!\n");
		fprintf(stderr, "Font Size: %d x %d\n", image->w, image->h);
		exit(EXIT_FAILURE);
	}
	
	IntroFont_Light.numchars = image->h / 33;	
	IntroFont_Light.FontWidth[32] = 5;
	
	for (c=33; c<(32+IntroFont_Light.numchars); c++) {
		int x,y;
		int y1 = 1+(c-32)*33;
		
		IntroFont_Light.FontWidth[c]=31;
		
		for (x=29; x>0; x--) {
			int blank = 1;
			
			for (y=y1; y<y1+31; y++) {
				unsigned char *s = &srcPtr[(x + y*image->w) * 4];
				if (s[2]) {
					blank = 0;
					break;
				}
			}
			
			if (blank) {
				IntroFont_Light.FontWidth[c]--;
			} else {
				break;
			}
		}
	}
}

}

static void ResetAcidMenuAtlas(void);
static void ResetTyrargoTitleAtlas(void);
static void UnloadMenuFont()
{
	ReleaseDDSurface(IntroFont_Light.info.ImagePtr);
	IntroFont_Light.info.ImagePtr = NULL;
 ResetTyrargoTitleAtlas();
 ResetAcidMenuAtlas();
}

int LengthOfMenuText(const char *textPtr)
{
	int width = 0;

	while (textPtr && *textPtr) {
		unsigned char glyph = AvPTextNextGlyphConst(&textPtr);
		width += IntroFont_Light.FontWidth[glyph];
	}
	return width;
}

/* Cache the separable glow only while this title atlas is alive. */
static unsigned char TyrargoTitleMask[512*64],TyrargoTitleGlow[512*64];
static D3DTexture *TyrargoTitleAtlas;
static int TyrargoTitleWidth;
static void ResetTyrargoTitleAtlas(void){TyrargoTitleAtlas=NULL;TyrargoTitleWidth=0;}
void RenderTyrargoTitle(int centreX,int top,int alpha,double pulse)
{
 D3DTexture *atlas=IntroFont_Light.info.ImagePtr;int x,y,i,width,height=57;
 const char *name=TYRARGO_SHORT_NAME;
 if(!alpha||!atlas||!surface)return;
 width=LengthOfMenuText(name)+24;if(width>512)return;
 /* Rebuild on load (UnloadMenuFont invalidates the pointer) or changed atlas. */
 if(TyrargoTitleAtlas!=atlas||TyrargoTitleWidth!=width){
  unsigned char temp[512*64];int pen=12;
  memset(TyrargoTitleMask,0,sizeof TyrargoTitleMask);
  for(i=0;name[i];i++){
   int c=(unsigned char)name[i],w=IntroFont_Light.FontWidth[c],v=1+(c-32)*33;
   for(y=0;y<33;y++)for(x=0;x<w;x++){
    const unsigned char *src=atlas->buf+4*((v+y)*atlas->w+x);
    int a=src[0];if(src[1]>a)a=src[1];if(src[2]>a)a=src[2];
    TyrargoTitleMask[(y+12)*width+pen+x]=(unsigned char)a;
   }pen+=w;
  }
  memcpy(TyrargoTitleGlow,TyrargoTitleMask,sizeof TyrargoTitleGlow);
  for(i=0;i<3;i++){
   for(y=0;y<height;y++)for(x=0;x<width;x++){
    int sum=0,k;for(k=-3;k<=3;k++)if(x+k>=0&&x+k<width)sum+=TyrargoTitleGlow[y*width+x+k];
    temp[y*width+x]=(unsigned char)(sum/7);
   }
   for(y=0;y<height;y++)for(x=0;x<width;x++){
    int sum=0,k;for(k=-3;k<=3;k++)if(y+k>=0&&y+k<height)sum+=temp[(y+k)*width+x];
    TyrargoTitleGlow[y*width+x]=(unsigned char)(sum/7);
   }
  }
  TyrargoTitleAtlas=atlas;TyrargoTitleWidth=width;
 }
 if(SDL_MUSTLOCK(surface)&&!SDL_LockSurface(surface))return;
 for(y=0;y<height;y++)for(x=0;x<width;x++){
  int sx=centreX-width/2+x,sy=top-12+y;
  if(sx<0||sy<0||sx>=surface->w||sy>=surface->h)continue;
  double strength=alpha/65536.0*pulse;
  double core=TyrargoTitleMask[y*width+x]*strength,glow=TyrargoTitleGlow[y*width+x]*strength*2.4;
  unsigned short *dst=(unsigned short*)((unsigned char*)surface->pixels+sy*surface->pitch)+sx;
  int r=((*dst>>11)&31)*255/31+(int)(core*.34+glow*.08);
  int g=((*dst>>5)&63)*255/63+(int)(core*.62+glow*.30);
  int b=(*dst&31)*255/31+(int)(core+glow);
  if(r>255)r=255;if(g>255)g=255;if(b>255)b=255;
  *dst=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
 }
 if(SDL_MUSTLOCK(surface))SDL_UnlockSurface(surface);
}

/* Coloured atlas glyphs with a cached green bloom, including composed Latin
 * glyphs. This does not recolour the backdrop or other difficulty entries. */
static unsigned char AcidTextMask[640*64],AcidTextGlow[640*64];
static D3DTexture *AcidTextAtlas;
static char AcidTextCached[256];
static int AcidTextWidth;
static void ResetAcidMenuAtlas(void){AcidTextAtlas=NULL;AcidTextCached[0]=0;}
static void AcidCopyGlyph(D3DTexture *atlas,unsigned char c,int pen,int width)
{
 if(c<32)return;
 int v=1+(c-32)*33,w=IntroFont_Light.FontWidth[c];
 if(v+33>atlas->h)return;
 for(int y=0;y<33;y++)for(int x=0;x<w&&x<atlas->w;x++){
  int px=pen+x;if(px<0||px>=width)continue;
  const unsigned char *src=atlas->buf+4*((v+y)*atlas->w+x);
  int a=src[0];if(src[1]>a)a=src[1];if(src[2]>a)a=src[2];
  if(a>AcidTextMask[(y+6)*width+px])AcidTextMask[(y+6)*width+px]=(unsigned char)a;
 }
}
/* Match the existing selection fade: dark endpoint 1/4, selected endpoint 1.
 * Attenuate emission before RGB saturation, never jump on a selection change. */
static int MenuGlowScale(int alpha,int darkPercent,int selectedPercent)
{
 int selection=alpha-BRIGHTNESS_OF_DARKENED_ELEMENT;
 int range=BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT-BRIGHTNESS_OF_DARKENED_ELEMENT;
 if(selection<0)selection=0;if(selection>range)selection=range;
 return (int)(((long long)darkPercent*ONE_FIXED+
  (long long)(selectedPercent-darkPercent)*ONE_FIXED*selection/range)/100);
}
void RenderAcidMenuText(char *name,int centreX,int top,int alpha)
{
 D3DTexture *atlas=IntroFont_Light.info.ImagePtr;
 int width=LengthOfMenuText(name)+12,height=45;
 if(!atlas||!surface||width>640||width<=12)return;
 if(AcidTextAtlas!=atlas||AcidTextWidth!=width||strcmp(AcidTextCached,name)){
  unsigned char temp[640*64];char *ptr=name;int pen=6;
  memset(AcidTextMask,0,sizeof AcidTextMask);
  while(*ptr){unsigned char accent=0,c=AvPTextNextGlyphWithAccent(&ptr,&accent);
   AcidCopyGlyph(atlas,c,pen,width);
   if(accent)AcidCopyGlyph(atlas,accent,pen+(IntroFont_Light.FontWidth[c]-IntroFont_Light.FontWidth[accent])/2,width);
   pen+=IntroFont_Light.FontWidth[c];
  }
  for(int y=0;y<height;y++)for(int x=0;x<width;x++){
   int sum=0;for(int k=-3;k<=3;k++)if(x+k>=0&&x+k<width)sum+=AcidTextMask[y*width+x+k];
   temp[y*width+x]=(unsigned char)(sum/7);
  }
  for(int y=0;y<height;y++)for(int x=0;x<width;x++){
   int sum=0;for(int k=-3;k<=3;k++)if(y+k>=0&&y+k<height)sum+=temp[(y+k)*width+x];
   AcidTextGlow[y*width+x]=(unsigned char)(sum/7);
  }
  AcidTextAtlas=atlas;AcidTextWidth=width;snprintf(AcidTextCached,sizeof AcidTextCached,"%s",name);
 }
 if(SDL_MUSTLOCK(surface)&&!SDL_LockSurface(surface))return;
 int strength=32768+alpha/2;if(strength>65536)strength=65536;
 strength=MUL_FIXED(strength,MenuGlowScale(alpha,80,90));
 for(int y=0;y<height;y++)for(int x=0;x<width;x++){
  int sx=centreX-width/2+x,sy=top-6+y;
  if(sx<0||sy<0||sx>=surface->w||sy>=surface->h)continue;
  int core=MUL_FIXED(strength,AcidTextMask[y*width+x]),glow=MUL_FIXED(strength,AcidTextGlow[y*width+x]);
  if(!core&&!glow)continue;
  unsigned short *dst=(unsigned short*)((unsigned char*)surface->pixels+sy*surface->pitch)+sx;
  int r=((*dst>>11)&31)*255/31+(core+2*glow)*165/255;
  int g=((*dst>>5)&63)*255/63+(core+2*glow)*204/255;
  int b=(*dst&31)*255/31+(core+2*glow)*118/255;
  if(r>255)r=255;if(g>255)g=255;if(b>255)b=255;
  *dst=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
 }
 if(SDL_MUSTLOCK(surface))SDL_UnlockSurface(surface);
}

int LengthOfSmallMenuText(char *textPtr)
{
	return textPtr ? AvPTextStringWidth(textPtr, AAFontWidths) : 0;
}

/* The large title atlas is ASCII-only as well.  This runs while the caller
   owns the surface lock and paints a mark over the already-rendered base. */
static int MenuAVContrast;
/* Coverage compositing is scoped to the selected quick-load row only. */
static int MenuQuickLoadAlpha = -1;
static void MenuContrastPixel(int x,int y,int coverage,int r,int g,int b);
static void RenderLargeMenuAccent(D3DTexture *image, unsigned char c, int sx, int sy, int alpha, int clipped, int topY, int bottomY)
{
	unsigned char *srcPtr;
	unsigned short *destPtr;
	unsigned int x, y;
	unsigned int width;
	unsigned int remainder = 0;
	unsigned int stride;
	unsigned int topLeftV;

	if (!c || c < ' ' || c > '~') return;
	width = IntroFont_Light.FontWidth[c];
	topLeftV = 1 + (c - 32) * 33;
	stride = width;
	if (image->w > width) remainder = image->w - width; else stride = image->w;
	srcPtr = &image->buf[topLeftV * image->w * 4];
	for (y = sy; y < sy + 33; ++y) {
		if (!clipped || (y >= (unsigned int)topY && y <= (unsigned int)bottomY)) {
			destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y * surface->pitch) + sx;
			for (x = stride; x > 0; --x) {
				if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
					unsigned int destR = (*destPtr & 0xF800) >> 8;
					unsigned int destG = (*destPtr & 0x07E0) >> 3;
					unsigned int destB = (*destPtr & 0x001F) << 3;
					int r = MUL_FIXED(alpha, CloudTable[(x + sx + CloakingPhase / 64) & 127][(y + CloakingPhase / 128) & 127]);
					destR += MUL_FIXED(r, srcPtr[0]); destG += MUL_FIXED(r, srcPtr[1]); destB += MUL_FIXED(r, srcPtr[2]);
					if (destR > 255) destR = 255; if (destG > 255) destG = 255; if (destB > 255) destB = 255;
					*destPtr = ((destR >> 3) << 11) | ((destG >> 2) << 5) | (destB >> 3);
				}
				srcPtr += 4; ++destPtr;
			}
			srcPtr += remainder * 4;
		} else {
			srcPtr += image->w * 4;
		}
	}
}
int RenderMenuText(char *textPtr, int sx, int sy, int alpha, enum AVPMENUFORMAT_ID format)
{
	int width;
	
	width = LengthOfMenuText(textPtr);
	
	switch(format) {
		default:
			GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			return 0;
		case AVPMENUFORMAT_LEFTJUSTIFIED:
			break;
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
			sx -= width;
			break;
		case AVPMENUFORMAT_CENTREJUSTIFIED:
			sx -= width / 2;
			break;
	}
	
	LOCALASSERT(x>0);
	
	if (alpha >BRIGHTNESS_OF_DARKENED_ELEMENT) {
		int size = width - 18;
		if (size<18) size = 18;
		
		DrawAvPMenuGfx(AVPMENUGFX_GLOWY_LEFT,sx+18,sy-8,alpha,AVPMENUFORMAT_RIGHTJUSTIFIED);
		DrawAvPMenuGlowyBar(sx+18,sy-8,alpha,size-18);
		DrawAvPMenuGfx(AVPMENUGFX_GLOWY_RIGHT,sx+size,sy-8,alpha,AVPMENUFORMAT_LEFTJUSTIFIED);
	}
{
	unsigned char *srcPtr;
	unsigned short *destPtr;
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	
	gfxPtr = &IntroFont_Light.info;
	image = gfxPtr->ImagePtr;
	
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return 0; /* ... */
		}
	}
	
	while( *textPtr ) {
		unsigned char accent = 0;
		unsigned char c = AvPTextNextGlyphWithAccent(&textPtr, &accent);

		if (c>=' ') {
			unsigned int topLeftU = 0;
			unsigned int topLeftV = 1+(c-32)*33;
			unsigned int x, y;
			unsigned int width = IntroFont_Light.FontWidth[(unsigned char) c];
			unsigned int remainder = 0;
			unsigned int stride = width;

			if (image->w > width) {
				remainder = image->w - width;
			} else {
				stride = image->w;
			}

			srcPtr = &image->buf[(topLeftU+topLeftV*image->w)*4];

			for (y=sy; y<33+sy; y++) {
				destPtr = (unsigned short *)(((unsigned char *)surface->pixels)+y*surface->pitch) + sx;
				
				for (x=stride; x>0; x--) {
					if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
						unsigned int destR, destG, destB;
						
						int r = CloudTable[(x+sx+CloakingPhase/64)&127][(y+CloakingPhase/128)&127];
						r = MUL_FIXED(alpha, r);
						
						destR = (*destPtr & 0xF800)>>8;
						destG = (*destPtr & 0x07E0)>>3;
						destB = (*destPtr & 0x001F)<<3;
						
						destR += MUL_FIXED(r, srcPtr[0]);
						destG += MUL_FIXED(r, srcPtr[1]);
						destB += MUL_FIXED(r, srcPtr[2]);
						if (destR > 0x00FF) destR = 0x00FF;
						if (destG > 0x00FF) destG = 0x00FF;
						if (destB > 0x00FF) destB = 0x00FF;
						
						*destPtr =	((destR>>3)<<11) |
								((destG>>2)<<5 ) |
								((destB>>3));
					}
					srcPtr += 4;
					destPtr++;
				}
				srcPtr += remainder * 4;
			}
			if (accent) RenderLargeMenuAccent(image, accent, sx + ((int)width - (int)IntroFont_Light.FontWidth[accent]) / 2, sy, alpha, 0, 0, 0);
			sx += width;
		}
	}

	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
	
	return sx;
}
	
}

int RenderMenuText_Clipped(char *textPtr, int sx, int sy, int alpha, enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{	
	int width = LengthOfMenuText(textPtr);
	
	switch(format) {
		default:
			GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			return 0;
		case AVPMENUFORMAT_LEFTJUSTIFIED:
			break;
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
			sx -= width;
			break;
		case AVPMENUFORMAT_CENTREJUSTIFIED:
			sx -= width / 2;
			break;
	}
	
	LOCALASSERT(x>0);
	
	if (alpha > BRIGHTNESS_OF_DARKENED_ELEMENT) {
		int size = width - 18;
		if (size<18) size = 18;
		
		DrawAvPMenuGfx_Clipped(AVPMENUGFX_GLOWY_LEFT,sx+18,sy-8,alpha,AVPMENUFORMAT_RIGHTJUSTIFIED,topY,bottomY);
		DrawAvPMenuGlowyBar_Clipped(sx+18,sy-8,alpha,size-18,topY,bottomY);
		DrawAvPMenuGfx_Clipped(AVPMENUGFX_GLOWY_RIGHT,sx+size,sy-8,alpha,AVPMENUFORMAT_LEFTJUSTIFIED,topY,bottomY);
	}
{
	unsigned char *srcPtr;
	unsigned short *destPtr;
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	
	gfxPtr = &IntroFont_Light.info;
	image = gfxPtr->ImagePtr;
	
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return 0; /* ... */
		}
	}
	
	while( *textPtr ) {
		unsigned char accent = 0;
		unsigned char c = AvPTextNextGlyphWithAccent(&textPtr, &accent);

		if (c>=' ') {
			unsigned int topLeftU = 0;
			unsigned int topLeftV = 1+(c-32)*33;
			unsigned int x, y;
			unsigned int width = IntroFont_Light.FontWidth[(unsigned char) c];
			unsigned int remainder = 0;
			unsigned int stride = width;

			if (image->w > width) {
				remainder = image->w - width;
			} else {
				stride = image->w;
			}

			srcPtr = &image->buf[(topLeftU+topLeftV*image->w)*4];

			for (y=sy; y<33+sy; y++) {
				if(y>=topY && y<=bottomY) {
					destPtr = (unsigned short *)(((unsigned char *)surface->pixels)+y*surface->pitch) + sx;
				
					for (x=stride; x>0; x--) {
						if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
							unsigned int destR, destG, destB;
						
							int r = CloudTable[(x+sx+CloakingPhase/64)&127][(y+CloakingPhase/128)&127];
							r = MUL_FIXED(alpha, r);
							
							destR = (*destPtr & 0xF800)>>8;
							destG = (*destPtr & 0x07E0)>>3;
							destB = (*destPtr & 0x001F)<<3;
						
							destR += MUL_FIXED(r, srcPtr[0]);
							destG += MUL_FIXED(r, srcPtr[1]);
							destB += MUL_FIXED(r, srcPtr[2]);
							if (destR > 0x00FF) destR = 0x00FF;
							if (destG > 0x00FF) destG = 0x00FF;
							if (destB > 0x00FF) destB = 0x00FF;
						
							*destPtr =	((destR>>3)<<11) |
									((destG>>2)<<5 ) |
									((destB>>3));
						}
						srcPtr += 4;
						destPtr++;
					}
					srcPtr += remainder * 4;
				} else {
					srcPtr += image->w * 4;
				}
			}
			if (accent) RenderLargeMenuAccent(image, accent, sx + ((int)width - (int)IntroFont_Light.FontWidth[accent]) / 2, sy, alpha, 1, topY, bottomY);
			sx += width;
		}
	}

	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
	
	return sx;
}

}

/* aa_font.rim has the ASCII page only. Compose Western-Latin accents from an
   existing ASCII mark without consuming an extra layout cell. */
/* Scoped to main-menu AV controls/help; titles and all other menus use the
 * original renderer. 1=idle ivory, 2=selected amber, 3=help ivory. */

void SetMenuAVContrast(int mode){MenuAVContrast=mode;}
static void MenuContrastPixel(int x,int y,int coverage,int r,int g,int b){
 if(x<0||y<0||x>=surface->w||y>=surface->h)return;
 unsigned short *p=(unsigned short*)((unsigned char*)surface->pixels+y*surface->pitch)+x;
 int dr=(*p>>11)*255/31,dg=((*p>>5)&63)*255/63,db=(*p&31)*255/31;
 r=(r*coverage+dr*(255-coverage)+127)/255;
 g=(g*coverage+dg*(255-coverage)+127)/255;
 b=(b*coverage+db*(255-coverage)+127)/255;
 *p=((r>>3)<<11)|((g>>2)<<5)|(b>>3);
}
static void MenuContrastMask(D3DTexture *image,int u,int v,int w,int h,int sx,int sy,int slider){
 int r=MenuAVContrast==2?255:232,g=MenuAVContrast==2?202:240,b=MenuAVContrast==2?72:245;
 /* Draw the complete outline before the foreground, retaining antialiased edges. */
 for(int pass=0;pass<2;pass++)for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  const unsigned char *p=(unsigned char*)image->buf+4*((v+y)*image->w+u+x);
  int a=p[0];if(p[1]>a)a=p[1];if(p[2]>a)a=p[2];if(slider){a*=4;if(a>255)a=255;}
  if(!a)continue;
  if(pass)MenuContrastPixel(sx+x,sy+y,a,r,g,b);
  else for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)MenuContrastPixel(sx+x+dx,sy+y+dy,a,0,0,0);
 }
}

static void RenderSmallFontGlyph(D3DTexture *image, unsigned char c, int sx, int sy, int alphaR, int alphaG, int alphaB)
{
	unsigned char *srcPtr;
	unsigned short *destPtr;
	int topLeftU, topLeftV, x, y;
	if (c < ' ' || c > 'z') return;
	topLeftU = 1 + ((c - 32) & 15) * 16;
	topLeftV = 1 + ((c - 32) >> 4) * 16;
 if(MenuAVContrast){MenuContrastMask(image,topLeftU,topLeftV,HUD_FONT_WIDTH,HUD_FONT_HEIGHT,sx,sy,0);return;}
	srcPtr = &image->buf[(topLeftU + topLeftV * image->w) * 4];
	for (y = sy; y < HUD_FONT_HEIGHT + sy; y++) {
		destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y * surface->pitch) + sx;
		for (x = 0; x < HUD_FONT_WIDTH; x++) {
			if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
                if (MenuQuickLoadAlpha >= 0) {
                    int coverage=srcPtr[0];
                    if(srcPtr[1]>coverage)coverage=srcPtr[1];
                    if(srcPtr[2]>coverage)coverage=srcPtr[2];
                    coverage=MUL_FIXED(coverage,MenuQuickLoadAlpha);
                    if(coverage>255)coverage=255;
                    MenuContrastPixel(sx+x,y,coverage,178,24,240);
                    srcPtr+=4;destPtr++;continue;
                }
				unsigned int destR = (*destPtr & 0xF800) >> 8;
				unsigned int destG = (*destPtr & 0x07E0) >> 3;
				unsigned int destB = (*destPtr & 0x001F) << 3;
				destR += MUL_FIXED(alphaR, srcPtr[0]); destG += MUL_FIXED(alphaG, srcPtr[1]); destB += MUL_FIXED(alphaB, srcPtr[2]);
				if (destR > 0x00FF) destR = 0x00FF; if (destG > 0x00FF) destG = 0x00FF; if (destB > 0x00FF) destB = 0x00FF;
				*destPtr = ((destR >> 3) << 11) | ((destG >> 2) << 5) | (destB >> 3);
			}
			srcPtr += 4; destPtr++;
		}
		srcPtr += (image->w - HUD_FONT_WIDTH) * 4;
	}
}
static int RenderSmallFontString(char *textPtr,int sx,int sy,int alpha, int red, int green, int blue)
{
	int alphaR = MUL_FIXED(alpha,red);
	int alphaG = MUL_FIXED(alpha,green);
	int alphaB = MUL_FIXED(alpha,blue);
	D3DTexture *image = AvPMenuGfxStorage[AVPMENUGFX_SMALL_FONT].ImagePtr;
	if (SDL_MUSTLOCK(surface) && SDL_LockSurface(surface) < 0) return 0;
	while (*textPtr) {
		unsigned char accent = 0;
		unsigned char c = AvPTextNextGlyphWithAccent(&textPtr, &accent);
		if (c >= ' ') {
			int glyphWidth = AAFontWidths[c];
			RenderSmallFontGlyph(image, c, sx, sy, alphaR, alphaG, alphaB);
			if (accent) RenderSmallFontGlyph(image, accent, sx + (glyphWidth - AAFontWidths[accent]) / 2, sy, alphaR, alphaG, alphaB);
			sx += glyphWidth;
		}
	}
	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
	return sx;
}
void RenderSmallFontString_Wrapped(const char *textPtr,RECT* area,int alpha,int* output_x,int* output_y)
{
	unsigned char *srcPtr;
	unsigned short *destPtr;
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	int wordWidth;
	int sx=area->left;
	int sy=area->top;
	
	gfxPtr = &AvPMenuGfxStorage[AVPMENUGFX_SMALL_FONT];
	image = gfxPtr->ImagePtr;

/*
Determine area used by text , so we can draw it centrally
*/                        	
{
	const char *textPtr2=textPtr;
	while (*textPtr2) {
		int widthFromSpaces=0;
		int widthFromChars=0;
		
		while(*textPtr2 && *textPtr2==' ') {
			widthFromSpaces+=AAFontWidths[AvPTextNextGlyphConst(&textPtr2)];
		}
		
		while(*textPtr2 && *textPtr2!=' ') {
			widthFromChars+=AAFontWidths[AvPTextNextGlyphConst(&textPtr2)];
		}
		
		wordWidth=widthFromSpaces+widthFromChars;
		
		if(wordWidth> area->right-sx) {
			if(wordWidth >area->right-area->left) {
				int extraLinesNeeded=0;
				
				wordWidth-=(area->right-sx);
				
				sy+=HUD_FONT_HEIGHT;
				sx=area->left;
				
				extraLinesNeeded=wordWidth/(area->right-area->left);
				
				sy+=HUD_FONT_HEIGHT*extraLinesNeeded;
				wordWidth %= (area->right-area->left);
				
				if(sy+HUD_FONT_HEIGHT> area->bottom) break;
			} else {
				sy+=HUD_FONT_HEIGHT;
				sx=area->left;
				
				if(sy+HUD_FONT_HEIGHT> area->bottom) break;
				
				if(wordWidth> area->right-sx) break;
				
				wordWidth-=widthFromSpaces;
			}
		}
		sx+=wordWidth;
	}
	
	if(sy==area->top) {
		sx=area->left+ (area->right-sx)/2;
	} else {
		sx=area->left;
	}
	
	sy+=HUD_FONT_HEIGHT;
	if(sy<area->bottom) {
		sy=area->top + (area->bottom-sy)/2;
	} else {
		sy=area->top;
	}
}

	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	while ( *textPtr ) {
		const char* textPtr2=textPtr;
		wordWidth=0;
		
		while(*textPtr2 && *textPtr2==' ') {
			wordWidth+=AAFontWidths[AvPTextNextGlyphConst(&textPtr2)];
		}
		
		while(*textPtr2 && *textPtr2!=' ') {
			wordWidth+=AAFontWidths[AvPTextNextGlyphConst(&textPtr2)];
		}
		
		if(wordWidth> area->right-sx) {
			if(wordWidth>area->right - area->left) {
				/* 
				  word is too long too fit on one line 
				  so we'll just have to allow it to be split
				 */
			} else {
				sy+=HUD_FONT_HEIGHT;
				sx=area->left;
				
				if(sy+HUD_FONT_HEIGHT> area->bottom) break;
				
				if(wordWidth> area->right-sx) break;
				
				while(*textPtr && *textPtr==' ') {
					textPtr++;
				}
			}
		}
		
		while(*textPtr && *textPtr==' ') {
			sx+=AAFontWidths[AvPTextNextGlyphConst((const char **)&textPtr)];
		}
		
		if(sx>area->right) {
			while(sx>area->right) {
				sx-=(area->right-area->left);
				sy+=HUD_FONT_HEIGHT;
			}
			
			if(sy+HUD_FONT_HEIGHT> area->bottom) break;
		}
		
		while(*textPtr && *textPtr!=' ') {
			unsigned char accent = 0;
			unsigned char c = AvPTextNextGlyphConstWithAccent(&textPtr, &accent);
			int letterWidth = AAFontWidths[c];
			
			if(sx+letterWidth>area->right) {
				sx=area->left;
				sy+=HUD_FONT_HEIGHT;
				
				if(sy+HUD_FONT_HEIGHT> area->bottom) break;
			}
			
			if (c >= ' ') {
				RenderSmallFontGlyph(image, c, sx, sy, alpha, alpha, alpha);
				if (accent) RenderSmallFontGlyph(image, accent, sx + (letterWidth - AAFontWidths[accent]) / 2, sy, alpha, alpha, alpha);
				sx += letterWidth;
			}
		}
	}
	
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
	
	if(output_x) *output_x=sx;
	if(output_y) *output_y=sy;
}

int RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format)
{
	int length;
	char *ptr;
	
	switch(format) {
		default:
			GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			return 0;
		case AVPMENUFORMAT_LEFTJUSTIFIED:
			break;
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
			length = LengthOfSmallMenuText(textPtr);
			
			x -= length;
			break;
		case AVPMENUFORMAT_CENTREJUSTIFIED:
			length = LengthOfSmallMenuText(textPtr);
			
			x -= length / 2;
			break;
	}
	
	LOCALASSERT(x>0);
	
	return RenderSmallFontString(textPtr,x,y,alpha,ONE_FIXED,ONE_FIXED,ONE_FIXED);
}

int RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue)
{
	int length;
	char *ptr;
	
	switch(format) {
		default:
			GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			return 0;
		case AVPMENUFORMAT_LEFTJUSTIFIED:
			break;
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
			length = LengthOfSmallMenuText(textPtr);
			
			x -= length;
			break;
		case AVPMENUFORMAT_CENTREJUSTIFIED:
			length = LengthOfSmallMenuText(textPtr);
			
			x -= length / 2;
			break;
	}
	
	LOCALASSERT(x>0);
	
	return RenderSmallFontString(textPtr,x,y,alpha,red,green,blue);			
}

int RenderSmallMenuText_QuickLoad(char *text,int x,int y,int alpha,enum AVPMENUFORMAT_ID format)
{
 int saved=MenuQuickLoadAlpha;
 MenuQuickLoadAlpha=alpha<0?0:alpha;
 int result=RenderSmallMenuText_Coloured(text,x,y,alpha,format,ONE_FIXED,ONE_FIXED/8,ONE_FIXED/8);
 MenuQuickLoadAlpha=saved;
 return result;
}

void RenderCampaignModeText(char *text,int x,int y,int alpha,int mode)
{
 static const int colors[3][3]={{90,220,255},{70,110,255},{85,255,105}};
 if(mode<0 || mode>2)mode=0;
 int rgb[3];for(int i=0;i<3;i++)rgb[i]=colors[mode][i]*ONE_FIXED/255;
 int strength=32768+alpha/2;
 if(strength>65536)strength=65536;
 strength=MUL_FIXED(strength,MenuGlowScale(alpha,100,95));
 for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
  int distance=abs(dx)+abs(dy);if(distance<1 || distance>3)continue;
  RenderSmallMenuText_Coloured(text,x+dx,y+dy,strength/(10+distance*5),AVPMENUFORMAT_CENTREJUSTIFIED,rgb[0],rgb[1],rgb[2]);
 }
 RenderSmallMenuText_Coloured(text,x,y,strength,AVPMENUFORMAT_CENTREJUSTIFIED,rgb[0],rgb[1],rgb[2]);
}

void RenderOptimizedAmmoText(char *text,int x,int y,int alpha,int mode)
{
 static const int colors[2][3]={{190,145,28},{180,42,78}};
 if(mode<0 || mode>1)mode=0;
 int rgb[3];for(int i=0;i<3;i++)rgb[i]=colors[mode][i]*ONE_FIXED/255;
 int strength=32768+alpha/2;
 if(strength>65536)strength=65536;
 strength=MUL_FIXED(strength,MenuGlowScale(alpha,100,100));
 for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
  int distance=abs(dx)+abs(dy);if(distance<1 || distance>3)continue;
  RenderSmallMenuText_Coloured(text,x+dx,y+dy,strength/(10+distance*5),AVPMENUFORMAT_CENTREJUSTIFIED,rgb[0],rgb[1],rgb[2]);
 }
 RenderSmallMenuText_Coloured(text,x,y,strength,AVPMENUFORMAT_CENTREJUSTIFIED,rgb[0],rgb[1],rgb[2]);
}

static void CalculateWidthsOfAAFont()
{
	unsigned char *srcPtr;
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	int c;
	
	gfxPtr = &AvPMenuGfxStorage[AVPMENUGFX_SMALL_FONT];
	image = gfxPtr->ImagePtr;
	
	srcPtr = image->buf;
	
	AAFontWidths[32]=3;
	
	for (c=33; c<255; c++) {
		int x,y;
		int x1 = 1+((c-32)&15)*16;
		int y1 = 1+((c-32)>>4)*16;
		
		AAFontWidths[c]=17;
		
		for (x=x1+HUD_FONT_WIDTH; x>x1; x--) {
			int blank = 1;
			
			for (y=y1; y<y1+HUD_FONT_HEIGHT; y++) {
				unsigned char *s = &srcPtr[(x + y*image->w) * 4];
				if (s[2] >= 0x80) {
					blank = 0;
					break;
				}
			}
			
			if (blank) {
				AAFontWidths[c]--;
			} else {
				break;
			}
		}
	}
}

void RenderKeyConfigRectangle(int alpha)
{
	int x1 = 10;
	int x2 = ScreenDescriptorBlock.SDB_Width-10;
	int y1 = ScreenDescriptorBlock.SDB_Height/2+25-115;
	int y2 = ScreenDescriptorBlock.SDB_Height/2+25-115+250;
	int x,y;
	unsigned short c, *destPtr;
	
	c =	((MUL_FIXED(0xFF,alpha)>>3)<<11) |
		((MUL_FIXED(0xFF,alpha)>>2)<<5 ) |
		((MUL_FIXED(0xFF,alpha)>>3));
	
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	y = y1;
	destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch) + x1;
	for (x=x1; x<=x2; x++) {
		*destPtr |= c;
		destPtr++;
	}
	
	y = y2;
	destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch) + x1;
	for (x=x1; x<=x2; x++) {
		*destPtr |= c;
		destPtr++;
	}
	
	for (y=y1+1; y<y2; y++) {
		destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch) + x1;
		*destPtr |= c;
	}
	
	for (y=y1+1; y<y2; y++) {
		destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch) + x2;
		*destPtr |= c;
	}
	
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

void RenderHighlightRectangle(int x1, int y1, int x2, int y2, int r, int g, int b)
{
	int x, y;
	unsigned short c;

	c = 	((r>>3)<<11) |
		((g>>2)<<5 ) |
		((b>>3));

	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}

	for (y=y1; y<=y2; y++) {
		unsigned short *destPtr = (unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch) + x1;
		
		for (x = x1; x <= x2; x++) {
			*destPtr |= c;
			
			destPtr++;
		}
	}
	
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

void LoadAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	AVPMENUGFX *gfxPtr;
	char buffer[100];
	size_t fastFileLength;
	void const *pFastFileData;
	
	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
		
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	
	CL_GetImageFileName(buffer, 100, gfxPtr->FilenamePtr, LIO_RELATIVEPATH);

	pFastFileData = ffreadbuf(buffer, &fastFileLength);
	
	if (pFastFileData) {
		gfxPtr->ImagePtr = AwCreateSurface(
			"pxfXY",
			pFastFileData,
			fastFileLength,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	} else {
		gfxPtr->ImagePtr = AwCreateSurface(
			"sfXY",
			buffer,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	}
	
	GLOBALASSERT(gfxPtr->ImagePtr);
	GLOBALASSERT(gfxPtr->Width>0);
	GLOBALASSERT(gfxPtr->Height>0);
	
	gfxPtr->hBackup = 0;
}

static void ReleaseAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	AVPMENUGFX *gfxPtr;
	
	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
	
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	
	GLOBALASSERT(gfxPtr);
	GLOBALASSERT(gfxPtr->ImagePtr);
	
	ReleaseDDSurface(gfxPtr->ImagePtr);
	
	gfxPtr->ImagePtr = NULL;
}

void LoadAllAvPMenuGfx()
{
	int i;
	
	for (i = 0; i < AVPMENUGFX_WINNER_SCREEN; i++) {
		LoadAvPMenuGfx(i);
	}
	
	LoadMenuFont();
{
	unsigned char *srcPtr;
	AVPMENUGFX *gfxPtr = &AvPMenuGfxStorage[AVPMENUGFX_CLOUDY];
	D3DTexture *image;
	
	int x, y;
	
	image = gfxPtr->ImagePtr;
	srcPtr = image->buf;
	
	for (y=0; y<gfxPtr->Height; y++) {
		for (x=0; x<gfxPtr->Width; x++) {
			
			int r = srcPtr[0];
			
			r = DIV_FIXED(r, 0xFF);
			CloudTable[x][y]=r;
			
			srcPtr += 4;
		}
	}
}

	CalculateWidthsOfAAFont();
}
	
void LoadAllSplashScreenGfx()
{
	int i;
	
	for (i = AVPMENUGFX_SPLASH_SCREEN1; i < MAX_NO_OF_AVPMENUGFXS; i++) {
		LoadAvPMenuGfx(i);
	}
}

void InitialiseMenuGfx()
{
	int i;
	
	for (i = 0; i < MAX_NO_OF_AVPMENUGFXS; i++) {
		AvPMenuGfxStorage[i].ImagePtr = NULL;
	}	
}

int MenuBackgroundMode=MENU_BG_LETTERBOXED; /* Only background framing; UI canvas is independent. */
static D3DTexture *SettingsBackdrop;
static int SettingsBackdropAttempted,SettingsBackdropWidth,SettingsBackdropHeight;
int DrawSettingsMenuBackdrop(int biomech)
{
 D3DTexture *image=(D3DTexture*)AvPMenuGfxStorage[AVPMENUGFX_BACKDROP].ImagePtr;
 int iw=AvPMenuGfxStorage[AVPMENUGFX_BACKDROP].Width,ih=AvPMenuGfxStorage[AVPMENUGFX_BACKDROP].Height;
 if(biomech && !SettingsBackdropAttempted){
  size_t length=0;const void *data;char name[100];int w=0,h=0;
  SettingsBackdropAttempted=1;
  CL_GetImageFileName(name,sizeof name,"Menus\\biomechmenu.RIM",LIO_RELATIVEPATH);
  data=ffreadbuf(name,&length);
  if(data)SettingsBackdrop=AwCreateSurface("pxfXY",data,length,0,&w,&h);
  else { FILE *f=fopen(name,"rb");if(f){fclose(f);SettingsBackdrop=AwCreateSurface("sfXY",name,0,&w,&h);} }
  SettingsBackdropWidth=w;SettingsBackdropHeight=h;
 }
 if(biomech && SettingsBackdrop){image=SettingsBackdrop;iw=SettingsBackdropWidth;ih=SettingsBackdropHeight;}
 if(!surface||!image||!image->buf||iw<=0||ih<=0)return 0;
 if(SDL_MUSTLOCK(surface)&&!SDL_LockSurface(surface))return 0;
 /* RIM and Bink use the same fill/letterbox/center mapping.
  * Authored bands are removed only for fullscreen; UI scale stays separate. */
 MenuBackdropMapping map=MenuBackdropMap(surface->w,surface->h,iw,ih,MenuBackgroundMode);
 int sourceTop=map.top;ih=map.height;
 double scaleX=map.scaleX,scaleY=map.scaleY,ox=map.ox,oy=map.oy;
 for(int y=0;y<surface->h;y++){
  unsigned short *dst=(unsigned short*)((unsigned char*)surface->pixels+y*surface->pitch);
  double sourceY=oy+(y+.5)/scaleY;int sy=(int)sourceY;
  for(int x=0;x<surface->w;x++){
   double sourceX=ox+(x+.5)/scaleX;int sx=(int)sourceX;
   if(sourceX<0||sourceY<0||sx>=iw||sy>=ih){dst[x]=0;continue;}
   const unsigned char *rgb=(const unsigned char*)image->buf+4*((sy+sourceTop)*image->w+sx);
   dst[x]=((rgb[0]>>3)<<11)|((rgb[1]>>2)<<5)|(rgb[2]>>3);
  }
 }
 if(SDL_MUSTLOCK(surface))SDL_UnlockSurface(surface);
 return 1;
}

void ReleaseAllAvPMenuGfx()
{
 if(SettingsBackdrop)ReleaseDDSurface(SettingsBackdrop);
 SettingsBackdrop=NULL;SettingsBackdropAttempted=0;
	int i;
	
	for (i = 0; i < MAX_NO_OF_AVPMENUGFXS; i++) {
		if (AvPMenuGfxStorage[i].ImagePtr) {
			ReleaseAvPMenuGfx(i);
		}
	}

	UnloadMenuFont();	
}

void DrawAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	unsigned char *srcPtr;
	unsigned short *destPtr;
	int length;

	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	image = gfxPtr->ImagePtr;
	
	switch(format) {
		default:
			GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			return;
		case AVPMENUFORMAT_LEFTJUSTIFIED:
			break;
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
			topleftX -= gfxPtr->Width;
			break;
		case AVPMENUFORMAT_CENTREJUSTIFIED:
			topleftX -= gfxPtr->Width/2;
			break;
	}
	
 if(MenuAVContrast&&(menuGfxID==AVPMENUGFX_SLIDERBAR||menuGfxID==AVPMENUGFX_SLIDER)){
  if(SDL_MUSTLOCK(surface)&&!SDL_LockSurface(surface))return;
  MenuContrastMask(image,0,0,gfxPtr->Width,gfxPtr->Height,topleftX,topleftY,1);
  if(SDL_MUSTLOCK(surface))SDL_UnlockSurface(surface);return;
 }
	srcPtr = (unsigned char *)image->buf;
	length = gfxPtr->Width;
	
	if (ScreenDescriptorBlock.SDB_Width - topleftX < length) {
		length = ScreenDescriptorBlock.SDB_Width - topleftX;
	}
	if (length <= 0) return;
			
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	if (alpha > ONE_FIXED) {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch)) + topleftX;
			
			for (x=0; x<length; x++) {
				*destPtr =	((srcPtr[0]>>3)<<11) |
						((srcPtr[1]>>2)<<5 ) |
						((srcPtr[2]>>3));
				srcPtr += 4;
				destPtr++;
			}
			
			srcPtr += (image->w - length) * 4;
		}
	} else {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch)) + topleftX;
			
			for (x=0; x<length; x++) {
				if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
					unsigned int destR, destG, destB;
						
					destR = (*destPtr & 0xF800)>>8;
					destG = (*destPtr & 0x07E0)>>3;
					destB = (*destPtr & 0x001F)<<3;
						
					destR += MUL_FIXED(alpha, srcPtr[0]);
					destG += MUL_FIXED(alpha, srcPtr[1]);
					destB += MUL_FIXED(alpha, srcPtr[2]);
					if (destR > 0x00FF) destR = 0x00FF;
					if (destG > 0x00FF) destG = 0x00FF;
					if (destB > 0x00FF) destB = 0x00FF;
						
					*destPtr =	((destR>>3)<<11) |
							((destG>>2)<<5 ) |
							((destB>>3));
				}
				
				srcPtr += 4;
				destPtr++;
			}
			
			srcPtr += (image->w - length) * 4;
		}
	}
		
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

void DrawAvPMenuGfx_CrossFade(enum AVPMENUGFX_ID menuGfxID,enum AVPMENUGFX_ID menuGfxID2,int alpha)
{
	AVPMENUGFX *gfxPtr, *gfxPtr2;
	D3DTexture *image, *image2;
	unsigned char *srcPtr, *srcPtr2;
	unsigned short *destPtr;
	int length;
	
	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
	GLOBALASSERT(menuGfxID2 < MAX_NO_OF_AVPMENUGFXS);
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	image = gfxPtr->ImagePtr;
	gfxPtr2 = &AvPMenuGfxStorage[menuGfxID2];
	image2 = gfxPtr2->ImagePtr;

	srcPtr = image->buf;
	srcPtr2 = image2->buf;
	
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	length = 640;
	
	if (alpha == ONE_FIXED) {
		int x, y;
		
		for (y=0; y<480; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch));
			
			for (x=0; x<640; x++) {
				*destPtr =	((srcPtr[0]>>3)<<11) |
						((srcPtr[1]>>2)<<5 ) |
						((srcPtr[2]>>3));
				srcPtr += 4;
				destPtr++;
			}
			srcPtr += (image->w - length) * 4;
		}
	} else {
		int x, y;
		
		for (y=0; y<480; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch));
			
			for (x=0; x<640; x++) {
				unsigned int srcR1, srcR2;
				unsigned int srcG1, srcG2;
				unsigned int srcB1, srcB2;
				
				srcR1 = srcPtr[0];
				srcR2 = srcPtr2[0];
				srcG1 = srcPtr[1];
				srcG2 = srcPtr2[1];
				srcB1 = srcPtr[2];
				srcB2 = srcPtr2[2];
				
				srcR2 = MUL_FIXED(ONE_FIXED-alpha,srcR2)+MUL_FIXED(alpha,srcR1);
				srcG2 = MUL_FIXED(ONE_FIXED-alpha,srcG2)+MUL_FIXED(alpha,srcG1);
				srcB2 = MUL_FIXED(ONE_FIXED-alpha,srcB2)+MUL_FIXED(alpha,srcB1);
				
				*destPtr =	((srcR2>>3)<<11) |
						((srcG2>>2)<<5 ) |
						((srcB2>>3));
				srcPtr += 4;
				srcPtr2 += 4;
				destPtr++;
			}
		}
	}
	
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
	
}

void DrawAvPMenuGfx_Faded(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	unsigned char *srcPtr;
	unsigned short *destPtr;
	int length;

	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	image = gfxPtr->ImagePtr;
	
	switch(format) {
		default:
			GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			return;
		case AVPMENUFORMAT_LEFTJUSTIFIED:
			break;
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
			topleftX -= gfxPtr->Width;
			break;
		case AVPMENUFORMAT_CENTREJUSTIFIED:
			topleftX -= gfxPtr->Width/2;
			break;
	}
	
	srcPtr = (unsigned char *)image->buf;
	length = gfxPtr->Width;
	
	if (ScreenDescriptorBlock.SDB_Width - topleftX < length) {
		length = ScreenDescriptorBlock.SDB_Width - topleftX;
	}
	if (length <= 0) return;
			
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	{
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch)) + topleftX;
			
			for (x=0; x<length; x++) {
				unsigned int srcR,srcG,srcB;
				
				if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
					srcR = MUL_FIXED(srcPtr[0], alpha);
					srcG = MUL_FIXED(srcPtr[1], alpha);
					srcB = MUL_FIXED(srcPtr[2], alpha);
					*destPtr =	((srcR>>3)<<11) |
							((srcG>>2)<<5 ) |
							((srcB>>3));
				}
				
				srcPtr += 4;
				destPtr++;
			}
			
			srcPtr += (image->w - length) * 4;
		}
	}
		
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

void DrawAvPMenuGfx_Clipped(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{
	AVPMENUGFX *gfxPtr;
	D3DTexture *image;
	unsigned char *srcPtr;
	unsigned short *destPtr;
	int length;

	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	image = gfxPtr->ImagePtr;
	
	switch(format) {
		default:
			GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
			return;
		case AVPMENUFORMAT_LEFTJUSTIFIED:
			break;
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
			topleftX -= gfxPtr->Width;
			break;
		case AVPMENUFORMAT_CENTREJUSTIFIED:
			topleftX -= gfxPtr->Width/2;
			break;
	}
	
	srcPtr = (unsigned char *)image->buf;
	length = gfxPtr->Width;
	
	if (ScreenDescriptorBlock.SDB_Width - topleftX < length) {
		length = ScreenDescriptorBlock.SDB_Width - topleftX;
	}
	if (length <= 0) return;
			
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	if (alpha > ONE_FIXED) {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch)) + topleftX;
			
			if(y>=topY && y<=bottomY) {
				for (x=0; x<length; x++) {
					*destPtr =	((srcPtr[0]>>3)<<11) |
							((srcPtr[1]>>2)<<5 ) |
							((srcPtr[2]>>3));
					srcPtr += 4;
					destPtr++;
				}		
				srcPtr += (image->w - length) * 4;
			} else {
				srcPtr += image->w * 4;
			}
		}
	} else {
		int x, y;
		
		for (y=topleftY; y<gfxPtr->Height+topleftY; y++) {
			destPtr = ((unsigned short *)(((unsigned char *)surface->pixels) + y*surface->pitch)) + topleftX;
			
			if(y>=topY && y<=bottomY) {
				for (x=0; x<length; x++) {
					if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
						unsigned int destR, destG, destB;
						
						destR = (*destPtr & 0xF800)>>8;
						destG = (*destPtr & 0x07E0)>>3;
						destB = (*destPtr & 0x001F)<<3;
						
						destR += MUL_FIXED(alpha, srcPtr[0]);
						destG += MUL_FIXED(alpha, srcPtr[1]);
						destB += MUL_FIXED(alpha, srcPtr[2]);
						if (destR > 0x00FF) destR = 0x00FF;
						if (destG > 0x00FF) destG = 0x00FF;
						if (destB > 0x00FF) destB = 0x00FF;
						
						*destPtr =	((destR>>3)<<11) |
								((destG>>2)<<5 ) |
								((destB>>3));
					}
					srcPtr += 4;
					destPtr++;
				}
				srcPtr += (image->w - length) * 4;
			} else {
				srcPtr += image->w * 4;
			}
		}
	}
		
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

int HeightOfMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	return AvPMenuGfxStorage[menuGfxID].Height;
}

void FadedScreen(int alpha)
{
	int x, y;
	if (!surface) return;
	if (alpha < 0) alpha = 0;
	if (alpha > ONE_FIXED) alpha = ONE_FIXED;
	unsigned short *ptr;
	
	if (SDL_MUSTLOCK(surface)) {
		if (!SDL_LockSurface(surface)) {
			return; /* ... */
		}
	}
	
	/* Fade the canvas already composed by DrawSettingsMenuBackdrop.
	 * Fullscreen includes its edges; letterbox bars remain black. */
	for (y = 0; y < surface->h; y++) {
		ptr = (unsigned short *)(((unsigned char *)surface->pixels)+y*surface->pitch);
		for (x = 0; x < surface->w; x++) {
			unsigned int red5, green6, blue5;
			unsigned int red8, green8, blue8;

			/* The menu framebuffer is RGB565.  Do the fade in expanded RGB8
			 * space, then quantise once when storing it.  The former code
			 * treated the 5/6-bit components as RGB8 and shifted them again;
			 * at the start of a fade that discarded red and blue much earlier
			 * than green, producing a brief green cast after the logo FMV. */
			red5 = (*ptr & 0xF800) >> 11;
			green6 = (*ptr & 0x07E0) >> 5;
			blue5 = (*ptr & 0x001F);

			red8 = (red5 << 3) | (red5 >> 2);
			green8 = (green6 << 2) | (green6 >> 4);
			blue8 = (blue5 << 3) | (blue5 >> 2);

			red8 = MUL_FIXED(red8, alpha);
			green8 = MUL_FIXED(green8, alpha);
			blue8 = MUL_FIXED(blue8, alpha);

			*ptr = (unsigned short)(
				(((red8 * 31 + 127) / 255) << 11) |
				(((green8 * 63 + 127) / 255) << 5) |
				 ((blue8 * 31 + 127) / 255));
			ptr++;
		}
	}
	
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}

void ClearScreenToBlack()
{	
	int x, y;
	unsigned short *ptr;
	
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) < 0) {
			return; /* ... */
		}
	}
	
	for (y = 0; y < surface->h; y++) {
		ptr = (unsigned short *)(((unsigned char *)surface->pixels)+y*surface->pitch);
		for (x = 0; x < surface->w; x++) {
			*ptr = 0;
			
			ptr++;
		}
	}
	
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
}
