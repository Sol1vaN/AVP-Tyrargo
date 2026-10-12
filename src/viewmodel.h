#ifndef AVP_VIEWMODEL_H
#define AVP_VIEWMODEL_H
/* Render-only camera-space pose. Gameplay world transforms remain authoritative. */
struct section_data;
extern int ViewmodelDrawActive;
extern int ViewmodelMuzzleActive;
double ViewmodelNearClip(void);
int ViewmodelFlameAnchor(double out[3]);
void ViewmodelFlameDirection(double viewDelta[3]);
void ViewmodelBeginMuzzle(void);
void ViewmodelMuzzlePoint(const VECTORCH *local,double out[3]);
extern int ViewmodelShapeActive;
extern double ViewmodelPoints[maxrotpts][3];
extern double ViewmodelLocalEye[3];
void ViewmodelUpdatePose(const VECTORCH *offset, const EULER *direction, int aim);
void ViewmodelUpdateFlash(int size, int length, const MATRIXCH *roll);
void ViewmodelBegin(DISPLAYBLOCK *dptr);
void ViewmodelEnd(void);
void ViewmodelSection(struct section_data *section);
int ViewmodelTransform(SHAPEINSTR *instruction);
#endif
