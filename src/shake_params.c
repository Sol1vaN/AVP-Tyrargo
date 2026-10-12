#include "shake_params.h"
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
int ShakeEnabled;
const SHAKE_PARAM ShakeParams[HP_COUNT]={
#define SP(id,key,en,fr,es,de,it,sv,unit,lo,hi,step,def,kind) {key,{en,fr,es,de,it,sv},unit,lo,hi,step,def,kind},
#include "shake_params.def"
#undef SP
};
double ShakeValues[HP_COUNT]={
#define SP(id,key,en,fr,es,de,it,sv,unit,lo,hi,step,def,kind) def,
#include "shake_params.def"
#undef SP
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
 FILE*f=openPath(csvPath,"rb");char line[256];double next[HP_COUNT];int seen[HP_COUNT]={0},count=0,valid=1;
 if(!f){if(errno!=ENOENT)status=4;return;}
 for(int i=0;i<HP_COUNT;i++)next[i]=ShakeParams[i].initial;
 if(!fgets(line,sizeof line,f))valid=0;
 else{char*h=line;h[strcspn(h,"\r\n")]=0;if(!strncmp(h,"\xef\xbb\xbf",3))h+=3;if(strcmp(h,"parameter,value"))valid=0;}
 while(valid&&fgets(line,sizeof line,f)){
  char*sep=strchr(line,','),*end;int i;double value;
  if(!sep){valid=0;break;}*sep++=0;sep[strcspn(sep,"\r\n")]=0;
  errno=0;value=strtod(sep,&end);
  if(errno||end==sep||*end||!isfinite(value)){valid=0;break;}
  for(i=0;i<HP_COUNT;i++)if(!strcmp(line,ShakeParams[i].key))break;
  if(i==HP_COUNT||seen[i]||value<ShakeParams[i].min||value>ShakeParams[i].max){valid=0;break;}
  seen[i]=1;next[i]=value;count++;
 }
 if(ferror(f))valid=0;fclose(f);
 /* Accept complete exports from 190 (209), 191 (211), or this version.
  * Only exact historic prefixes qualify; arbitrary missing rows are invalid. */
 int legacySize=count==HP_RUN_BOTTOM_COMPENSATION?HP_RUN_BOTTOM_COMPENSATION:
                count==HP_WALK_BOTTOM_COMPENSATION?HP_WALK_BOTTOM_COMPENSATION:-1;
 int legacy=legacySize>=0;
 for(int i=0;i<HP_COUNT;i++)if(seen[i]!=(i<legacySize))legacy=0;
 if(valid&&(count==HP_COUNT||legacy)){
  memcpy(ShakeValues,next,sizeof next);status=1;
 }else status=4;
}
void ShakeParamsInitialize(const char*base){
 if(!SHAKE_CALIBRATION_AVAILABLE)ShakeEnabled=0;
 int i;for(i=0;i<HP_COUNT;i++)ShakeValues[i]=ShakeParams[i].initial;
 status=0;directory[0]=csvPath[0]=0;
 if(!ShakeEnabled)return;
 if(!base||strlen(base)>sizeof directory-64){status=4;return;}
 snprintf(directory,sizeof directory,"%s%slogs",base,(*base&&base[strlen(base)-1]!='/'&&base[strlen(base)-1]!='\\')?"/":"");
 snprintf(csvPath,sizeof csvPath,"%s/marine_hud_shake.csv",directory);load();
}
void ShakeParamAdjust(int i,int direction){
 const SHAKE_PARAM*p;if(!ShakeEnabled||i<0||i>=HP_COUNT)return;p=&ShakeParams[i];
 double v=ShakeValues[i]+direction*p->step;ShakeValues[i]=fmax(p->min,fmin(p->max,v));status=2;
}
void ShakeParamDefault(int i){if(ShakeEnabled&&i>=0&&i<HP_COUNT){ShakeValues[i]=ShakeParams[i].initial;status=2;}}
int ShakeParamsExport(void){
 char temp[4096];FILE*f;int ok=1,i;
 if(!ShakeEnabled||!csvPath[0]){status=4;return 0;}
#ifdef _WIN32
 wchar_t dir[4096],src[4096],dst[4096];
 if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,directory,-1,dir,4096)||(!CreateDirectoryW(dir,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)){status=4;return 0;}
