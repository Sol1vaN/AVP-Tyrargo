#include "tyrargo_version.h"
/*KJL***************************************
*    Language Internationalization Code    *
***************************************KJL*/
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "gamedef.h"


#include "langenum.h"
#include "language.h"
#include "huffman.hpp"

// DHM 12 Nov 97: hooks for C++ string handling code:
#include "strtab.hpp"

#define UseLocalAssert Yes
#include "ourasert.h"
#include "avp_menus.h"


#ifdef AVP_DEBUG_VERSION
	#define USE_LANGUAGE_TXT 0
#else
	#define USE_LANGUAGE_TXT 1
#endif

static char EmptyString[]="";

static char *TextStringPtr[MAX_NO_OF_TEXTSTRINGS] = { EmptyString };
static char *TextBufferPtr;

/* Release data can keep AvP.Language at I_English even when language.txt is
   Spanish. The native Auto-centre label is the reliable live-language marker. */
static int UsingSpanishTextStrings(void)
{
	char *label = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
	return label && (!strcmp(label,"Auto-centrar") || !strcmp(label,"Auto Centrar"));
}

void InitTextStrings(void)
{
	char *filename;
	char *textPtr;
	int i;

	/* language select here! */
	GLOBALASSERT(AvP.Language>=0);
	GLOBALASSERT(AvP.Language<I_MAX_NO_OF_LANGUAGES);
	
#if MARINE_DEMO
	filename = "menglish.txt";
#elif ALIEN_DEMO
	filename = "aenglish.txt";
#elif USE_LANGUAGE_TXT
	filename = "language.txt";
#else
	filename = LanguageFilename[AvP.Language];
#endif
	TextBufferPtr = LoadTextFile(filename);
		
	if (TextBufferPtr == NULL) {
		/* NOTE:
		   if this load fails, then most likely the game is not 
		   installed correctly. 
		   SBF
		  */ 
		fprintf(stderr, "ERROR: unable to load %s language text file\n",
			 filename);
		exit(1);
	}
	
	if (!strncmp (TextBufferPtr, "REBCRIF1", 8))
	{
		textPtr = (char*)HuffmanDecompress((HuffmanPackage*)(TextBufferPtr)); 		
		DeallocateMem(TextBufferPtr);
		TextBufferPtr=textPtr;
	}
	else
	{
		textPtr = TextBufferPtr;
	}

	AddToTable( EmptyString );

	for (i=1; i<MAX_NO_OF_TEXTSTRINGS; i++)
	{	
		/* scan for a quote mark */
		while (*textPtr++ != '"')  {
			if (*textPtr == '@') {
				// should be an error as this language file
				// doesn't match the game.
				return; /* '@' should be EOF */
			}
		}

		/* now pointing to a text string after quote mark*/
		TextStringPtr[i] = textPtr;

		/* scan for a quote mark */
		while (*textPtr != '"')
		{	
			textPtr++;
		}

		/* change quote mark to zero terminator */
		*textPtr = 0;
		textPtr++;

		AddToTable( TextStringPtr[i] );
	}
}

void KillTextStrings(void)
{
	UnloadTextFile(LanguageFilename[AvP.Language],TextBufferPtr);

	UnloadTable();
}

