#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "equipmnt.h"
#include "player_fov.h"
#include "dev_viewmodel.h"
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/stat.h>
#endif

int DevViewmodelEnabled;
extern int Alien_Visible_Weapon; /* 0: arms, 1: tail; both use WEAPON_ALIEN_CLAW. */
typedef struct { int species, weapon; const char *speciesKey, *weaponKey, *label; int compiledRestZ; } DevWeapon;
/* Final calibration imported by user request; CSV SHA256:
 * 8d204f3930750d4d5962fa3033efb052109e780a1591235db60f40a7ee5efbc4
 * These absolute RestPosition Z contributions also apply without -dev. */
static const DevWeapon DevWeapons[DEV_VIEWMODEL_COUNT] = {
 {I_Marine,WEAPON_MARINE_PISTOL,"marine","WEAPON_MARINE_PISTOL","Marine: Pistola",-68},
 {I_Marine,WEAPON_TWO_PISTOLS,"marine","WEAPON_TWO_PISTOLS","Marine: Pistolas",-48},
 {I_Marine,WEAPON_PULSERIFLE,"marine","WEAPON_PULSERIFLE","Marine: Rifle de pulsos",-46},
 {I_Marine,WEAPON_SMARTGUN,"marine","WEAPON_SMARTGUN","Marine: Smartgun",-45},
 {I_Marine,WEAPON_MINIGUN,"marine","WEAPON_MINIGUN","Marine: Minigun",-7},
 {I_Marine,WEAPON_FLAMETHROWER,"marine","WEAPON_FLAMETHROWER","Marine: Lanzallamas",-29},
 {I_Marine,WEAPON_GRENADELAUNCHER,"marine","WEAPON_GRENADELAUNCHER","Marine: Lanzagranadas",-99},
 {I_Marine,WEAPON_SADAR,"marine","WEAPON_SADAR","Marine: SADAR",-76},
 {I_Marine,WEAPON_FRISBEE_LAUNCHER,"marine","WEAPON_FRISBEE_LAUNCHER","Marine: Lanza-discos",58},
 {I_Predator,WEAPON_PRED_WRISTBLADE,"predator","WEAPON_PRED_WRISTBLADE","Predator: Cuchilla",21},
 {I_Predator,WEAPON_PRED_SHOULDERCANNON,"predator","WEAPON_PRED_SHOULDERCANNON","Predator: Canon hombro",24},
 {I_Predator,WEAPON_PRED_MEDICOMP,"predator","WEAPON_PRED_MEDICOMP","Predator: Medicomp",1},
 {I_Predator,WEAPON_PRED_DISC,"predator","WEAPON_PRED_DISC","Predator: Disco",36},
 {I_Predator,WEAPON_PRED_PISTOL,"predator","WEAPON_PRED_PISTOL","Predator: Pistola",-176},
 {I_Predator,WEAPON_PRED_RIFLE,"predator","WEAPON_PRED_RIFLE","Predator: Speargun",-50},
 {I_Alien,WEAPON_ALIEN_CLAW,"alien","WEAPON_ALIEN_CLAW","Alien: Brazos",-230},
 {I_Alien,WEAPON_ALIEN_CLAW,"alien","ALIEN_TAIL","Alien: Cola",67}
};
static int DevZ[DEV_VIEWMODEL_COUNT], DevReady, DevEdit = -1;
static int64_t DevKeyFraction;
static char DevDirectory[4096], DevCSVPath[4096], DevMessage[160];
/* Probe 200: user export SHA256 14b6137716dc8febb025391268636fdf858691bad934c0fe8c5c499adbee7127 */
static const int DevFlameDefaults[6]={-44,6,0,0,0,0};
static int DevFlame[6];
static int64_t DevFlameFraction[3];
static char DevFlamePath[4096];
static const char *DevFlameNames[6]={"x","y","z","pitch","yaw","roll"};
static const char *DevFlameLabels[6]={"Fuego: X (derecha)","Fuego: Y (abajo)","Fuego: Z (adelante)","Fuego: pitch","Fuego: yaw","Fuego: roll"};
static int DevFlameExport(void);
static void DevFlameLoad(void);
int DevViewmodelMinimum(int row){return row<17?DEV_VIEWMODEL_Z_MIN:row<20?-1000:-1800;}
int DevViewmodelMaximum(int row){return -DevViewmodelMinimum(row);}


