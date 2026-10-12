#include "tyrargo_instrumentation.h"
#include "motion_params.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif
const MOTION_PARAM MotionParams[MP_COUNT]={
#define MP(id,key,en,fr,es,de,it,sv,unit,lo,hi,step,def,group) {key,{en,fr,es,de,it,sv},unit,lo,hi,step,def,group},
#include "motion_params.def"
#undef MP
};
double MotionValues[MP_COUNT]={
#define MP(id,key,en,fr,es,de,it,sv,unit,lo,hi,step,def,group) def,
#include "motion_params.def"
#undef MP
};
static char directory[4096],csvPath[4096];
static int status; /* 0 defaults, 1 loaded, 2 changed, 3 exported, 4 error */
static FILE *openPath(const char *path,const char *mode){
#ifdef _WIN32
 wchar_t w[4096],m[8];
 if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,w,4096)||!MultiByteToWideChar(CP_UTF8,0,mode,-1,m,8))return NULL;
 return _wfopen(w,m);
#else
 return fopen(path,mode);
#endif
}
static void load(void){
 FILE*f=openPath(csvPath,"rb");char line[256];double next[MP_COUNT];int seen[MP_COUNT]={0},count=0,valid=1;
 if(!f){if(errno!=ENOENT)status=4;return;}
 for(int i=0;i<MP_COUNT;i++)next[i]=MotionParams[i].initial;
 if(!fgets(line,sizeof line,f))valid=0;
 else{char*h=line;h[strcspn(h,"\r\n")]=0;if(!strncmp(h,"\xef\xbb\xbf",3))h+=3;if(strcmp(h,"parameter,value"))valid=0;}
 while(valid&&fgets(line,sizeof line,f)){
  char*sep=strchr(line,','),*end;int i;double value;
  if(!sep){valid=0;break;}*sep++=0;sep[strcspn(sep,"\r\n")]=0;
  errno=0;value=strtod(sep,&end);
  if(errno||end==sep||*end||!isfinite(value)){valid=0;break;}
  for(i=0;i<MP_COUNT;i++)if(!strcmp(line,MotionParams[i].key))break;
  if(i==MP_COUNT||seen[i]||value<MotionParams[i].min||value>MotionParams[i].max){valid=0;break;}
  seen[i]=1;next[i]=value;count++;
 }
 if(ferror(f))valid=0;fclose(f);
 int completeLegacy=(count==MP_IMPACT_WRESPONSE||count==MP_JUMP_DURATION||count==MP_W_JUMP_IMPULSE);
 for(int i=0;i<MP_COUNT;i++)if(seen[i]!=(i<count))completeLegacy=0;
 if(valid&&(count==MP_COUNT||completeLegacy)){
  /* Old exports had shared impulses/durations. Seed the new weapon controls
     once from those loaded values; full exports retain independent settings. */
  if(completeLegacy){
   next[MP_W_JUMP_IMPULSE]=next[MP_JUMP_IMPULSE];next[MP_W_LAND_IMPULSE]=next[MP_LAND_IMPULSE];
   next[MP_W_JUMP_DURATION]=next[MP_JUMP_DURATION];next[MP_W_LAND_DURATION]=next[MP_LAND_DURATION];
  }
  memcpy(MotionValues,next,sizeof next);status=1;
 }else status=4;
}
void MotionParamsInitialize(const char*base){
 int i;for(i=0;i<MP_COUNT;i++)MotionValues[i]=MotionParams[i].initial;
 status=0;directory[0]=csvPath[0]=0;
 if(!TYRARGO_MOTION_CALIBRATION)return;
 if(!base||strlen(base)>sizeof directory-64){status=4;return;}
 snprintf(directory,sizeof directory,"%s%slogs",base,(*base&&base[strlen(base)-1]!='/'&&base[strlen(base)-1]!='\\')?"/":"");
 snprintf(csvPath,sizeof csvPath,"%s/marine_camera_motion.csv",directory);load();
}
void MotionParamAdjust(int i,int direction){
 const MOTION_PARAM*p;if(!TYRARGO_MOTION_CALIBRATION||i<0||i>=MP_COUNT)return;p=&MotionParams[i];
 double v=MotionValues[i]+direction*p->step;MotionValues[i]=fmax(p->min,fmin(p->max,v));status=2;
}
void MotionParamDefault(int i){if(TYRARGO_MOTION_CALIBRATION&&i>=0&&i<MP_COUNT){MotionValues[i]=MotionParams[i].initial;status=2;}}
int MotionParamsExport(void){
 char temp[4096];FILE*f;int ok=1,i;
 if(!TYRARGO_MOTION_CALIBRATION||!csvPath[0]){status=4;return 0;}
#ifdef _WIN32
 wchar_t dir[4096],src[4096],dst[4096];
 if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,directory,-1,dir,4096)||(!CreateDirectoryW(dir,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)){status=4;return 0;}