char *GetTextString(enum TEXTSTRING_ID stringID)
{
    if (stringID == TEXTSTRING_PREDATOR_KEY_CYCLE_VISION) {
    static const char *const t[6]={"Next vision mode","Mode de vision suivant","Modo de visiÃ³n siguiente","NÃ¤chster Sichtmodus","ModalitÃ  visiva successiva","NÃ¤sta synlÃ¤ge"};
    int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;
    return (char*)t[l>=0&&l<6?l:0];
 }
    if(stringID>=TEXTSTRING_CAMERA_MODE_V2_0 && stringID<=TEXTSTRING_CAMERA_MODE_V2_7){
        static char *modes[8]={"Vanilla","Sway","Bobbing","Shake","Sway+Bobbing","Sway+Shake","Bobbing+Shake","Sway+Bobbing+Shake"};
        return modes[stringID-TEXTSTRING_CAMERA_MODE_V2_0];
    }
    if(stringID==TEXTSTRING_HUD_ANIMATION_INTENSITY_NEW){
        static const char *text[6]={"HUD shake intensity","Intensit\303\251 du shake HUD","HUD shake intensity","HUD-Shake-St\303\244rke","Intensit\303\240 shake HUD","HUD-skakintensitet"};
        int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)text[l>=0&&l<6?l:0];
    }
    if(stringID==TEXTSTRING_HUD_ANIMATION_INTENSITY_HELP_NEW){
        static const char *text[6]={"Adjusts only shake amplitude when enabled in Camera mode. 1: subtle (15%); 50: calibrated baseline (50%); 100: stronger (75%). ENTER restores 50. Animation timing stays unchanged.","R\303\250gle uniquement l\342\200\231amplitude du shake actif dans le mode cam\303\251ra. 1 : discret (15%) ; 50 : base calibr\303\251e (50%) ; 100 : plus fort (75%). ENTR\303\211E r\303\251tablit 50. Le rythme reste inchang\303\251.","Ajusta solo la amplitud de shake cuando est\303\241 habilitado en C\303\241mara modo. 1: sutil (15%); 50: base calibrada (50%); 100: m\303\241s intenso (75%). ENTER restaura 50. Conserva el ritmo de las animaciones.","Regelt nur die shake-St\303\244rke, wenn im Kameramodus aktiviert. 1: dezent (15%); 50: kalibrierte Basis (50%); 100: st\303\244rker (75%). ENTER setzt 50 zur\303\274ck. Der Rhythmus bleibt unver\303\244ndert.","Regola solo l\342\200\231ampiezza del shake attivo nella modalit\303\240 telecamera. 1: lieve (15%); 50: base calibrata (50%); 100: pi\303\271 forte (75%). INVIO ripristina 50. Il ritmo resta invariato.","Justerar bara amplituden f\303\266r shake n\303\244r det \303\244r aktivt i kameral\303\244get. 1: subtilt (15%); 50: kalibrerad bas (50%); 100: starkare (75%). ENTER \303\245terst\303\244ller 50. Rytmen \303\244r of\303\266r\303\244ndrad."};
        int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)text[l>=0&&l<6?l:0];
    }
    if(stringID==TEXTSTRING_HUD_SWAY_INTENSITY_NEW){
        static const char *text[6]={"HUD sway intensity","Intensit\303\251 du sway HUD","HUD sway intensity","HUD-Sway-St\303\244rke","Intensit\303\240 sway HUD","HUD-swayintensitet"};
        int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)text[l>=0&&l<6?l:0];
    }
    if(stringID==TEXTSTRING_HUD_SWAY_INTENSITY_HELP_NEW){
        static const char *text[6]={"Adjusts only sway amplitude when enabled in Camera mode. 1: subtle (15%); 50: calibrated baseline (50%); 100: stronger (75%). ENTER restores 50. Animation timing stays unchanged.","R\303\250gle uniquement l\342\200\231amplitude du sway actif dans le mode cam\303\251ra. 1 : discret (15%) ; 50 : base calibr\303\251e (50%) ; 100 : plus fort (75%). ENTR\303\211E r\303\251tablit 50. Le rythme reste inchang\303\251.","Ajusta solo la amplitud de sway cuando est\303\241 habilitado en C\303\241mara modo. 1: sutil (15%); 50: base calibrada (50%); 100: m\303\241s intenso (75%). ENTER restaura 50. Conserva el ritmo de las animaciones.","Regelt nur die sway-St\303\244rke, wenn im Kameramodus aktiviert. 1: dezent (15%); 50: kalibrierte Basis (50%); 100: st\303\244rker (75%). ENTER setzt 50 zur\303\274ck. Der Rhythmus bleibt unver\303\244ndert.","Regola solo l\342\200\231ampiezza del sway attivo nella modalit\303\240 telecamera. 1: lieve (15%); 50: base calibrata (50%); 100: pi\303\271 forte (75%). INVIO ripristina 50. Il ritmo resta invariato.","Justerar bara amplituden f\303\266r sway n\303\244r det \303\244r aktivt i kameral\303\244get. 1: subtilt (15%); 50: kalibrerad bas (50%); 100: starkare (75%). ENTER \303\245terst\303\244ller 50. Rytmen \303\244r of\303\266r\303\244ndrad."};
        int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)text[l>=0&&l<6?l:0];
    }
    if(stringID==TEXTSTRING_HUD_BOBBING_INTENSITY_NEW){
        static const char *text[6]={"HUD bobbing intensity","Intensit\303\251 du bobbing HUD","HUD bobbing intensity","HUD-Bobbing-St\303\244rke","Intensit\303\240 bobbing HUD","HUD-bobbingintensitet"};
        int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)text[l>=0&&l<6?l:0];
    }
    if(stringID==TEXTSTRING_HUD_BOBBING_INTENSITY_HELP_NEW){
        static const char *text[6]={"Adjusts only bobbing amplitude when enabled in Camera mode. 1: subtle (15%); 50: calibrated baseline (50%); 100: stronger (65%). ENTER restores 50. Animation timing stays unchanged.","R\303\250gle uniquement l\342\200\231amplitude du bobbing actif dans le mode cam\303\251ra. 1 : discret (15%) ; 50 : base calibr\303\251e (50%) ; 100 : plus fort (65%). ENTR\303\211E r\303\251tablit 50. Le rythme reste inchang\303\251.","Ajusta solo la amplitud de bobbing cuando est\303\241 habilitado en C\303\241mara modo. 1: sutil (15%); 50: base calibrada (50%); 100: m\303\241s intenso (65%). ENTER restaura 50. Conserva el ritmo de las animaciones.","Regelt nur die bobbing-St\303\244rke, wenn im Kameramodus aktiviert. 1: dezent (15%); 50: kalibrierte Basis (50%); 100: st\303\244rker (65%). ENTER setzt 50 zur\303\274ck. Der Rhythmus bleibt unver\303\244ndert.","Regola solo l\342\200\231ampiezza del bobbing attivo nella modalit\303\240 telecamera. 1: lieve (15%); 50: base calibrata (50%); 100: pi\303\271 forte (65%). INVIO ripristina 50. Il ritmo resta invariato.","Justerar bara amplituden f\303\266r bobbing n\303\244r det \303\244r aktivt i kameral\303\244get. 1: subtilt (15%); 50: kalibrerad bas (50%); 100: starkare (65%). ENTER \303\245terst\303\244ller 50. Rytmen \303\244r of\303\266r\303\244ndrad."};
        int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)text[l>=0&&l<6?l:0];
    }
    if (stringID == TEXTSTRING_MAINMENU_TITLE) return TYRARGO_MENU_TITLE;
    if (stringID == TEXTSTRING_MAINMENU_SUBTITLE) return TYRARGO_SHORT_NAME;
	/* The Spanish legacy file starts this question with a glyph unsupported by
	   the in-game font. Strip only that leading marker and preserve its text
	   and final question mark exactly as authored. */
	if (stringID == TEXTSTRING_AUTOWEAPONCHANGE_HELP && UsingSpanishTextStrings()) {
		char *legacyHelp = TextStringPtr[stringID];
		if (legacyHelp != NULL && legacyHelp[0] == '?') return legacyHelp + 1;
		if (legacyHelp != NULL && (unsigned char)legacyHelp[0] == 0xBF) return legacyHelp + 1;
		if (legacyHelp != NULL && (unsigned char)legacyHelp[0] == 0xC2 && (unsigned char)legacyHelp[1] == 0xBF) return legacyHelp + 2;
	}
	// Not good.
	// These strings do not exist in data.
	if (stringID > MIN_NEW_TEXTSTRINGS && stringID < MAX_NEW_TEXTSTRINGS) {
		switch (stringID) {
 case TEXTSTRING_IMAGE_BRIGHTNESS_NEW: { static const char *const t[6]={"Brightness adjustment","Réglage luminosité","Ajuste brillo","Helligkeit einstellen","Regola luminosità","Justera ljusstyrka"}; int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)t[l>=0&&l<6?l:0];}
 case TEXTSTRING_IMAGE_BRIGHTNESS_HELP_NEW: { static const char *const t[6]={"Lightens or darkens the final image. Min -50%: darker. Max +50%: lighter. ENTER: neutral 0%.","Éclaircit ou assombrit l'image finale. Min -50%: sombre. Max +50%: claire. ENTRÉE: neutre 0%.","Aclara u oscurece la imagen final. Mín -50%: más oscura. Máx +50%: más clara. ENTER: neutro 0%.","Hellt das fertige Bild auf oder dunkelt es ab. Min -50%: dunkler. Max +50%: heller. ENTER: neutral 0%.","Schiarisce o scurisce l'immagine finale. Min -50%: scura. Max +50%: chiara. INVIO: neutro 0%.","Ljusar upp eller mörkar ned slutbilden. Min -50%: mörkare. Max +50%: ljusare. ENTER: neutral 0%."}; int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)t[l>=0&&l<6?l:0];}
 case TEXTSTRING_IMAGE_CONTRAST_NEW: { static const char *const t[6]={"Contrast adjustment","Réglage contraste","Ajuste contraste","Kontrast einstellen","Regola contrasto","Justera kontrast"}; int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)t[l>=0&&l<6?l:0];}
 case TEXTSTRING_IMAGE_CONTRAST_HELP_NEW: { static const char *const t[6]={"Controls the difference between light and dark tones. Min 0%: flat gray. Max 200%: strong contrast. ENTER: neutral 100%.","Règle l'écart entre tons clairs et sombres. Min 0%: gris uniforme. Max 200%: contraste fort. ENTRÉE: neutre 100%.","Controla la diferencia entre tonos claros y oscuros. Mín 0%: gris plano. Máx 200%: contraste fuerte. ENTER: neutro 100%.","Regelt den Unterschied zwischen hellen und dunklen Tönen. Min 0%: einheitliches Grau. Max 200%: starker Kontrast. ENTER: neutral 100%.","Regola la differenza tra toni chiari e scuri. Min 0%: grigio uniforme. Max 200%: forte contrasto. INVIO: neutro 100%.","Styr skillnaden mellan ljusa och mörka toner. Min 0%: jämngrått. Max 200%: stark kontrast. ENTER: neutral 100%."}; int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)t[l>=0&&l<6?l:0];}
 case TEXTSTRING_IMAGE_SATURATION_NEW: { static const char *const t[6]={"Saturation","Saturation","Saturación","Sättigung","Saturazione","Mättnad"}; int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)t[l>=0&&l<6?l:0];}
 case TEXTSTRING_IMAGE_SATURATION_HELP_NEW: { static const char *const t[6]={"Controls color intensity. Min 0%: grayscale. Max 200%: intense colors. ENTER: original 100%.","Règle l'intensité des couleurs. Min 0%: niveaux de gris. Max 200%: couleurs intenses. ENTRÉE: original 100%.","Controla la intensidad del color. Mín 0%: escala de grises. Máx 200%: colores intensos. ENTER: original 100%.","Regelt die Farbintensität. Min 0%: Graustufen. Max 200%: intensive Farben. ENTER: original 100%.","Regola l'intensità dei colori. Min 0%: scala di grigi. Max 200%: colori intensi. INVIO: originale 100%.","Styr färgintensiteten. Min 0%: gråskala. Max 200%: starka färger. ENTER: ursprungliga 100%."}; int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)t[l>=0&&l<6?l:0];}
 case TEXTSTRING_CAMERA_MODE_NEW:
 {
  if(UsingSpanishTextStrings())return "Cámara modo";
  switch(AvP.Language){
   case I_Spanish: return "Cámara modo";
   default: return "Camera mode";
   case I_French: return "Mode caméra";
   case I_German: return "Kameramodus";
   case I_Italian: return "Modalità telecamera";
   case I_Swedish: return "Kameraläge";
  }
 }

 case TEXTSTRING_CAMERA_MODE_HELP_NEW: {
 static const char *const t[6]={"Procedurally enhances camera motion with mouse movement, walking, running, jumping and crouching; bobbing adds footstep sounds and camera animation when reloading or switching weapons; sway adds inertia to weapon models when moving the mouse, crouching or standing up; shake adds HUD feedback when moving the mouse, jumping/landing, reloading/switching weapons and taking damage.","Enrichit les mouvements procÃ©duraux de la camÃ©ra avec la souris, la marche, la course, les sauts et l'accroupissement ; bobbing ajoute des bruits de pas et des animations de camÃ©ra lors des rechargements et changements d'arme ; sway ajoute de l'inertie aux armes avec la souris et en s'accroupissant ou se relevant ; shake fait rÃ©agir le HUD Ã  la souris, aux sauts/atterrissages, aux rechargements/changements d'arme et aux dÃ©gÃ¢ts.","cambia proceduralmente el comportamiento de la cÃ¡mara ampliando los gestos relativos al mover el mouse y caminar/correr/saltar/agacharse; bobbing = agrega sonidos pisadas y animaciÃ³n cÃ¡mara al recargar y cambiar entre armas; sway = agrega gestos de inercia a los modelos de las armas, reacciona al mover el mouse y al agacharse/levantarse; shake = agrega feedback al HUD al mover el mouse, saltar/aterrizar, recargar/cambiar armas y recibir daÃ±o.","Erweitert die Kamerabewegung bei Mausbewegungen, Gehen, Laufen, Springen und Ducken; Bobbing ergÃ¤nzt SchrittgerÃ¤usche und Kameraanimationen beim Nachladen und Waffenwechsel; Sway verleiht Waffenmodellen TrÃ¤gheit bei Mausbewegungen, Ducken und Aufstehen; Shake lÃ¤sst das HUD auf Mausbewegungen, SprÃ¼nge/Landungen, Nachladen/Waffenwechsel und Schaden reagieren.","Arricchisce i movimenti della telecamera con il mouse, camminando, correndo, saltando e accovacciandosi; bobbing aggiunge passi e animazioni della telecamera durante ricariche e cambi d'arma; sway aggiunge inerzia ai modelli delle armi con il mouse e quando ci si accovaccia o rialza; shake fa reagire l'HUD al mouse, ai salti/atterraggi, alle ricariche/cambi d'arma e ai danni subiti.","UtÃ¶kar kamerans rÃ¶relser med musen, gÃ¥ng, lÃ¶pning, hopp och hukning; bobbing lÃ¤gger till fotstegsljud och kameraanimationer vid omladdning och vapenbyte; sway ger vapenmodeller trÃ¶ghet vid musrÃ¶relser, hukning och uppresning; shake fÃ¥r HUD att reagera pÃ¥ musrÃ¶relser, hopp/landning, omladdning/vapenbyte och skador."};
 int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;return (char*)t[l>=0&&l<6?l:0];
 }

 case TEXTSTRING_CAMERA_MODE_VANILLA_NEW:
 {
  if(UsingSpanishTextStrings())return "Vanilla";
  switch(AvP.Language){
   case I_Spanish: return "Vanilla";
   default: return "Vanilla";
   case I_French: return "Vanilla";
   case I_German: return "Vanilla";
   case I_Italian: return "Vanilla";
   case I_Swedish: return "Vanilla";
  }
 }

 case TEXTSTRING_CAMERA_MODE_SWAY_NEW:
 {
  if(UsingSpanishTextStrings())return "Sway";
  switch(AvP.Language){
   case I_Spanish: return "Sway";
   default: return "Sway";
   case I_French: return "Sway";
   case I_German: return "Sway";
   case I_Italian: return "Sway";
   case I_Swedish: return "Sway";
  }
 }

 case TEXTSTRING_CAMERA_MODE_BOBBING_NEW:
 {
  if(UsingSpanishTextStrings())return "Bobbing";
  switch(AvP.Language){
   case I_Spanish: return "Bobbing";
   default: return "Bobbing";
   case I_French: return "Bobbing";
   case I_German: return "Bobbing";
   case I_Italian: return "Bobbing";
   case I_Swedish: return "Bobbing";
  }
 }

 case TEXTSTRING_CAMERA_MODE_SHAKE_NEW: return "Shake";
 case TEXTSTRING_CAMERA_MODE_ALL_NEW: return "Sway+Bobbing+Shake";
 case TEXTSTRING_CAMERA_MODE_BOTH_NEW:
 {
  if(UsingSpanishTextStrings())return "Sway+Bobbing";
  switch(AvP.Language){
   case I_Spanish: return "Sway+Bobbing";
   default: return "Sway+Bobbing";
   case I_French: return "Sway+Bobbing";
   case I_German: return "Sway+Bobbing";
   case I_Italian: return "Sway+Bobbing";
   case I_Swedish: return "Sway+Bobbing";
  }
 }

 case TEXTSTRING_EXPERIMENTAL2_NEW: return "experimental feats 2";
 case TEXTSTRING_EXPERIMENTAL2_HELP_NEW:
 {
  if(UsingSpanishTextStrings())return "Calibra el Shake del HUD marine. Export guarda los sliders en CSV.";
  switch(AvP.Language){
   case I_French:return "Calibrez le Shake du HUD marine. Export sauvegarde les curseurs en CSV.";
   case I_Spanish:return "Calibra el Shake del HUD marine. Export guarda los sliders en CSV.";
   case I_German:return "Shake des Marine-HUD einstellen. Export speichert die Regler als CSV.";
   case I_Italian:return "Calibra lo Shake del HUD marine. Export salva i cursori in CSV.";
   case I_Swedish:return "Kalibrera Shake i marinens HUD. Export sparar reglagen till CSV.";
   default:return "Calibrate marine HUD Shake. Export saves the sliders to CSV.";
  }
 }
 case TEXTSTRING_EXPERIMENTAL_FEATS_NEW:
 {
  if(UsingSpanishTextStrings())return "experimental feats";
  switch(AvP.Language){
   case I_Spanish: return "experimental feats";
   default: return "experimental feats";
   case I_French: return "experimental feats";
   case I_German: return "experimental feats";
   case I_Italian: return "experimental feats";
   case I_Swedish: return "experimental feats";
  }
 }

 case TEXTSTRING_EXPERIMENTAL_FEATS_HELP_NEW:
 {
  if(UsingSpanishTextStrings())return "Calibra el movimiento de cámara y arma del marine. Export guarda los sliders en CSV.";
  switch(AvP.Language){
   default: return "Calibrate marine camera and weapon motion. Export saves the sliders to CSV.";
   case I_French: return "Réglez les mouvements du marine et de son arme. Export enregistre les réglages en CSV.";
   case I_Spanish: return "Calibra el movimiento de cámara y arma del marine. Export guarda los sliders en CSV.";
   case I_German: return "Kamera- und Waffenbewegung des Marines einstellen. Export speichert die Regler als CSV.";
   case I_Italian: return "Calibra i movimenti della telecamera e arma del marine. Export salva i valori in CSV.";
   case I_Swedish: return "Kalibrera marinens kamera- och vapenrörelser. Export sparar reglagen till CSV.";
  }
 }

 case TEXTSTRING_EXPERIMENTAL_CAMERA_NEW:
 {
  if(UsingSpanishTextStrings())return "Oscilación de cámara";
  switch(AvP.Language){
   case I_Spanish: return "Oscilación de cámara";
   default: return "Camera oscillation";
   case I_French: return "Oscillation de caméra";
   case I_German: return "Kameraschwingung";
   case I_Italian: return "Oscillazione della telecamera";
   case I_Swedish: return "Kamerasvängning";
  }
 }

 case TEXTSTRING_EXPERIMENTAL_CAMERA_HELP_NEW:
 {
  if(UsingSpanishTextStrings())return "Activa una pequeña oscilación sinusoidal continua de la cámara. No modifica la inclinación al desplazarse lateralmente ni el apuntado del arma. Opción temporal para esta sesión.";
  switch(AvP.Language){
   case I_Spanish: return "Activa una pequeña oscilación sinusoidal continua de la cámara. No modifica la inclinación al desplazarse lateralmente ni el apuntado del arma. Opción temporal para esta sesión.";
   default: return "Enables a small continuous sinusoidal camera oscillation. Strafe lean and weapon aiming are unchanged. Temporary option for this session.";
   case I_French: return "Active une légère oscillation sinusoïdale continue de la caméra. Ne modifie ni l inclinaison latérale ni la visée de l arme. Option temporaire pour cette session.";
   case I_German: return "Aktiviert eine leichte kontinuierliche Sinusschwingung der Kamera. Seitliche Neigung und Waffenzielen bleiben unverändert. Temporäre Option für diese Sitzung.";
   case I_Italian: return "Attiva una lieve oscillazione sinusoidale continua della telecamera. Non modifica l inclinazione laterale né la mira dell arma. Opzione temporanea per questa sessione.";
   case I_Swedish: return "Aktiverar en liten kontinuerlig sinusformad kamerarörelse. Sidolutning och vapensikte ändras inte. Tillfälligt alternativ för denna session.";
  }
 }

			case TEXTSTRING_MAINMENU_EXITGAME_HELP_NEW: return "Exit the game.";
			case TEXTSTRING_MAINMENU_CREDITS_NEW: return "Credits";
			case TEXTSTRING_MAINMENU_CREDITS_HELP_NEW: return "View the credits.";
			case TEXTSTRING_MOUSECONTROLS_ZOOMSENSITIVITY_NEW:
			{
				char *autoCentreLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (autoCentreLabel && (!strcmp(autoCentreLabel,"Auto-centrar") || !strcmp(autoCentreLabel,"Auto Centrar"))) return "Sensibilidad Zoom";
				switch (AvP.Language) {
					case I_French:  return "Sensibilite zoom";
					case I_Spanish: return "Sensibilidad Zoom";
					case I_German:  return "Zoom-Empfindlichkeit";
					case I_Italian: return "Sensibilita zoom";
					case I_Swedish: return "Zoomkanslighet";
					case I_English:
					default:        return "Zoom Sensitivity";
				}
			}
			case TEXTSTRING_MOUSECONTROLS_ZOOMSENSITIVITY_HELP_NEW:
			{
				char *autoCentreLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (autoCentreLabel && (!strcmp(autoCentreLabel,"Auto-centrar") || !strcmp(autoCentreLabel,"Auto Centrar"))) return "modifica la escala de la sensibilidad hor/ver. presionar ENTER resetea su valor por defecto";
				switch (AvP.Language) {
					case I_French:  return "Modifie l'echelle de sensibilite hor./vert. Appuyez sur ENTREE pour retablir la valeur par defaut.";
					case I_Spanish: return "modifica la escala de la sensibilidad hor/ver. presionar ENTER resetea su valor por defecto";
					case I_German:  return "Andert die horizontale/vertikale Empfindlichkeitsskala. ENTER setzt den Standardwert zuruck.";
					case I_Italian: return "Modifica la scala della sensibilita orizz./vert. Premi INVIO per ripristinare il valore predefinito.";
					case I_Swedish: return "Andrar den horisontella/vertikala kanslighetsskalan. Tryck ENTER for att aterstalla standardvardet.";
					case I_English:
					default:        return "Adjusts horizontal/vertical sensitivity scaling. Press ENTER to reset the default value.";
				}
			}			case TEXTSTRING_CROUCH_TOGGLE_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Alternar agachar";
				switch (AvP.Language) { case I_French: return "Accroupissement bascule"; case I_Spanish: return "Alternar agachar"; case I_German: return "Duckmodus umschalten"; case I_Italian: return "Accovacciamento alternato"; case I_Swedish: return "Vaxla hukning"; default: return "Toggle crouch"; }
			}
			case TEXTSTRING_CROUCH_TOGGLE_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "cambia la funciÃ³n de agachar entre modo alternar o modo sostenido";
				switch (AvP.Language) { case I_French: return "Change l'accroupissement entre les modes bascule et maintien."; case I_Spanish: return "cambia la funciÃ³n de agachar entre modo alternar o modo sostenido"; case I_German: return "Wechselt Ducken zwischen Umschalten und Gedruckthalten."; case I_Italian: return "Cambia l'accovacciamento tra modalita alternata e tenuta."; case I_Swedish: return "Vaxlar hukning mellan vaxla och halla ned."; default: return "Changes crouch between toggle and hold modes."; }
			}
			case TEXTSTRING_UNLIMITED_SAVES_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Guardado partidas ilimitadas";
				switch (AvP.Language) { case I_French: return "Sauvegardes illimitees"; case I_Spanish: return "Guardado partidas ilimitadas"; case I_German: return "Unbegrenzte Spielstande"; case I_Italian: return "Salvataggi illimitati"; case I_Swedish: return "Obegransade sparningar"; default: return "Unlimited game saves"; }
			}
			case TEXTSTRING_UNLIMITED_SAVES_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "permite guardar tus partidas de manera ilimitada sin importar el nivel de dificultad seleccionado";
				switch (AvP.Language) { case I_French: return "Permet des sauvegardes illimitees quel que soit le niveau de difficulte choisi."; case I_Spanish: return "permite guardar tus partidas de manera ilimitada sin importar el nivel de dificultad seleccionado"; case I_German: return "Ermoglicht unbegrenztes Speichern unabhangig vom gewahlten Schwierigkeitsgrad."; case I_Italian: return "Consente salvataggi illimitati indipendentemente dalla difficolta selezionata."; case I_Swedish: return "Tillater obegransade sparningar oavsett vald svarighetsgrad."; default: return "Allows unlimited game saves regardless of the selected difficulty."; }
			}
			case TEXTSTRING_SMART_JETPACK_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Jetpack inteligente";
				switch (AvP.Language) { case I_French: return "Jetpack intelligent"; case I_Spanish: return "Jetpack inteligente"; case I_German: return "Intelligentes Jetpack"; case I_Italian: return "Jetpack intelligente"; case I_Swedish: return "Smart jetpack"; default: return "Smart jetpack"; }
			}
			case TEXTSTRING_SMART_JETPACK_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "alterna el modo jetpack mientras estÃ© activo, tecla jump se comporta como jetpack (vuelo), tecla jetpack funciona como interruptor encendido/apagado.";
				switch (AvP.Language) { case I_French: return "Active ou desactive le mode jetpack. Lorsqu il est actif, saut utilise le jetpack (vol) et Jetpack sert d interrupteur."; case I_Spanish: return "alterna el modo jetpack mientras estÃ© activo, tecla jump se comporta como jetpack (vuelo), tecla jetpack funciona como interruptor encendido/apagado."; case I_German: return "Schaltet den Jetpack-Modus um. Wenn aktiv, nutzt Sprung das Jetpack (Flug) und Jetpack ist der Ein-Aus-Schalter."; case I_Italian: return "Attiva o disattiva la modalita jetpack. Quando attiva, Salto usa il jetpack (volo) e Jetpack e l interruttore."; case I_Swedish: return "Vaxlar jetpacklaget. Nar aktivt anvander Hopp jetpacken (flygning) och Jetpack ar pa-av-brytaren."; default: return "Toggles jetpack mode. While active, Jump behaves as jetpack (flight); Jetpack is the on/off switch."; }
			}			case TEXTSTRING_MARINE_CROSSHAIR_THICKNESS_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Crosshair grosor";
				switch (AvP.Language) { case I_French: return "Epaisseur du viseur"; case I_Spanish: return "Crosshair grosor"; case I_German: return "Fadenkreuzstarke"; case I_Italian: return "Spessore mirino"; case I_Swedish: return "Riktmedelstjocklek"; default: return "Crosshair thickness"; }
			}
			case TEXTSTRING_MARINE_CROSSHAIR_THICKNESS_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "permite aumentar el grosor del punto de mira para el marine";
				switch (AvP.Language) { case I_French: return "Permet d augmenter l epaisseur du viseur du Marine."; case I_Spanish: return "permite aumentar el grosor del punto de mira para el marine"; case I_German: return "Erhoht die Starke des Marine-Fadenkreuzes."; case I_Italian: return "Permette di aumentare lo spessore del mirino del Marine."; case I_Swedish: return "Gor det mojligt att oka tjockleken pa marinsiktet."; default: return "Allows increasing the Marine crosshair thickness."; }
			}
			case TEXTSTRING_MARINE_CROSSHAIR_THICKNESS_X10_NEW: return "x1.0";
			case TEXTSTRING_MARINE_CROSSHAIR_THICKNESS_X15_NEW: return "x1.5";
			case TEXTSTRING_MARINE_CROSSHAIR_THICKNESS_X20_NEW: return "x2.0";
			case TEXTSTRING_SMART_ZOOM_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Zoom inteligente";
				switch (AvP.Language) { case I_French: return "Zoom intelligent"; case I_Spanish: return "Zoom inteligente"; case I_German: return "Intelligenter Zoom"; case I_Italian: return "Zoom intelligente"; case I_Swedish: return "Smart zoom"; default: return "Smart zoom"; }
			}
			case TEXTSTRING_SMART_ZOOM_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "agrega \"zoom sostenido\" a los controles del depredador. mientras se mantiene pulsado, las teclas compartidas de \"aumentar zoom\" y \"disminuir zoom\" sustituyen temporalmente a siguiente/anterior arma, intensificador imagen y lanzar bengala; al soltarlo, cada tecla recupera su funciÃ³n normal. se permiten asignaciones compartidas.";
				switch (AvP.Language) { case I_French: return "Ajoute zoom maintenu aux controles du Predator. Tant qu il est maintenu, les touches partagees de zoom remplacent temporairement arme suivante/precedente, intensificateur et fusee eclairante; en le relachant, chaque touche retrouve son action normale."; case I_Spanish: return "agrega \"zoom sostenido\" a los controles del depredador. mientras se mantiene pulsado, las teclas compartidas de \"aumentar zoom\" y \"disminuir zoom\" sustituyen temporalmente a siguiente/anterior arma, intensificador imagen y lanzar bengala; al soltarlo, cada tecla recupera su funciÃ³n normal. se permiten asignaciones compartidas."; case I_German: return "Fugt Zoom halten zu den Predator-Steuerungen hinzu. Wahrend es gehalten wird, ersetzen geteilte Zoom-Tasten vorubergehend nachste/vorherige Waffe, Bildverstarker und Leuchtfackel; beim Loslassen kehrt jede Taste zu ihrer normalen Aktion zuruck."; case I_Italian: return "Aggiunge zoom tenuto ai controlli del Predator. Mentre e tenuto, i tasti zoom condivisi sostituiscono temporaneamente arma successiva/precedente, intensificatore e razzo luminoso; al rilascio ogni tasto torna alla sua azione normale."; case I_Swedish: return "Lagger till hall zoom i Predator-kontrollerna. Medan den halls ned ersatter delade zoomtangenter tillfalligt nasta/foregaende vapen, bildforstarkare och ljusbloss; vid slappt aterfar varje tangent sin vanliga funktion."; default: return "Adds Zoom holding to Predator controls. While held, shared Zoom in/Zoom out keys temporarily replace next/previous weapon, image intensifier, and flare; releasing it restores each key's normal action. Shared assignments are allowed."; }
			}
			case TEXTSTRING_VERTICAL_PITCH_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "InclinaciÃ³n vertical";
				switch (AvP.Language) { case I_French: return "Inclinaison verticale"; case I_Spanish: return "InclinaciÃ³n vertical"; case I_German: return "Vertikale Neigung"; case I_Italian: return "Inclinazione verticale"; case I_Swedish: return "Vertikal lutning"; default: return "Vertical pitch"; }
			}
			case TEXTSTRING_VERTICAL_PITCH_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "controla el comportamiento de la inclinaciÃ³n vertical del mouse. \"total\" permite mirar hacia arriba/abajo hasta 90 grados; \"vanilla\" restaura el comportamiento og aplicando un lÃ­mite de inclinaciÃ³n de 78.75 grados.";
				switch (AvP.Language) { case I_French: return "Controle l'inclinaison verticale de la souris. Complet permet de regarder vers le haut/bas jusqu'a 90Â°; vanilla conserve 78.75Â° pour marine/predateur et 90Â° pour alien."; case I_Spanish: return "controla el comportamiento de la inclinaciÃ³n vertical del mouse. \"total\" permite mirar hacia arriba/abajo hasta 90 grados; \"vanilla\" restaura el comportamiento og aplicando un lÃ­mite de inclinaciÃ³n de 78.75 grados."; case I_German: return "Steuert die vertikale Mausneigung. Voll erlaubt Blick nach oben/unten bis 90Â°; Vanilla behalt 78.75Â° fur Marine/Predator und 90Â° fur Alien."; case I_Italian: return "Controlla l'inclinazione verticale del mouse. Completo consente di guardare su/giu fino a 90Â°; vanilla mantiene 78.75Â° per marine/predatore e 90Â° per alien."; case I_Swedish: return "Styr musens vertikala lutning. Full tillater blick upp/ner till 90Â°; vanilla behaller 78.75Â° for marine/predator och 90Â° for alien."; default: return "Controls vertical mouse look. Full allows looking up/down to 90Â°; vanilla retains 78.75Â° for Marine/Predator and 90Â° for Alien."; }
			}
			case TEXTSTRING_VERTICAL_PITCH_VANILLA_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "vanilla";
				return "vanilla";
			}
			case TEXTSTRING_VERTICAL_PITCH_FULL_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "total";
				switch (AvP.Language) { case I_French: return "complet"; case I_Spanish: return "total"; case I_German: return "voll"; case I_Italian: return "completo"; case I_Swedish: return "full"; default: return "full"; }
			}
			case TEXTSTRING_PREDATOR_ZOOM_HOLDING_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Zoom sostenido";
				switch (AvP.Language) { case I_French: return "Zoom maintenu"; case I_Spanish: return "Zoom sostenido"; case I_German: return "Zoom halten"; case I_Italian: return "Zoom tenuto"; case I_Swedish: return "Hall zoom"; default: return "Zoom holding"; }
			}
			case TEXTSTRING_SMART_ZOOM_ACTIVATED_NEW:
			{
				char *nativeLabel = GetTextString(TEXTSTRING_SMART_ZOOM_NEW);
				if (nativeLabel) {
					if (!strcmp(nativeLabel,"Zoom inteligente")) return "zoom inteligente activado";
					if (!strcmp(nativeLabel,"Zoom intelligent")) return "zoom intelligent active";
					if (!strcmp(nativeLabel,"Intelligenter Zoom")) return "intelligenter Zoom aktiviert";
					if (!strcmp(nativeLabel,"Zoom intelligente")) return "zoom intelligente attivato";
					if (!strcmp(nativeLabel,"Smart zoom") && AvP.Language==I_Swedish) return "smart zoom aktiverad";
				}
				switch (AvP.Language) { case I_French: return "zoom intelligent active"; case I_Spanish: return "zoom inteligente activado"; case I_German: return "intelligenter Zoom aktiviert"; case I_Italian: return "zoom intelligente attivato"; case I_Swedish: return "smart zoom aktiverad"; default: return "smart zoom activated"; }
			}
			case TEXTSTRING_SMART_ZOOM_DEACTIVATED_NEW:
			{
				char *nativeLabel = GetTextString(TEXTSTRING_SMART_ZOOM_NEW);
				if (nativeLabel) {
					if (!strcmp(nativeLabel,"Zoom inteligente")) return "zoom inteligente desactivado";
					if (!strcmp(nativeLabel,"Zoom intelligent")) return "zoom intelligent desactive";
					if (!strcmp(nativeLabel,"Intelligenter Zoom")) return "intelligenter Zoom deaktiviert";
					if (!strcmp(nativeLabel,"Zoom intelligente")) return "zoom intelligente disattivato";
					if (!strcmp(nativeLabel,"Smart zoom") && AvP.Language==I_Swedish) return "smart zoom avaktiverad";
				}
				switch (AvP.Language) { case I_French: return "zoom intelligent desactive"; case I_Spanish: return "zoom inteligente desactivado"; case I_German: return "intelligenter Zoom deaktiviert"; case I_Italian: return "zoom intelligente disattivato"; case I_Swedish: return "smart zoom avaktiverad"; default: return "smart zoom deactivated"; }
			}
			case TEXTSTRING_RELOAD_WEAPON_NEW:
			{
				char *flashbackLabel = TextStringPtr[TEXTSTRING_MARINE_KEY_FLASHBACK_WEAPON];

				/* Release builds load the active language.txt, while AvP.Language
				   remains I_English.  Use the already-localised native bind label
				   as the same live language source as the rest of this menu. */
				if (flashbackLabel && (!strcmp(flashbackLabel,"Recordar Arma") || !strcmp(flashbackLabel,"Recordar arma"))) {
					return "Recargar arma";
				}

				/* Kept for configurations that use the engine's language enum. */
				switch (AvP.Language) {
					case I_French:  return "Recharger l'arme";
					case I_Spanish: return "Recargar arma";
					case I_German:  return "Waffe nachladen";
					case I_Italian: return "Ricarica arma";
					case I_Swedish: return "Ladda om vapen";
					case I_English:
					default:        return "Reload weapon";
				}
			}
			case TEXTSTRING_VIDEO_RENDERER_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Render";
				switch (AvP.Language) { case I_French: return "Rendu"; case I_Spanish: return "Render"; case I_German: return "Renderer"; case I_Italian: return "Rendering"; case I_Swedish: return "Renderare"; default: return "Renderer"; }
			}
			case TEXTSTRING_VIDEO_ASPECT_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "RelaciÃ³n aspecto";
				switch (AvP.Language) { case I_French: return "Format d image"; case I_Spanish: return "RelaciÃ³n aspecto"; case I_German: return "Seitenverhaltnis"; case I_Italian: return "Rapporto d aspetto"; case I_Swedish: return "Bildformat"; default: return "Aspect ratio"; }
			}
			case TEXTSTRING_VIDEO_RESOLUTION_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "ResoluciÃ³n";
				switch (AvP.Language) { case I_French: return "Resolution"; case I_Spanish: return "ResoluciÃ³n"; case I_German: return "Auflosung"; case I_Italian: return "Risoluzione"; case I_Swedish: return "Upplosning"; default: return "Resolution"; }
			}
			case TEXTSTRING_VIDEO_REFRESH_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Tasa refresco";
				switch (AvP.Language) { case I_French: return "Frequence"; case I_Spanish: return "Tasa refresco"; case I_German: return "Bildwiederholrate"; case I_Italian: return "Frequenza"; case I_Swedish: return "Uppdateringsfrekvens"; default: return "Refresh rate"; }
			}
			case TEXTSTRING_VIDEO_SCREENMODE_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Modo pantalla";
				switch (AvP.Language) { case I_French: return "Mode d ecran"; case I_Spanish: return "Modo pantalla"; case I_German: return "Bildschirmmodus"; case I_Italian: return "Modalita schermo"; case I_Swedish: return "Bildskarmslage"; default: return "Screen mode"; }
			}			case TEXTSTRING_VIDEO_VSYNC_NEW: return "VSync";
			case TEXTSTRING_VIDEO_VSYNC_ON_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Encendido";
				switch (AvP.Language) { case I_French: return "Active"; case I_Spanish: return "Encendido"; case I_German: return "Ein"; case I_Italian: return "Attivo"; case I_Swedish: return "Pa"; default: return "On"; }
			}
			case TEXTSTRING_VIDEO_VSYNC_OFF_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Apagado";
				switch (AvP.Language) { case I_French: return "Desactive"; case I_Spanish: return "Apagado"; case I_German: return "Aus"; case I_Italian: return "Disattivo"; case I_Swedish: return "Av"; default: return "Off"; }
			}
			case TEXTSTRING_VIDEO_VSYNC_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Establece sincronizaciÃ³n vertical de fotogramas";
				switch (AvP.Language) { case I_French: return "Definit la synchronisation verticale des images"; case I_Spanish: return "Establece sincronizaciÃ³n vertical de fotogramas"; case I_German: return "Legt die vertikale Bildsynchronisierung fest"; case I_Italian: return "Imposta la sincronizzazione verticale dei fotogrammi"; case I_Swedish: return "Staller in vertikal bildsynkronisering"; default: return "Sets vertical frame synchronization"; }
			}
			case TEXTSTRING_VIDEO_FPSCAP_NEW:
{
    char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
    if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Limitador FPS";
    switch (AvP.Language) { case I_French: return "Limite FPS"; case I_Spanish: return "Limitador FPS"; case I_German: return "FPS-Limit"; case I_Italian: return "Limite FPS"; case I_Swedish: return "FPS-grans"; default: return "FPS Cap"; }
}
case TEXTSTRING_VIDEO_FPSCAP_60_NEW: return "60";
case TEXTSTRING_VIDEO_FPSCAP_100_NEW: return "100";
case TEXTSTRING_VIDEO_FPSCAP_142_NEW: return "142";
case TEXTSTRING_VIDEO_FPSCAP_UNCAPPED_NEW:
{
    char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
    if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Ilimitado";
    switch (AvP.Language) { case I_French: return "Illimite"; case I_Spanish: return "Ilimitado"; case I_German: return "Unbegrenzt"; case I_Italian: return "Illimitato"; case I_Swedish: return "Obegransad"; default: return "Uncapped"; }
}
case TEXTSTRING_VIDEO_FPSCAP_CUSTOM_NEW:
{
    char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
    if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Personalizado";
    switch (AvP.Language) { case I_French: return "Personnalise"; case I_Spanish: return "Personalizado"; case I_German: return "Benutzerdefiniert"; case I_Italian: return "Personalizzato"; case I_Swedish: return "Anpassad"; default: return "Custom"; }
}
case TEXTSTRING_VIDEO_ASPECT_HELP_NEW:
{
    char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
    if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Selecciona un grupo de resoluciones acordes a la relacion aspecto";
    switch (AvP.Language) { case I_French: return "Selectionne un groupe de resolutions correspondant au format d image"; case I_Spanish: return "Selecciona un grupo de resoluciones acordes a la relacion aspecto"; case I_German: return "Wahlt eine Auflosungsgruppe passend zum Seitenverhaltnis"; case I_Italian: return "Seleziona un gruppo di risoluzioni adatto al rapporto d aspetto"; case I_Swedish: return "Valj en upplosningsgrupp som passar bildformatet"; default: return "Select a resolution group that matches the aspect ratio"; }
}
case TEXTSTRING_VIDEO_RESOLUTION_HELP_NEW:
{
    char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
    if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Selecciona una resolucion disponible dentro del grupo Relacion aspecto establecido anteriormente. Solo se listan resoluciones validas para SDL";
    switch (AvP.Language) { case I_French: return "Selectionne une resolution disponible dans le groupe Format d image choisi precedemment. Seules les resolutions SDL valides sont listees"; case I_Spanish: return "Selecciona una resolucion disponible dentro del grupo Relacion aspecto establecido anteriormente. Solo se listan resoluciones validas para SDL"; case I_German: return "Wahlt eine verfugbare Auflosung aus der zuvor gewahlten Seitenverhaltnisgruppe. Nur gultige SDL-Auflosungen werden angezeigt"; case I_Italian: return "Seleziona una risoluzione disponibile nel gruppo Rapporto d aspetto scelto prima. Sono elencate solo risoluzioni SDL valide"; case I_Swedish: return "Valj en tillganglig upplosning i den tidigare valda bildformatsgruppen. Endast giltiga SDL-upplosningar visas"; default: return "Select an available resolution from the Aspect ratio group selected above. Only SDL-valid resolutions are listed"; }
}
case TEXTSTRING_VIDEO_REFRESH_HELP_NEW: {
    static const char *const t[6]={"Select a refresh rate supported by your monitor. Only rates valid for SDL are listed. Using 100 Hz at 100 FPS is recommended for the smoothest, most fluid experience.","SÃ©lectionne une frÃ©quence prise en charge par ton Ã©cran. Seules les frÃ©quences valides pour SDL sont proposÃ©es. Il est recommandÃ© de jouer Ã  100 Hz et 100 FPS pour une fluiditÃ© optimale.","Selecciona los Hz soportados por tu monitor. Solo se listan Hz vÃ¡lidos para SDL. Se recomienda usar 100 Hz a 100 FPS para obtener la mejor fluidez y suavidad posible.","WÃ¤hle eine vom Monitor unterstÃ¼tzte Bildwiederholrate. Es werden nur fÃ¼r SDL gÃ¼ltige Werte angezeigt. FÃ¼r ein mÃ¶glichst flÃ¼ssiges Spielerlebnis werden 100 Hz bei 100 FPS empfohlen.","Seleziona una frequenza supportata dal monitor. Sono elencate solo le frequenze valide per SDL. Si consigliano 100 Hz a 100 FPS per ottenere la massima fluiditÃ .","VÃ¤lj en uppdateringsfrekvens som skÃ¤rmen stÃ¶der. Endast frekvenser som Ã¤r giltiga fÃ¶r SDL visas. 100 Hz vid 100 FPS rekommenderas fÃ¶r bÃ¤sta mÃ¶jliga flyt och mjukhet."};
    int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;
    return (char*)t[l>=0&&l<6?l:0];
 }

