/*
 * KJL 15:13:43 7/17/97 - frustrum.c
 *
 * Contains all the functions connected
 * to the view frustrum and clipping
 *
 */
#include "3dc.h"
#include "projection.h"
#include <math.h>
#define AVP_WIDE_X (AvpProjection.clipX)
#define AVP_WIDE_Y (AvpProjection.clipY)
#include "viewmodel.h"
#include "module.h"
#include "inline.h"

#include "stratdef.h"
#include "gamedef.h"

#include "kshape.h"
#include "render_geometry.h"
#include "kzsort.h"
#include "frustum.h"

#include "particle.h"

#define UseLocalAssert Yes
#include "ourasert.h"

/*KJL****************************************************************************************
*  										G L O B A L S 	            					    *
****************************************************************************************KJL*/
extern VECTORCH RotatedPts[];

extern DISPLAYBLOCK *Global_ODB_Ptr;
extern SHAPEHEADER *Global_ShapeHeaderPtr;
extern int *Global_ShapePoints;
extern int *Global_ShapeNormals;

extern VECTORCH LocalView;

#define FAR_Z_CLIP 0
#define FAR_Z_CLIP_RANGE 49000
/*KJL****************************************************************************************
*                                    P R O T O T Y P E S	                                *
****************************************************************************************KJL*/
/* GOURAUD POLYGON CLIPPING */
void (*GouraudPolygon_ClipWithNegativeX)(void);
void (*GouraudPolygon_ClipWithPositiveY)(void);
void (*GouraudPolygon_ClipWithNegativeY)(void);
void (*GouraudPolygon_ClipWithPositiveX)(void);

/* TEXTURED POLYGON CLIPPING */
void (*TexturedPolygon_ClipWithNegativeX)(void);
void (*TexturedPolygon_ClipWithPositiveY)(void);
void (*TexturedPolygon_ClipWithNegativeY)(void);
void (*TexturedPolygon_ClipWithPositiveX)(void);

/* GOURAUD TEXTURED POLYGON CLIPPING */
void (*GouraudTexturedPolygon_ClipWithNegativeX)(void);
void (*GouraudTexturedPolygon_ClipWithPositiveY)(void);
void (*GouraudTexturedPolygon_ClipWithNegativeY)(void);
void (*GouraudTexturedPolygon_ClipWithPositiveX)(void);

/* FRUSTRUM TESTS */
int (*ObjectWithinFrustrum)(DISPLAYBLOCK *dbPtr);
int (*ObjectCompletelyWithinFrustrum)(DISPLAYBLOCK *dbPtr);
int (*VertexWithinFrustrum)(RENDERVERTEX *vertexPtr);
void (*TestVerticesWithFrustrum)(void);


static void GouraudPolygon_Norm_ClipWithNegativeX(void);
static void GouraudPolygon_Wide_ClipWithNegativeX(void);
static void GouraudPolygon_Norm_ClipWithPositiveY(void);
static void GouraudPolygon_Wide_ClipWithPositiveY(void);
static void GouraudPolygon_Norm_ClipWithNegativeY(void);
static void GouraudPolygon_Wide_ClipWithNegativeY(void);
static void GouraudPolygon_Norm_ClipWithPositiveX(void);
static void GouraudPolygon_Wide_ClipWithPositiveX(void);

static void TexturedPolygon_Norm_ClipWithNegativeX(void);
static void TexturedPolygon_Wide_ClipWithNegativeX(void);
static void TexturedPolygon_Norm_ClipWithPositiveY(void);
static void TexturedPolygon_Wide_ClipWithPositiveY(void);
static void TexturedPolygon_Norm_ClipWithNegativeY(void);
static void TexturedPolygon_Wide_ClipWithNegativeY(void);
static void TexturedPolygon_Norm_ClipWithPositiveX(void);
static void TexturedPolygon_Wide_ClipWithPositiveX(void);

static void GouraudTexturedPolygon_Norm_ClipWithNegativeX(void);
static void GouraudTexturedPolygon_Wide_ClipWithNegativeX(void);
static void GouraudTexturedPolygon_Norm_ClipWithPositiveY(void);
static void GouraudTexturedPolygon_Wide_ClipWithPositiveY(void);
static void GouraudTexturedPolygon_Norm_ClipWithNegativeY(void);
static void GouraudTexturedPolygon_Wide_ClipWithNegativeY(void);
static void GouraudTexturedPolygon_Norm_ClipWithPositiveX(void);
static void GouraudTexturedPolygon_Wide_ClipWithPositiveX(void);

