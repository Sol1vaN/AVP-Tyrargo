#include "3dc.h"
#include "projection.h"
#include <math.h>

AVP_PROJECTION_STATE AvpProjection = {0,0,0,0,0,3,3};

double AvpProjectionDistance(int width, int height, double fov43)
{
    double referenceWidth;
    if(width<=0 || height<=0) return 1.0;
    if(fov43<1.0) fov43=1.0;
    if(fov43>179.0) fov43=179.0;
    referenceWidth=height*(4.0/3.0);
    if(width<referenceWidth) referenceWidth=width;
    return (referenceWidth*0.5)/tan(fov43*(3.14159265358979323846/360.0));
}
void AvpSetProjection(VIEWDESCRIPTORBLOCK *view, double distance)
{
    double horizontalExtent,verticalExtent;
    if(!view) return;
    if(distance<1.0) distance=1.0;
    AvpProjection.view=view;
    AvpProjection.x=distance;
    /* TranslateShapeVertices/TranslationSetup already multiply view Y by 4/3. */
    AvpProjection.y=distance*0.75;
    view->VDB_ProjX=AvpProjection.legacyX=(int)(distance+0.5);
    view->VDB_ProjY=AvpProjection.legacyY=(int)(distance*0.75+0.5);
    if(view->VDB_ProjY<1) view->VDB_ProjY=AvpProjection.legacyY=1;
    horizontalExtent=fmax(view->VDB_CentreX-view->VDB_ClipLeft,view->VDB_ClipRight-view->VDB_CentreX)/distance;
    verticalExtent=fmax(view->VDB_CentreY-view->VDB_ClipUp,view->VDB_ClipDown-view->VDB_CentreY)/AvpProjection.y;
    /* Integer guard bands retain the original fixed-point clipper. OpenGL clips
       exactly at the screen edges. Never cut the newly exposed Hor+ sides. */
    AvpProjection.clipX=(int)ceil(fmax(3.0,horizontalExtent));
    AvpProjection.clipY=(int)ceil(fmax(3.0,verticalExtent));
}
void AvpSetViewFOV(VIEWDESCRIPTORBLOCK *view, double fov43)
{
    if(!view) return;
    AvpSetProjection(view,AvpProjectionDistance(view->VDB_ClipRight-view->VDB_ClipLeft,
        view->VDB_ClipDown-view->VDB_ClipUp,fov43));
}
static int AvpProjectionMatches(const VIEWDESCRIPTORBLOCK *view)
{
    return view && AvpProjection.view==view && view->VDB_ProjX==AvpProjection.legacyX && view->VDB_ProjY==AvpProjection.legacyY;
}
double AvpProjectionX(const VIEWDESCRIPTORBLOCK *view)
{
    return AvpProjectionMatches(view) ? AvpProjection.x : view->VDB_ProjX;
}
double AvpProjectionY(const VIEWDESCRIPTORBLOCK *view)
{
    return AvpProjectionMatches(view) ? AvpProjection.y : view->VDB_ProjY;
}