static void DevDefaults(void)
{
 int i;
 if (DevReady) return;
 for (i=0;i<DEV_VIEWMODEL_COUNT;i++) DevZ[i]=DevWeapons[i].compiledRestZ;
 memcpy(DevFlame,DevFlameDefaults,sizeof DevFlame);
 DevReady=1;
}
static int DevRow(int weaponID)
{
 int i;
 /* The tail is a visible variant, not a separate inventory weapon. */
 if(weaponID==WEAPON_ALIEN_CLAW) {
  if(Alien_Visible_Weapon==0)return 15;
  if(Alien_Visible_Weapon==1)return 16;
  return -1;
 }
 for(i=0;i<DEV_VIEWMODEL_COUNT;i++) if(DevWeapons[i].weapon==weaponID) return i;
 return -1;
}
#if defined(_WIN32)
static int DevWidePath(const char *path, wchar_t *wide)
{
 return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,4096)>0;
}
#endif
static FILE *DevOpen(const char *path, const char *mode)
{
#if defined(_WIN32)
 wchar_t wide[4096], wmode[8];
 if(!DevWidePath(path,wide) || !MultiByteToWideChar(CP_UTF8,0,mode,-1,wmode,8)) return NULL;
 return _wfopen(wide,wmode);
#else
 return fopen(path,mode);
#endif
}
static int DevMakeDirectory(void)
{
#if defined(_WIN32)
 wchar_t wide[4096];
 return DevWidePath(DevDirectory,wide) && (CreateDirectoryW(wide,NULL) || GetLastError()==ERROR_ALREADY_EXISTS);
#else
 return mkdir(DevDirectory,0755)==0 || errno==EEXIST;
#endif
}
static int DevReplacePath(const char *temp,const char *destination)
{
#if defined(_WIN32)
 wchar_t source[4096],dest[4096];
 return DevWidePath(temp,source) && DevWidePath(destination,dest) && MoveFileExW(source,dest,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
#else
 return rename(temp,destination)==0;
#endif
}
static void DevRemoveTemp(const char *path)
{
#if defined(_WIN32)
 wchar_t wide[4096];if(DevWidePath(path,wide)) DeleteFileW(wide);
#else
 remove(path);
#endif
}
static void DevLoad(void)
{
 FILE *f;char line[256];int seen[DEV_VIEWMODEL_COUNT]={0},values[DEV_VIEWMODEL_COUNT],count=0,valid=1;
 memcpy(values,DevZ,sizeof values); /* New entries retain source defaults on a legacy CSV. */
 f=DevOpen(DevCSVPath,"rb");
 if(!f) {snprintf(DevMessage,sizeof DevMessage,errno==ENOENT?"Sin CSV: bases del source":"No se pudo leer el CSV");return;}
 if(!fgets(line,sizeof line,f)) valid=0;
 else {line[strcspn(line,"\r\n")]=0;if(strcmp(line,"species,weapon,rest_z")!=0)valid=0;}
 while(valid && fgets(line,sizeof line,f)) {
  char *first,*second,*end;long z;int row=-1,i;
  line[strcspn(line,"\r\n")]=0;if(!line[0])continue;
  first=strchr(line,',');if(!first){valid=0;break;}*first++=0;
  second=strchr(first,',');if(!second){valid=0;break;}*second++=0;
  errno=0;z=strtol(second,&end,10);
  if(errno || end==second || *end || z<DEV_VIEWMODEL_Z_MIN || z>DEV_VIEWMODEL_Z_MAX){valid=0;break;}
  for(i=0;i<DEV_VIEWMODEL_COUNT;i++)if(!strcmp(line,DevWeapons[i].speciesKey)&&!strcmp(first,DevWeapons[i].weaponKey)){row=i;break;}
  if(row<0 || seen[row]){valid=0;break;}
  seen[row]=1;values[row]=(int)z;count++;
 }
 if(ferror(f))valid=0;
 fclose(f);
 /* Accept the complete original schema or the complete expanded schema. */
 if(count==14){int i;for(i=0;i<14;i++)if(!seen[i])valid=0;}
 else if(count!=DEV_VIEWMODEL_COUNT)valid=0;
 if(valid){memcpy(DevZ,values,sizeof DevZ);snprintf(DevMessage,sizeof DevMessage,count==14?"CSV 14 cargado; 3 bases nuevas":"CSV cargado: 17 modelos");}
 else snprintf(DevMessage,sizeof DevMessage,"CSV invalido: se conservan las bases del source");
}
void DevViewmodelInitialize(const char *executableDirectory)
{
 int n;
 if(!TYRARGO_DEVELOPMENT_TOOLS)DevViewmodelEnabled=0;
 if(!DevViewmodelEnabled)return; /* Ordinary startup never even opens the CSV. */
 DevReady=0;DevDefaults();DevEdit=-1;DevKeyFraction=0;DevCSVPath[0]=DevDirectory[0]=0;
 if(!executableDirectory || !*executableDirectory){snprintf(DevMessage,sizeof DevMessage,"Ruta de export no disponible");return;}
 n=snprintf(DevDirectory,sizeof DevDirectory,"%s%slogs",executableDirectory,
  executableDirectory[strlen(executableDirectory)-1]=='/' || executableDirectory[strlen(executableDirectory)-1]=='\\'?"":"/");
 if(n<0 || n>=(int)sizeof DevDirectory){DevDirectory[0]=0;snprintf(DevMessage,sizeof DevMessage,"Ruta demasiado larga");return;}
 n=snprintf(DevCSVPath,sizeof DevCSVPath,"%s/dev_viewmodel_z.csv",DevDirectory);
 if(n<0 || n>=(int)sizeof DevCSVPath){DevCSVPath[0]=0;snprintf(DevMessage,sizeof DevMessage,"Ruta demasiado larga");return;}
 DevLoad(); /* No files or directories are created until the export action. */
 memcpy(DevFlame,DevFlameDefaults,sizeof DevFlame);memset(DevFlameFraction,0,sizeof DevFlameFraction);
 n=snprintf(DevFlamePath,sizeof DevFlamePath,"%s/dev_flamethrower.csv",DevDirectory);
 if(n<0 || n>=(int)sizeof DevFlamePath){DevFlamePath[0]=0;snprintf(DevMessage,sizeof DevMessage,"Ruta demasiado larga");return;}
 DevFlameLoad();
}
int DevViewmodelZ(int weaponID,int sourceZ)
{
 int row=DevRow(weaponID);
 if(row<0)return sourceZ;
 /* Normal play uses the compiled calibration and never reads DevZ/CSV state. */
 if(!DevViewmodelEnabled)return DevWeapons[row].compiledRestZ;
 DevDefaults();return DevZ[row];
}
int DevViewmodelValue(int row){DevDefaults();return row>=17&&row<23?DevFlame[row-17]:row>=0&&row<17?DevZ[row]:0;}
const char *DevViewmodelLabel(int row){return row>=17&&row<23?DevFlameLabels[row-17]:row>=0&&row<17?DevWeapons[row].label:"";}
const char *DevViewmodelStatus(void){return DevMessage;}
int DevViewmodelEditing(int row){return DevViewmodelEnabled && DevEdit==row && row>=0;}
void DevViewmodelToggle(int row)
{
 if(!DevViewmodelEnabled || row<0 || row>=DEV_VIEWMODEL_COUNT+DEV_FLAME_COUNT)return;
 DevEdit=DevEdit==row?-1:row;DevKeyFraction=0;memset(DevFlameFraction,0,sizeof DevFlameFraction);
}
void DevViewmodelAdjust(int row,int delta)
{
 if(!DevViewmodelEnabled || row<0 || row>=23)return;
 DevDefaults();int *v=row<17?&DevZ[row]:&DevFlame[row-17];
 int64_t n=(int64_t)*v+delta;
 if(n<DevViewmodelMinimum(row))n=DevViewmodelMinimum(row);
 if(n>DevViewmodelMaximum(row))n=DevViewmodelMaximum(row);
 if(*v!=n){*v=(int)n;snprintf(DevMessage,sizeof DevMessage,"Cambios sin exportar");}
}

int DevViewmodelHandleKeys(int species,int weaponID,int forward,int backward,int toggleStyle,int frameTime)
{
 int direction;int64_t whole;
 if(!DevViewmodelEnabled || DevEdit<0 || DevEdit>=17 || DevWeapons[DevEdit].species!=species || DevRow(weaponID)!=DevEdit){DevKeyFraction=0;return 0;}
 if(toggleStyle) WeaponFOVStyle=WeaponFOVStyle==WEAPON_FOV_STYLE_EXACT?WEAPON_FOV_STYLE_STRETCHED:WEAPON_FOV_STYLE_EXACT;
 direction=!!forward-!!backward;
 if(frameTime<0)frameTime=0;if(frameTime>ONE_FIXED/4)frameTime=ONE_FIXED/4;
 /* 100 source units/second; fractional travel survives high frame rates. */
 DevKeyFraction+=(int64_t)direction*frameTime*100;whole=DevKeyFraction/ONE_FIXED;DevKeyFraction-=whole*ONE_FIXED;
 DevViewmodelAdjust(DevEdit,(int)whole);
 return 1; /* Caller consumes W/S/Z only for this equipped editable weapon. */
}
int DevViewmodelExport(void)
{
 FILE *f;char temp[4096];int i,ok=1,n;
 if(!DevViewmodelEnabled)return 0;
 DevDefaults();
 n=snprintf(temp,sizeof temp,"%s.tmp",DevCSVPath);
 if(!DevCSVPath[0] || n<0 || n>=(int)sizeof temp || !DevMakeDirectory())goto failed;
 f=DevOpen(temp,"wb");if(!f)goto failed;
 if(fputs("species,weapon,rest_z\n",f)<0)ok=0;
 for(i=0;i<DEV_VIEWMODEL_COUNT;i++)if(fprintf(f,"%s,%s,%d\n",DevWeapons[i].speciesKey,DevWeapons[i].weaponKey,DevZ[i])<0)ok=0;
 if(fclose(f)!=0)ok=0;
 if(!ok || !DevReplacePath(temp,DevCSVPath)){DevRemoveTemp(temp);goto failed;}
 if(!DevFlameExport()){snprintf(DevMessage,sizeof DevMessage,"Z exportado; ERROR export fuego");return 0;}
 snprintf(DevMessage,sizeof DevMessage,"Export OK: dev_viewmodel_z.csv + dev_flamethrower.csv");return 1;
failed:
 snprintf(DevMessage,sizeof DevMessage,"ERROR export: no se guardaron los cambios");return 0;
}

/* Marine: sidearms, rifles/heavy weapons, launchers. Predator: melee,
 * speargun/pistol/disc, then the two wrist devices. Alien: arms/tail.
 * Keep storage IDs unchanged (legacy CSVs and the alien variant lookup use them). */
int DevViewmodelMenuIndex(int position)
{
 static const int order[DEV_VIEWMODEL_COUNT]={0,1,2,3,4,5,6,7,8,9,14,13,12,10,11,15,16};
 return position>=0 && position<DEV_VIEWMODEL_COUNT ? order[position] : -1;
}

void DevViewmodelFormat(int row,char *text,int size){
 if(row<17)snprintf(text,size,"%s   Z %d",DevViewmodelLabel(row),DevViewmodelValue(row));
 else if(row<20)snprintf(text,size,"%s   %d mm",DevViewmodelLabel(row),DevViewmodelValue(row));
 else snprintf(text,size,"%s   %.1f deg",DevViewmodelLabel(row),DevViewmodelValue(row)/10.0);
}
const char *DevViewmodelHelp(void){
 return "ENTER: edit on/off | Flechas: valor | Fuego: W/S A/D Q/E";
}
int DevFlameEditing(void){return DevViewmodelEnabled && DevEdit>=17 && DevEdit<23;}
double DevFlameValue(int axis){
 if(axis<0||axis>=6)return 0;
 if(DevViewmodelEnabled)DevDefaults();
 return (DevViewmodelEnabled?DevFlame[axis]:DevFlameDefaults[axis])/(axis<3?1.0:10.0);
}
void DevFlameKeys(int forward,int back,int left,int right,int down,int up,int frameTime){
 int dirs[3]={!!right-!!left,!!down-!!up,!!forward-!!back};
 if(!DevFlameEditing())return;
 if(frameTime<0)frameTime=0;if(frameTime>ONE_FIXED/4)frameTime=ONE_FIXED/4;
 for(int j=0;j<3;j++){
  DevFlameFraction[j]+=(int64_t)dirs[j]*frameTime*100;
  int whole=(int)(DevFlameFraction[j]/ONE_FIXED);DevFlameFraction[j]-=(int64_t)whole*ONE_FIXED;
  DevViewmodelAdjust(17+j,whole);
 }
}
void DevFlameRotate(double v[3]){
 for(int axis=0;axis<3;axis++){
  double a=DevFlameValue(axis+3)*3.14159265358979323846/180.0,c=cos(a),s=sin(a);
  int i=(axis+1)%3,j=(axis+2)%3;double x=v[i],y=v[j];
  v[i]=c*x-s*y;v[j]=s*x+c*y;
 }
}
static void DevFlameLoad(void){
 FILE *f=DevOpen(DevFlamePath,"rb");char line[128];int values[6]={0},seen=0,ok=1;
 if(!f)return;
 if(!fgets(line,sizeof line,f))ok=0;
 else {line[strcspn(line,"\r\n")]=0;if(strcmp(line,"parameter,value"))ok=0;}
 while(ok&&fgets(line,sizeof line,f)){
  char key[32],tail;double v;
  if(sscanf(line,"%31[^,],%lf %c",key,&v,&tail)!=2 || !isfinite(v)){ok=0;break;}
  int row;for(row=0;row<6;row++)if(!strcmp(key,DevFlameNames[row]))break;
  if(row==6 || (seen&(1<<row))){ok=0;break;}
  v*=row<3?1:10;
  if(v<DevViewmodelMinimum(row+17)||v>DevViewmodelMaximum(row+17)){ok=0;break;}
  values[row]=(int)lround(v);seen|=1<<row;
 }
 if(ferror(f))ok=0;fclose(f);
 if(ok&&seen==63)memcpy(DevFlame,values,sizeof values);
}
static int DevFlameExport(void){
 char temp[4096];FILE *f;int ok=1;
 int n=snprintf(temp,sizeof temp,"%s.tmp",DevFlamePath);
 if(!DevFlamePath[0]||n<0||n>=sizeof temp)return 0;
 f=DevOpen(temp,"wb");if(!f)return 0;
 if(fputs("parameter,value\n",f)<0)ok=0;
 for(int i=0;i<6;i++)if(fprintf(f,"%s,%.1f\n",DevFlameNames[i],DevFlameValue(i))<0)ok=0;
 if(fclose(f))ok=0;
 if(ok&&DevReplacePath(temp,DevFlamePath))return 1;
 DevRemoveTemp(temp);return 0;
}