static int VertexWithin_Norm_Frustrum(RENDERVERTEX *vertexPtr);
static int VertexWithin_Wide_Frustrum(RENDERVERTEX *vertexPtr);
static int ObjectWithin_Norm_Frustrum(DISPLAYBLOCK *dbPtr);
static int ObjectWithin_Wide_Frustrum(DISPLAYBLOCK *dbPtr);
static int ObjectCompletelyWithin_Norm_Frustrum(DISPLAYBLOCK *dbPtr);
static int ObjectCompletelyWithin_Wide_Frustrum(DISPLAYBLOCK *dbPtr);
static void TestVerticesWith_Norm_Frustrum(void);
static void TestVerticesWith_Wide_Frustrum(void);

/*KJL****************************************************************************************
*                                     F U N C T I O N S	                                    *
****************************************************************************************KJL*/

#define ZCLIPPINGVALUE 4

/* The weapon uses the same planes and winding rules with its unrounded pose. */
static double ViewmodelPlaneDistance(const RENDERVERTEX *v, int plane, double extent)
{
 switch(plane) {
  case 0: return v->ViewZ-ViewmodelNearClip();
  case 1: return extent*v->ViewZ+v->ViewX;
  case 2: return extent*v->ViewZ-v->ViewY;
  case 3: return extent*v->ViewZ+v->ViewY;
  default:return extent*v->ViewZ-v->ViewX;
 }
}
static void ViewmodelClip(RENDERVERTEX *input, RENDERVERTEX *output, int plane, double extent)
{
 unsigned int n=RenderPolygon.NumberOfVertices,j,count=0;
 if(n<3) { RenderPolygon.NumberOfVertices=0; return; }
 for(j=0;j<n;j++) {
  const RENDERVERTEX *a=&input[j], *b=&input[(j+1)%n];
  double da=ViewmodelPlaneDistance(a,plane,extent),db=ViewmodelPlaneDistance(b,plane,extent);
  if(da>=0) output[count++]=*a;
  if((da>=0)!=(db>=0)) {
   double u=da/(da-db);
   RENDERVERTEX *v=&output[count++];
   *v=*a;
   v->ViewX=a->ViewX+u*(b->ViewX-a->ViewX);
   v->ViewY=a->ViewY+u*(b->ViewY-a->ViewY);
   v->ViewZ=a->ViewZ+u*(b->ViewZ-a->ViewZ);
   v->X=(int)v->ViewX; v->Y=(int)v->ViewY; v->Z=(int)v->ViewZ;
   /* Coordinates retain double precision; fixed UVs and byte colours retain their formats. */
#define VM_LERP(field) v->field=a->field+u*((double)b->field-a->field)
   VM_LERP(U); VM_LERP(V); VM_LERP(R); VM_LERP(G); VM_LERP(B); VM_LERP(A);
   VM_LERP(SpecularR); VM_LERP(SpecularG); VM_LERP(SpecularB); VM_LERP(Fog);
#undef VM_LERP
  }
 }
 RenderPolygon.NumberOfVertices=count;
}
static int ViewmodelPolygonWithinFrustum(POLYHEADER *poly)
{
 int *index=&poly->Poly1stPt, all=31, any=0;
 if(poly->PolyFlags & iflag_notvis) return 0;
 RenderPolygon.NumberOfVertices=0;
 while(*index!=Term) {
  double *p=ViewmodelPoints[*index++];
  int flags=(p[2]>=ViewmodelNearClip() ? 1:0)|(p[0]<=AVP_WIDE_X*p[2] ? 2:0)|(-p[0]<=AVP_WIDE_X*p[2] ? 4:0)|
            (p[1]<=AVP_WIDE_Y*p[2] ? 8:0)|(-p[1]<=AVP_WIDE_Y*p[2] ? 16:0);
  all&=flags; any|=flags; RenderPolygon.NumberOfVertices++;
 }
 if(any!=31) return 0;
 if(!(Global_ShapeHeaderPtr->shapeflags & ShapeFlag_Sprite) && !(poly->PolyFlags & iflag_no_bfc)) {
  VECTORCH *normal=(VECTORCH*)(Global_ShapeNormals+poly->PolyNormalIndex);
  VECTORCH *point=&((VECTORCH*)Global_ShapePoints)[poly->Poly1stPt];
  double dot=(point->vx-ViewmodelLocalEye[0])*normal->vx+(point->vy-ViewmodelLocalEye[1])*normal->vy+(point->vz-ViewmodelLocalEye[2])*normal->vz;
  if(dot>0) return 0;
 }
 return all==31 ? 2 : 1;
}


/* A shared edge must intersect a clip plane identically in either direction.
   Canonical endpoint ordering makes the operations (not just the formula)
   identical. UV and colours stay polygon-local, preserving texture seams. */
