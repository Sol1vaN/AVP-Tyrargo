#ifndef NPC_AIM_MATH_H
#define NPC_AIM_MATH_H
#include <math.h>
/* Millimetres, seconds, positive Y down; stable low arc with target lead. */
static int NPCAimSolve(const double offset[3],const double velocity[3],double speed,double gravity,double direction[3],double *flight){
 double t=0,p[3];if(speed<=0)return 0;
 for(int k=0;k<12;k++){
  for(int j=0;j<3;j++)p[j]=offset[j]+velocity[j]*t;
  double c=p[0]*p[0]+p[1]*p[1]+p[2]*p[2],b=speed*speed+gravity*p[1],a=.25*gravity*gravity;
  if(c<1 || b<=0)return 0;double d=b*b-4*a*c;if(d<0)return 0;
  double next=sqrt(2*c/(b+sqrt(d)));if(!isfinite(next)||next>4)return 0;
  if(fabs(next-t)<.00001){t=next;break;}t=next;
 }
 for(int j=0;j<3;j++)p[j]=offset[j]+velocity[j]*t;
 direction[0]=p[0]/(speed*t);direction[1]=(p[1]-.5*gravity*t*t)/(speed*t);direction[2]=p[2]/(speed*t);
 double n=sqrt(direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2]);
 if(!isfinite(n)||fabs(n-1)>.002)return 0;
 for(int j=0;j<3;j++)direction[j]/=n;*flight=t;return 1;
}
/* Exact spherical exponential response; independent of frame rate. */
static void NPCAimConverge(double aim[3],const double target[3],double dt,double response){
 double dot=aim[0]*target[0]+aim[1]*target[1]+aim[2]*target[2];if(dot>1)dot=1;if(dot< -1)dot=-1;
 double alpha=1-exp(-response*fmax(0,dt)),angle=acos(dot),a,b;
 if(angle<.00001){for(int j=0;j<3;j++)aim[j]=target[j];return;}
 if(dot<-.9999){double axis[3]={-aim[2],0,aim[0]},n=hypot(axis[0],axis[2]);if(n<.01){axis[0]=1;axis[2]=0;n=1;}double theta=angle*alpha;for(int j=0;j<3;j++)aim[j]=aim[j]*cos(theta)+axis[j]/n*sin(theta);return;}
 a=sin((1-alpha)*angle)/sin(angle);b=sin(alpha*angle)/sin(angle);for(int j=0;j<3;j++)aim[j]=a*aim[j]+b*target[j];
}
#endif
