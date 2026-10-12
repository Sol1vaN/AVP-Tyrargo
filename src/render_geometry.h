#ifndef AVP_RENDER_GEOMETRY_H
#define AVP_RENDER_GEOMETRY_H
#include "kshape.h"
/* Render-only precision; game/asset/network vectors remain integer. */
#ifdef __cplusplus
extern "C" {
#endif
extern double GeometryPoints[maxrotpts][3];
extern int GeometryShapeActive;
struct section_data;
void GeometrySection(struct section_data *section);
void GeometrySetupView(double xs,double ys,double zs,int mirror);
void GeometryTransformShape(SHAPEINSTR *instruction,int deformation);
void GeometryTransformPoint(double out[3],double x,double y,double z);
void GeometryCopyVertex(RENDERVERTEX *vertex,int index);
int GeometryPolygonFacing(POLYHEADER *polygon);
int GeometryObjectVisible(DISPLAYBLOCK *object,double clipX,double clipY);
#ifdef __cplusplus
}
#endif
#endif