static double GeometryPlaneDistance(const RENDERVERTEX*v,int plane,double extent){
 switch(plane){case 0:return v->Z-ZCLIPPINGVALUE;case 1:return extent*v->Z+v->X;
 case 2:return extent*v->Z-v->Y;case 3:return extent*v->Z+v->Y;default:return extent*v->Z-v->X;}
}
static int GeometryVertexBefore(const RENDERVERTEX*a,const RENDERVERTEX*b){
 if(a->X!=b->X)return a->X<b->X;if(a->Y!=b->Y)return a->Y<b->Y;return a->Z<b->Z;
}
static void GeometryClip(RENDERVERTEX*input,RENDERVERTEX*output,int plane,double extent){
 int n=RenderPolygon.NumberOfVertices,count=0,j;
 for(j=0;j<n;j++){
  const RENDERVERTEX*a=&input[j],*b=&input[(j+1)%n];
  double da=GeometryPlaneDistance(a,plane,extent),db=GeometryPlaneDistance(b,plane,extent);
  if(da>=0){if(count>=maxpolypts){RenderPolygon.NumberOfVertices=0;return;}output[count++]=*a;}
  if((da<0 && db>0)||(da>0 && db<0)){
   const RENDERVERTEX*lo=a,*hi=b;double dl=da,dh=db,t;RENDERVERTEX*v;
   if(!GeometryVertexBefore(lo,hi)){lo=b;hi=a;dl=db;dh=da;}
   t=dl/(dl-dh);
   if(count>=maxpolypts){RenderPolygon.NumberOfVertices=0;return;}v=&output[count++];*v=*lo;
   v->X=lo->X+t*(hi->X-lo->X);v->Y=lo->Y+t*(hi->Y-lo->Y);v->Z=lo->Z+t*(hi->Z-lo->Z);
   switch(plane){case 0:v->Z=ZCLIPPINGVALUE;break;case 1:v->X=-extent*v->Z;break;
    case 2:v->Y=extent*v->Z;break;case 3:v->Y=-extent*v->Z;break;default:v->X=extent*v->Z;break;}
   #define INTERPOLATE(field) v->field=lo->field+t*((double)hi->field-lo->field)
   INTERPOLATE(U);INTERPOLATE(V);INTERPOLATE(R);INTERPOLATE(G);INTERPOLATE(B);INTERPOLATE(A);
   INTERPOLATE(SpecularR);INTERPOLATE(SpecularG);INTERPOLATE(SpecularB);INTERPOLATE(Fog);
   #undef INTERPOLATE
  }
 }
 RenderPolygon.NumberOfVertices=count;
}
static int GeometryPolygonWithinFrustum(POLYHEADER*poly){
 const int*idx=&poly->Poly1stPt;int all=31,any=0;
 if(poly->PolyFlags&iflag_notvis)return 0;
 RenderPolygon.NumberOfVertices=0;
 while(*idx!=Term){
  const double*p=GeometryPoints[*idx++];
  int flags=(p[2]>=ZCLIPPINGVALUE?1:0)|(p[0]<=AVP_WIDE_X*p[2]?2:0)|(-p[0]<=AVP_WIDE_X*p[2]?4:0)|
            (p[1]<=AVP_WIDE_Y*p[2]?8:0)|(-p[1]<=AVP_WIDE_Y*p[2]?16:0);
  all&=flags;any|=flags;RenderPolygon.NumberOfVertices++;
 }
 if(any!=31 || !GeometryPolygonFacing(poly))return 0;
 return all==31?2:1;
}
void SetFrustrumType(enum FrustrumType frustrumType)
{
	switch (frustrumType)
	{
		default:
  		case FRUSTRUM_TYPE_NORMAL:
		{
			/* GOURAUD POLYGON CLIPPING */
			GouraudPolygon_ClipWithNegativeX = GouraudPolygon_Wide_ClipWithNegativeX;
			GouraudPolygon_ClipWithPositiveY = GouraudPolygon_Wide_ClipWithPositiveY;
			GouraudPolygon_ClipWithNegativeY = GouraudPolygon_Wide_ClipWithNegativeY;
			GouraudPolygon_ClipWithPositiveX = GouraudPolygon_Wide_ClipWithPositiveX;

			/* TEXTURED POLYGON CLIPPING */
			TexturedPolygon_ClipWithNegativeX = TexturedPolygon_Wide_ClipWithNegativeX;
			TexturedPolygon_ClipWithPositiveY = TexturedPolygon_Wide_ClipWithPositiveY;
			TexturedPolygon_ClipWithNegativeY = TexturedPolygon_Wide_ClipWithNegativeY;
			TexturedPolygon_ClipWithPositiveX = TexturedPolygon_Wide_ClipWithPositiveX;

			/* GOURAUD TEXTURED POLYGON CLIPPING */
			GouraudTexturedPolygon_ClipWithNegativeX = GouraudTexturedPolygon_Wide_ClipWithNegativeX;
			GouraudTexturedPolygon_ClipWithPositiveY = GouraudTexturedPolygon_Wide_ClipWithPositiveY;
			GouraudTexturedPolygon_ClipWithNegativeY = GouraudTexturedPolygon_Wide_ClipWithNegativeY;
			GouraudTexturedPolygon_ClipWithPositiveX = GouraudTexturedPolygon_Wide_ClipWithPositiveX;

			/* FRUSTRUM TESTS */
			TestVerticesWithFrustrum = TestVerticesWith_Wide_Frustrum;
			ObjectWithinFrustrum = ObjectWithin_Wide_Frustrum;
			ObjectCompletelyWithinFrustrum = ObjectCompletelyWithin_Wide_Frustrum;
			VertexWithinFrustrum = VertexWithin_Wide_Frustrum;

			break;
		}

		case FRUSTRUM_TYPE_WIDE:
		{
			/* GOURAUD POLYGON CLIPPING */
			GouraudPolygon_ClipWithNegativeX = GouraudPolygon_Wide_ClipWithNegativeX;
			GouraudPolygon_ClipWithPositiveY = GouraudPolygon_Wide_ClipWithPositiveY;
			GouraudPolygon_ClipWithNegativeY = GouraudPolygon_Wide_ClipWithNegativeY;
			GouraudPolygon_ClipWithPositiveX = GouraudPolygon_Wide_ClipWithPositiveX;

			/* TEXTURED POLYGON CLIPPING */
			TexturedPolygon_ClipWithNegativeX = TexturedPolygon_Wide_ClipWithNegativeX;
			TexturedPolygon_ClipWithPositiveY = TexturedPolygon_Wide_ClipWithPositiveY;
			TexturedPolygon_ClipWithNegativeY = TexturedPolygon_Wide_ClipWithNegativeY;
			TexturedPolygon_ClipWithPositiveX = TexturedPolygon_Wide_ClipWithPositiveX;

			/* GOURAUD TEXTURED POLYGON CLIPPING */
			GouraudTexturedPolygon_ClipWithNegativeX = GouraudTexturedPolygon_Wide_ClipWithNegativeX;
			GouraudTexturedPolygon_ClipWithPositiveY = GouraudTexturedPolygon_Wide_ClipWithPositiveY;
			GouraudTexturedPolygon_ClipWithNegativeY = GouraudTexturedPolygon_Wide_ClipWithNegativeY;
			GouraudTexturedPolygon_ClipWithPositiveX = GouraudTexturedPolygon_Wide_ClipWithPositiveX;

			/* FRUSTRUM TESTS */
			TestVerticesWithFrustrum = TestVerticesWith_Wide_Frustrum;
			ObjectWithinFrustrum = ObjectWithin_Wide_Frustrum;
			ObjectCompletelyWithinFrustrum = ObjectCompletelyWithin_Wide_Frustrum;
			VertexWithinFrustrum = VertexWithin_Wide_Frustrum;
			
			break;
		}
	}
}

