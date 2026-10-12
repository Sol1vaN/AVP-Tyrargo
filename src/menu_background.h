#ifndef TYRARGO_MENU_BACKGROUND_H
#define TYRARGO_MENU_BACKGROUND_H
/* Stable preference values since schema 231. UI and movie share this mapping. */
enum { MENU_BG_FULLSCREEN=0, MENU_BG_LETTERBOXED=1, MENU_BG_CENTERED=2 };
typedef struct MenuBackdropMapping { int top,height; double scaleX,scaleY,ox,oy; } MenuBackdropMapping;
static inline MenuBackdropMapping MenuBackdropMap(int w,int h,int iw,int ih,int mode)
{
 MenuBackdropMapping m;
 m.top=(mode==MENU_BG_FULLSCREEN&&iw==640&&ih==480)?60:0;
 m.height=ih-2*m.top;
 double x=(double)w/iw,y=(double)h/m.height;
 if(mode==MENU_BG_FULLSCREEN)m.scaleX=m.scaleY=x>y?x:y;
 else if(mode==MENU_BG_LETTERBOXED){
  /* Stretch the original 4:3 presentation horizontally, up to 16:9.
   * Keep its authored bands; movies occupy their height in that canvas. */
  double width=w;if(width>h*(16.0/9.0))width=h*(16.0/9.0);
  m.scaleX=width/iw;
  m.scaleY=(double)h/(iw*.75);
  if(m.scaleY>m.scaleX)m.scaleY=m.scaleX;
 }else {
  /* Center the movie inside its authored 4:3 menu presentation, rather than
   * enlarging a short movie to the viewport height. Preserve source aspect. */
  m.scaleX=m.scaleY=x<y?x:y;
  double referenceScale=(double)h/(iw*.75);
  if(m.scaleY>referenceScale)m.scaleX=m.scaleY=referenceScale;
 }
 m.ox=(iw-w/m.scaleX)*.5;m.oy=(m.height-h/m.scaleY)*.5;
 return m;
}
#endif