#else
 if(mkdir(directory,0755)&&errno!=EEXIST){status=4;return 0;}
#endif
 snprintf(temp,sizeof temp,"%s.tmp",csvPath);f=openPath(temp,"wb");if(!f){status=4;return 0;}
 if(fprintf(f,"parameter,value\n")<0)ok=0;
 for(i=0;i<HP_COUNT;i++)if((!isfinite(ShakeValues[i])||ShakeValues[i]<ShakeParams[i].min||ShakeValues[i]>ShakeParams[i].max)||fprintf(f,"%s,%.17g\n",ShakeParams[i].key,ShakeValues[i])<0)ok=0;
 if(fclose(f))ok=0;
#ifdef _WIN32
 if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,temp,-1,src,4096)||!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,csvPath,-1,dst,4096))ok=0;
 if(ok&&!MoveFileExW(src,dst,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))ok=0;
#else
 if(ok&&rename(temp,csvPath))ok=0;
#endif
 status=ok?3:4;return ok;
}
const char*ShakeParamsStatus(int language){
 static const char*text[5][6]={
 {"Source defaults","Valeurs source","Bases del source","Quellcodewerte","Valori sorgente","Källkodsvärden"},
 {"CSV loaded","CSV chargé","CSV cargado","CSV geladen","CSV caricato","CSV inläst"},
 {"Changes not exported","Modifications non exportées","Cambios sin exportar","Änderungen nicht exportiert","Modifiche non esportate","Ändringar ej exporterade"},
 {"CSV exported to logs","CSV exporté dans logs","CSV exportado en logs","CSV nach logs exportiert","CSV esportato in logs","CSV exporterat till logs"},
 {"CSV error; previous values kept","Erreur CSV ; valeurs conservées","Error CSV; valores conservados","CSV-Fehler; Werte beibehalten","Errore CSV; valori conservati","CSV-fel; värden behållna"}};
 return text[status][language>=0&&language<6?language:0];
}