/* clipping code macros - these are used as building blocks to assemble
clipping fns for different polygon types with the minimum of fuss */
//64
#define Clip_Z_Test(v) (ZCLIPPINGVALUE <= (v)->Z) 
#define Clip_NX_Test(v) (-(v)->X <= (v)->Z)
#define Clip_PX_Test(v) ((v)->X <= (v)->Z)
#define Clip_NY_Test(v) (-(v)->Y <= (v)->Z)
#define Clip_PY_Test(v) ((v)->Y <= (v)->Z)	  


/* Guard-plane classification also applies to non-mesh primitives. */
#define Clip_Wide_NX_Test(v) (-(v)->X <= (v)->Z*AVP_WIDE_X)
#define Clip_Wide_PX_Test(v) ((v)->X <= (v)->Z*AVP_WIDE_X)
#define Clip_Wide_NY_Test(v) (-(v)->Y <= (v)->Z*AVP_WIDE_Y)
#define Clip_Wide_PY_Test(v) ((v)->Y <= (v)->Z*AVP_WIDE_Y)

/*KJL**************
* GOURAUD POLYGON *
**************KJL*/

/* Clip against Z plane */
void GouraudPolygon_ClipWithZ(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),0,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),0,1.0);
}



/* Clip against negative X plane */
static void GouraudPolygon_Norm_ClipWithNegativeX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,1,1.0); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,1,1.0);
}

