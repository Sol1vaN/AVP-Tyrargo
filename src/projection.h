#ifndef AVP_PROJECTION_H
#define AVP_PROJECTION_H
/* FOV controls describe horizontal degrees at 4:3. Wider views are Hor+. */
typedef struct AVP_PROJECTION_STATE {
    VIEWDESCRIPTORBLOCK *view;
    int legacyX, legacyY;
    double x, y;
    int clipX, clipY;
} AVP_PROJECTION_STATE;
#ifdef __cplusplus
extern "C" {
#endif
extern AVP_PROJECTION_STATE AvpProjection;
double AvpProjectionDistance(int width, int height, double fov43);
void AvpSetProjection(VIEWDESCRIPTORBLOCK *view, double distance);
void AvpSetViewFOV(VIEWDESCRIPTORBLOCK *view, double fov43);
double AvpProjectionX(const VIEWDESCRIPTORBLOCK *view);
double AvpProjectionY(const VIEWDESCRIPTORBLOCK *view);
#ifdef __cplusplus
}
#endif
#endif