#else
 if(mkdir(directory,0755)&&errno!=EEXIST){status=4;return 0;}
#endif
 snprintf(temp,sizeof temp,"%s.tmp",csvPath);f=openPath(temp,"wb");if(!f){status=4;return 0;}
 if(fprintf(f,"parameter,value\n")<0)ok=0;
 for(i=0;i<MP_COUNT;i++)if(!isfinite(MotionValues[i])||fprintf(f,"%s,%.17g\n",MotionParams[i].key,MotionValues[i])<0)ok=0;
 if(fclose(f))ok=0;
#ifdef _WIN32
 if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,temp,-1,src,4096)||!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,csvPath,-1,dst,4096))ok=0;
 if(ok&&!MoveFileExW(src,dst,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))ok=0;
#else
 if(ok&&rename(temp,csvPath))ok=0;
#endif
 status=ok?3:4;return ok;
}
const char*MotionParamsStatus(int language){
 static const char*text[5][6]={
 {"Source defaults","Valeurs source","Bases del source","Quellcodewerte","Valori sorgente","Källkodsvärden"},
 {"CSV loaded","CSV chargé","CSV cargado","CSV geladen","CSV caricato","CSV inläst"},
 {"Changes not exported","Modifications non exportées","Cambios sin exportar","Änderungen nicht exportiert","Modifiche non esportate","Ändringar ej exporterade"},
 {"CSV exported to logs","CSV exporté dans logs","CSV exportado en logs","CSV nach logs exportiert","CSV esportato in logs","CSV exporterat till logs"},
 {"CSV error; previous values kept","Erreur CSV ; valeurs conservées","Error CSV; valores conservados","CSV-Fehler; Werte beibehalten","Errore CSV; valori conservati","CSV-fel; värden behållna"}};
 return text[status][language>=0&&language<6?language:0];
}
#include "motion_help.inc"

const char*MotionParamsHelp(int language,int exporting){
 static const char*text[2][6]={
 {"Marine. Left/right: adjust. ENTER: source default. Enable Sway/Bobbing in Camera mode. Export to save.","Marine. Gauche/droite : régler. ENTRÉE : valeur source. Activer Sway/Bobbing dans Mode caméra. Exporter pour conserver.","Marine. Flechas: ajustar. ENTER: valor del source. Activa Sway/Bobbing en Cámara modo. Usa export para guardar.","Marine. Links/rechts: ändern. ENTER: Quellcodewert. Sway/Bobbing im Kameramodus aktivieren. Export speichert.","Marine. Sinistra/destra: regola. INVIO: valore sorgente. Attiva Sway/Bobbing in Modalità telecamera. Export salva.","Marine. Vänster/höger: justera. ENTER: källkodsvärde. Aktivera Sway/Bobbing i Kameraläge. Export sparar."},
 {"ENTER: save all parameters to logs/marine_camera_motion.csv. Loaded on next launch; Vanilla bypasses motion.","ENTRÉE : enregistrer dans logs/marine_camera_motion.csv. Chargé au prochain démarrage ; Vanilla ignore les effets.","ENTER: guarda todos los parámetros en logs/marine_camera_motion.csv. Se lee al iniciar; Vanilla omite los efectos.","ENTER: alle Werte nach logs/marine_camera_motion.csv speichern. Beim Start geladen; Vanilla ohne Effekte.","INVIO: salva in logs/marine_camera_motion.csv. Caricato al prossimo avvio; Vanilla esclude gli effetti.","ENTER: spara till logs/marine_camera_motion.csv. Läses vid nästa start; Vanilla utan effekter."}};
 return text[!!exporting][language>=0&&language<6?language:0];
}

