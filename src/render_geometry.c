#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "kshape.h"
#include "render_geometry.h"
#include "camera_motion.h"
#include "avp/hmodel.h"
#include <string.h>
#include <math.h>
#include "geometry_rounding.h"

extern VECTORCH RotatedPts[];
extern DISPLAYBLOCK *Global_ODB_Ptr;
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern SHAPEHEADER *Global_ShapeHeaderPtr;
extern int *Global_ShapePoints,*Global_ShapeNormals;
extern MORPHDISPLAY MorphDisplay;
extern int HierarchicalObjectsLowestYValue;
double GeometryPoints[maxrotpts][3];
int GeometryShapeActive;
static double camera[3],view[12],scale[3],orientationSign;
static SECTION_DATA *drawSection;
void GeometrySection(struct section_data *section){drawSection=section;}
static void matrix(double *r,const MATRIXCH *m){
 r[0]=m->mat11/65536.0;r[1]=m->mat21/65536.0;r[2]=m->mat31/65536.0;
 r[3]=m->mat12/65536.0;r[4]=m->mat22/65536.0;r[5]=m->mat32/65536.0;
 r[6]=m->mat13/65536.0;r[7]=m->mat23/65536.0;r[8]=m->mat33/65536.0;
}
static double determinant(const double*r){return r[0]*(r[4]*r[8]-r[5]*r[7])-r[1]*(r[3]*r[8]-r[5]*r[6])+r[2]*(r[3]*r[7]-r[4]*r[6]);}
void GeometrySetupView(double xs,double ys,double zs,int mirror){
 double r[9];int i,j;matrix(r,&Global_VDB_Ptr->VDB_Mat);
 scale[0]=xs;scale[1]=ys;scale[2]=zs;
 camera[0]=Global_VDB_Ptr->VDB_World.vx;camera[1]=Global_VDB_Ptr->VDB_World.vy;camera[2]=Global_VDB_Ptr->VDB_World.vz;
 for(i=0;i<3;i++){for(j=0;j<3;j++)view[i*4+j]=r[i*3+j]*scale[i];view[i*4+3]=0;}
 /* Use the same fractional procedural camera pose, without float downcasts. */
 MarineMotionGeometryView(view,camera,xs,ys,zs);
 if(mirror)for(j=0;j<3;j++)view[j]=-view[j];
}
void GeometryTransformPoint(double out[3],double x,double y,double z){
 /* Subtract before rotation: adjacent modules share a camera-relative origin. */
 double q[3]={x-camera[0],y-camera[1],z-camera[2]};int i;
 for(i=0;i<3;i++)out[i]=view[i*4]*q[0]+view[i*4+1]*q[1]+view[i*4+2]*q[2];
}
void GeometryTransformShape(SHAPEINSTR *instruction,int deformation){
 DISPLAYBLOCK *ob=Global_ODB_Ptr;const VECTORCH *v=(const VECTORCH*)*(int**)instruction->sh_instr_data;
 const VECTORCH *v2=NULL;double lerp=0,r[9],vr[9],origin[3]={ob->ObWorld.vx,ob->ObWorld.vy,ob->ObWorld.vz};int i,j;
 matrix(r,&ob->ObMat);
 if(drawSection && drawSection->RenderPoseValid){memcpy(r,drawSection->RenderWorldMatrix,sizeof r);memcpy(origin,drawSection->RenderWorldOffset,sizeof origin);}
 if(deformation==1){v=(const VECTORCH*)*MorphDisplay.md_sptr1->points;v2=(const VECTORCH*)*MorphDisplay.md_sptr2->points;lerp=MorphDisplay.md_lerp==0xffff?1.0:MorphDisplay.md_lerp/65536.0;}
 for(i=0;i<3;i++)for(j=0;j<3;j++)vr[i*3+j]=view[i*4+j];
 orientationSign=determinant(vr)*((deformation==1 || ob->ObFlags&ObFlag_ArbRot)?1.0:determinant(r));
 for(i=0;i<instruction->sh_numitems;i++){
  double q[3]={v[i].vx,v[i].vy,v[i].vz},w[3];
  if(v2){q[0]+=(v2[i].vx-q[0])*lerp;q[1]+=(v2[i].vy-q[1])*lerp;q[2]+=(v2[i].vz-q[2])*lerp;}
  if(ob->ObFlags&ObFlag_ArbRot && !deformation){
   GeometryTransformPoint(w,origin[0],origin[1],origin[2]);
   GeometryPoints[i][0]=w[0]+q[0];GeometryPoints[i][1]=w[1]+q[1]*4.0/3.0;GeometryPoints[i][2]=w[2]+q[2];
  }else{
   for(j=0;j<3;j++)w[j]=deformation==1?q[j]:r[j*3]*q[0]+r[j*3+1]*q[1]+r[j*3+2]*q[2];
   if(deformation==2){double k=ob->ObFlags2/65536.0;w[0]*=1.5-k*.5;w[2]*=1.5-k*.5;}
   w[0]+=origin[0];w[1]+=origin[1];w[2]+=origin[2];
   if(deformation==2)w[1]=HierarchicalObjectsLowestYValue+(w[1]-HierarchicalObjectsLowestYValue)*(ob->ObFlags2/65536.0);
   GeometryTransformPoint(GeometryPoints[i],w[0],w[1],w[2]);
  }
  /* Compatibility copies for lighting/sorting. Never used for precise drawing. */
  GeometryRoundCompatibility(&RotatedPts[i],GeometryPoints[i]);
 }
 GeometryShapeActive=1;
}
void GeometryCopyVertex(RENDERVERTEX *v,int i){
 if(!GeometryShapeActive)return;
 v->X=GeometryPoints[i][0]+(v->X-RotatedPts[i].vx);
 v->Y=GeometryPoints[i][1]+(v->Y-RotatedPts[i].vy);
 v->Z=GeometryPoints[i][2]+(v->Z-RotatedPts[i].vz);
}
int GeometryPolygonFacing(POLYHEADER *poly){
 const int *idx=&poly->Poly1stPt;const VECTORCH*base=(const VECTORCH*)Global_ShapePoints;
 const VECTORCH*n=(const VECTORCH*)(Global_ShapeNormals+poly->PolyNormalIndex);
 double original[3]={0,0,0},normal[3]={0,0,0};int count=0,i;
 if(Global_ShapeHeaderPtr->shapeflags&ShapeFlag_Sprite || poly->PolyFlags&iflag_no_bfc)return 1;
 while(idx[count]!=Term)count++;
 if(count<3)return 0;
 /* Area normals of the actual deformed face, double throughout. Keep the
    authored orientation; smoothing/lighting normals are not rewritten. */
 for(i=1;i<count-1;i++){
  double a[3],b[3],u[3],v[3];int j;const VECTORCH*p0=&base[idx[0]],*p1=&base[idx[i]],*p2=&base[idx[i+1]];
  u[0]=(double)p1->vx-p0->vx;u[1]=(double)p1->vy-p0->vy;u[2]=(double)p1->vz-p0->vz;
  v[0]=(double)p2->vx-p0->vx;v[1]=(double)p2->vy-p0->vy;v[2]=(double)p2->vz-p0->vz;
  for(j=0;j<3;j++){a[j]=GeometryPoints[idx[i]][j]-GeometryPoints[idx[0]][j];b[j]=GeometryPoints[idx[i+1]][j]-GeometryPoints[idx[0]][j];}
  for(j=0;j<3;j++){int k=(j+1)%3,l=(j+2)%3;original[j]+=u[k]*v[l]-u[l]*v[k];normal[j]+=a[k]*b[l]-a[l]*b[k];}
 }
 {double agreement=original[0]*n->vx+original[1]*n->vy+original[2]*n->vz;
  double*point=GeometryPoints[idx[0]],facing=normal[0]*point[0]+normal[1]*point[1]+normal[2]*point[2];
  if(agreement==0 || orientationSign==0)return 1; /* malformed/degenerate metadata: conservative */
  return facing*(agreement<0?-1.0:1.0)*(orientationSign<0?-1.0:1.0)<=0;
 }
}
int GeometryObjectVisible(DISPLAYBLOCK *ob,double cx,double cy){
 double p[3],radius=ob->ObShapeData->shaperadius+2.0;int i;
 const double planes[5][4]={{0,0,1,-4},{1,0,cx,0},{-1,0,cx,0},{0,1,cy,0},{0,-1,cy,0}};
 if(ob->ObFlags&ObFlag_ArbRot)return 1; /* view-aligned/scaled sprites are tested per vertex */
 GeometryTransformPoint(p,ob->ObWorld.vx,ob->ObWorld.vy,ob->ObWorld.vz);
 for(i=0;i<5;i++){
  const double*n=planes[i];double norm2=0;int j;
  for(j=0;j<3;j++){double c=n[0]*view[j]+n[1]*view[4+j]+n[2]*view[8+j];norm2+=c*c;}
  if(n[0]*p[0]+n[1]*p[1]+n[2]*p[2]+n[3]<-radius*sqrt(norm2))return 0;
 }
 return 1;
}