static void GouraudPolygon_Wide_ClipWithNegativeX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,1,AVP_WIDE_X); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,1,AVP_WIDE_X);
}


/* Clip against positive Y plane*/
static void GouraudPolygon_Norm_ClipWithPositiveY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),2,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),2,1.0);
}

static void GouraudPolygon_Wide_ClipWithPositiveY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),2,AVP_WIDE_Y); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),2,AVP_WIDE_Y);
}


/* Clip against negative Y plane*/
static void GouraudPolygon_Norm_ClipWithNegativeY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,3,1.0); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,3,1.0);
}

static void GouraudPolygon_Wide_ClipWithNegativeY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,3,AVP_WIDE_Y); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,3,AVP_WIDE_Y);
}


/* Clip against positive X plane */
static void GouraudPolygon_Norm_ClipWithPositiveX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),4,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),4,1.0);
}

static void GouraudPolygon_Wide_ClipWithPositiveX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),4,AVP_WIDE_X); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),4,AVP_WIDE_X);
}


/*KJL***************
* TEXTURED POLYGON *
***************KJL*/

/* Clip against Z plane */
void TexturedPolygon_ClipWithZ(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),0,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),0,1.0);
}


/* Clip against negative X plane */
static void TexturedPolygon_Norm_ClipWithNegativeX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,1,1.0); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,1,1.0);
}

static void TexturedPolygon_Wide_ClipWithNegativeX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,1,AVP_WIDE_X); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,1,AVP_WIDE_X);
}


/* Clip against positive Y plane*/
static void TexturedPolygon_Norm_ClipWithPositiveY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),2,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),2,1.0);
}

static void TexturedPolygon_Wide_ClipWithPositiveY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),2,AVP_WIDE_Y); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),2,AVP_WIDE_Y);
}


/* Clip against negative Y plane*/
static void TexturedPolygon_Norm_ClipWithNegativeY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,3,1.0); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,3,1.0);
}

static void TexturedPolygon_Wide_ClipWithNegativeY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,3,AVP_WIDE_Y); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,3,AVP_WIDE_Y);
}


/* Clip against positive X plane */
static void TexturedPolygon_Norm_ClipWithPositiveX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),4,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),4,1.0);
}

static void TexturedPolygon_Wide_ClipWithPositiveX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),4,AVP_WIDE_X); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),4,AVP_WIDE_X);
}





/*KJL************************
* GOURAUD TEXTURED POLYGONS *
************************KJL*/
						 
/* Clip against Z plane */
void GouraudTexturedPolygon_ClipWithZ(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),0,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),0,1.0);
}


/* Clip against negative X plane */
static void GouraudTexturedPolygon_Norm_ClipWithNegativeX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,1,1.0); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,1,1.0);
}

static void GouraudTexturedPolygon_Wide_ClipWithNegativeX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,1,AVP_WIDE_X); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,1,AVP_WIDE_X);
}


/* Clip against positive Y plane*/
static void GouraudTexturedPolygon_Norm_ClipWithPositiveY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),2,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),2,1.0);
}

static void GouraudTexturedPolygon_Wide_ClipWithPositiveY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),2,AVP_WIDE_Y); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),2,AVP_WIDE_Y);
}


/* Clip against negative Y plane*/
static void GouraudTexturedPolygon_Norm_ClipWithNegativeY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,3,1.0); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,3,1.0);
}

static void GouraudTexturedPolygon_Wide_ClipWithNegativeY(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip((RenderPolygon.Vertices),VerticesBuffer,3,AVP_WIDE_Y); return; }
 GeometryClip((RenderPolygon.Vertices),VerticesBuffer,3,AVP_WIDE_Y);
}


/* Clip against positive X plane */
static void GouraudTexturedPolygon_Norm_ClipWithPositiveX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),4,1.0); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),4,1.0);
}

static void GouraudTexturedPolygon_Wide_ClipWithPositiveX(void)
{
 if(ViewmodelShapeActive){ ViewmodelClip(VerticesBuffer,(RenderPolygon.Vertices),4,AVP_WIDE_X); return; }
 GeometryClip(VerticesBuffer,(RenderPolygon.Vertices),4,AVP_WIDE_X);
}