case TEXTSTRING_VIDEO_SCREENMODE_HELP_NEW: {
    static const char *const t[6]={"Sets the screen presentation mode. Windowed mode is not recommended because it still has issues with stability, smoothness and physics behavior.","DÃ©finit le mode d'affichage. Le mode fenÃªtrÃ© est dÃ©conseillÃ© car il prÃ©sente encore des problÃ¨mes de stabilitÃ©, de fluiditÃ© et de comportement physique.","Establece el modo de representaciÃ³n de pantalla. No se recomienda el modo ventana, dado que aÃºn posee problemas de estabilidad, fluidez y comportamiento de las fÃ­sicas.","Legt den Anzeigemodus fest. Der Fenstermodus wird nicht empfohlen, da er weiterhin Probleme mit StabilitÃ¤t, flÃ¼ssiger Darstellung und Physikverhalten aufweist.","Imposta la modalitÃ  di visualizzazione. La modalitÃ  finestra Ã¨ sconsigliata perchÃ© presenta ancora problemi di stabilitÃ , fluiditÃ  e comportamento della fisica.","Anger skÃ¤rmens visningslÃ¤ge. FÃ¶nsterlÃ¤ge rekommenderas inte eftersom det fortfarande har problem med stabilitet, flyt och fysikens beteende."};
    int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;
    return (char*)t[l>=0&&l<6?l:0];
 }

