#ifndef AVP_MENU_VIEWPORT_H
#define AVP_MENU_VIEWPORT_H
/* Reference bands and title clearance remain unchanged at 640x480. */
static void MenuListBounds(int height,int scale,int help,int mainMenu,int *top,int *bottom)
{
 int pad=(int)((long long)24*scale/65536),reserve=help+pad;
 if(reserve<80)reserve=80;
 *top=mainMenu?130:150;
 *bottom=height-reserve;
 if(*bottom<*top+1)*bottom=*top+1;
}
/* Scale the whole frontend uniformly from its original 480-high design.
 * Wider displays gain horizontal canvas, not smaller text. */
static void MenuCanvasSize(int width,int height,int expanded,int *w,int *h)
{
 *w=640;*h=480;
 if(!expanded || height<=0 || width<=0)return;
 *w=(int)((long long)480*width/height);*w=(*w+3)&~3;
 if(*w<640)*w=640;
}
/* Exactly the same integer geometry used by the AV colour calibration bars. */
static void MenuAVBandBounds(int height,int textScale,int *top,int *bottom){
 int scale=(int)((long long)height*65536/480),h=scale/2048,offset=scale/4096;
 int pad=(int)((long long)4*textScale/65536);
 *top=offset*2+h*2+pad;*bottom=height-offset*2-h*2-pad;
}
static void MenuVisibleRange(const int *heights,int count,int selected,int available,int *first,int *last,int *used)
{
 int a=selected,b=selected,total;
 if(count<=0){*first=0;*last=-1;*used=0;return;}
 if(a<0)a=0;if(a>=count)a=count-1;b=a;total=heights[a];
 /* Centre highlight where possible, then fill all remaining usable height. */
 while(a>0 || b+1<count){
  int before=a>0 && total+heights[a-1]<=available;
  int after=b+1<count && total+heights[b+1]<=available;
  if(!before&&!after)break;
  if(before&&(!after || selected-a<=b-selected))total+=heights[--a];
  else total+=heights[++b];
 }
 *first=a;*last=b;*used=total;
}
/* Keep rows stationary until the selection actually crosses a viewport edge. */
static void MenuStableVisibleRange(const int *heights,int count,int selected,int available,int *first,int *last,int *used)
{
 int a=*first,b,total;
 if(count<=0){*first=0;*last=-1;*used=0;return;}
 if(selected<0)selected=0;if(selected>=count)selected=count-1;
 if(a<0||a>=count)a=0;if(selected<a)a=selected;
 for(;;){
  total=heights[a];b=a;
  while(b+1<count && total+heights[b+1]<=available)total+=heights[++b];
  if(selected<=b||a>=selected)break;
  ++a;
 }
 *first=a;*last=b;*used=total;
}
/* Key bindings expand with the physical viewport after reserving scaled help.
 * The header is top-anchored rather than tied to the screen centre. */
static void MenuKeyConfigBounds(int height,int scale,int help,int *header,int *top,int *bottom)
{
 int row=(int)((long long)20*scale/65536),pad=(int)((long long)10*scale/65536);
 int margin=(int)((long long)24*scale/65536);
 if(margin>height/20)margin=height/20;
 *header=margin;*top=margin+2*row+pad;*bottom=height-help-12;
 if(*bottom<*top+row+2*pad)*bottom=*top+row+2*pad;
}
#endif