/* Presentation order only: stable parameter IDs preserve CSV schemas and defaults. */
int MotionParamMenuIndex(int position)
{
 static const int order[MP_COUNT]={
  MP_SPEED, MP_SPEED_RESPONSE, MP_ZOOM_MIN,
  MP_WEAPON_NEAR,
  MP_WB_RATE, MP_WB_MAX_HZ, MP_WB_X, MP_WB_Y, MP_WB_Z, MP_WB_ROLL, MP_WB_RESPONSE, MP_BOB_CROUCH,
  MP_SWAY_RESPONSE, MP_SWAY_GAIN, MP_SWAY_LIMIT, MP_SWAY_X, MP_SWAY_Y, MP_SWAY_PITCH, MP_SWAY_YAW, MP_SWAY_ROLL, MP_SWAY_CAMERA,
  MP_LOOK_GAIN, MP_LOOK_RESPONSE,
  MP_MOVE_RESPONSE, MP_MOVE_SIDE, MP_MOVE_DEPTH, MP_MOVE_ROLL,
  MP_HEAD_RATE, MP_HEAD_X, MP_HEAD_Y, MP_HEAD_ROLL,
  MP_EYE_RESPONSE, MP_STANCE_GAIN, MP_STANCE_LIMIT, MP_STANCE_RESPONSE,
  MP_STAIR_RESPONSE, MP_STAIR_HEIGHT, MP_STAIR_WEAPON,
  /* Independent weapon family, then independent camera family. */
  MP_IMPACT_WEAPON, MP_W_JUMP_IMPULSE, MP_W_JUMP_DURATION, MP_W_LAND_IMPULSE, MP_W_LAND_DURATION, MP_IMPACT_WLIMIT, MP_IMPACT_WRESPONSE, MP_W_IMPACT_ROLL,
  MP_IMPACT_RESPONSE, MP_JUMP_IMPULSE, MP_JUMP_DURATION, MP_LAND_IMPULSE, MP_LAND_DURATION, MP_IMPACT_LIMIT, MP_IMPACT_ROLL,
  MP_RELOAD_RESPONSE, MP_RELOAD_X, MP_RELOAD_Y, MP_RELOAD_Z, MP_RELOAD_PITCH, MP_RELOAD_ROLL, MP_RELOAD_LIMIT, MP_RELOAD_ANGLE, MP_RELOAD_WEAPON, MP_RELOAD_GUN_0, MP_RELOAD_GUN_1, MP_RELOAD_GUN_2, MP_RELOAD_GUN_3, MP_RELOAD_GUN_4, MP_RELOAD_GUN_5, MP_RELOAD_GUN_6, MP_RELOAD_GUN_7, MP_RELOAD_GUN_8,
  MP_EQUIP_GAIN, MP_EQUIP_RESPONSE, MP_EQUIP_LIMIT, MP_EQUIP_ANGLE, MP_EQUIP_CAMERA, MP_EQUIP_CRESPONSE, MP_EQUIP_CLIMIT, MP_EQUIP_CPITCH, MP_EQUIP_CROLL, MP_EQUIP_GUN_0, MP_EQUIP_GUN_1, MP_EQUIP_GUN_2, MP_EQUIP_GUN_3, MP_EQUIP_GUN_4, MP_EQUIP_GUN_5, MP_EQUIP_GUN_6, MP_EQUIP_GUN_7, MP_EQUIP_GUN_8,
  MP_STEPS_ENABLE, MP_STEP_LENGTH, MP_RUN_LENGTH, MP_CROUCH_LENGTH, MP_VOL_GENERIC, MP_VOL_METAL, MP_VOL_SOFT, MP_VOL_WOOD, MP_VOL_WATER, MP_VOL_CROUCH, MP_PITCH_METAL, MP_PITCH_SOFT, MP_PITCH_WOOD, MP_PITCH_VARIATION,
 };
 return position>=0 && position<MP_COUNT ? order[position] : -1;
}