case TEXTSTRING_VIDEO_FPSCAP_HELP_NEW:
{
    char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
    if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "limita los fotogramas dentro del juego: 60 = experiencia og, 100 = maximo recomendado, 142 = limite jugable aunque muy inestable, Ilimitado = no recomendado, el tickrate del juego se rompe, Personalizado = establece un valor manual";
    switch (AvP.Language) { case I_French: return "limite les images en jeu: 60 = experience originale, 100 = maximum recommande, 142 = limite jouable mais tres instable, Illimite = non recommande, le tickrate du jeu se casse, Personnalise = definit une valeur manuelle"; case I_Spanish: return "limita los fotogramas dentro del juego: 60 = experiencia og, 100 = maximo recomendado, 142 = limite jugable aunque muy inestable, Ilimitado = no recomendado, el tickrate del juego se rompe, Personalizado = establece un valor manual"; case I_German: return "begrenzt die Spielbilder: 60 = Originalerlebnis, 100 = empfohlenes Maximum, 142 = spielbares aber sehr instabiles Limit, Unbegrenzt = nicht empfohlen, die Tickrate bricht, Benutzerdefiniert = manuellen Wert festlegen"; case I_Italian: return "limita i fotogrammi in gioco: 60 = esperienza originale, 100 = massimo consigliato, 142 = limite giocabile ma molto instabile, Illimitato = non consigliato, il tickrate del gioco si rompe, Personalizzato = imposta un valore manuale"; case I_Swedish: return "begransar bildrutor i spelet: 60 = originalupplevelse, 100 = rekommenderat maximum, 142 = spelbar men mycket instabil grans, Obegransad = rekommenderas inte, spelets tickrate bryts, Anpassad = ange ett manuellt varde"; default: return "Limits in-game frames: 60 = original experience, 100 = recommended maximum, 142 = playable limit but highly unstable, Uncapped = not recommended, the game tick rate breaks, Custom = set a manual value"; }
}case TEXTSTRING_VIDEO_ASPECT_OTHER_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Otros";
				switch (AvP.Language) { case I_French: return "Autre"; case I_Spanish: return "Otros"; case I_German: return "Andere"; case I_Italian: return "Altro"; case I_Swedish: return "Annat"; default: return "Other"; }
			}
			case TEXTSTRING_VIDEO_WINDOWED_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Ventana";
				switch (AvP.Language) { case I_French: return "Fenetre"; case I_Spanish: return "Ventana"; case I_German: return "Fenster"; case I_Italian: return "Finestra"; case I_Swedish: return "Fonster"; default: return "Windowed"; }
			}
			case TEXTSTRING_VIDEO_BORDERLESS_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Completa sin bordes";
				switch (AvP.Language) { case I_French: return "Plein ecran sans bordure"; case I_Spanish: return "Completa sin bordes"; case I_German: return "Rahmenloser Vollbildmodus"; case I_Italian: return "Schermo intero senza bordi"; case I_Swedish: return "Ramlost helskarmslage"; default: return "Borderless fullscreen"; }
			}
			case TEXTSTRING_VIDEO_EXCLUSIVE_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "Completa exclusiva";
				switch (AvP.Language) { case I_French: return "Plein ecran exclusif"; case I_Spanish: return "Completa exclusiva"; case I_German: return "Exklusiver Vollbildmodus"; case I_Italian: return "Schermo intero esclusivo"; case I_Swedish: return "Exklusivt helskarmslage"; default: return "Exclusive fullscreen"; }
			}
			case TEXTSTRING_HUD_ANCHORED_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "HUD anclado";
				switch (AvP.Language) { case I_French: return "HUD ancre"; case I_Spanish: return "HUD anclado"; case I_German: return "HUD-Verankerung"; case I_Italian: return "HUD ancorato"; case I_Swedish: return "HUD-forankring"; default: return "HUD anchored"; }
			}
			case TEXTSTRING_HUD_ANCHORED_HELP_NEW: {
    static const char *const t[6]={"Preserves the original HUD proportions and adjusts its side anchors. \"Unlocked\" mode enables the \"HUD position\" slider further up this menu.","Conserve les proportions originales du HUD et ajuste ses ancrages latÃ©raux. Le mode libre active le rÃ©glage de position du HUD plus haut dans ce menu.","Mantiene la proporciÃ³n original del HUD y ajusta sus anclajes laterales; la opciÃ³n \"libre\" habilita el slider \"HUD posicionamiento\" mÃ¡s arriba en este menÃº.","Bewahrt die ursprÃ¼nglichen HUD-Proportionen und passt die seitlichen Anker an. Der freie Modus aktiviert den Regler fÃ¼r die HUD-Position weiter oben in diesem MenÃ¼.","Mantiene le proporzioni originali dell'HUD e regola gli ancoraggi laterali. La modalitÃ  libera abilita il cursore della posizione HUD piÃ¹ in alto nel menu.","BehÃ¥ller HUD:ens ursprungliga proportioner och justerar dess sidofÃ¤sten. Fritt lÃ¤ge aktiverar reglaget fÃ¶r HUD-position lÃ¤ngre upp i menyn."};
    int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;
    return (char*)t[l>=0&&l<6?l:0];
 }

			case TEXTSTRING_HUD_ANCHORED_4_3_NEW: return "4:3";
			case TEXTSTRING_HUD_ANCHORED_16_9_NEW: return "16:9";
			case TEXTSTRING_HUD_ANCHORED_UNLOCKED_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "libre";
				switch (AvP.Language) { case I_French: return "libre"; case I_Spanish: return "libre"; case I_German: return "frei"; case I_Italian: return "libero"; case I_Swedish: return "fri"; default: return "unlocked"; }
			}
			case TEXTSTRING_HUD_POSITION_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "HUD posicionamiento";
				switch (AvP.Language) { case I_French: return "Position HUD"; case I_Spanish: return "HUD posicionamiento"; case I_German: return "HUD-Position"; case I_Italian: return "Posizione HUD"; case I_Swedish: return "HUD-position"; default: return "HUD position"; }
			}
			case TEXTSTRING_HUD_POSITION_HELP_NEW:
			{
				char *nativeLabel = TextStringPtr[TEXTSTRING_MOUSECONTROLS_AUTOCENTRE];
				if (nativeLabel && (!strcmp(nativeLabel,"Auto-centrar") || !strcmp(nativeLabel,"Auto Centrar"))) return "ajusta la distancia horizontal del HUD. ENTER alterna 4:3, 16:9 y el Ãºltimo valor libre";
				switch (AvP.Language) { case I_French: return "Ajuste la distance horizontale du HUD. ENTREE alterne 4:3, 16:9 et la derniere valeur libre."; case I_Spanish: return "ajusta la distancia horizontal del HUD. ENTER alterna 4:3, 16:9 y el Ãºltimo valor libre"; case I_German: return "Passt den horizontalen HUD-Abstand an. ENTER wechselt 4:3, 16:9 und den letzten freien Wert."; case I_Italian: return "Regola la distanza orizzontale dell HUD. INVIO alterna 4:3, 16:9 e l ultimo valore libero."; case I_Swedish: return "Justerar HUD:ens horisontella avstand. ENTER vaxlar 4:3, 16:9 och senaste fria varde."; default: return "Adjusts HUD horizontal distance. ENTER cycles 4:3, 16:9, and the last unlocked value."; }
				}
			case TEXTSTRING_HUD_SCALE_NEW:
			{
				if (UsingSpanishTextStrings()) return "HUD escala";
				switch (AvP.Language) { case I_French: return "Echelle HUD"; case I_Spanish: return "HUD escala"; case I_German: return "HUD-Skalierung"; case I_Italian: return "Scala HUD"; case I_Swedish: return "HUD-skala"; default: return "HUD scale"; }
			}
			case TEXTSTRING_HUD_SCALE_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "modifica la escala del HUD. presionar ENTER resetea su valor por defecto";
				switch (AvP.Language) { case I_French: return "Modifie l echelle du HUD. ENTREE restaure la valeur par defaut."; case I_Spanish: return "modifica la escala del HUD. presionar ENTER resetea su valor por defecto"; case I_German: return "Andert die HUD-Skalierung. ENTER setzt den Standardwert zuruck."; case I_Italian: return "Modifica la scala dell HUD. INVIO ripristina il valore predefinito."; case I_Swedish: return "Andrar HUD-skalan. ENTER aterstaller standardvardet."; default: return "Changes the HUD scale. Press ENTER to reset its default value."; }
			}			case TEXTSTRING_CROSSHAIR_SCALE_NEW:
			{
				if (UsingSpanishTextStrings()) return "Crosshair escala";
				switch (AvP.Language) { case I_French: return "Echelle du viseur"; case I_Spanish: return "Crosshair escala"; case I_German: return "Fadenkreuz-Skalierung"; case I_Italian: return "Scala mirino"; case I_Swedish: return "Riktmedelsskala"; default: return "Crosshair scale"; }
			}
			case TEXTSTRING_CROSSHAIR_SCALE_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "modifica la escala del crosshair. presionar ENTER resetea su valor por defecto";
				switch (AvP.Language) { case I_French: return "Modifie l echelle du viseur. ENTREE restaure la valeur par defaut."; case I_Spanish: return "modifica la escala del crosshair. presionar ENTER resetea su valor por defecto"; case I_German: return "Andert die Fadenkreuz-Skalierung. ENTER setzt den Standardwert zuruck."; case I_Italian: return "Modifica la scala del mirino. INVIO ripristina il valore predefinito."; case I_Swedish: return "Andrar riktmedelsskalan. ENTER aterstaller standardvardet."; default: return "Changes the crosshair scale. Press ENTER to reset its default value."; }
			}
			case TEXTSTRING_NOISE_SCALE_NEW:
			{
				if (UsingSpanishTextStrings()) return "escala de ruido";
				switch (AvP.Language) { case I_French: return "Echelle du bruit"; case I_Spanish: return "escala de ruido"; case I_German: return "Rauschskalierung"; case I_Italian: return "Scala del rumore"; case I_Swedish: return "Brusskala"; default: return "Noise scale"; }
			}
            case TEXTSTRING_ALWAYS_RUN_NO_NEW:
                if(UsingSpanishTextStrings())return "No";
                switch(AvP.Language){case I_French:return "Non";case I_German:return "Nein";case I_Swedish:return "Nej";default:return "No";}
            case TEXTSTRING_ALWAYS_RUN_YES_NEW:
                if(UsingSpanishTextStrings())return "Sí";
                switch(AvP.Language){case I_Spanish:return "Sí";case I_French:return "Oui";case I_German:return "Ja";case I_Italian:return "Sì";case I_Swedish:return "Ja";default:return "Yes";}
            case TEXTSTRING_ALWAYS_RUN_NEW:
            {
                if(UsingSpanishTextStrings())return "Correr siempre";
                switch(AvP.Language){
                default:return "Always run";
                case I_Spanish:return "Correr siempre";
                case I_French:return "Toujours courir";
                case I_German:return "Immer rennen";
                case I_Italian:return "Corri sempre";
                case I_Swedish:return "Spring alltid";
                }
            }
            case TEXTSTRING_RUN_ACTION_NEW:
            {
                if(UsingSpanishTextStrings())return "Correr";
                switch(AvP.Language){
                default:return "Run";
                case I_Spanish:return "Correr";
                case I_French:return "Courir";
                case I_German:return "Rennen";
                case I_Italian:return "Correre";
                case I_Swedish:return "Spring";
                }
            }
            case TEXTSTRING_WALK_ACTION_NEW:
            {
                if(UsingSpanishTextStrings())return "Caminar";
                switch(AvP.Language){
                default:return "Walk";
                case I_Spanish:return "Caminar";
                case I_French:return "Marcher";
                case I_German:return "Gehen";
                case I_Italian:return "Camminare";
                case I_Swedish:return "Gå";
                }
            }
            case TEXTSTRING_ALWAYS_RUN_HELP_NEW:
            {
                if(UsingSpanishTextStrings())return "no = camina normalmente; mantén pulsado Correr para correr. sí = corre normalmente; mantén pulsado Caminar para caminar. Conserva la misma tecla asignada y guarda esta preferencia por separado para cada especie.";
                switch(AvP.Language){
                default:return "Off = walk by default; hold Run to run. On = run by default; hold Walk to walk. Uses the same key binding and saves this preference separately for each species.";
                case I_Spanish:return "no = camina normalmente; mantén pulsado Correr para correr. sí = corre normalmente; mantén pulsado Caminar para caminar. Conserva la misma tecla asignada y guarda esta preferencia por separado para cada especie.";
                case I_French:return "Non = marche par défaut ; maintenez Courir pour courir. Oui = court par défaut ; maintenez Marcher pour marcher. Conserve la même touche et enregistre ce réglage séparément pour chaque espèce.";
                case I_German:return "Nein = normalerweise gehen; Rennen gedrückt halten, um zu rennen. Ja = normalerweise rennen; Gehen gedrückt halten, um zu gehen. Behält dieselbe Taste und speichert die Einstellung für jede Spezies getrennt.";
                case I_Italian:return "No = cammina normalmente; tieni premuto Correre per correre. Sì = corri normalmente; tieni premuto Camminare per camminare. Mantiene lo stesso tasto e salva questa preferenza separatamente per ogni specie.";
                case I_Swedish:return "Nej = gå normalt; håll Spring intryckt för att springa. Ja = spring normalt; håll Gå intryckt för att gå. Behåller samma tangent och sparar inställningen separat för varje art.";
                }
            }
			case TEXTSTRING_NOISE_SCALE_HELP_NEW:
            {
                if(UsingSpanishTextStrings())return "ajusta la densidad de los efectos \"ruido electroestático\" y \"escaneo de líneas\". 1.0 = conserva la densidad original adaptada a resoluciones modernas; 0.5 = aumenta la densidad del \"ruido electroestático\" y \"escaneo de líneas\" otorgando un resultado granular más fino; se aplica a los efectos de marine = \"amplificador de imagen\"; depredador = \"aumentar zoom/reducir zoom\" y \"camuflaje\".";
                switch(AvP.Language){
                case I_Spanish:return "ajusta la densidad de los efectos \"ruido electroestático\" y \"escaneo de líneas\". 1.0 = conserva la densidad original adaptada a resoluciones modernas; 0.5 = aumenta la densidad del \"ruido electroestático\" y \"escaneo de líneas\" otorgando un resultado granular más fino; se aplica a los efectos de marine = \"amplificador de imagen\"; depredador = \"aumentar zoom/reducir zoom\" y \"camuflaje\".";
                default:return "Adjusts the density of the \"electrostatic noise\" and \"scanline\" effects. 1.0 = preserves the original density adapted to modern resolutions; 0.5 = increases the density of \"electrostatic noise\" and \"scanlines\", producing finer grain. Applies to Marine = \"image intensifier\"; Predator = \"zoom in/zoom out\" and \"cloaking\".";
                case I_French:return "Ajuste la densité des effets \"bruit électrostatique\" et \"lignes de balayage\". 1.0 = conserve la densité originale adaptée aux résolutions modernes ; 0.5 = augmente la densité du bruit et des lignes de balayage pour un grain plus fin. Effets concernés : Marine = \"amplificateur d'image\"; Predator = \"zoom avant/arrière\" et \"camouflage\".";
                case I_German:return "Passt die Dichte der Effekte \"elektrostatisches Rauschen\" und \"Scanlinien\" an. 1.0 = erhält die ursprüngliche Dichte, angepasst an moderne Auflösungen; 0.5 = erhöht die Dichte von Rauschen und Scanlinien für eine feinere Körnung. Gilt für Marine = \"Bildverstärker\"; Predator = \"Heranzoomen/Herauszoomen\" und \"Tarnung\".";
                case I_Italian:return "Regola la densità degli effetti \"rumore elettrostatico\" e \"linee di scansione\". 1.0 = conserva la densità originale adattata alle risoluzioni moderne; 0.5 = aumenta la densità del rumore e delle linee di scansione, producendo una grana più fine. Si applica a Marine = \"intensificatore d'immagine\"; Predator = \"aumenta/riduci zoom\" e \"mimetizzazione\".";
                case I_Swedish:return "Justerar tätheten hos effekterna \"elektrostatiskt brus\" och \"skanningslinjer\". 1.0 = behåller den ursprungliga tätheten anpassad till moderna upplösningar; 0.5 = ökar tätheten hos brus och skanningslinjer för finare kornighet. Gäller Marine = \"bildförstärkare\"; Predator = \"zooma in/zooma ut\" och \"kamouflage\".";
                }
            }
			case TEXTSTRING_NOISE_SCALE_1X_NEW: return "1.0";
			case TEXTSTRING_NOISE_SCALE_HALF_NEW: return "0.5";
			case TEXTSTRING_PARTICLE_COMPLEXITY_NEW:
			{
				if (UsingSpanishTextStrings()) return "efectos de partÃ­culas complejas";
				switch (AvP.Language) { case I_French: return "Effets de particules complexes"; case I_Spanish: return "efectos de partÃ­culas complejas"; case I_German: return "Komplexe Partikeleffekte"; case I_Italian: return "Effetti particellari complessi"; case I_Swedish: return "Komplexa partikeleffekter"; default: return "Complex particle effects"; }
			}
			case TEXTSTRING_COLOR_BIT_DEPTH_NEW:
			{
				if (UsingSpanishTextStrings()) return "profundidad bits color";
				switch (AvP.Language) { case I_French: return "Profondeur de couleur"; case I_Spanish: return "profundidad bits color"; case I_German: return "Farbtiefe"; case I_Italian: return "Profondita colore"; case I_Swedish: return "Fargdjup"; default: return "Color bit depth"; }
			}
			case TEXTSTRING_COLOR_BIT_DEPTH_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "establece la profundidad de color de la escena. 16 bits = aplica RGB 5:6:5, 32 bits = aplica RGB 8:8:8";
				switch (AvP.Language) { case I_French: return "Definit la profondeur de couleur de la scene. 16 bits = RGB 5:6:5, 32 bits = RGB 8:8:8."; case I_Spanish: return "establece la profundidad de color de la escena. 16 bits = aplica RGB 5:6:5, 32 bits = aplica RGB 8:8:8"; case I_German: return "Legt die Farbtiefe der Szene fest. 16 Bit = RGB 5:6:5, 32 Bit = RGB 8:8:8."; case I_Italian: return "Imposta la profondita di colore della scena. 16 bit = RGB 5:6:5, 32 bit = RGB 8:8:8."; case I_Swedish: return "Staller in scenens fargdjup. 16 bit = RGB 5:6:5, 32 bit = RGB 8:8:8."; default: return "Sets the scene color depth. 16 bits = RGB 5:6:5, 32 bits = RGB 8:8:8."; }
			}
			case TEXTSTRING_COLOR_BIT_DEPTH_16_NEW: return "16 bits";
			case TEXTSTRING_COLOR_BIT_DEPTH_32_NEW: return "32 bits";			case TEXTSTRING_HUD_TEXTURE_FILTER_NEW:
			{
				if (UsingSpanishTextStrings()) return "HUD filtro textura";
				switch (AvP.Language) { case I_French: return "Filtre de texture HUD"; case I_Spanish: return "HUD filtro textura"; case I_German: return "HUD-Texturfilter"; case I_Italian: return "Filtro texture HUD"; case I_Swedish: return "HUD-texturfilter"; default: return "HUD texture filter"; }
			}
			case TEXTSTRING_HUD_TEXTURE_FILTER_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "aplica filtrado bilinear/nearest al HUD";
				switch (AvP.Language) { case I_French: return "Applique le filtrage bilineaire/nearest au HUD."; case I_Spanish: return "aplica filtrado bilinear/nearest al HUD"; case I_German: return "Wendet bilineare/nearest-Filterung auf das HUD an."; case I_Italian: return "Applica il filtraggio bilineare/nearest all HUD."; case I_Swedish: return "Anvander bilinear/nearest-filtrering pa HUD."; default: return "Applies bilinear/nearest filtering to the HUD."; }
			}
			case TEXTSTRING_HUD_TEXTURE_FILTER_BILINEAR_NEW: return "bilinear";
			case TEXTSTRING_HUD_TEXTURE_FILTER_NEAREST_NEW: return "nearest";
			case TEXTSTRING_ANTIALIASING_NEW: return "MSAA";
			case TEXTSTRING_ANTIALIASING_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "selecciona el alisado de bordes. se aplica al recrear la pantalla";
				switch (AvP.Language) { case I_French: return "SÃ©lectionne l anticrÃ©nelage. AppliquÃ© lors de la recrÃ©ation de l affichage."; case I_Spanish: return "selecciona el alisado de bordes. se aplica al recrear la pantalla"; case I_German: return "WÃ¤hlt die KantenglÃ¤ttung. Wird beim Neuerstellen der Anzeige angewendet."; case I_Italian: return "Seleziona l antialiasing. Viene applicato ricreando lo schermo."; case I_Swedish: return "Valjer kantutjÃ¤mning. TillÃ¤mpas nÃ¤r skÃ¤rmen Ã¥terskapas."; default: return "Selects edge anti-aliasing. Applied when the display is recreated."; }
			}
			case TEXTSTRING_ANTIALIASING_OFF_NEW: return "off";
            case TEXTSTRING_ANTIALIASING_X2_NEW: return "x2";
			case TEXTSTRING_ANTIALIASING_X4_NEW: return "x4";
			case TEXTSTRING_ANTIALIASING_X8_NEW: return "x8";
			case TEXTSTRING_ANISOTROPY_NEW:
			{
				if (UsingSpanishTextStrings()) return "AnisotropÃ­a";
				switch (AvP.Language) { case I_French: return "Anisotropie"; case I_Spanish: return "AnisotropÃ­a"; case I_German: return "Anisotropie"; case I_Italian: return "Anisotropia"; case I_Swedish: return "Anisotropi"; default: return "Anisotropy"; }
			}
			case TEXTSTRING_ANISOTROPY_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "ajusta el filtrado anisotrÃ³pico de las texturas";
				switch (AvP.Language) { case I_French: return "Ajuste le filtrage anisotrope des textures."; case I_Spanish: return "ajusta el filtrado anisotrÃ³pico de las texturas"; case I_German: return "Passt die anisotrope Texturfilterung an."; case I_Italian: return "Regola il filtraggio anisotropico delle texture."; case I_Swedish: return "Justerar anisotropisk texturfiltrering."; default: return "Adjusts anisotropic texture filtering."; }
			}
			case TEXTSTRING_ANISOTROPY_OFF_NEW: return "off";
            case TEXTSTRING_ANISOTROPY_X2_NEW: return "x2";
			case TEXTSTRING_ANISOTROPY_X4_NEW: return "x4";
			case TEXTSTRING_ANISOTROPY_X8_NEW: return "x8";
			case TEXTSTRING_ANISOTROPY_X16_NEW: return "x16";
			case TEXTSTRING_TEXTURE_FILTER_NEW:
			{
				if (UsingSpanishTextStrings()) return "Filtro Textura";
				switch (AvP.Language) { case I_French: return "Filtre de texture"; case I_Spanish: return "Filtro Textura"; case I_German: return "Texturfilter"; case I_Italian: return "Filtro texture"; case I_Swedish: return "Texturfilter"; default: return "Texture filter"; }
			}
			case TEXTSTRING_TEXTURE_FILTER_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "selecciona el filtrado general de las texturas";
				switch (AvP.Language) { case I_French: return "SÃ©lectionne le filtrage gÃ©nÃ©ral des textures."; case I_Spanish: return "selecciona el filtrado general de las texturas"; case I_German: return "WÃ¤hlt die allgemeine Texturfilterung."; case I_Italian: return "Seleziona il filtraggio generale delle texture."; case I_Swedish: return "Valjer allmÃ¤n texturfiltrering."; default: return "Selects general texture filtering."; }
			}
			case TEXTSTRING_TEXTURE_FILTER_NEAREST_NEW: return "Nearest";
			case TEXTSTRING_TEXTURE_FILTER_BILINEAR_NEW: return "Bilinear";
			case TEXTSTRING_TEXTURE_FILTER_TRILINEAR_NEW: return "Trilinear";
			case TEXTSTRING_MONITOR_NO_SIGNAL_EFFECT_NEW:
			{
				if (UsingSpanishTextStrings()) return "efecto sin seÃ±al monitores";
				switch (AvP.Language) { case I_French: return "Effet sans signal des moniteurs"; case I_Spanish: return "efecto sin seÃ±al monitores"; case I_German: return "Monitor-Keine-Signal-Effekt"; case I_Italian: return "Effetto nessun segnale monitor"; case I_Swedish: return "Monitorers ingen-signal-effekt"; default: return "Monitor no-signal effect"; }
			}
			case TEXTSTRING_MONITOR_NO_SIGNAL_EFFECT_HELP_NEW: {
    static const char *const t[6]={"Changes the no-video signal on in-game monitors and panels; rgb noise = displays static noise; rgb scanline = displays a vertical pattern","Change le signal sans vidéo des écrans et panneaux du jeu ; rgb noise = affiche du bruit statique ; rgb scanline = affiche un motif vertical","alterna la señal sin video de los monitores/paneles dentro del juego; rgb noise = muestra ruido estático; rgb scanline = muestra patrón vertical","Ändert das Signal ohne Video auf Monitoren und Bedienfeldern im Spiel; rgb noise = zeigt statisches Rauschen; rgb scanline = zeigt ein vertikales Muster","Cambia il segnale senza video dei monitor e pannelli nel gioco; rgb noise = mostra rumore statico; rgb scanline = mostra un motivo verticale","Ändrar signalen utan video på skärmar och paneler i spelet; rgb noise = visar statiskt brus; rgb scanline = visar ett vertikalt mönster"};
    int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;
    return (char*)t[l>=0&&l<6?l:0];
 }

			case TEXTSTRING_MONITOR_NO_SIGNAL_EFFECT_RGB_NOISE_NEW: return "rgb noise";
			case TEXTSTRING_MONITOR_NO_SIGNAL_EFFECT_RGB_SCANLINES_NEW: return "rgb scanlines";
            case TEXTSTRING_CONSOLE_TEXT_SCALE_NEW:
            {
             static const char *const labels[6]={"ingame menu/console text scaler","échelle texte menu en jeu/console","escalar textos menú ingame/consola","Textskalierung Spielmenü/Konsole","scala testo menu in gioco/console","textskala spelmeny/konsol"};
             int l=UsingSpanishTextStrings()?2:AvP.Language;
             return (char*)labels[l>=0&&l<6?l:0];
            }
			case TEXTSTRING_CONSOLE_TEXT_SCALE_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "cambia el tamaÃ±o de consola, transiciÃ³n y menÃºs durante juego; no modifica HUD ni menÃºs principales";
				switch (AvP.Language) { case I_French: return "Modifie seulement la taille de la console et de son texte."; case I_German: return "Andert nur die Grosse der Konsole und ihres Textes."; case I_Italian: return "Modifica solo le dimensioni della console e del suo testo."; case I_Swedish: return "Andrar bara konsolens och dess texts storlek."; default: return "Changes console, transition-briefing, and in-game menu text size; HUD and main menus are unchanged."; }
			}
			case TEXTSTRING_CONSOLE_TEXT_SCALE_X1_NEW: return "x1.0";
			case TEXTSTRING_CONSOLE_TEXT_SCALE_X2_NEW: return "x1.5";
		case TEXTSTRING_CONSOLE_TEXT_SCALE_X20_NEW: return "x2.0";
		case TEXTSTRING_CONSOLE_TEXT_SCALE_X25_NEW: return "x2.5";
		case TEXTSTRING_CONSOLE_TEXT_SCALE_X30_NEW: return "x3.0";
        case TEXTSTRING_CONSOLE_TEXT_SCALE_X35_NEW: return "x3.5";
        case TEXTSTRING_CONSOLE_TEXT_SCALE_X40_NEW: return "x4.0";
        case TEXTSTRING_CONSOLE_TEXT_SCALE_X45_NEW: return "x4.5";
        case TEXTSTRING_CONSOLE_TEXT_SCALE_X50_NEW: return "x5.0";
			case TEXTSTRING_MUSIC_PLAY_MODE_NEW:
			{
				if (UsingSpanishTextStrings()) return "mÃºsica modo reproducciÃ³n";
				switch (AvP.Language) { case I_French: return "mode de lecture de la musique"; case I_German: return "Musikwiedergabemodus"; case I_Italian: return "modalitÃ  di riproduzione musicale"; case I_Swedish: return "musikuppspelningslÃ¤ge"; default: return "music play mode"; }
			}
			case TEXTSTRING_MUSIC_PLAY_MODE_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "reproducir solo una vez = solo reproduce la música 1 sola vez al cargar el mapa; repetir con silencio aleatorio = repite la música en bucle, pero añade un tiempo de silencio. la duración del silencio es aleatorio, varía desde 5segs hasta 5mins; repetir siempre = repite la música en un bucle infinito, no hay tiempos de silencio entre repeticiones.";
				switch (AvP.Language) {
					case I_French: return "une seule fois = joue la musique une seule fois au chargement de la carte; répéter avec un silence aléatoire = joue la musique en boucle, mais ajoute un temps de silence. Sa durée est aléatoire, de 5 secondes à 5 minutes; toujours répéter = joue la musique en boucle infinie, sans silence entre les répétitions.";
					case I_Spanish: return "reproducir solo una vez = solo reproduce la música 1 sola vez al cargar el mapa; repetir con silencio aleatorio = repite la música en bucle, pero añade un tiempo de silencio. la duración del silencio es aleatorio, varía desde 5segs hasta 5mins; repetir siempre = repite la música en un bucle infinito, no hay tiempos de silencio entre repeticiones.";
					case I_German: return "einmal abspielen = spielt die Musik beim Laden der Karte nur einmal; mit zufälliger Pause wiederholen = wiederholt die Musik in einer Schleife, fügt aber eine Pause hinzu. Ihre Dauer ist zufällig und reicht von 5 Sekunden bis 5 Minuten; immer wiederholen = wiederholt die Musik endlos ohne Pausen zwischen den Wiederholungen.";
					case I_Italian: return "riproduci una volta = riproduce la musica una sola volta al caricamento della mappa; ripeti con silenzio casuale = ripete la musica in ciclo, ma aggiunge un periodo di silenzio. La durata è casuale, da 5 secondi a 5 minuti; ripeti sempre = ripete la musica in un ciclo infinito, senza silenzi tra le ripetizioni.";
					case I_Swedish: return "spela en gång = spelar musiken en gång när kartan laddas; upprepa med slumpmässig tystnad = upprepar musiken i en slinga, men lägger till en tyst period. Längden är slumpmässig, från 5 sekunder till 5 minuter; upprepa alltid = spelar musiken i en oändlig slinga utan tystnad mellan upprepningarna.";
					default: return "one time only = plays music only once when the map loads; repeat with random silence = loops the music but adds a period of silence. Its duration is random, ranging from 5 seconds to 5 minutes; always repeat = loops the music forever with no silence between repeats.";
				}
			}
			case TEXTSTRING_MUSIC_PLAY_MODE_ONE_TIME_ONLY_NEW:
			{
				if (UsingSpanishTextStrings()) return "reproducir solo una vez";
				switch (AvP.Language) { case I_French: return "une seule fois"; case I_German: return "einmal abspielen"; case I_Italian: return "riproduci una volta"; case I_Swedish: return "spela en gÃ¥ng"; default: return "one time only"; }
			}
			case TEXTSTRING_MUSIC_PLAY_MODE_REPEAT_RANDOM_SILENCES_NEW:
			{
				if (UsingSpanishTextStrings()) return "repetir con silencio aleatorio";
				switch (AvP.Language) { case I_French: return "rÃ©pÃ©ter avec un silence alÃ©atoire"; case I_German: return "mit zufÃ¤lliger Pause wiederholen"; case I_Italian: return "ripeti con silenzio casuale"; case I_Swedish: return "upprepa med slumpmÃ¤ssig tystnad"; default: return "repeat with random silence"; }
			}
			case TEXTSTRING_MUSIC_PLAY_MODE_ALWAYS_REPEAT_NEW:
			{
				if (UsingSpanishTextStrings()) return "repetir siempre";
				switch (AvP.Language) { case I_French: return "toujours rÃ©pÃ©ter"; case I_German: return "immer wiederholen"; case I_Italian: return "ripeti sempre"; case I_Swedish: return "upprepa alltid"; default: return "always repeat"; }
			}			case TEXTSTRING_FPS_DISPLAY_NEW:
			{
				if (UsingSpanishTextStrings()) return "mostrar FPS";
				switch (AvP.Language) { case I_French: return "afficher FPS"; case I_German: return "FPS anzeigen"; case I_Italian: return "mostra FPS"; case I_Swedish: return "visa FPS"; default: return "show FPS"; }
			}
			case TEXTSTRING_FPS_DISPLAY_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "muestra fotogramas por segundo";
				switch (AvP.Language) { case I_French: return "affiche les images par seconde"; case I_German: return "zeigt Bilder pro Sekunde an"; case I_Italian: return "mostra fotogrammi al secondo"; case I_Swedish: return "visar bildrutor per sekund"; default: return "shows frames per second"; }
			}
			case TEXTSTRING_SCREENSHOT_SAVED_NEW:
 if(UsingSpanishTextStrings()) return "captura %s realizada: %s";
 switch(AvP.Language) {case I_French:return "capture %s enregistrée : %s";case I_German:return "%s-Bildschirmfoto gespeichert: %s";case I_Italian:return "cattura %s salvata: %s";case I_Swedish:return "%s-skärmbild sparad: %s";default:return "%s screenshot saved: %s";}
