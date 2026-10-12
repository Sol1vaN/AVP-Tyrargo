#ifndef _included_GammaControl_h_
#define _included_GammaControl_h_ 1

/* Vanilla frontend default used when a new profile-select session begins. */
#define DEFAULT_GAMMA_SETTING 128

extern void InitialiseGammaSettings(int gamma);
extern void UpdateGammaSettings(void);
extern int RequestedGammaSetting;


#endif
