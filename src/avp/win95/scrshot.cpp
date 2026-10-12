#include "3dc.h"
#include "screenshot_image.h"
#include "gadget.h"
#include "psnd.h"
#include "psndproj.h"
#include "rootgadg.hpp"
#include "endianio.h"
#include "string.hpp"
#include "scrshot.hpp"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "ourasert.h"
#include "frontend/avp_menus.h"

extern "C"{
 extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
 extern int VideoModeTypeScreen;
 extern unsigned char *ScreenBuffer;
 extern unsigned char TestPalette[];
 extern unsigned char KeyboardInput[];
 extern unsigned char DebouncedKeyboardInput[];
 extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
 extern MODULE* playerPherModule;	
};

typedef VOID (*PutWord_F)(WORD, FILE *);
typedef VOID (*PutDword_F)(DWORD, FILE *);

extern void LoadModuleData();

void LogCameraPosForModuleLinking()
{
	fprintf(stderr, "STUB: LogCameraPosForModuleLinking()\n");
	
#if 0 /* TODO: commented out because I want to know if its actually used */
	if(!playerPherModule) return;
	if(!playerPherModule->name) return;
	
	char Filename[100]={"avp_rifs/"};
	
	strcat(Filename,Env_List[AvP.CurrentEnv]->main);
	strcat(Filename,".mlf");

	FILE* file=fopen(Filename,"ab");
	if(!file)return;

	char output_buffer[300];
	int length=0;
	
	strcpy(output_buffer,playerPherModule->name);
	length+=(strlen(playerPherModule->name)+4)&~3;

	*(VECTORCH*)&output_buffer[length]=playerPherModule->m_world;
	length+=sizeof(VECTORCH);
	*(MATRIXCH*)&output_buffer[length]=Global_VDB_Ptr->VDB_Mat;
	length+=sizeof(MATRIXCH);
	*(VECTORCH*)&output_buffer[length]=Global_VDB_Ptr->VDB_World;
	length+=sizeof(VECTORCH);
	
	if(length%4 !=0)
	{
		GLOBALASSERT(0);
	}

	fwrite(&output_buffer[0],4,length/4,file);
	fclose(file);		
	textprint("Saving camera for module links");
#endif	
}
int SaveCameraPosKeyPressed=0;
#ifdef AVP_DEBUG_VERSION
static BOOL ModuleLinkAssist=FALSE;
#endif

void HandleScreenShot()
{
	#ifdef AVP_DEBUG_VERSION

	if (DebouncedKeyboardInput[KEY_F8])
		ScreenShot();

	if (KeyboardInput[KEY_F7])
	{
		if(!SaveCameraPosKeyPressed)
		{
			if(KeyboardInput[KEY_LEFTSHIFT]||KeyboardInput[KEY_RIGHTSHIFT])
			{
				ModuleLinkAssist=TRUE;
				DeleteFile("avp_rifs\\module.aaa");
			}
			else
			{
				LogCameraPosForModuleLinking();
				SaveCameraPosKeyPressed=1;
			}
		}
	}
	else
		SaveCameraPosKeyPressed=0;
	
	if(AvP.MainLoopRunning && ModuleLinkAssist)LoadModuleData();

	#endif
}

extern "C" {
unsigned char *GetScreenShot24(int *width, int *height);
};
				 
static void NotifyScreenshotSaved(const char *name, int format)
{
    char location[64];
    snprintf(location, sizeof(location), "....avp\\%s", name);
    const char *extension = ScreenshotExtension(format);
    const char *pattern = GetTextString(TEXTSTRING_SCREENSHOT_SAVED_NEW);
    const size_t size = strlen(pattern) + strlen(extension) + strlen(location) + 1;
    char *message = (char *)malloc(size);
    if (message) {
        snprintf(message, size, pattern, extension, location);
        /* Capture may also run before a HUD exists (frontend/intro). */
        if (RootGadget::GetRoot() && RootGadget::GetRoot()->GetHUD())
            GADGET_NewScreenshotMessage(message);
        else
            Sound_Play(SID_TELETEXT,NULL); /* No HUD/daemon exists in the frontend yet. */
        printf("%s\n", message);
        free(message);
    }
}

void ScreenShot()
{
    char name[48];
    const int format = ScreenshotFormat >= 0 && ScreenshotFormat < SCREENSHOT_FORMAT_COUNT ? ScreenshotFormat : SCREENSHOT_BMP;
    int index, width, height, ok;
    FILE *fp;
    unsigned char *pixels;
    /* Preserve the existing directory/names, continuing beyond avp99. */
    for (index=0; index<1000000; ++index) {
        snprintf(name, sizeof(name), "avp%02d.%s", index, ScreenshotExtension(format));
        fp = OpenGameFile(name, FILEMODE_READONLY, FILETYPE_CONFIG);
        if (!fp) break;
        fclose(fp);
    }
    if (index==1000000) return;
    pixels = GetScreenShot24(&width, &height);
    if (!pixels) return;
    fp = OpenGameFile(name, FILEMODE_WRITEONLY, FILETYPE_CONFIG);
    if (!fp) { free(pixels); return; }
    ok = WriteScreenshotImage(fp, format, width, height, pixels);
    free(pixels);
    if (fclose(fp) != 0) ok = 0;
    if (!ok) {
        DeleteGameFile(name);
        fprintf(stderr, "Could not write screenshot: %s\n", name);
    } else {
        NotifyScreenshotSaved(name, format);
    }
}