case TEXTSTRING_MOLECULAR_ACID_NEW:
 if(UsingSpanishTextStrings())return "Ácido molecular";
 switch(AvP.Language){default:return "Molecular acid";case I_French:return "Acide moléculaire";case I_German:return "Molekulare Säure";case I_Italian:return "Acido molecolare";case I_Spanish:return "Ácido molecular";case I_Swedish:return "Molekylär syra";}
case TEXTSTRING_MOLECULAR_ACID_HELP_NEW:
 if(UsingSpanishTextStrings())return "Mueve las flechas izq/der para cambiar el multiplicador de dificultad. La base de comportamiento corresponde a Versión del director, pero la supervivencia se vuelve todavía más severa y desafiante. Resumen: Versión del director + más enemigos + balance de recursos del jugador.";
 switch(AvP.Language){default:return "Use Left/Right to change the difficulty multiplier. Behaviour is based on Director's Cut, but survival becomes even more demanding. Summary: Director's Cut + more enemies + player resource balancing.";case I_French:return "Utilisez Gauche/Droite pour changer le multiplicateur. Le comportement reprend la Version du réalisateur, mais la survie devient encore plus exigeante. Résumé : Version du réalisateur + plus d'ennemis + ressources du joueur rééquilibrées.";case I_German:return "Mit Links/Rechts den Multiplikator ändern. Das Verhalten basiert auf Director's Cut, doch das Überleben wird noch anspruchsvoller. Kurz: Director's Cut + mehr Gegner + angepasste Spielerressourcen.";case I_Italian:return "Usa Sinistra/Destra per cambiare il moltiplicatore. Il comportamento si basa sulla Versione del regista, ma sopravvivere diventa ancora più difficile. In breve: Versione del regista + più nemici + risorse del giocatore bilanciate.";case I_Spanish:return "Mueve las flechas izq/der para cambiar el multiplicador de dificultad. La base de comportamiento corresponde a Versión del director, pero la supervivencia se vuelve todavía más severa y desafiante. Resumen: Versión del director + más enemigos + balance de recursos del jugador.";case I_Swedish:return "Använd vänster/höger för att ändra multiplikatorn. Beteendet bygger på Director's Cut, men överlevnaden blir ännu mer krävande. Kort sagt: Director's Cut + fler fiender + balanserade spelarresurser.";}
