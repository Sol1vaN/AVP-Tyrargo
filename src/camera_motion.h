#ifndef AVP_CAMERA_MOTION_H
#define AVP_CAMERA_MOTION_H
#ifdef __cplusplus
extern "C" {
#endif
/* Menu indices are stable through explicit profile encoding. Core bits:
 * 1=sway, 2=bobbing, 4=HUD shake. */
int CameraMotionFlags(int selection);
extern int MarineCameraMode;
extern int HUDShakeIntensity;
extern int HUDSwayIntensity, HUDBobbingIntensity;
double CameraBobbingIntensityGain(int value);
double CameraMotionIntensityGain(int value);
typedef struct {
 double dt, distance, yawDelta, pitchDelta, pitch, eyeHeight, verticalVelocity, zoom;
 double localSide,localForward,bodyHeight,reloadSample[3],reloadGain;
 /* Signed grounded support movement; only descent bypasses stair smoothing. */
 double supportHeightDelta;
 int reloadSampleValid;
 double equipSample[3],equipProfile[3],equipMass,equipSize,equipGain,equipProgress;
 int equipPhase,equipSampleValid;
 int active, grounded, crouched, reloading, weapon;
} CAMERA_MOTION_INPUT;
typedef struct {
 double camera[3], cameraPitch, cameraRoll, weapon[3], weaponAngle[3];
 int step;
} CAMERA_MOTION_OUTPUT;
typedef struct { double x,v; } MOTION_SPRING;
typedef struct {
 MOTION_SPRING speed, eye, impact, yaw, pitch, reload;
 MOTION_SPRING look,side,forward,stance,stair,bob[3],reloadMotion[3],weaponImpact,equipMotion[5],equipCamera[5];
 double impactTime[2],impactAmplitude[2];
 double weaponImpactTime[2],weaponImpactAmplitude[2];
 double phase,headPhase,weaponPhase,reloadTime,bodyHeight,reloadBase[3];
 int reloadBaseValid;
 double equipBase[3]; int equipBaseValid,equipPhase;
 int initialized,grounded,reloading,weapon,mode;
 CAMERA_MOTION_OUTPUT output;
 /* Independent, unscaled presentation contributions; integrators stay authored. */
 CAMERA_MOTION_OUTPUT bobbingOutput, swayOutput;
} CAMERA_MOTION_STATE;
void CameraMotionReset(CAMERA_MOTION_STATE *s);
void CameraMotionCompose(const CAMERA_MOTION_STATE *s,CAMERA_MOTION_OUTPUT *out,int sway,int bobbing);
void CameraMotionUpdate(CAMERA_MOTION_STATE *s,const CAMERA_MOTION_INPUT *in,int mode);
int CameraMotionSurface(const char *texture,int water);
unsigned int CameraMotionPack(unsigned int hud,int mode);
int CameraMotionUnpack(unsigned int packed);
/* Game adapter: one update per view; render-only weapon transform. */
void MarineMotionUpdateCamera(void);
void MarineMotionReset(void);
void MarineMotionGeometryView(double *view,double *eye,double xs,double ys,double zs);
void MarineMotionViewMatrix(float *view,double xScale,double yScale,double zScale);
void MarineMotionWeaponPose(double m[3][3],double t[3]);
void MarineMotionWater(int water);
int MarineMotionOwnsWaterSteps(void);
#ifdef __cplusplus
}
#endif
#endif
