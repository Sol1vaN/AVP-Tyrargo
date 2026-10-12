#ifndef TYRARGO_HUD_MOTION_H
#define TYRARGO_HUD_MOTION_H
#ifdef __cplusplus
extern "C" {
#endif
/* Presentation units are metres, camera-local: right, down, forward.
 * Original reference: Game03 CHUD_UnifiedAsset (PC depth .85, quaternion
 * difference scalar x4, impulse damping .9 twice). See probe reference notes. */
typedef struct { double w,x,y,z; } HUD_QUAT;
typedef struct {
 int initialized,mode;
 double remainder;
 HUD_QUAT lastView,history,previousRelative,relative,renderRelative;
 double impulse[3],position[3];
 /* Isolated smoothed stride and damped screen-plane event channels. */
 double stepPending,stepImpulse,stepPosition;
 double eventX[3][3],eventV[3][3];
 /* Independent weapon responses: rifle, smartgun, disc kick/follow-through. */
 double weaponX[4][3],weaponV[4][3];
 unsigned int weaponShotSerial[3];
 double fireTarget[3],fireX[3],fireV[3];
 double flameTime,flameLevel,flameVelocity,flameX[3],flameV[3];
 int flameActive;
 /* Run-only stride history survives run/walk transitions until it decays. */
 double runPending,runImpulse,runPosition;
 /* Presentation-only pitch recovery, never fed back to the gameplay camera. */
 double pitchInput,pitchPosition,pitchVelocity;
 /* Walk-only history allows independent compensation and a continuous tail. */
 double walkPending,walkImpulse,walkPosition;
} HUD_MOTION_STATE;
/* event kind: 0=shot, 1=jump/landing, 2=damage; x/y in screen-height
 * fractions, roll in radians. Impulses change velocity, never position. */
void HudMotionEvent(HUD_MOTION_STATE *state,int kind,double x,double y,double roll);
void HudMotionStep(HUD_MOTION_STATE *state);
void HudMotionWalkStep(HUD_MOTION_STATE *state,double gain);
void HudMotionRunStep(HUD_MOTION_STATE *state,double gain);
double HudMotionBottomCompensation(const HUD_MOTION_STATE *state,int width,int height,int ox,int oy,const double matrix[9]);
float HudMotionTransformPoint(const double matrix[9],double bottomCorrection,int top,int height,float *x,float *y);
void HudMotionStepScaled(HUD_MOTION_STATE *state,double gain);
void HudMotionWeaponShot(HUD_MOTION_STATE *state,int weapon,int count);
void HudMotionFirePose(HUD_MOTION_STATE *state,const double pose[3]);
void HudMotionFlame(HUD_MOTION_STATE *state,int active);
void MarineHUDMotionDamage(double damage);
void HudMotionReset(HUD_MOTION_STATE *state);
HUD_QUAT HudMotionOrientation(const double rotation[3][3]);
HUD_QUAT HudMotionAngles(double pitch,double yaw,double roll);
void HudMotionUpdate(HUD_MOTION_STATE *state,double dt,HUD_QUAT view,const double impulse[3],int mode);
void HudMotionMatrix(const HUD_MOTION_STATE *state,int width,int height,int ox,int oy,double matrix[9]);
void HudMotionMatrixIntensity(const HUD_MOTION_STATE*,int,int,int,int,double[9],double);
double HudMotionBottomCompensationIntensity(const HUD_MOTION_STATE*,int,int,int,int,const double[9],double);
/* Engine adapter. Updates once after the native weapon pose; drawing is read-only. */
void MarineHUDMotionUpdate(void);
void MarineHUDMotionPrepare(int width,int height,int ox,int oy);
float MarineHUDMotionTransform(float *x,float *y);
#ifdef __cplusplus
}
#endif
#endif
