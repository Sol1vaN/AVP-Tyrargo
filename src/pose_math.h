#ifndef TYRARGO_POSE_MATH_H
#define TYRARGO_POSE_MATH_H
#include <math.h>
#include <string.h>
/* Render-only, unit quaternions in x,y,z,w order. No filtering or phase lag. */
static inline void PoseNormalize(double q[4]) {
 double n=sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);int i;
 if(n<1e-15){q[0]=q[1]=q[2]=0;q[3]=1;return;}
 for(i=0;i<4;i++)q[i]/=n;
}
static inline void PoseSlerp(double out[4],const double qa[4],const double qb[4],double t) {
 double a[4],b[4],dot=0,wa,wb;int i;
 memcpy(a,qa,sizeof a);memcpy(b,qb,sizeof b);PoseNormalize(a);PoseNormalize(b);
 for(i=0;i<4;i++)dot+=a[i]*b[i];
 if(dot<0){dot=-dot;for(i=0;i<4;i++)b[i]=-b[i];}
 if(dot>1)dot=1;if(t<0)t=0;if(t>1)t=1;
 if(dot>.9995){wa=1-t;wb=t;}
 else{double theta=acos(dot),den=sin(theta);wa=sin((1-t)*theta)/den;wb=sin(t*theta)/den;}
 for(i=0;i<4;i++)out[i]=wa*a[i]+wb*b[i];PoseNormalize(out);
}
static inline void PoseMultiplyQuat(double out[4],const double a[4],const double b[4]) {
 double r[4]={a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
 a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
 a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
 a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]};
 memcpy(out,r,sizeof r);PoseNormalize(out);
}
static inline void PoseQuatMatrix(double r[9],const double q[4]) {
 /* Transpose QuatToMat's storage to the engine's vector convention. */
 double x=q[0],y=q[1],z=q[2],w=q[3];
 r[0]=1-2*(y*y+z*z);r[1]=2*(x*y+w*z);r[2]=2*(x*z-w*y);
 r[3]=2*(x*y-w*z);r[4]=1-2*(x*x+z*z);r[5]=2*(y*z+w*x);
 r[6]=2*(x*z+w*y);r[7]=2*(y*z-w*x);r[8]=1-2*(x*x+y*y);
}
static inline void PoseCompose(double outR[9],double outT[3],const double pR[9],const double pT[3],const double r[9],const double t[3]) {
 double resultR[9],resultT[3];int i,j,k;
 for(i=0;i<3;i++){
  resultT[i]=pT[i];for(k=0;k<3;k++)resultT[i]+=pR[i*3+k]*t[k];
  for(j=0;j<3;j++){resultR[i*3+j]=0;for(k=0;k<3;k++)resultR[i*3+j]+=pR[i*3+k]*r[k*3+j];}
 }
 memcpy(outR,resultR,sizeof resultR);memcpy(outT,resultT,sizeof resultT);
}
#endif