case TEXTSTRING_COMPLETED_ACID_NEW:
 if(UsingSpanishTextStrings())return "Completado en Ácido molecular";
 switch(AvP.Language){default:return "Completed on Molecular acid";case I_French:return "Terminé en Acide moléculaire";case I_German:return "Auf Molekulare Säure abgeschlossen";case I_Italian:return "Completato ad Acido molecolare";case I_Spanish:return "Completado en Ácido molecular";case I_Swedish:return "Klarad på Molekylär syra";}
case TEXTSTRING_DIFFICULTY_IMPOSSIBLE_NEW:
 if(UsingSpanishTextStrings())return "Imposible";
 switch(AvP.Language){case I_Spanish:return "Imposible";default:return "Impossible";case I_French:return "Impossible";case I_German:return "Unmöglich";case I_Italian:return "Impossibile";case I_Swedish:return "Omöjlig";}
case TEXTSTRING_DIFFICULTY_IMPOSSIBLE_HELP_NEW:
 if(UsingSpanishTextStrings())return "Dificultad Imposible. Se desbloquea al completar esta etapa en Realista o superior.";
 switch(AvP.Language){case I_Spanish:return "Dificultad Imposible. Se desbloquea al completar esta etapa en Realista o superior.";default:return "Impossible difficulty. Unlocked by completing this episode on Realistic or higher.";case I_French:return "Difficulté Impossible. Débloquée en terminant cet épisode en Réaliste ou plus difficile.";case I_German:return "Schwierigkeit Unmöglich. Wird durch Abschluss dieser Episode auf Realistisch oder höher freigeschaltet.";case I_Italian:return "Difficoltà Impossibile. Si sblocca completando questo episodio a difficoltà Realistica o superiore.";case I_Swedish:return "Svårighetsgrad Omöjlig. Låses upp genom att klara denna bana på Realistisk eller högre.";}
case TEXTSTRING_COMPLETED_IMPOSSIBLE_NEW:
 if(UsingSpanishTextStrings())return "Completado en Imposible";
 switch(AvP.Language){case I_Spanish:return "Completado en Imposible";default:return "Completed on Impossible";case I_French:return "Terminé en Impossible";case I_German:return "Auf Unmöglich abgeschlossen";case I_Italian:return "Completato a Impossibile";case I_Swedish:return "Klarad på Omöjlig";}
case TEXTSTRING_OPTIMIZED_AMMO_NEW:
 if(UsingSpanishTextStrings())return "Munición optimizada";
 switch(AvP.Language){default:return "Optimized ammunition";case I_Spanish:return "Munición optimizada";case I_French:return "Munitions optimisées";case I_German:return "Optimierte Munition";case I_Italian:return "Munizioni ottimizzate";case I_Swedish:return "Optimerad ammunition";}
case TEXTSTRING_OPTIMIZED_AMMO_HELP_NEW:
 if(UsingSpanishTextStrings())return "No = comportamiento vanilla: cada recarga vacía el compartimiento del arma y descarta las balas residuales; sí = cada recarga rellena solo la munición que falta sin perder balas residuales. El último cargador conserva las balas sobrantes y, al usarlo, carga únicamente las que quedan.";
 switch(AvP.Language){case I_Spanish:return "No = comportamiento vanilla: cada recarga vacía el compartimiento del arma y descarta las balas residuales; sí = cada recarga rellena solo la munición que falta sin perder balas residuales. El último cargador conserva las balas sobrantes y, al usarlo, carga únicamente las que quedan.";default:return "No = vanilla behaviour: each reload empties the weapon and discards its remaining ammunition; Yes = each reload tops up only the missing ammunition without losing leftover rounds. Spare rounds remain in the last magazine; using it loads only the rounds it still contains.";case I_French:return "Non = comportement original : chaque rechargement vide le compartiment de l'arme et perd les munitions restantes ; Oui = chaque rechargement ajoute seulement les munitions manquantes, sans perte. Les munitions restantes sont conservées dans le dernier chargeur ; celui-ci ne recharge que ce qu'il contient encore.";case I_German:return "Nein = Originalverhalten: Beim Nachladen wird die Restmunition in der Waffe verworfen; Ja = beim Nachladen wird nur die fehlende Munition ergänzt, ohne restliche Patronen zu verlieren. Der Rest bleibt im letzten Magazin; beim Einsetzen werden nur die darin verbliebenen Patronen geladen.";case I_Italian:return "No = comportamento originale: ogni ricarica svuota l'arma e scarta le munizioni rimaste; Sì = ogni ricarica aggiunge soltanto le munizioni mancanti, senza perdere i colpi residui. I colpi avanzati restano nell'ultimo caricatore, che ricarica soltanto quelli ancora disponibili.";case I_Swedish:return "Nej = originalbeteende: varje omladdning tömmer vapnet och kastar bort kvarvarande ammunition; Ja = varje omladdning fyller endast på det som saknas, utan att förlora patroner. Överblivna patroner sparas i det sista magasinet; det laddar bara de patroner som finns kvar.";}
case TEXTSTRING_RETAIN_STATE_NEW:
 if(UsingSpanishTextStrings())return "Persistencia estado jugador";
 switch(AvP.Language){case I_French:return "Conserver l'état du joueur";case I_Spanish:return "Persistencia estado jugador";case I_German:return "Spielerzustand beibehalten";case I_Italian:return "Mantieni stato giocatore";case I_Swedish:return "Behåll spelarstatus";default:return "Retain Player State";}
case TEXTSTRING_RETAIN_STATE_HELP_NEW:
 if(UsingSpanishTextStrings())return "No = los episodios se recorren individualmente de forma separada, como en vanilla; encadenado = los episodios se recorren de manera continua, pero cada misión comienza con el estado normal definido por el juego, como en vanilla; sí = también aplica episodios encadenados, pero conserva el estado del jugador entre niveles: salud, armadura, armas/equipamiento. Cambia la sensación de campaña otorgando un recorrido único y continuo.";
 switch(AvP.Language){case I_Spanish:return "No = los episodios se recorren individualmente de forma separada, como en vanilla; encadenado = los episodios se recorren de manera continua, pero cada misión comienza con el estado normal definido por el juego, como en vanilla; sí = también aplica episodios encadenados, pero conserva el estado del jugador entre niveles: salud, armadura, armas/equipamiento. Cambia la sensación de campaña otorgando un recorrido único y continuo.";default:return "Off = episodes are played separately, as in vanilla; Chained = episodes continue automatically, but each mission starts with the normal state defined by the game, as in vanilla; On = also chains episodes and retains the player's health, armour, weapons and equipment between levels. Changes the campaign experience into a unique, continuous journey.";case I_French:return "Non = les épisodes se jouent séparément, comme dans le jeu original ; Enchaîné = les épisodes se suivent automatiquement, mais chaque mission commence avec l'état normal prévu par le jeu ; Oui = enchaîne aussi les épisodes et conserve la santé, l'armure, les armes et l'équipement entre les niveaux. Transforme la campagne en un parcours unique et continu.";case I_German:return "Aus = Episoden werden wie im Original einzeln gespielt; Verkettet = Episoden folgen automatisch aufeinander, jede Mission beginnt jedoch mit dem normalen, vom Spiel vorgegebenen Zustand; Ein = verkettet Episoden und übernimmt Gesundheit, Rüstung, Waffen und Ausrüstung zwischen den Levels. So wird die Kampagne zu einer einzigartigen, durchgehenden Reise.";case I_Italian:return "No = gli episodi si giocano separatamente, come nel gioco originale; Concatenato = gli episodi proseguono automaticamente, ma ogni missione inizia con lo stato normale previsto dal gioco; Sì = concatena gli episodi e conserva salute, armatura, armi ed equipaggiamento tra i livelli. Trasforma la campagna in un percorso unico e continuo.";case I_Swedish:return "Av = avsnitten spelas separat, som i originalet; Sammanlänkat = avsnitten följer automatiskt på varandra, men varje uppdrag börjar med spelets normala startvärden; På = länkar också avsnitten och behåller hälsa, rustning, vapen och utrustning mellan banorna. Gör kampanjen till en unik, sammanhängande resa.";}
case TEXTSTRING_RETAIN_CHAINED_NEW:
 if(UsingSpanishTextStrings())return "Encadenado";
 switch(AvP.Language){case I_Spanish:return "Encadenado";default:return "Chained";case I_French:return "Enchaîné";case I_German:return "Verkettet";case I_Italian:return "Concatenato";case I_Swedish:return "Sammanlänkat";}
case TEXTSTRING_RETAIN_OFF_NEW:
 if(UsingSpanishTextStrings())return "No";
 switch(AvP.Language){case I_French:return "Non";case I_Spanish:return "No";case I_German:return "Aus";case I_Italian:return "No";case I_Swedish:return "Av";default:return "Off";}
case TEXTSTRING_RETAIN_ON_NEW:
 if(UsingSpanishTextStrings())return "Sí";
 switch(AvP.Language){case I_French:return "Oui";case I_Spanish:return "Sí";case I_German:return "Ein";case I_Italian:return "Sì";case I_Swedish:return "På";default:return "On";}
case TEXTSTRING_FMV_FILTER_NEW:
 if(UsingSpanishTextStrings())return "filtro FMV's";
 switch(AvP.Language){case I_French:return "filtre FMV";case I_German:return "FMV-Filter";case I_Italian:return "filtro FMV";case I_Swedish:return "FMV-filter";default:return "FMV filter";}
case TEXTSTRING_FMV_FILTER_HELP_NEW:
 if(UsingSpanishTextStrings())return "nearest conserva los píxeles del vídeo; bilinear suaviza el escalado. Independiente del filtro de texturas.";
 switch(AvP.Language){case I_French:return "nearest conserve les pixels de la vidéo ; bilinear adoucit la mise à l'échelle. Indépendant du filtre des textures.";case I_German:return "nearest erhält Videopixel; bilinear glättet die Skalierung. Unabhängig vom Texturfilter.";case I_Italian:return "nearest conserva i pixel del video; bilinear ammorbidisce la scala. Indipendente dal filtro texture.";case I_Swedish:return "nearest bevarar videons pixlar; bilinear jämnar ut skalningen. Oberoende av texturfiltret.";default:return "nearest preserves video pixels; bilinear smooths scaling. Independent of the texture filter.";}
case TEXTSTRING_FMV_FILTER_NEAREST_NEW:return "nearest";
case TEXTSTRING_FMV_FILTER_BILINEAR_NEW:return "bilinear";
case TEXTSTRING_SCREENSHOT_FORMAT_NEW:
 if(UsingSpanishTextStrings()) return "formato captura pantalla";
 switch(AvP.Language) {case I_French:return "format capture d'écran";case I_German:return "Bildschirmfotoformat";case I_Italian:return "formato cattura schermo";case I_Swedish:return "skärmbildsformat";default:return "screenshot format";}
case TEXTSTRING_SCREENSHOT_FORMAT_HELP_NEW:
 if(UsingSpanishTextStrings()) return "tipo de formato para las capturas de pantalla";
 switch(AvP.Language) {case I_French:return "format de fichier des captures d'écran";case I_German:return "Dateiformat für Bildschirmfotos";case I_Italian:return "tipo di formato per le catture dello schermo";case I_Swedish:return "filformat för skärmbilder";default:return "file format for screenshots";}
case TEXTSTRING_SCREENSHOT_BMP_NEW: return "bmp";
case TEXTSTRING_SCREENSHOT_PNG_NEW: return "png";
case TEXTSTRING_SCREENSHOT_JPG_NEW: return "jpg";
case TEXTSTRING_MENU_BACKGROUND_TYPE_NEW:
 if(UsingSpanishTextStrings())return "fondo menú tipo";
 switch(AvP.Language){case I_French:return "type du fond de menu";case I_German:return "Menühintergrundtyp";case I_Italian:return "tipo sfondo menu";case I_Swedish:return "menybakgrundstyp";default:return "menu background type";}
case TEXTSTRING_MENU_BACKGROUND_TYPE_HELP_NEW:
 if(UsingSpanishTextStrings())return "Cambia el fondo del menú principal: starfield usa Starfield.RIM; animated reproduce Menubackground.bik en bucle. Opciones de audio/video conserva su propio fondo.";
 switch(AvP.Language){case I_French:return "Change le fond du menu principal : starfield utilise Starfield.RIM ; animated lit Menubackground.bik en boucle. Les options audio/vidéo conservent leur propre fond.";case I_German:return "Ändert den Hintergrund des Hauptmenüs: starfield verwendet Starfield.RIM; animated spielt Menubackground.bik in einer Schleife. Audio-/Videooptionen behalten ihren eigenen Hintergrund.";case I_Italian:return "Cambia lo sfondo del menu principale: starfield usa Starfield.RIM; animated riproduce Menubackground.bik in ciclo. Le opzioni audio/video mantengono il proprio sfondo.";case I_Swedish:return "Ändrar huvudmenyns bakgrund: starfield använder Starfield.RIM; animated spelar Menubackground.bik i en slinga. Ljud-/bildalternativ behåller sin egen bakgrund.";default:return "Changes the main menu backdrop: starfield uses Starfield.RIM; animated loops Menubackground.bik. Audio/video options keep their own backdrop.";}
case TEXTSTRING_MENU_BACKGROUND_STARFIELD_NEW:return "starfield";
case TEXTSTRING_MENU_BACKGROUND_ANIMATED_NEW:return "animated";
case TEXTSTRING_MENU_BACKGROUND_MUSIC_NEW:
 if(UsingSpanishTextStrings())return "fondo menú música";
 switch(AvP.Language){case I_French:return "musique de fond du menu";case I_German:return "Menühintergrundmusik";case I_Italian:return "musica sfondo menu";case I_Swedish:return "menybakgrundsmusik";default:return "menu background music";}
case TEXTSTRING_MENU_BACKGROUND_MUSIC_HELP_NEW:
 if(UsingSpanishTextStrings())return "Selecciona la versión de música para el fondo del menú; golden = avp classic/avp golden; 99 = avp 99";
 switch(AvP.Language){case I_French:return "Choisit la version musicale du fond de menu ; golden = avp classic/avp golden ; 99 = avp 99";case I_German:return "Wählt die Musikversion für den Menühintergrund; golden = avp classic/avp golden; 99 = avp 99";case I_Italian:return "Sceglie la versione musicale dello sfondo del menu; golden = avp classic/avp golden; 99 = avp 99";case I_Swedish:return "Väljer musikversion för menybakgrunden; golden = avp classic/avp golden; 99 = avp 99";default:return "Selects the music version for the menu backdrop; golden = avp classic/avp golden; 99 = avp 99";}
case TEXTSTRING_MENU_BACKGROUND_GOLDEN_NEW:return "golden";
case TEXTSTRING_MENU_BACKGROUND_99_NEW:return "99";
case TEXTSTRING_MENU_BG_FULL_V231:return GetTextString(TEXTSTRING_BACKGROUND_FULLSCREEN_NEW);
case TEXTSTRING_MENU_BG_LETTER_V231:return "letterboxed";
case TEXTSTRING_MENU_BG_CENTER_V231:
 if(UsingSpanishTextStrings())return "centrado";
 switch(AvP.Language){case I_French:return "centré";case I_German:return "zentriert";case I_Italian:return "centrato";case I_Swedish:return "centrerad";default:return "centered";}
case TEXTSTRING_BACKGROUND_MODE_NEW:
 if(UsingSpanishTextStrings())return "fondo menú modo";
 switch(AvP.Language){case I_French:return "mode du fond de menu";case I_German:return "Menühintergrundmodus";case I_Italian:return "modalità sfondo menu";case I_Swedish:return "menybakgrundsläge";default:return "menu background mode";}
