#ifndef TYRARGO_SCREEN_LAYOUT_H
#define TYRARGO_SCREEN_LAYOUT_H
/* Existing profile modes 0..8 remain x1.0..x5.0. */
static int ConsoleScaleFixed(int mode) {
 if(mode==9)return 52429;
 if(mode==10)return 58982;
 if(mode<0||mode>8)mode=0;
 return 65536+mode*32768;
}
static int ConsoleScaleStep(int mode,int forward) {
 static const int order[]={9,10,0,1,2,3,4,5,6,7,8};
 int i;
 for(i=0;i<11;i++)if(order[i]==mode)return order[(i+(forward?1:10))%11];
 return 0;
}
/* Reserve each column's actual glyph extent, not a fixed short English label. */
static int MarineAmmoColumnCentre(int width,int scale,int textWidth,int primary) {
 int half=(int)((long long)textWidth*scale/131072)+2;
 int inset=(int)((long long)40*scale/65536);
 int gap=(int)((long long)8*scale/65536)+2;
 int spacing=(int)((long long)80*scale/65536);
 if(inset<half+gap)inset=half+gap;
 if(spacing<2*half+gap)spacing=2*half+gap;
 return width-inset-(primary?0:spacing);
}
#endif