int PolygonWithinFrustrum(POLYHEADER *polyPtr)
{
 if(GeometryShapeActive) return GeometryPolygonWithinFrustum(polyPtr);

	if(ViewmodelShapeActive) return ViewmodelPolygonWithinFrustum(polyPtr);			
    char inFrustrumFlag=0;
   	char noClippingFlag=INSIDE_FRUSTRUM;
	int *vertexNumberPtr = &polyPtr->Poly1stPt;
	
	if (polyPtr->PolyFlags & iflag_notvis) return 0;
	
	RenderPolygon.NumberOfVertices=0; 
	while(*vertexNumberPtr != Term)
	{
		int vertexNumber = *vertexNumberPtr++;

		inFrustrumFlag |= FrustrumFlagForVertex[vertexNumber];
		noClippingFlag &= FrustrumFlagForVertex[vertexNumber];
	
		/* count the number of points in the polygon; this is used for all the loops that follow */
	   	RenderPolygon.NumberOfVertices++; 
	}
	
	if (inFrustrumFlag != INSIDE_FRUSTRUM) return 0;

	/* at this point we know that the poly is inside the view frustrum */

	/* if not a sprite, test direction of poly */
	if (!( (Global_ShapeHeaderPtr->shapeflags&ShapeFlag_Sprite) || (polyPtr->PolyFlags & iflag_no_bfc) ))
	{
		VECTORCH pop;
		VECTORCH *normalPtr = (VECTORCH*)(Global_ShapeNormals + polyPtr->PolyNormalIndex);
		
		#if 1
		if(Global_ODB_Ptr->ObMorphCtrl)
		{
			extern MORPHDISPLAY MorphDisplay;
		   	SHAPEHEADER *shape1Ptr;
		   	VECTORCH *shape1PointsPtr;
			VECTORCH *shape2PointsPtr;
							
			/* Set up the morph data */
			GetMorphDisplay(&MorphDisplay, Global_ODB_Ptr);

			shape1Ptr = MorphDisplay.md_sptr1;

			if(MorphDisplay.md_lerp == 0x0000)
			{
				shape1PointsPtr = (VECTORCH *)*shape1Ptr->points;
				pop = shape1PointsPtr[polyPtr->Poly1stPt];	
					
			}
			else if(MorphDisplay.md_lerp == 0xffff)
			{
				SHAPEHEADER *shape2Ptr = MorphDisplay.md_sptr2;
				
				shape2PointsPtr = (VECTORCH *)*shape2Ptr->points;
				pop = shape2PointsPtr[polyPtr->Poly1stPt];	
			}
			else
			{
				SHAPEHEADER *shape2Ptr = MorphDisplay.md_sptr2;
			    
				shape1PointsPtr = (VECTORCH *)(*shape1Ptr->points);
				shape2PointsPtr = (VECTORCH *)(*shape2Ptr->points);

				{
				   	VECTORCH vertex1 = shape1PointsPtr[polyPtr->Poly1stPt];
				   	VECTORCH vertex2 = shape2PointsPtr[polyPtr->Poly1stPt];
				
					if( (vertex1.vx == vertex2.vx && vertex1.vy == vertex2.vy && vertex1.vz == vertex2.vz) )
					{
						pop = vertex1;
					}
					else
					{
						/* KJL 15:27:20 05/22/97 - I've changed this to speed things up, If a vertex
						component has a magnitude greater than 32768 things will go wrong. */
						pop.vx = vertex1.vx + (((vertex2.vx-vertex1.vx)*MorphDisplay.md_lerp)>>16);
						pop.vy = vertex1.vy + (((vertex2.vy-vertex1.vy)*MorphDisplay.md_lerp)>>16);
						pop.vz = vertex1.vz + (((vertex2.vz-vertex1.vz)*MorphDisplay.md_lerp)>>16);
					}
				}
			}
 		}
		else
		#endif
		{
			/* Get the 1st polygon point as the POP */
			VECTORCH *pointsArray = (VECTORCH*)(Global_ShapePoints);
			pop = pointsArray[polyPtr->Poly1stPt];
		}
		pop.vx -= LocalView.vx;
		pop.vy -= LocalView.vy;
		pop.vz -= LocalView.vz;
	
		if (Dot(&pop, normalPtr)>0) return 0;
	}

	if (noClippingFlag == INSIDE_FRUSTRUM) return 2;

	/* yes, we need to draw poly */
	return 1;
}	