case TEXTSTRING_BACKGROUND_HELP_NEW:
 if(UsingSpanishTextStrings())return "Pantalla completa: cubre la pantalla. Letterboxed: estira horizontalmente hasta 16:9, con bandas negras. Centrado: conserva el aspecto original sin estirar. No cambia la escala de textos ni controles.";
 switch(AvP.Language){case I_French:return "Plein écran : remplit l'écran. Letterboxed : étire horizontalement jusqu'à 16:9, avec bandes noires. Centré : conserve les proportions originales. Ne change pas la taille du texte ni des commandes.";case I_German:return "Vollbild: füllt den Bildschirm. Letterboxed: streckt horizontal bis 16:9 mit schwarzen Balken. Zentriert: erhält das Originalformat. Text und Bedienelemente behalten ihre Größe.";case I_Italian:return "Schermo intero: riempie lo schermo. Letterboxed: estende orizzontalmente fino a 16:9, con bande nere. Centrato: mantiene le proporzioni originali. Non cambia la scala di testo e controlli.";case I_Swedish:return "Helskärm: fyller skärmen. Letterboxed: sträcker horisontellt upp till 16:9 med svarta kanter. Centrerad: behåller originalformatet. Text och reglage behåller sin storlek.";default:return "Fullscreen: fills the screen. Letterboxed: stretches horizontally up to 16:9, with black bars. Centered: preserves the original aspect ratio. Text and controls keep their scale.";}
case TEXTSTRING_BACKGROUND_LETTERBOXED_NEW:
 if(UsingSpanishTextStrings())return "letterboxed";
 switch(AvP.Language){case I_French:return "letterboxed";case I_German:return "Letterboxed";case I_Italian:return "letterboxed";case I_Swedish:return "letterboxed";default:return "letterboxed";}
case TEXTSTRING_BACKGROUND_FULLSCREEN_NEW:
 if(UsingSpanishTextStrings())return "pantalla completa";
 switch(AvP.Language){case I_French:return "plein écran";case I_German:return "Vollbild";case I_Italian:return "schermo intero";case I_Swedish:return "helskärm";default:return "fullscreen";}
case TEXTSTRING_WATER_EFFECT_NEW:
 if(UsingSpanishTextStrings()) return "efecto agua";
 switch(AvP.Language){case I_French:return "effet de l'eau";case I_German:return "Wassereffekt";case I_Italian:return "effetto dell'acqua";case I_Swedish:return "vatteneffekt";default:return "Water effect";}
case TEXTSTRING_WATER_EFFECT_HELP_NEW:
 if(UsingSpanishTextStrings()) return "Vanilla: agua original con ondas corregidas. Modern: ondas suaves, reflejos, refracción y cáusticas; conserva el material original.";
 switch(AvP.Language){case I_French:return "Vanilla : eau originale avec ondes corrigées. Modern : vagues douces, reflets, réfraction et caustiques ; conserve le matériau original.";case I_German:return "Vanilla: Originalwasser mit korrigierten Wellen. Modern: sanfte Wellen, Reflexion, Brechung und Kaustik; behält das Originalmaterial.";case I_Italian:return "Vanilla: acqua originale con onde corrette. Modern: onde fluide, riflessi, rifrazione e caustiche; mantiene il materiale originale.";case I_Swedish:return "Vanilla: originalvatten med korrigerade vågor. Modern: mjuka vågor, reflektioner, brytning och kaustik; behåller originalmaterialet.";default:return "Vanilla: original water with repaired ripples. Modern: smooth waves, reflections, refraction and caustics; keeps the original material.";}
