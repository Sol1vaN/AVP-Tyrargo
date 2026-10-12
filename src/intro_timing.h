#ifndef TYRARGO_INTRO_TIMING_H
#define TYRARGO_INTRO_TIMING_H
/* Absolute elapsed seconds; no frame-count increments or time buckets. */
#define INTRO99_LOGO_FADE_IN_START_SECONDS 29.0
#define INTRO99_LOGO_FADE_IN_END_SECONDS 37.0
#define INTRO99_LOGO_PULSE_START_SECONDS 36.6
#define INTRO99_LOGO_PULSE_END_SECONDS 39.0
#define INTRO99_LOGO_FADE_OUT_START_SECONDS 40.0
#define INTRO99_LOGO_FADE_OUT_END_SECONDS 44.0
#define INTRO99_LOGO_PEAK_GAIN 1.15
static inline double Intro99SmoothStep(double t) { return t*t*(3.0-2.0*t); }
static inline double Intro99LogoAlpha(double seconds) {
 static const double times[]={29.0,30.0,33.0,34.0,35.0,36.0,37.0};
 static const double values[]={0.0,.05,.50,.60,.70,.80,1.0};
 const int count=(int)(sizeof(times)/sizeof(times[0]));
 double t;
 if(seconds<INTRO99_LOGO_FADE_IN_START_SECONDS || seconds>=INTRO99_LOGO_FADE_OUT_END_SECONDS)return 0.0;
 if(seconds<INTRO99_LOGO_FADE_IN_END_SECONDS){
  /* Monotone cubic Hermite interpolation (weighted harmonic tangents).
   * Anchor values are NOT steps: interpolate at every real render time.
   * Zero endpoint slopes avoid a cut at first appearance and full opacity. */
  int i=0;while(i<count-2 && seconds>times[i+1])++i;
  double h=times[i+1]-times[i],slopes[2];
  for(int k=0;k<2;k++){
   int n=i+k;slopes[k]=0.0;
   if(n>0 && n<count-1){
    double hp=times[n]-times[n-1],hn=times[n+1]-times[n];
    double dp=(values[n]-values[n-1])/hp,dn=(values[n+1]-values[n])/hn;
    double w1=2.0*hn+hp,w2=hn+2.0*hp;
    slopes[k]=(w1+w2)/(w1/dp+w2/dn);
   }
  }
  t=(seconds-times[i])/h;
  return (2*t*t*t-3*t*t+1)*values[i]+(t*t*t-2*t*t+t)*h*slopes[0]
        +(-2*t*t*t+3*t*t)*values[i+1]+(t*t*t-t*t)*h*slopes[1];
 }
 if(seconds<INTRO99_LOGO_FADE_OUT_START_SECONDS)return 1.0;
 /* Preserve the existing smooth fade-out shape/duration, shifted to 40 s. */
 t=(seconds-INTRO99_LOGO_FADE_OUT_START_SECONDS)/(INTRO99_LOGO_FADE_OUT_END_SECONDS-INTRO99_LOGO_FADE_OUT_START_SECONDS);
 return 1.0-t*t*(3.0-2.0*t);
}
static inline double Intro99LogoGain(double seconds) {
 double alpha=Intro99LogoAlpha(seconds),pulse=0.0;
 /* Quick, continuous musical accent at 37 s, followed by a longer
  * two-second return to normal. Full opacity is held until fade-out at 40 s. */
 if(seconds>INTRO99_LOGO_PULSE_START_SECONDS && seconds<INTRO99_LOGO_FADE_IN_END_SECONDS)
  pulse=Intro99SmoothStep((seconds-INTRO99_LOGO_PULSE_START_SECONDS)/(INTRO99_LOGO_FADE_IN_END_SECONDS-INTRO99_LOGO_PULSE_START_SECONDS));
 else if(seconds>=INTRO99_LOGO_FADE_IN_END_SECONDS && seconds<INTRO99_LOGO_PULSE_END_SECONDS)
  pulse=1.0-Intro99SmoothStep((seconds-INTRO99_LOGO_FADE_IN_END_SECONDS)/(INTRO99_LOGO_PULSE_END_SECONDS-INTRO99_LOGO_FADE_IN_END_SECONDS));
 return alpha+(INTRO99_LOGO_PEAK_GAIN-1.0)*pulse;
}
#endif