int PolygonShouldBeDrawn(POLYHEADER *polyPtr)
{
 if(GeometryShapeActive) return GeometryPolygonWithinFrustum(polyPtr);

	if(ViewmodelShapeActive) return ViewmodelPolygonWithinFrustum(polyPtr);

	/* at this point we know that the poly is inside the view frustrum */
	if (polyPtr->PolyFlags & iflag_notvis) return 0;

	#if 1	
	/* if not a sprite, test direction of poly */
	if (!( (Global_ShapeHeaderPtr->shapeflags&ShapeFlag_Sprite) || (polyPtr->PolyFlags & iflag_no_bfc) ))
	{
		/* KJL 16:49:14 7/10/97 -  
		
		***** MORPHED NORMALS SUPPORT NOT YET ADDED *****
		
		*/
		VECTORCH pop;
		VECTORCH *normalPtr = (VECTORCH*)(Global_ShapeNormals + polyPtr->PolyNormalIndex);
		VECTORCH *pointsArray = (VECTORCH*)(Global_ShapePoints);
		/* Get the 1st polygon point as the POP */

		pop.vx = pointsArray[polyPtr->Poly1stPt].vx - LocalView.vx;
		pop.vy = pointsArray[polyPtr->Poly1stPt].vy - LocalView.vy;
		pop.vz = pointsArray[polyPtr->Poly1stPt].vz - LocalView.vz;
	
		if (Dot(&pop, normalPtr)>0) return 0;
	}
	#endif
	#if 0
	{
		int *vertexNumberPtr = &polyPtr->Poly1stPt;
		RenderPolygon.NumberOfVertices=0; 
		while(*vertexNumberPtr++ != Term)
		{
			/* count the number of points in the polygon; this is used for all the loops that follow */
		   	RenderPolygon.NumberOfVertices++; 
		}
	}
	#elif 0
	RenderPolygon.NumberOfVertices = 3;
	#else
	{
		int *vertexNumberPtr = &polyPtr->Poly1stPt;
		if (vertexNumberPtr[3] == Term)
		{
			RenderPolygon.NumberOfVertices = 3;
		}
		else
		{
			RenderPolygon.NumberOfVertices = 4;
		}
	}
	#endif

	return 2;
}	