case TEXTSTRING_WATER_VANILLA_NEW:return "vanilla";
case TEXTSTRING_WATER_MODERN_NEW:return "modern";
case TEXTSTRING_CRT_FILTER_NEW:
			{
				if (UsingSpanishTextStrings()) return "filtro CRT";
				switch (AvP.Language) { case I_French: return "filtre CRT"; case I_German: return "CRT-Filter"; case I_Italian: return "filtro CRT"; case I_Swedish: return "CRT-filter"; default: return "CRT filter"; }
			}
			case TEXTSTRING_CRT_FILTER_HELP_NEW: {
    static const char *const t[6]={"applies a nice GLSL CRT filter based on Mattias Gustavsson/HelelSingh shader. It is recommended to use it only at your monitor's native resolution; with virtual/scaled/downscaled resolutions, the CRT filter's mask grid/dot pitch does not align 1:1, resulting in poor image quality.","Applique un beau filtre CRT GLSL basÃ© sur le shader de Mattias Gustavsson/HelelSingh. Utilise de prÃ©fÃ©rence la rÃ©solution native de ton Ã©cran : avec une rÃ©solution virtuelle, redimensionnÃ©e ou rÃ©duite, la grille et le pas du masque CRT ne correspondent plus aux pixels Ã  1:1, ce qui dÃ©grade l'image.","Aplica un bonito filtro CRT GLSL basado en el shader de Mattias Gustavsson/HelelSingh. Se recomienda usarlo solo con la resoluciÃ³n nativa de tu monitor; con resoluciones virtuales/escaladas/reducidas, la mÃ¡scara grilla/dot pitch del filtro CRT no calza 1:1, dando como resultado una mala calidad de imagen.","Wendet einen schÃ¶nen GLSL-CRT-Filter auf Basis des Shaders von Mattias Gustavsson/HelelSingh an. Empfohlen wird die native MonitorauflÃ¶sung. Bei virtuellen, skalierten oder reduzierten AuflÃ¶sungen stimmt das Maskenraster des CRT-Filters nicht 1:1 mit den Pixeln Ã¼berein, was die BildqualitÃ¤t verschlechtert.","Applica un bel filtro CRT GLSL basato sullo shader di Mattias Gustavsson/HelelSingh. Si consiglia la risoluzione nativa del monitor: con risoluzioni virtuali, scalate o ridotte, la griglia e il passo della maschera CRT non coincidono 1:1 con i pixel, peggiorando la qualitÃ  dell'immagine.","TillÃ¤mpar ett fint GLSL-CRT-filter baserat pÃ¥ Mattias Gustavssons/HelelSinghs shader. AnvÃ¤nd helst skÃ¤rmens inbyggda upplÃ¶sning. Med virtuella, skalade eller nedskalade upplÃ¶sningar passar CRT-maskens rutnÃ¤t och punktavstÃ¥nd inte 1:1 mot pixlarna, vilket fÃ¶rsÃ¤mrar bildkvaliteten."};
    int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;
    return (char*)t[l>=0&&l<6?l:0];
 }

			case TEXTSTRING_CRT_FILTER_OFF_NEW: return "off";
			case TEXTSTRING_CRT_FILTER_ON_NEW: return "on";
			case TEXTSTRING_FPS_DISPLAY_OFF_NEW:
			{
				if (UsingSpanishTextStrings()) return "no";
				switch (AvP.Language) { case I_French: return "non"; case I_German: return "nein"; case I_Italian: return "no"; case I_Swedish: return "nej"; default: return "no"; }
			}
			case TEXTSTRING_FPS_DISPLAY_TOP_RIGHT_NEW:
			{
				if (UsingSpanishTextStrings()) return "superior derecha";
				switch (AvP.Language) { case I_French: return "en haut a droite"; case I_German: return "oben rechts"; case I_Italian: return "in alto a destra"; case I_Swedish: return "uppe till hoger"; default: return "top right"; }
			}
			case TEXTSTRING_FPS_DISPLAY_TOP_LEFT_NEW:
			{
				if (UsingSpanishTextStrings()) return "superior izquierda";
				switch (AvP.Language) { case I_French: return "en haut a gauche"; case I_German: return "oben links"; case I_Italian: return "in alto a sinistra"; case I_Swedish: return "uppe till vanster"; default: return "top left"; }
			}
			case TEXTSTRING_FPS_DISPLAY_BOTTOM_RIGHT_NEW:
			{
				if (UsingSpanishTextStrings()) return "inferior derecha";
				switch (AvP.Language) { case I_French: return "en bas a droite"; case I_German: return "unten rechts"; case I_Italian: return "in basso a destra"; case I_Swedish: return "nere till hoger"; default: return "bottom right"; }
			}
			case TEXTSTRING_FPS_DISPLAY_BOTTOM_LEFT_NEW:
			{
				if (UsingSpanishTextStrings()) return "inferior izquierda";
				switch (AvP.Language) { case I_French: return "en bas a gauche"; case I_German: return "unten links"; case I_Italian: return "in basso a sinistra"; case I_Swedish: return "nere till vanster"; default: return "bottom left"; }
			}			
			case TEXTSTRING_NOTIFICATION_MESSAGE_DURATION_NEW:
			{
				if (UsingSpanishTextStrings()) return "duraciÃ³n notificaciones/mensajes";
				switch (AvP.Language) { case I_French: return "durÃ©e des notifications/messages"; case I_German: return "Benachrichtigungs-/Nachrichtendauer"; case I_Italian: return "durata notifiche/messaggi"; case I_Swedish: return "varaktighet fÃ¶r notiser/meddelanden"; default: return "notification/message duration"; }
			}
			case TEXTSTRING_NOTIFICATION_MESSAGE_DURATION_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "establece el tiempo de duraciÃ³n de las notificaciones/mensajes";
				switch (AvP.Language) { case I_French: return "dÃ©finit la durÃ©e d'affichage des notifications/messages"; case I_German: return "legt die Anzeigedauer von Benachrichtigungen/Nachrichten fest"; case I_Italian: return "imposta la durata delle notifiche/dei messaggi"; case I_Swedish: return "anger visningstiden fÃ¶r notiser/meddelanden"; default: return "sets how long notifications/messages remain on screen"; }
			}
			case TEXTSTRING_NOTIFICATION_MESSAGE_DURATION_6S_NEW: return "6s";
			case TEXTSTRING_NOTIFICATION_MESSAGE_DURATION_8S_NEW: return "8s";
			case TEXTSTRING_NOTIFICATION_MESSAGE_DURATION_10S_NEW: return "10s";
			case TEXTSTRING_NOTIFICATION_MESSAGE_DURATION_12S_NEW: return "12s";
			case TEXTSTRING_FMV_SCALE_NEW:
			{
				if (UsingSpanishTextStrings()) return "escalar FMV's";
				switch (AvP.Language) { case I_French: return "Echelle FMV"; case I_Spanish: return "escalar FMV's"; case I_German: return "FMV-Skalierung"; case I_Italian: return "Scala FMV"; case I_Swedish: return "FMV-skala"; default: return "FMV scale"; }
			}
			case TEXTSTRING_FMV_SCALE_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "cambia la representaciÃ³n de reproducciÃ³n de los videos. panorÃ¡mico x1, x1.5, x2, x2.5, x3, x3.5, x4 = mantienen el aspecto original 16:9 con diferentes escalados, ultra panorÃ¡mico x1, x1.5, x2, x2.5, x3, x3.5, x4 = aplican un aspecto 21:9 con diferentes escalados, pantalla completa panorÃ¡mica = estira conservando 16:9, pantalla completa ultra panorÃ¡mica = estira aplicando 21:9";
				switch (AvP.Language) { case I_French: return "Change l affichage des videos. panoramique x1 a x4 garde 16:9 a plusieurs echelles; ultra panoramique x1 a x4 applique 21:9; plein ecran panoramique garde 16:9; plein ecran ultra panoramique applique 21:9."; case I_Spanish: return "cambia la representaciÃ³n de reproducciÃ³n de los videos. panorÃ¡mico x1, x1.5, x2, x2.5, x3, x3.5, x4 = mantienen el aspecto original 16:9 con diferentes escalados, ultra panorÃ¡mico x1, x1.5, x2, x2.5, x3, x3.5, x4 = aplican un aspecto 21:9 con diferentes escalados, pantalla completa panorÃ¡mica = estira conservando 16:9, pantalla completa ultra panorÃ¡mica = estira aplicando 21:9"; case I_German: return "Andert die Videodarstellung. Breitbild x1 bis x4 behalt 16:9 in verschiedenen Grossen; Ultrabreit x1 bis x4 nutzt 21:9; Breitbild-Vollbild behalt 16:9; Ultrabreit-Vollbild nutzt 21:9."; case I_Italian: return "Cambia la riproduzione dei video. panoramico x1 fino a x4 mantiene 16:9 a scale diverse; ultra panoramico x1 fino a x4 applica 21:9; schermo intero panoramico conserva 16:9; ultra panoramico applica 21:9."; case I_Swedish: return "Andrar videovisningen. bredbild x1 till x4 behaller 16:9 i olika skalor; ultrabred x1 till x4 anvander 21:9; bredbild helskarm behaller 16:9; ultrabred helskarm anvander 21:9."; default: return "Changes video playback presentation. widescreen x1, x2, x3, x4 retain 16:9 at different scales; ultrawide x1, x2, x3, x4 apply 21:9 at different scales; widescreen full screen retains 16:9; ultrawide full screen applies 21:9."; }
			}
			case TEXTSTRING_FMV_SCALE_ORIGINAL_NEW:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x1";
				switch (AvP.Language) { case I_French: return "panoramique x1"; case I_Spanish: return "panorÃ¡mico x1"; case I_German: return "Breitbild x1"; case I_Italian: return "panoramico x1"; case I_Swedish: return "bredbild x1"; default: return "widescreen x1"; }
			}
			case TEXTSTRING_FMV_SCALE_PRESERVE_NEW:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x2";
				switch (AvP.Language) { case I_French: return "panoramique x2"; case I_Spanish: return "panorÃ¡mico x2"; case I_German: return "Breitbild x2"; case I_Italian: return "panoramico x2"; case I_Swedish: return "bredbild x2"; default: return "widescreen x2"; }
			}
			case TEXTSTRING_FMV_SCALE_COVER_NEW:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x3";
				switch (AvP.Language) { case I_French: return "panoramique x3"; case I_Spanish: return "panorÃ¡mico x3"; case I_German: return "Breitbild x3"; case I_Italian: return "panoramico x3"; case I_Swedish: return "bredbild x3"; default: return "widescreen x3"; }
			}
			case TEXTSTRING_FMV_SCALE_WIDESCREEN_NEW:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x4";
				switch (AvP.Language) { case I_French: return "panoramique x4"; case I_Spanish: return "panorÃ¡mico x4"; case I_German: return "Breitbild x4"; case I_Italian: return "panoramico x4"; case I_Swedish: return "bredbild x4"; default: return "widescreen x4"; }
			}
			case TEXTSTRING_FMV_SCALE_ULTRAWIDE_NEW:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x1";
				switch (AvP.Language) { case I_French: return "ultra panoramique x1"; case I_Spanish: return "ultra panorÃ¡mico x1"; case I_German: return "Ultrabreit x1"; case I_Italian: return "ultra panoramico x1"; case I_Swedish: return "ultrabred x1"; default: return "ultrawide x1"; }
			}
			case TEXTSTRING_FMV_SCALE_ULTRAWIDE_X2_NEW:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x2";
				switch (AvP.Language) { case I_French: return "ultra panoramique x2"; case I_Spanish: return "ultra panorÃ¡mico x2"; case I_German: return "Ultrabreit x2"; case I_Italian: return "ultra panoramico x2"; case I_Swedish: return "ultrabred x2"; default: return "ultrawide x2"; }
			}
			case TEXTSTRING_FMV_SCALE_ULTRAWIDE_X3_NEW:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x3";
				switch (AvP.Language) { case I_French: return "ultra panoramique x3"; case I_Spanish: return "ultra panorÃ¡mico x3"; case I_German: return "Ultrabreit x3"; case I_Italian: return "ultra panoramico x3"; case I_Swedish: return "ultrabred x3"; default: return "ultrawide x3"; }
			}
			case TEXTSTRING_FMV_SCALE_ULTRAWIDE_X4_NEW:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x4";
				switch (AvP.Language) { case I_French: return "ultra panoramique x4"; case I_Spanish: return "ultra panorÃ¡mico x4"; case I_German: return "Ultrabreit x4"; case I_Italian: return "ultra panoramico x4"; case I_Swedish: return "ultrabred x4"; default: return "ultrawide x4"; }
			}
			case TEXTSTRING_FMV_SCALE_FULLSCREEN_WIDESCREEN_NEW:
			{
				if (UsingSpanishTextStrings()) return "pantalla completa panorÃ¡mica";
				switch (AvP.Language) { case I_French: return "plein ecran panoramique"; case I_Spanish: return "pantalla completa panorÃ¡mica"; case I_German: return "Breitbild Vollbild"; case I_Italian: return "schermo intero panoramico"; case I_Swedish: return "bredbild helskarm"; default: return "widescreen full screen"; }
			}
			case TEXTSTRING_FMV_SCALE_FULLSCREEN_ULTRAWIDE_NEW:
			{
				if (UsingSpanishTextStrings()) return "pantalla completa ultra panorÃ¡mica";
				switch (AvP.Language) { case I_French: return "plein ecran ultra panoramique"; case I_Spanish: return "pantalla completa ultra panorÃ¡mica"; case I_German: return "Ultrabreit Vollbild"; case I_Italian: return "schermo intero ultra panoramico"; case I_Swedish: return "ultrabred helskarm"; default: return "ultrawide full screen"; }
			}			case TEXTSTRING_FMV_SCALE_P072_PANORAMIC_X1:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x1";
				switch (AvP.Language) { case I_French: return "panoramique x1"; case I_Spanish: return "panorÃ¡mico x1"; case I_German: return "Breitbild x1"; case I_Italian: return "panoramico x1"; case I_Swedish: return "bredbild x1"; default: return "widescreen x1"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_PANORAMIC_X15:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x1.5";
				switch (AvP.Language) { case I_French: return "panoramique x1.5"; case I_Spanish: return "panorÃ¡mico x1.5"; case I_German: return "Breitbild x1.5"; case I_Italian: return "panoramico x1.5"; case I_Swedish: return "bredbild x1.5"; default: return "widescreen x1.5"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_PANORAMIC_X2:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x2";
				switch (AvP.Language) { case I_French: return "panoramique x2"; case I_Spanish: return "panorÃ¡mico x2"; case I_German: return "Breitbild x2"; case I_Italian: return "panoramico x2"; case I_Swedish: return "bredbild x2"; default: return "widescreen x2"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_PANORAMIC_X25:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x2.5";
				switch (AvP.Language) { case I_French: return "panoramique x2.5"; case I_Spanish: return "panorÃ¡mico x2.5"; case I_German: return "Breitbild x2.5"; case I_Italian: return "panoramico x2.5"; case I_Swedish: return "bredbild x2.5"; default: return "widescreen x2.5"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_PANORAMIC_X3:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x3";
				switch (AvP.Language) { case I_French: return "panoramique x3"; case I_Spanish: return "panorÃ¡mico x3"; case I_German: return "Breitbild x3"; case I_Italian: return "panoramico x3"; case I_Swedish: return "bredbild x3"; default: return "widescreen x3"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_PANORAMIC_X35:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x3.5";
				switch (AvP.Language) { case I_French: return "panoramique x3.5"; case I_Spanish: return "panorÃ¡mico x3.5"; case I_German: return "Breitbild x3.5"; case I_Italian: return "panoramico x3.5"; case I_Swedish: return "bredbild x3.5"; default: return "widescreen x3.5"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_PANORAMIC_X4:
			{
				if (UsingSpanishTextStrings()) return "panorÃ¡mico x4";
				switch (AvP.Language) { case I_French: return "panoramique x4"; case I_Spanish: return "panorÃ¡mico x4"; case I_German: return "Breitbild x4"; case I_Italian: return "panoramico x4"; case I_Swedish: return "bredbild x4"; default: return "widescreen x4"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_ULTRAWIDE_X1:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x1";
				switch (AvP.Language) { case I_French: return "ultra panoramique x1"; case I_Spanish: return "ultra panorÃ¡mico x1"; case I_German: return "Ultrabreit x1"; case I_Italian: return "ultra panoramico x1"; case I_Swedish: return "ultrabred x1"; default: return "ultrawide x1"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_ULTRAWIDE_X15:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x1.5";
				switch (AvP.Language) { case I_French: return "ultra panoramique x1.5"; case I_Spanish: return "ultra panorÃ¡mico x1.5"; case I_German: return "Ultrabreit x1.5"; case I_Italian: return "ultra panoramico x1.5"; case I_Swedish: return "ultrabred x1.5"; default: return "ultrawide x1.5"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_ULTRAWIDE_X2:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x2";
				switch (AvP.Language) { case I_French: return "ultra panoramique x2"; case I_Spanish: return "ultra panorÃ¡mico x2"; case I_German: return "Ultrabreit x2"; case I_Italian: return "ultra panoramico x2"; case I_Swedish: return "ultrabred x2"; default: return "ultrawide x2"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_ULTRAWIDE_X25:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x2.5";
				switch (AvP.Language) { case I_French: return "ultra panoramique x2.5"; case I_Spanish: return "ultra panorÃ¡mico x2.5"; case I_German: return "Ultrabreit x2.5"; case I_Italian: return "ultra panoramico x2.5"; case I_Swedish: return "ultrabred x2.5"; default: return "ultrawide x2.5"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_ULTRAWIDE_X3:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x3";
				switch (AvP.Language) { case I_French: return "ultra panoramique x3"; case I_Spanish: return "ultra panorÃ¡mico x3"; case I_German: return "Ultrabreit x3"; case I_Italian: return "ultra panoramico x3"; case I_Swedish: return "ultrabred x3"; default: return "ultrawide x3"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_ULTRAWIDE_X35:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x3.5";
				switch (AvP.Language) { case I_French: return "ultra panoramique x3.5"; case I_Spanish: return "ultra panorÃ¡mico x3.5"; case I_German: return "Ultrabreit x3.5"; case I_Italian: return "ultra panoramico x3.5"; case I_Swedish: return "ultrabred x3.5"; default: return "ultrawide x3.5"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_ULTRAWIDE_X4:
			{
				if (UsingSpanishTextStrings()) return "ultra panorÃ¡mico x4";
				switch (AvP.Language) { case I_French: return "ultra panoramique x4"; case I_Spanish: return "ultra panorÃ¡mico x4"; case I_German: return "Ultrabreit x4"; case I_Italian: return "ultra panoramico x4"; case I_Swedish: return "ultrabred x4"; default: return "ultrawide x4"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_FULLSCREEN_WIDESCREEN:
			{
				if (UsingSpanishTextStrings()) return "pantalla completa panorÃ¡mica";
				switch (AvP.Language) { case I_French: return "plein ecran panoramique"; case I_Spanish: return "pantalla completa panorÃ¡mica"; case I_German: return "Breitbild Vollbild"; case I_Italian: return "schermo intero panoramico"; case I_Swedish: return "bredbild helskarm"; default: return "widescreen full screen"; }
			}
			case TEXTSTRING_FMV_SCALE_P072_FULLSCREEN_ULTRAWIDE:
			{
				if (UsingSpanishTextStrings()) return "pantalla completa ultra panorÃ¡mica";
				switch (AvP.Language) { case I_French: return "plein ecran ultra panoramique"; case I_Spanish: return "pantalla completa ultra panorÃ¡mica"; case I_German: return "Ultrabreit Vollbild"; case I_Italian: return "schermo intero ultra panoramico"; case I_Swedish: return "ultrabred helskarm"; default: return "ultrawide full screen"; }
			}
			case TEXTSTRING_WEAPON_FOV_NEW:
			{
				if (UsingSpanishTextStrings()) return "FOV Arma";
				switch (AvP.Language) { case I_French: return "FOV arme"; case I_Spanish: return "FOV Arma"; case I_German: return "Waffen-FOV"; case I_Italian: return "FOV arma"; case I_Swedish: return "Vapen-FOV"; default: return "Weapon FOV"; }
			}
			case TEXTSTRING_WEAPON_FOV_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "modifica el campo visual del modelo del arma. presionar ENTER resetea su valor por defecto";
				switch (AvP.Language) { case I_French: return "Modifie le champ de vision du modele d arme. ENTREE restaure la valeur par defaut."; case I_Spanish: return "modifica el campo visual del modelo del arma. presionar ENTER resetea su valor por defecto"; case I_German: return "Andert das Sichtfeld des Waffenmodells. ENTER setzt den Standardwert zuruck."; case I_Italian: return "Modifica il campo visivo del modello dell arma. INVIO ripristina il valore predefinito."; case I_Swedish: return "Andrar vapenmodellens synfalt. ENTER aterstaller standardvardet."; default: return "Changes the weapon model field of view. Press ENTER to reset its default value."; }
			}
			case TEXTSTRING_WEAPON_FOV_STYLE_NEW:
			{
				if (UsingSpanishTextStrings()) return "Weapon FOV estilo";
				switch (AvP.Language) { case I_French: return "Style FOV arme"; case I_Spanish: return "Weapon FOV estilo"; case I_German: return "Waffen-FOV-Stil"; case I_Italian: return "Stile FOV arma"; case I_Swedish: return "Vapen-FOV-stil"; default: return "Weapon FOV style"; }
			}
			case TEXTSTRING_WEAPON_FOV_STYLE_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "exacto = conserva la representación geométrica sin deformaciones dando un resultado 1:1, el modelo tiende a verse menos deforme pero más achatado; estirado = conserva la proyección del FOV global, aunque el modelo sigue teniendo su propio viewport separado, el modelo tiende a verse más deforme pero más aerodinámico.";
				switch (AvP.Language) {
					case I_French: return "exact = préserve la représentation géométrique sans déformation pour un résultat 1:1; le modèle paraît moins déformé mais plus aplati; étiré = conserve la projection du FOV global, bien que le modèle garde son propre viewport séparé; le modèle paraît plus déformé mais plus aérodynamique.";
					case I_Spanish: return "exacto = conserva la representación geométrica sin deformaciones dando un resultado 1:1, el modelo tiende a verse menos deforme pero más achatado; estirado = conserva la proyección del FOV global, aunque el modelo sigue teniendo su propio viewport separado, el modelo tiende a verse más deforme pero más aerodinámico.";
					case I_German: return "exakt = bewahrt die geometrische Darstellung ohne Verzerrungen mit einem Ergebnis von 1:1; das Modell wirkt weniger verzerrt, aber flacher; gestreckt = bewahrt die Projektion des globalen FOV, wobei das Modell weiterhin einen eigenen separaten Viewport hat; es wirkt stärker verzerrt, aber aerodynamischer.";
					case I_Italian: return "esatto = conserva la rappresentazione geometrica senza deformazioni con un risultato 1:1; il modello appare meno deformato ma più appiattito; stirato = conserva la proiezione del FOV globale, anche se il modello mantiene il proprio viewport separato; il modello appare più deformato ma più aerodinamico.";
					case I_Swedish: return "exakt = bevarar den geometriska återgivningen utan förvrängning med ett resultat på 1:1; modellen ser mindre förvrängd men plattare ut; utsträckt = bevarar den globala FOV-projektionen, även om modellen behåller en egen separat viewport; modellen ser mer förvrängd men mer strömlinjeformad ut.";
					default: return "exact = preserves the geometric representation without distortion, giving a 1:1 result; the model tends to look less distorted but flatter; stretched = preserves the global FOV projection, although the model still has its own separate viewport; the model tends to look more distorted but more streamlined.";
				}
			}
			case TEXTSTRING_WEAPON_FOV_STYLE_EXACT_NEW:
			{
				if (UsingSpanishTextStrings()) return "exacto";
				switch (AvP.Language) { case I_French: return "exact"; case I_Spanish: return "exacto"; case I_German: return "exakt"; case I_Italian: return "esatto"; case I_Swedish: return "exakt"; default: return "exact"; }
			}
			case TEXTSTRING_WEAPON_FOV_STYLE_STRETCHED_NEW:
			{
				if (UsingSpanishTextStrings()) return "estirado";
				switch (AvP.Language) { case I_French: return "etire"; case I_Spanish: return "estirado"; case I_German: return "gestreckt"; case I_Italian: return "stirato"; case I_Swedish: return "utstrackt"; default: return "streched"; }
			}
			case TEXTSTRING_FOV_NEW: return "FOV";
			case TEXTSTRING_FOV_HELP_NEW:
			{
				if (UsingSpanishTextStrings()) return "modifica el campo visual. presionar ENTER resetea el valor por defecto de la especie actual";
				switch (AvP.Language)
				{
					case I_French: return "Modifie le champ de vision. ENTREE restaure la valeur par defaut de l espece actuelle.";
					case I_Spanish: return "modifica el campo visual. presionar ENTER resetea el valor por defecto de la especie actual";
					case I_German: return "Andert das Sichtfeld. ENTER setzt den Standardwert der aktuellen Spezies zuruck.";
					case I_Italian: return "Modifica il campo visivo. INVIO ripristina il valore predefinito della specie attuale.";
					case I_Swedish: return "Andrar synfaltet. ENTER aterstaller den aktuella artens standardvarde.";
					default: return "Changes the field of view. Press ENTER to reset the current species default.";
				}
			}
			case TEXTSTRING_GLOBAL_SCREENSHOT_NEW:
				if (UsingSpanishTextStrings()) return "Captura de pantalla";
				switch (AvP.Language) { case I_French: return "Capture d ecran"; case I_German: return "Bildschirmfoto"; case I_Italian: return "Schermata"; case I_Swedish: return "Skarmbild"; default: return "Screenshot"; }
            case TEXTSTRING_PREDATOR_PREVIOUS_VISION_NEW: {
    static const char *const t[6]={"Previous vision mode","Mode de vision prÃ©cÃ©dent","Modo de visiÃ³n anterior","Vorheriger Sichtmodus","ModalitÃ  visiva precedente","FÃ¶regÃ¥ende synlÃ¤ge"};
    int l=UsingSpanishTextStrings()?I_Spanish:AvP.Language;
    return (char*)t[l>=0&&l<6?l:0];
 }

            case TEXTSTRING_PREDATOR_PREVIOUS_VISION_HELP_NEW:
                if(UsingSpanishTextStrings())return "Retrocede un modo en el ciclo de visiones del depredador con cada pulsaciÃ³n. Pulsa ENTER para asignar una tecla y RETROCESO para borrar el bind.";
                switch(AvP.Language){
                    case I_French:return "Revient au mode de vision prÃ©cÃ©dent Ã  chaque pression. ENTRÃ‰E attribue une touche ; RETOUR ARRIÃˆRE efface l'affectation.";
                    case I_German:return "Wechselt pro Tastendruck zum vorherigen Sichtmodus. EINGABE weist eine Taste zu, RÃœCKTASTE lÃ¶scht die Belegung.";
                    case I_Italian:return "Torna alla modalitÃ  visiva precedente a ogni pressione. INVIO assegna un tasto, BACKSPACE cancella l'assegnazione.";
                    case I_Swedish:return "VÃ¤xlar till fÃ¶regÃ¥ende synlÃ¤ge med varje tryck. RETUR tilldelar en tangent och BACKSTEG tar bort bindningen.";
                    default:return "Steps back one Predator vision mode per press. Press ENTER to bind a key and BACKSPACE to clear it.";
                }
			case TEXTSTRING_GLOBAL_QUICKSAVE_NEW:
				if (UsingSpanishTextStrings()) return "Guardado rÃ¡pido";
				switch (AvP.Language) { case I_French: return "Sauvegarde rapide"; case I_German: return "Schnellspeichern"; case I_Italian: return "Salvataggio rapido"; case I_Swedish: return "Snabbspara"; default: return "Quick save"; }
			case TEXTSTRING_GLOBAL_QUICKLOAD_NEW:
				if (UsingSpanishTextStrings()) return "Carga rÃ¡pida";
				switch (AvP.Language) { case I_French: return "Chargement rapide"; case I_German: return "Schnellladen"; case I_Italian: return "Caricamento rapido"; case I_Swedish: return "Snabbladda"; default: return "Quick load"; }
			case TEXTSTRING_GLOBAL_SAVEGAME_NEW:
				if (UsingSpanishTextStrings()) return "Guardar partida";
				switch (AvP.Language) { case I_French: return "Sauvegarder la partie"; case I_German: return "Spiel speichern"; case I_Italian: return "Salva partita"; case I_Swedish: return "Spara spel"; default: return "Save game"; }
			case TEXTSTRING_GLOBAL_LOADGAME_NEW:
				if (UsingSpanishTextStrings()) return "Cargar partida";
				switch (AvP.Language) { case I_French: return "Charger la partie"; case I_German: return "Spiel laden"; case I_Italian: return "Carica partita"; case I_Swedish: return "Ladda spel"; default: return "Load game"; }
			case TEXTSTRING_SMART_JETPACK_MODE_ON_NEW:
				if (UsingSpanishTextStrings()) return "modo jetpack activado";
				switch (AvP.Language) { case I_French: return "mode jetpack active"; case I_German: return "Jetpack-Modus aktiviert"; case I_Italian: return "modalita jetpack attivata"; case I_Swedish: return "jetpacklage aktiverat"; default: return "jetpack mode activated"; }
			case TEXTSTRING_SMART_JETPACK_MODE_OFF_NEW:
				if (UsingSpanishTextStrings()) return "modo jetpack desactivado";
				switch (AvP.Language) { case I_French: return "mode jetpack desactive"; case I_German: return "Jetpack-Modus deaktiviert"; case I_Italian: return "modalita jetpack disattivata"; case I_Swedish: return "jetpacklage avaktiverat"; default: return "jetpack mode disabled"; }
			default: break;
		}
	}

	LOCALASSERT(stringID<MAX_NO_OF_TEXTSTRINGS);
	if (stringID < MAX_NO_OF_TEXTSTRINGS) {
		return TextStringPtr[stringID];
	}
	return EmptyString;
}