const char *ShakeParamsHelp(int language,int exporting){
 static const char *text[2][6]={
{"Marine HUD only. Enable Shake in Camera mode. Arrows: adjust; ENTER: source default. Export to save.","HUD du marine uniquement. Activer Shake dans Mode caméra. Flèches : régler ; ENTRÉE : valeur source. Export pour sauvegarder.","Sólo HUD marine. Activa Shake en Cámara modo. Flechas: ajustar; ENTER: valor del source. Export para guardar.","Nur Marine-HUD. Shake im Kameramodus aktivieren. Pfeile: einstellen; ENTER: Quellcodewert. Export speichert.","Solo HUD marine. Attiva Shake in Modalità telecamera. Frecce: regola; INVIO: valore sorgente. Export salva.","Endast marinens HUD. Aktivera Shake i Kameraläge. Pilar: justera; ENTER: källkodsvärde. Export sparar."},{"ENTER: save all sliders to logs/marine_hud_shake.csv. Read on launch with -dev2 only.","ENTRÉE : sauver tous les réglages dans logs/marine_hud_shake.csv. Lu uniquement au démarrage avec -dev2.","ENTER: guarda todos los sliders en logs/marine_hud_shake.csv. Se lee al iniciar sólo con -dev2.","ENTER: alle Regler nach logs/marine_hud_shake.csv speichern. Nur beim Start mit -dev2 gelesen.","INVIO: salva tutti i cursori in logs/marine_hud_shake.csv. Letto all’avvio solo con -dev2.","ENTER: spara alla reglage till logs/marine_hud_shake.csv. Läses bara vid start med -dev2."}};
 return text[!!exporting][language>=0&&language<6?language:0];
}
const char *ShakeParamHelp(int id,int language){
 static char text[1800];int lang=language>=0&&language<6?language:0;
 const char *low="",*high="";
 if(id<0||id>=HP_COUNT)return "";
 const SHAKE_PARAM *p=&ShakeParams[id];
 if(!strcmp(p->kind,"gain")){static const char *v[6][2]={{"Minimum: weaker or disabled reaction.","Maximum: stronger reaction."},{"Minimum : réaction faible ou nulle.","Maximum : réaction plus forte."},{"Mínimo: reacción menor o desactivada.","Máximo: reacción más intensa."},{"Minimum: schwache oder keine Reaktion.","Maximum: stärkere Reaktion."},{"Minimo: reazione debole o disattivata.","Massimo: reazione più forte."},{"Minimum: svag eller avstängd reaktion.","Maximum: starkare reaktion."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"signed")){static const char *v[6][2]={{"Minimum: motion toward the negative axis. Zero: no contribution.","Maximum: motion toward the opposite axis."},{"Minimum : mouvement vers le côté négatif. Zéro : aucun effet.","Maximum : mouvement vers le côté opposé."},{"Mínimo: gesto hacia el lado negativo del eje. Cero: sin aporte.","Máximo: gesto hacia el lado opuesto."},{"Minimum: Bewegung zur negativen Achse. Null: kein Beitrag.","Maximum: Bewegung zur Gegenseite."},{"Minimo: movimento verso il lato negativo. Zero: nessun effetto.","Massimo: movimento verso il lato opposto."},{"Minimum: rörelse mot negativ axel. Noll: inget bidrag.","Maximum: rörelse åt motsatt håll."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"limit")){static const char *v[6][2]={{"Minimum: motion is restricted early.","Maximum: permits wider motion before limiting it."},{"Minimum : mouvement limité tôt.","Maximum : mouvement plus ample avant la limite."},{"Mínimo: limita pronto el recorrido del gesto.","Máximo: permite mayor recorrido antes de limitarlo."},{"Minimum: Bewegung früh begrenzen.","Maximum: größeren Bewegungsweg zulassen."},{"Minimo: limita presto il movimento.","Massimo: consente maggiore movimento."},{"Minimum: begränsar rörelsen tidigt.","Maximum: tillåter större rörelse."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"frequency")){static const char *v[6][2]={{"Minimum: slower response or oscillations.","Maximum: faster response or oscillations."},{"Minimum : réaction ou oscillations plus lentes.","Maximum : réaction ou oscillations plus rapides."},{"Mínimo: respuesta u oscilaciones más lentas.","Máximo: respuesta u oscilaciones más rápidas."},{"Minimum: langsamere Reaktion oder Schwingungen.","Maximum: schnellere Reaktion oder Schwingungen."},{"Minimo: risposta o oscillazioni più lente.","Massimo: risposta o oscillazioni più rapide."},{"Minimum: långsammare reaktion eller svängningar.","Maximum: snabbare reaktion eller svängningar."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"damping")){static const char *v[6][2]={{"Minimum: more settling oscillation.","Maximum: less bouncing, closer to a direct return."},{"Minimum : davantage de rebonds.","Maximum : moins de rebonds, retour plus direct."},{"Mínimo: más oscilación al asentarse, como un resorte.","Máximo: menos rebote y retorno más directo."},{"Minimum: mehr Nachschwingen.","Maximum: weniger Federn, direktere Rückkehr."},{"Minimo: più oscillazioni residue.","Massimo: meno rimbalzi, ritorno più diretto."},{"Minimum: mer eftersvängning.","Maximum: mindre studs, rakare återgång."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"decay")){static const char *v[6][2]={{"Minimum: settles sooner.","Maximum: gesture persists longer."},{"Minimum : retour au repos rapide.","Maximum : geste plus persistant."},{"Mínimo: se asienta antes.","Máximo: el gesto tarda más en apagarse."},{"Minimum: schnelleres Abklingen.","Maximum: längere Nachwirkung."},{"Minimo: si ferma prima.","Massimo: gesto più persistente."},{"Minimum: stannar tidigare.","Maximum: rörelsen varar längre."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"time")){static const char *v[6][2]={{"Minimum: quicker onset.","Maximum: more gradual onset."},{"Minimum : départ rapide.","Maximum : départ plus progressif."},{"Mínimo: inicio más rápido.","Máximo: inicio más gradual."},{"Minimum: schnellerer Beginn.","Maximum: sanfterer Beginn."},{"Minimo: avvio rapido.","Massimo: avvio più graduale."},{"Minimum: snabbare start.","Maximum: mjukare start."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"weight")){static const char *v[6][2]={{"Minimum: stronger camera-follow reaction.","Maximum: weaker camera-follow reaction."},{"Minimum : suivi plus marqué.","Maximum : suivi plus discret."},{"Mínimo: reacción más fuerte al giro.","Máximo: reacción más discreta al giro."},{"Minimum: stärkeres Folgen.","Maximum: schwächeres Folgen."},{"Minimo: reazione più forte alla rotazione.","Massimo: reazione più debole."},{"Minimum: starkare följreaktion.","Maximum: svagare följreaktion."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"depth")){static const char *v[6][2]={{"Minimum: stronger perspective displacement.","Maximum: weaker perspective displacement."},{"Minimum : déplacement plus marqué.","Maximum : déplacement plus discret."},{"Mínimo: desplazamiento de perspectiva más marcado.","Máximo: desplazamiento más discreto."},{"Minimum: stärkere Perspektivverschiebung.","Maximum: schwächere Verschiebung."},{"Minimo: spostamento prospettico più forte.","Massimo: spostamento più debole."},{"Minimum: starkare perspektivförskjutning.","Maximum: svagare förskjutning."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"perspective")){static const char *v[6][2]={{"Minimum: stronger edge perspective.","Maximum: flatter perspective."},{"Minimum : perspective des bords plus forte.","Maximum : perspective plus plate."},{"Mínimo: perspectiva más pronunciada en los bordes.","Máximo: perspectiva más plana."},{"Minimum: stärkere Randperspektive.","Maximum: flachere Perspektive."},{"Minimo: prospettiva ai bordi più marcata.","Massimo: prospettiva più piatta."},{"Minimum: starkare kantperspektiv.","Maximum: plattare perspektiv."}};low=v[lang][0];high=v[lang][1];}
 if(!strcmp(p->kind,"compensation")){
  static const char *v[6][2]={
   {"Minimum: original lower HUD movement while running.","Maximum: compensate its inward displacement from running. Upper HUD and walking are unchanged."},
   {"Minimum : mouvement inférieur original en course.","Maximum : compense son déplacement vers le centre en course. HUD supérieur et marche inchangés."},
   {"Mínimo: movimiento inferior original al correr.","Máximo: compensa su separación del borde inferior al correr. No cambia el HUD superior ni caminar."},
   {"Minimum: ursprüngliche untere HUD-Bewegung beim Laufen.","Maximum: Einwärtsbewegung beim Laufen ausgleichen. Oberes HUD und Gehen bleiben unverändert."},
   {"Minimo: movimento inferiore originale durante la corsa.","Massimo: compensa lo spostamento verso il centro durante la corsa. HUD superiore e cammino invariati."},
   {"Minimum: ursprunglig nedre HUD-rörelse vid löpning.","Maximum: kompenserar rörelsen inåt vid löpning. Övre HUD och gång är oförändrade."}};
  low=v[lang][0];high=v[lang][1];
 }
 if(!strcmp(p->kind,"walk_compensation")){
  static const char *v[6][2]={
   {"Minimum: original lower HUD movement while walking.","Maximum: compensates its inward displacement while walking. Upper HUD and running settings stay unchanged."},
   {"Minimum : mouvement inférieur original en marchant.","Maximum : compense son déplacement vers le centre en marchant. HUD supérieur et réglages de course inchangés."},
   {"Mínimo: movimiento inferior original al caminar.","Máximo: compensa su separación del borde inferior al caminar. No cambia el HUD superior ni los ajustes de correr."},
   {"Minimum: ursprüngliche untere HUD-Bewegung beim Gehen.","Maximum: gleicht die Einwärtsbewegung beim Gehen aus. Oberes HUD und Laufeinstellungen bleiben unverändert."},
   {"Minimo: movimento inferiore originale camminando.","Massimo: compensa lo spostamento verso il centro camminando. HUD superiore e impostazioni di corsa invariati."},
   {"Minimum: ursprunglig nedre HUD-rörelse vid gång.","Maximum: kompenserar rörelsen inåt vid gång. Övre HUD och löpinställningar är oförändrade."}};
  low=v[lang][0];high=v[lang][1];
 }
 snprintf(text,sizeof text,"%s. [%g, %g %s] %s %s %s",p->label[lang],p->min,p->max,p->unit,low,high,ShakeParamsHelp(lang,0));return text;
}
int ShakeParamMenuIndex(int position){static const int order[HP_COUNT]={HP_LIMIT_ROLL,HP_LIMIT_X,HP_LIMIT_Y,HP_MOUSE_LIMIT,HP_MOUSE_PITCH,HP_MOUSE_PITCH_RESPONSE,HP_MOUSE_ROLL,HP_MOUSE_WEIGHT,HP_MOUSE_YAW,HP_PLANE_DEPTH,HP_PLANE_FOCAL,HP_PLANE_LIMIT,HP_RUN_GAIN,HP_RUN_BOTTOM_COMPENSATION,HP_STEP_DECAY,HP_STEP_EASE,HP_STEP_STRENGTH,HP_WALK_GAIN,HP_WALK_BOTTOM_COMPENSATION,HP_GESTURE_DECAY,HP_GESTURE_GAIN,HP_GESTURE_LIMIT,HP_RELOAD_FALLBACK_X,HP_RELOAD_FALLBACK_Y,HP_RELOAD_FALLBACK_Z,HP_RELOAD_NATIVE,HP_RELOAD_0_0,HP_RELOAD_0_1,HP_RELOAD_0_2,HP_RELOAD_1_0,HP_RELOAD_1_1,HP_RELOAD_1_2,HP_RELOAD_2_0,HP_RELOAD_2_1,HP_RELOAD_2_2,HP_RELOAD_3_0,HP_RELOAD_3_1,HP_RELOAD_3_2,HP_RELOAD_4_0,HP_RELOAD_4_1,HP_RELOAD_4_2,HP_RELOAD_5_0,HP_RELOAD_5_1,HP_RELOAD_5_2,HP_RELOAD_6_0,HP_RELOAD_6_1,HP_RELOAD_6_2,HP_RELOAD_7_0,HP_RELOAD_7_1,HP_RELOAD_7_2,HP_RELOAD_8_0,HP_RELOAD_8_1,HP_RELOAD_8_2,HP_EQUIP_ENVELOPE,HP_EQUIP_NATIVE,HP_EQUIP_0_0,HP_EQUIP_0_1,HP_EQUIP_0_2,HP_EQUIP_0_3,HP_EQUIP_NATIVE_0_0,HP_EQUIP_NATIVE_0_1,HP_EQUIP_NATIVE_0_2,HP_EQUIP_1_0,HP_EQUIP_1_1,HP_EQUIP_1_2,HP_EQUIP_1_3,HP_EQUIP_NATIVE_1_0,HP_EQUIP_NATIVE_1_1,HP_EQUIP_NATIVE_1_2,HP_EQUIP_2_0,HP_EQUIP_2_1,HP_EQUIP_2_2,HP_EQUIP_2_3,HP_EQUIP_NATIVE_2_0,HP_EQUIP_NATIVE_2_1,HP_EQUIP_NATIVE_2_2,HP_EQUIP_3_0,HP_EQUIP_3_1,HP_EQUIP_3_2,HP_EQUIP_3_3,HP_EQUIP_NATIVE_3_0,HP_EQUIP_NATIVE_3_1,HP_EQUIP_NATIVE_3_2,HP_EQUIP_4_0,HP_EQUIP_4_1,HP_EQUIP_4_2,HP_EQUIP_4_3,HP_EQUIP_NATIVE_4_0,HP_EQUIP_NATIVE_4_1,HP_EQUIP_NATIVE_4_2,HP_EQUIP_5_0,HP_EQUIP_5_1,HP_EQUIP_5_2,HP_EQUIP_5_3,HP_EQUIP_NATIVE_5_0,HP_EQUIP_NATIVE_5_1,HP_EQUIP_NATIVE_5_2,HP_EQUIP_6_0,HP_EQUIP_6_1,HP_EQUIP_6_2,HP_EQUIP_6_3,HP_EQUIP_NATIVE_6_0,HP_EQUIP_NATIVE_6_1,HP_EQUIP_NATIVE_6_2,HP_EQUIP_7_0,HP_EQUIP_7_1,HP_EQUIP_7_2,HP_EQUIP_7_3,HP_EQUIP_NATIVE_7_0,HP_EQUIP_NATIVE_7_1,HP_EQUIP_NATIVE_7_2,HP_EQUIP_8_0,HP_EQUIP_8_1,HP_EQUIP_8_2,HP_EQUIP_8_3,HP_EQUIP_NATIVE_8_0,HP_EQUIP_NATIVE_8_1,HP_EQUIP_NATIVE_8_2,HP_JUMP_ROLL,HP_JUMP_X,HP_JUMP_Y,HP_LAND_DAMPING,HP_LAND_FREQUENCY,HP_LAND_GAIN,HP_LAND_MAX,HP_LAND_ROLL,HP_LAND_X,HP_LAND_Y,HP_DAMAGE_DAMPING,HP_DAMAGE_FREQUENCY,HP_DAMAGE_GAIN,HP_DAMAGE_MAX,HP_DAMAGE_ROLL,HP_DAMAGE_X,HP_DAMAGE_Y,HP_FIRE_LIMIT,HP_FIRE_RESPONSE,HP_POSE_ANGLE,HP_POSE_POSITION,HP_POSE_SIDE,HP_SHOT_DAMPING,HP_SHOT_FREQUENCY,HP_SHOT_PHASE,HP_SHOT_0_0,HP_SHOT_0_1,HP_SHOT_0_2,HP_SHOT_1_0,HP_SHOT_1_1,HP_SHOT_1_2,HP_SHOT_4_0,HP_SHOT_4_1,HP_SHOT_4_2,HP_SHOT_6_0,HP_SHOT_6_1,HP_SHOT_6_2,HP_SHOT_7_0,HP_SHOT_7_1,HP_SHOT_7_2,HP_POSE_GAIN2,HP_RIFLE_DAMPING,HP_RIFLE_FREQUENCY,HP_RIFLE_GRENADE_ROLL,HP_RIFLE_GRENADE_Y,HP_RIFLE_HINT0,HP_RIFLE_HINT1,HP_RIFLE_HINT2,HP_RIFLE_HINT3,HP_RIFLE_HINT4,HP_RIFLE_HINT5,HP_RIFLE_HINT6,HP_RIFLE_ROLL,HP_RIFLE_X,HP_RIFLE_Y,HP_POSE_GAIN3,HP_SMART_DAMPING,HP_SMART_FREQUENCY,HP_SMART_HINT0,HP_SMART_HINT1,HP_SMART_HINT2,HP_SMART_HINT3,HP_SMART_HINT4,HP_SMART_HINT5,HP_SMART_HINT6,HP_SMART_HINT7,HP_SMART_ROLL,HP_SMART_X,HP_SMART_Y,HP_FLAME_ATTACK,HP_FLAME_RATE0,HP_FLAME_RATE1,HP_FLAME_RATE2,HP_FLAME_RELEASE,HP_FLAME_RESPONSE,HP_FLAME_ROLL,HP_FLAME_WEIGHT0,HP_FLAME_WEIGHT1,HP_FLAME_WEIGHT2,HP_FLAME_X,HP_FLAME_Y,HP_POSE_GAIN5,HP_DISC_ALTERNATE,HP_DISC_DAMPING,HP_DISC_FREQUENCY,HP_DISC_ROLL,HP_DISC_TAIL_DAMPING,HP_DISC_TAIL_FREQUENCY,HP_DISC_TAIL_ROLL,HP_DISC_TAIL_X,HP_DISC_TAIL_Y,HP_DISC_X,HP_DISC_Y,HP_POSE_GAIN8};return position>=0&&position<HP_COUNT?order[position]:-1;}