/* KJL 16:18:59 7/7/97 - simple vertex test to be used in first
pass of subdividing code */
static int VertexWithin_Norm_Frustrum(RENDERVERTEX *vertexPtr)
{
	int vertexFlag = 0;

	if(Clip_Z_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_Z_PLANE;
	if(Clip_PX_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_PX_PLANE;	
	if(Clip_NX_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_NX_PLANE;	
	if(Clip_PY_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_PY_PLANE;	
	if(Clip_NY_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_NY_PLANE;	
	
	return vertexFlag;
}

static int VertexWithin_Wide_Frustrum(RENDERVERTEX *vertexPtr)
{
	int vertexFlag = 0;

	if(Clip_Z_Test(vertexPtr))			vertexFlag |= INSIDE_FRUSTRUM_Z_PLANE;
	if(Clip_Wide_PX_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_PX_PLANE;	
	if(Clip_Wide_NX_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_NX_PLANE;	
	if(Clip_Wide_PY_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_PY_PLANE;	
	if(Clip_Wide_NY_Test(vertexPtr))	vertexFlag |= INSIDE_FRUSTRUM_NY_PLANE;	
	
	return vertexFlag;
}


/* KJL 15:32:52 7/17/97 - Test to see if an object is in the view frustrum */
static int ObjectWithin_Norm_Frustrum(DISPLAYBLOCK *dbPtr)
{ return GeometryObjectVisible(dbPtr,1.0,1.0); }

static int ObjectCompletelyWithin_Norm_Frustrum(DISPLAYBLOCK *dbPtr)
{
 //	LOCALASSERT(dbPtr->ObShapeData->shaperadius);
	if (dbPtr->ObView.vz-dbPtr->ObShapeData->shaperadius>=ZCLIPPINGVALUE)
	{
		/* scale radius by square root of 2 */
		int radius = MUL_FIXED(92682,dbPtr->ObShapeData->shaperadius);

		if ((dbPtr->ObView.vz-dbPtr->ObView.vx)>=radius)
			if ((dbPtr->ObView.vz+dbPtr->ObView.vx)>=radius)
				if ((dbPtr->ObView.vz-dbPtr->ObView.vy)>=radius)
					if ((dbPtr->ObView.vz+dbPtr->ObView.vy)>=radius)
						return 1;
	}
	return 0;
}
static int ObjectCompletelyWithin_Wide_Frustrum(DISPLAYBLOCK *dbPtr)
{
	return 0;
}

static int ObjectWithin_Wide_Frustrum(DISPLAYBLOCK *dbPtr)
{ return GeometryObjectVisible(dbPtr,AVP_WIDE_X,AVP_WIDE_Y); }



char FrustrumFlagForVertex[maxrotpts];

void TestVerticesWith_Norm_Frustrum(void)
{
	int v = Global_ShapeHeaderPtr->numpoints;

	GLOBALASSERT(v>0);

	while(v--)
	{
		char vertexFlag = 0;
		
#if FAR_Z_CLIP
		if(ZCLIPPINGVALUE <= RotatedPts[v].vz && RotatedPts[v].vz<=FAR_Z_CLIP_RANGE)
#else
		if(ZCLIPPINGVALUE <= RotatedPts[v].vz)
#endif
			vertexFlag |= INSIDE_FRUSTRUM_Z_PLANE;
		
		if(-RotatedPts[v].vx <= RotatedPts[v].vz)
			vertexFlag |= INSIDE_FRUSTRUM_PX_PLANE;	
		
		if(RotatedPts[v].vx <= RotatedPts[v].vz)
			vertexFlag |= INSIDE_FRUSTRUM_NX_PLANE;	
		
		if(-RotatedPts[v].vy <= RotatedPts[v].vz)
			vertexFlag |= INSIDE_FRUSTRUM_PY_PLANE;	
		
		if(RotatedPts[v].vy <= RotatedPts[v].vz)
			vertexFlag |= INSIDE_FRUSTRUM_NY_PLANE;	
		
		FrustrumFlagForVertex[v] = vertexFlag;
	}
}
void TestVerticesWith_Wide_Frustrum(void)
{
	int v = Global_ShapeHeaderPtr->numpoints;

	GLOBALASSERT(v>0);

	while(v--)
	{
		char vertexFlag = 0;
		
		if(ZCLIPPINGVALUE <= RotatedPts[v].vz)
			vertexFlag |= INSIDE_FRUSTRUM_Z_PLANE;
		
		if(-RotatedPts[v].vx <= RotatedPts[v].vz*AVP_WIDE_X)
			vertexFlag |= INSIDE_FRUSTRUM_PX_PLANE;	
		
		if(RotatedPts[v].vx <= RotatedPts[v].vz*AVP_WIDE_X)
			vertexFlag |= INSIDE_FRUSTRUM_NX_PLANE;	
		
		if(-RotatedPts[v].vy <= RotatedPts[v].vz*AVP_WIDE_Y)
			vertexFlag |= INSIDE_FRUSTRUM_PY_PLANE;	
		
		if(RotatedPts[v].vy <= RotatedPts[v].vz*AVP_WIDE_Y)
			vertexFlag |= INSIDE_FRUSTRUM_NY_PLANE;	
		
		FrustrumFlagForVertex[v] = vertexFlag;
	}
}



int DecalWithinFrustrum(DECAL *decalPtr)
{
	char inFrustrumFlag;
	char noClippingFlag;

	if(!ModuleIsVisibleForRendering(decalPtr->ModuleIndex)) return 0;

    inFrustrumFlag=0;
   	noClippingFlag=INSIDE_FRUSTRUM;
	
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[0]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[1]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[2]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[3]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	
	if (inFrustrumFlag != INSIDE_FRUSTRUM) return 0;
	if (noClippingFlag == INSIDE_FRUSTRUM) return 2;

	/* yes, we need to draw poly */
	return 1;
}

int QuadWithinFrustrum(void)
{
    if(ViewmodelMuzzleActive){
        int all=31,any=0;
        for(int i=0;i<4;i++){
            RENDERVERTEX *v=&VerticesBuffer[i];
            int flags=(v->ViewZ>=ViewmodelNearClip()?1:0)|(v->ViewX<=AVP_WIDE_X*v->ViewZ?2:0)|(-v->ViewX<=AVP_WIDE_X*v->ViewZ?4:0)|
                (v->ViewY<=AVP_WIDE_Y*v->ViewZ?8:0)|(-v->ViewY<=AVP_WIDE_Y*v->ViewZ?16:0);
            all&=flags;any|=flags;
        }
        return any!=31?0:all==31?2:1;
    }

	char inFrustrumFlag;
	char noClippingFlag;

    inFrustrumFlag=0;
   	noClippingFlag=INSIDE_FRUSTRUM;
	
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[0]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[1]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[2]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[3]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	
	if (inFrustrumFlag != INSIDE_FRUSTRUM) return 0;
	if (noClippingFlag == INSIDE_FRUSTRUM) return 2;

	/* yes, we need to draw poly */
	return 1;
}

int TriangleWithinFrustrum(void)
{
	char inFrustrumFlag;
	char noClippingFlag;

    inFrustrumFlag=0;
   	noClippingFlag=INSIDE_FRUSTRUM;
	
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[0]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[1]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	{
		int vertexFlag = VertexWithinFrustrum(&VerticesBuffer[2]);
		inFrustrumFlag |= vertexFlag;
		noClippingFlag &= vertexFlag;
	}
	
	if (inFrustrumFlag != INSIDE_FRUSTRUM) return 0;
	if (noClippingFlag == INSIDE_FRUSTRUM) return 2;

	/* yes, we need to draw poly */
	return 1;
}
