static const char *const MotionNewHelp[5][6][3]={
{{"Duration of the jump/landing gesture, independent of response and physics.","Short, quick gesture.","Long, gradual gesture; does not change jump height."},{"Durée du geste de saut/réception, indépendante de la réponse et de la physique.","Geste court et rapide.","Geste long et progressif; hauteur de saut inchangée."},{"Duración del gesto de salto/aterrizaje, separada de la respuesta y de la física.","Gesto corto y rápido.","Gesto largo y gradual; no cambia la altura del salto."},{"Dauer der Sprung-/Landebewegung, getrennt von Reaktion und Physik.","Kurze, schnelle Bewegung.","Lange, sanfte Bewegung; Sprunghöhe bleibt gleich."},{"Durata del gesto di salto/atterraggio, separata da risposta e fisica.","Gesto breve e rapido.","Gesto lungo e graduale; altezza del salto invariata."},{"Hoppets/landningens rörelselängd, separat från respons och fysik.","Kort och snabb rörelse.","Lång och gradvis rörelse; hopphöjden ändras inte."}},
{{"Camera translation follows the original equip gesture and weapon inertia.","No added camera translation.","Strong camera movement, constrained by its travel limit."},{"La caméra suit le geste original et l'inertie de l'arme.","Aucun déplacement caméra ajouté.","Fort déplacement caméra, limité par sa course."},{"La cámara acompaña el gesto original y la inercia del arma al cambiarla.","Sin desplazamiento extra de cámara.","Movimiento fuerte de cámara, sujeto a su límite."},{"Die Kamera folgt der originalen Wechselbewegung und Waffenträgheit.","Keine zusätzliche Kameraverschiebung.","Starke Kamerabewegung, durch Weggrenze begrenzt."},{"La telecamera segue il gesto originale e l'inerzia dell'arma.","Nessuno spostamento extra della telecamera.","Forte movimento, vincolato al limite di spostamento."},{"Kameran följer originalrörelsen och vapnets tröghet vid byte.","Ingen extra kameraförflyttning.","Kraftig kamerarörelse, begränsad av rörelsegränsen."}},
{{"Smooths camera movement and tilt during weapon changes.","Slow, soft response with more lag.","Fast, firm response with less lag."},{"Lisse déplacement et inclinaison caméra au changement d'arme.","Réaction lente et douce, plus de retard.","Réaction rapide et ferme, moins de retard."},{"Suaviza el desplazamiento y la inclinación de cámara al cambiar armas.","Respuesta lenta y suave, con más retraso.","Respuesta rápida y firme, con menos retraso."},{"Glättet Kamerabewegung und Neigung beim Waffenwechsel.","Langsame, weiche Reaktion mit mehr Verzögerung.","Schnelle, feste Reaktion mit weniger Verzögerung."},{"Smussa movimento e inclinazione della telecamera cambiando arma.","Risposta lenta e morbida, con più ritardo.","Risposta rapida e decisa, con meno ritardo."},{"Jämnar ut kamerans rörelse och lutning vid vapenbyte.","Långsam och mjuk respons med mer fördröjning.","Snabb och fast respons med mindre fördröjning."}},
{{"Maximum extra camera travel per axis during weapon changes.","No extra camera translation.","Allows up to 60 mm per axis; tilt has separate controls."},{"Course caméra supplémentaire maximale par axe au changement d'arme.","Aucun déplacement caméra ajouté.","Jusqu'à 60 mm par axe; inclinaison réglée séparément."},{"Recorrido extra máximo de cámara por eje al cambiar armas.","Sin desplazamiento extra de cámara.","Permite hasta 60 mm por eje; la inclinación se ajusta aparte."},{"Maximaler zusätzlicher Kameraweg pro Achse beim Waffenwechsel.","Keine zusätzliche Kameraverschiebung.","Bis zu 60 mm pro Achse; Neigung separat einstellbar."},{"Spostamento extra massimo per asse durante il cambio arma.","Nessuno spostamento extra della telecamera.","Fino a 60 mm per asse; inclinazione regolata a parte."},{"Maximal extra kameraförflyttning per axel vid vapenbyte.","Ingen extra kameraförflyttning.","Upp till 60 mm per axel; lutning justeras separat."}},
{{"Share of the weapon equip tilt applied to the camera on this axis.","No extra camera tilt on this axis.","Camera follows the full extra weapon tilt on this axis."},{"Part de l'inclinaison de changement d'arme transmise à la caméra sur cet axe.","Aucune inclinaison caméra sur cet axe.","La caméra suit toute l'inclinaison ajoutée sur cet axe."},{"Parte de la inclinación del cambio de arma que acompaña la cámara en este eje.","Sin inclinación extra de cámara en este eje.","La cámara sigue toda la inclinación extra del arma en este eje."},{"Anteil der Wechselneigung, den die Kamera auf dieser Achse übernimmt.","Keine zusätzliche Kameraneigung auf dieser Achse.","Kamera folgt der gesamten zusätzlichen Waffenneigung dieser Achse."},{"Parte dell'inclinazione del cambio arma seguita dalla telecamera su questo asse.","Nessuna inclinazione extra su questo asse.","Segue tutta l'inclinazione extra dell'arma su questo asse."},{"Andel av vapnets byteslutning som kameran följer på denna axel.","Ingen extra kameralutning på denna axel.","Kameran följer hela vapnets extra lutning på denna axel."}}
};
static const char *const MotionExtraHelp[6][6][3]={
{{"Smooths only the weapon reaction when jumping/landing; camera response is separate.","Slow, soft onset and return.","Fast onset and return, with less smoothing."},{"Lisse seulement la réaction de l'arme au saut/à la réception ; caméra séparée.","Départ et retour lents et doux.","Départ et retour rapides, moins lissés."},{"Suaviza solo la reacción del arma al saltar/aterrizar; la cámara se ajusta aparte.","Inicio y regreso lentos y suaves.","Inicio y regreso rápidos, con menos suavizado."},{"Glättet nur die Waffenreaktion bei Sprung/Landung; Kamera separat.","Langsamer, weicher Beginn und Rücklauf.","Schneller Beginn und Rücklauf, weniger Glättung."},{"Smussa solo la reazione dell'arma al salto/atterraggio; telecamera separata.","Avvio e ritorno lenti e morbidi.","Avvio e ritorno rapidi, meno smussati."},{"Jämnar bara ut vapnets reaktion vid hopp/landning; kameran separat.","Långsam, mjuk start och återgång.","Snabb start och återgång, mindre utjämning."}},
{{"Distance from the camera where Marine weapon geometry starts being visible. Does not change FOV or world clipping.","Keeps geometry almost against the camera.","Cuts away more geometry near the camera. Geometry behind the eye cannot be recovered."},{"Distance caméra à partir de laquelle l'arme Marine est visible. Ne change ni FOV ni découpe du monde.","Garde la géométrie presque contre la caméra.","Coupe davantage près de la caméra. Rien derrière l'oeil ne peut être récupéré."},{"Distancia desde la cámara donde empieza a verse la geometría del arma marine. No cambia FOV ni recorta el mundo.","Conserva geometría casi pegada a la cámara.","Recorta más geometría cercana. No recupera partes que ya cruzaron detrás del ojo."},{"Kameraabstand, ab dem Marine-Waffengeometrie sichtbar ist. Ändert weder FOV noch Weltschnitt.","Erhält Geometrie fast direkt an der Kamera.","Schneidet mehr nahe Geometrie ab. Teile hinter dem Auge bleiben unsichtbar."},{"Distanza dalla telecamera dove l'arma Marine diventa visibile. Non cambia FOV o taglio del mondo.","Mantiene geometria quasi contro la telecamera.","Taglia più geometria vicina. Le parti dietro l'occhio restano invisibili."},{"Kameraavstånd där Marine-vapengeometri blir synlig. Ändrar inte FOV eller världsklippning.","Behåller geometri nästan mot kameran.","Klipper mer nära geometri. Delar bakom ögat kan inte återställas."}},
{{"Strength of added equip/holster motion, following native animation, weapon size and a visual weight profile.","Original weapon change animation, without the extra reaction.","Strong extra reaction, still bounded by travel and angle limits."},{"Force du mouvement ajouté au changement d'arme selon animation, taille et poids visuel.","Animation d'origine sans réaction ajoutée.","Forte réaction ajoutée, bornée en déplacement et angle."},{"Intensidad extra al equipar/guardar, según animación original, tamaño y perfil de peso visual.","Cambio de arma original sin reacción extra.","Reacción extra fuerte, sujeta a límites de desplazamiento y ángulo."},{"Zusätzliche Wechselbewegung nach Originalanimation, Größe und visuellem Gewichtsprofil.","Originaler Waffenwechsel ohne Zusatzreaktion.","Starke Zusatzreaktion innerhalb der Weg- und Winkelgrenzen."},{"Intensità extra del cambio arma secondo animazione originale, dimensioni e peso visivo.","Cambio arma originale senza reazione extra.","Reazione extra forte, entro limiti di corsa e angolo."},{"Extra bytesrörelse enligt originalanimation, storlek och visuell viktprofil.","Originalbyte utan extra reaktion.","Stark extra reaktion inom rörelse- och vinkelgränserna."}},
{{"How quickly added weapon-change motion settles; heavier visual profiles respond more slowly.","Slower, heavier-feeling motion.","Faster, firmer motion. Original animation timing is unchanged."},{"Vitesse de retour du mouvement ajouté ; profils lourds plus lents.","Mouvement plus lent, sensation de poids.","Mouvement rapide et ferme. Durée originale inchangée."},{"Qué tan pronto se asienta el movimiento extra del cambio; los perfiles pesados reaccionan más lento.","Movimiento más lento, con sensación de peso.","Movimiento rápido y firme. No cambia la duración original del equipado."},{"Rücklauftempo zusätzlicher Wechselbewegung; schwere Profile reagieren langsamer.","Langsamere, schwerer wirkende Bewegung.","Schnellere, straffere Bewegung. Originaldauer unverändert."},{"Quanto rapidamente si assesta il moto extra; i profili pesanti reagiscono più lentamente.","Movimento lento, sensazione di peso.","Movimento rapido e deciso. Durata originale invariata."},{"Hur snabbt extra bytesrörelse stabiliseras; tunga profiler svarar långsammare.","Långsammare rörelse med tyngdkänsla.","Snabbare, fastare rörelse. Originaltid oförändrad."}},
{{"Soft cap on extra travel when changing weapons; angles also derive from that travel.","No extra change motion.","Allows broad extra travel; gain and weapon profile determine its actual size."},{"Limite souple du déplacement ajouté au changement ; les angles en dérivent aussi.","Aucun mouvement de changement ajouté.","Grande course permise ; gain et profil fixent l'amplitude réelle."},{"Tope suave del recorrido extra al cambiar armas; sus ángulos también derivan de ese recorrido.","Sin movimiento extra de cambio de arma.","Permite un recorrido amplio; ganancia y perfil determinan cuánto se ve."},{"Weiche Grenze zusätzlichen Wechselwegs; Winkel werden daraus abgeleitet.","Keine zusätzliche Wechselbewegung.","Großer Zusatzweg möglich; Stärke und Profil bestimmen die tatsächliche Größe."},{"Limite morbido della corsa extra nel cambio; anche gli angoli derivano da essa.","Nessun movimento extra nel cambio.","Ampia corsa extra; guadagno e profilo ne fissano la misura reale."},{"Mjuk gräns för extra bytesrörelse; vinklar härleds också från den.","Ingen extra bytesrörelse.","Tillåter stor rörelse; styrka och profil avgör verkligt utslag."}},
{{"Soft cap on extra weapon pitch and roll during a change; travel has its own limit.","No extra rotation; translation can remain.","Allows strong extra rotation up to the displayed angle."},{"Limite souple du tangage et roulis ajoutés au changement ; déplacement séparé.","Aucune rotation ajoutée ; déplacement possible.","Permet une forte rotation jusqu'à l'angle indiqué."},{"Tope suave de inclinación vertical y lateral extra al cambiar armas; el recorrido se limita aparte.","Sin rotación extra; puede seguir el desplazamiento.","Permite una rotación extra marcada hasta el ángulo indicado."},{"Weiche Grenze zusätzlichen Nickens/Rollens beim Wechsel; Weg separat.","Keine Zusatzrotation; Verschiebung kann bleiben.","Starke Zusatzrotation bis zum angezeigten Winkel."},{"Limite morbido delle inclinazioni extra al cambio; corsa separata.","Nessuna rotazione extra; può restare lo spostamento.","Forte rotazione extra fino all'angolo indicato."},{"Mjuk gräns för extra nickning/rullning vid byte; rörelse separat.","Ingen extra rotation; förskjutning kan finnas kvar.","Tillåter stark extra rotation upp till angiven vinkel."}},
};
/* Specific calibration help, EN FR ES DE IT SV. Included by motion_params.c. */
static const char *const MotionHelpPatterns[37][6][3] = {
 /* speed */ {
  {"Speed needed for full motion effects; does not change player speed.", "Full effect at a gentle pace.", "Full effect requires much faster movement."},
  {"Vitesse nécessaire aux effets complets ; ne change pas la vitesse du joueur.", "Effet complet à faible allure.", "Effet complet seulement à grande vitesse."},
  {"Velocidad necesaria para alcanzar el efecto completo; no cambia tu velocidad.", "Efecto pleno incluso al ir despacio.", "Necesitas moverte mucho más rápido para alcanzar el efecto pleno."},
  {"Tempo für volle Bewegungseffekte; ändert nicht das Spielertempo.", "Volle Wirkung bei langsamem Gehen.", "Volle Wirkung erst bei sehr schnellem Laufen."},
  {"Velocità necessaria per l'effetto completo; non cambia la velocità del giocatore.", "Effetto pieno anche andando piano.", "Serve un movimento molto più rapido."},
  {"Fart som krävs för full rörelseeffekt; ändrar inte spelarens fart.", "Full effekt redan vid långsam gång.", "Full effekt kräver mycket snabbare rörelse."},
 },
 /* response */ {
  {"How quickly this effect follows its target, like a soft or firm spring.", "Slow, soft response with more lag.", "Quick, firm response with less lag."},
  {"Vitesse de suivi de cet effet, comme un ressort souple ou ferme.", "Réaction lente et souple, plus de retard.", "Réaction rapide et ferme, moins de retard."},
  {"Qué tan pronto este efecto alcanza su destino, como un resorte suave o firme.", "Reacción lenta y suave, con más retraso.", "Reacción rápida y firme, con menos retraso."},
  {"Wie schnell dieser Effekt seinem Ziel folgt, wie eine weiche oder feste Feder.", "Langsam und weich, mehr Verzögerung.", "Schnell und straff, weniger Verzögerung."},
  {"Quanto rapidamente questo effetto segue il bersaglio, come una molla morbida o rigida.", "Risposta lenta e morbida, più ritardo.", "Risposta rapida e decisa, meno ritardo."},
  {"Hur snabbt effekten följer sitt mål, som en mjuk eller fast fjäder.", "Långsam, mjuk reaktion med mer eftersläpning.", "Snabb, fast reaktion med mindre eftersläpning."},
 },
 /* zoom */ {
  {"Minimum share of motion kept while zooming.", "Allows the strongest reduction as zoom increases.", "Keeps full motion even at maximum zoom."},
  {"Part minimale du mouvement conservée au zoom.", "Permet la réduction la plus forte avec le zoom.", "Conserve tout le mouvement même au zoom maximal."},
  {"Porción mínima del movimiento que se conserva al usar zoom.", "Permite reducir más el movimiento al aumentar el zoom.", "Conserva todo el movimiento incluso con zoom máximo."},
  {"Mindestanteil der Bewegung beim Zoomen.", "Erlaubt stärkste Dämpfung bei hohem Zoom.", "Volle Bewegung auch bei maximalem Zoom."},
  {"Quota minima di movimento mantenuta con lo zoom.", "Consente la massima riduzione aumentando lo zoom.", "Mantiene tutto il movimento anche allo zoom massimo."},
  {"Minsta andel rörelse som behålls vid zoom.", "Tillåter starkast dämpning när zoom ökar.", "Behåller full rörelse även vid maximal zoom."},
 },
 /* rate */ {
  {"Multiplies the walking/running bob rhythm; does not change player speed.", "Freezes the cycle; can leave an offset while moving.", "Doubles the base rhythm; the weapon still obeys its cycle limit."},
  {"Multiplie le rythme du balancement en marche/course, pas la vitesse du joueur.", "Fige le cycle ; un décalage peut rester en mouvement.", "Double le rythme de base ; l'arme respecte sa limite de cycles."},
  {"Multiplica el ritmo del balanceo al caminar/correr; no cambia tu velocidad.", "Congela el ciclo; puede dejar un desplazamiento al moverte.", "Duplica el ritmo base; el arma sigue respetando su límite de ciclos."},
  {"Skaliert den Wipprhythmus beim Gehen/Laufen, nicht das Spielertempo.", "Hält den Zyklus an; ein Versatz kann beim Gehen bleiben.", "Doppelter Grundrhythmus; das Waffenlimit gilt weiterhin."},
  {"Moltiplica il ritmo dell'oscillazione camminando/correndo, non la velocità.", "Ferma il ciclo; può restare uno spostamento durante il moto.", "Raddoppia il ritmo base; resta valido il limite dei cicli dell'arma."},
  {"Skalar gungrytmen vid gång/löpning, inte spelarens fart.", "Fryser cykeln; en förskjutning kan finnas kvar vid rörelse.", "Dubbel grundrytm; vapnets cykelgräns gäller fortfarande."},
 },
 /* hz */ {
  {"Caps the weapon bob cycle rate, especially when running.", "At most one cycle every 5 seconds.", "At most 3 cycles per second; much faster bobbing."},
  {"Plafonne les cycles du balancement de l'arme, surtout en course.", "Au plus un cycle toutes les 5 secondes.", "Au plus 3 cycles par seconde ; balancement bien plus rapide."},
  {"Limita la rapidez del ciclo del arma, especialmente al correr.", "Como máximo un ciclo cada 5 segundos.", "Como máximo 3 ciclos por segundo; balanceo mucho más rápido."},
  {"Begrenzt Waffenwippzyklen, besonders beim Laufen.", "Höchstens ein Zyklus alle 5 Sekunden.", "Höchstens 3 Zyklen pro Sekunde; viel schnelleres Wippen."},
  {"Limita i cicli dell'oscillazione dell'arma, soprattutto correndo.", "Al massimo un ciclo ogni 5 secondi.", "Al massimo 3 cicli al secondo; oscillazione molto più rapida."},
  {"Begränsar vapnets gungcykler, särskilt vid löpning.", "Högst en cykel var femte sekund.", "Högst 3 cykler per sekund; mycket snabbare gungning."},
 },
 /* bobamp */ {
  {"Amount of walking/running bob in the indicated direction; does not set its rhythm.", "No added bob in this direction.", "Largest excursion in this direction at full movement intensity."},
  {"Amplitude du balancement en marche/course dans la direction indiquée, pas son rythme.", "Aucun balancement ajouté dans cette direction.", "Course maximale dans cette direction à pleine intensité."},
  {"Recorrido del balanceo al caminar/correr en la dirección indicada; no cambia el ritmo.", "Sin balanceo añadido en esa dirección.", "Mayor recorrido en esa dirección al alcanzar la intensidad plena."},
  {"Ausmaß des Wippens beim Gehen/Laufen in der angegebenen Richtung, nicht sein Rhythmus.", "Kein zusätzliches Wippen in dieser Richtung.", "Größter Ausschlag in dieser Richtung bei voller Stärke."},
  {"Ampiezza dell'oscillazione camminando/correndo nella direzione indicata, non il ritmo.", "Nessuna oscillazione aggiunta in questa direzione.", "Escursione massima in questa direzione a piena intensità."},
  {"Gungningens storlek vid gång/löpning i angiven riktning, inte rytmen.", "Ingen extra gungning i denna riktning.", "Största utslag i denna riktning vid full rörelseintensitet."},
 },
 /* crouchbob */ {
  {"Multiplies both head and weapon bob while crouched.", "No walking bob while crouched.", "Same bob strength as standing at the same speed."},
  {"Multiplie le balancement de la tête et de l'arme en position accroupie.", "Aucun balancement de marche accroupie.", "Même intensité que debout à vitesse égale."},
  {"Multiplica el balanceo de cabeza y arma mientras estás agachado.", "Sin balanceo de pasos al estar agachado.", "Misma intensidad que de pie a igual velocidad."},
  {"Skaliert Kopf- und Waffenwippen beim Ducken.", "Kein Gehwippen beim Ducken.", "Gleiche Stärke wie im Stehen bei gleichem Tempo."},
  {"Moltiplica l'oscillazione di testa e arma da accovacciato.", "Nessuna oscillazione di cammino da accovacciato.", "Stessa intensità che in piedi a pari velocità."},
  {"Skalar både huvudets och vapnets gungning i huk.", "Ingen gånggungning i huk.", "Samma styrka som stående vid samma fart."},
 },
 /* swaygain */ {
  {"How strongly mouse turns make the weapon lag behind. A flick shows it clearly.", "No mouse-driven sway.", "Strong lag, still softened by response and limit settings."},
  {"Force du retard de l'arme en tournant la souris, visible sur un geste rapide.", "Aucune inertie due à la souris.", "Retard fort, amorti par la réponse et la limite."},
  {"Cuánto se retrasa el arma al girar el mouse; se aprecia con un flick.", "Sin sway provocado por el mouse.", "Retraso fuerte, suavizado por los ajustes de respuesta y límite."},
  {"Wie stark die Waffe beim Mausdrehen nachläuft; gut bei schnellen Schwenks sichtbar.", "Kein Mausnachlauf.", "Starker Nachlauf, durch Reaktion und Grenze gedämpft."},
  {"Quanto l'arma resta indietro girando il mouse; visibile con uno scatto.", "Nessuna inerzia dovuta al mouse.", "Forte ritardo, smorzato da risposta e limite."},
  {"Hur mycket vapnet släpar efter när musen vrids; syns vid snabba svep.", "Ingen musstyrd tröghet.", "Stark eftersläpning, dämpad av respons och gräns."},
 },
 /* swaylimit */ {
  {"Soft limit on the mouse sway target, before movement multipliers.", "Very small sway range, even with fast turns.", "Much wider sway range; gains determine the visible amount."},
  {"Limite souple de l'inertie souris, avant ses multiplicateurs.", "Course très faible même en tournant vite.", "Course plus large ; les gains déterminent l'amplitude visible."},
  {"Límite suave del sway del mouse, antes de sus multiplicadores.", "Recorrido muy pequeño incluso con giros rápidos.", "Recorrido mucho mayor; las ganancias determinan cuánto se ve."},
  {"Weiche Grenze des Mausnachlaufs vor den Bewegungsfaktoren.", "Sehr kleiner Bereich auch bei schnellen Drehungen.", "Viel größerer Bereich; Faktoren bestimmen den sichtbaren Ausschlag."},
  {"Limite morbido dell'inerzia mouse, prima dei moltiplicatori.", "Escursione piccolissima anche girando rapidamente.", "Escursione maggiore; i guadagni determinano l'effetto visibile."},
  {"Mjuk gräns för musens eftersläpning före rörelsefaktorerna.", "Mycket litet utslag även vid snabba svängar.", "Större utslag; förstärkningarna avgör synlig storlek."},
 },
 /* swayscale */ {
  {"Scales weapon mouse sway in the indicated direction; does not change mouse sensitivity.", "No added sway in this direction.", "Strongest motion in this direction, within the shared sway limit."},
  {"Multiplie l'inertie souris de l'arme dans cette direction, pas la sensibilité souris.", "Aucune inertie ajoutée dans cette direction.", "Mouvement maximal dans cette direction, selon la limite commune."},
  {"Multiplica el sway del arma en la dirección indicada; no cambia la sensibilidad del mouse.", "Sin sway añadido en esta dirección.", "Movimiento más fuerte en esa dirección, sujeto al límite común del sway."},
  {"Skaliert Waffen-Mausnachlauf in dieser Richtung, nicht die Mausempfindlichkeit.", "Kein zusätzlicher Nachlauf in dieser Richtung.", "Stärkste Bewegung in dieser Richtung innerhalb der gemeinsamen Grenze."},
  {"Moltiplica l'inerzia mouse dell'arma nella direzione indicata, non la sensibilità mouse.", "Nessuna inerzia aggiunta in questa direzione.", "Movimento massimo in questa direzione, entro il limite comune."},
  {"Skalar vapnets musrörelse i angiven riktning, inte muskänsligheten.", "Ingen extra tröghet i denna riktning.", "Starkast rörelse i denna riktning inom den gemensamma gränsen."},
 },
 /* camerasway */ {
  {"Camera side tilt during horizontal mouse turns; strength, not smoothing speed.", "No added camera tilt from mouse sway.", "Most noticeable added camera tilt when turning."},
  {"Roulis caméra en tournant la souris horizontalement ; force, pas vitesse de lissage.", "Aucun roulis caméra ajouté par la souris.", "Roulis caméra ajouté le plus visible en tournant."},
  {"Inclinación lateral de cámara al girar horizontalmente el mouse; es intensidad, no rapidez de suavizado.", "Sin inclinación extra de cámara por sway.", "Inclinación extra más notoria al girar."},
  {"Kameraseitenneigung bei horizontalen Mausdrehungen; Stärke, nicht Glättungstempo.", "Keine zusätzliche Kameraneigung durch Mausnachlauf.", "Stärkste zusätzliche Kameraneigung beim Drehen."},
  {"Inclinazione laterale della telecamera girando il mouse orizzontalmente; intensità, non velocità.", "Nessuna inclinazione extra dovuta al mouse.", "Inclinazione extra più evidente girando."},
  {"Kamerans sidolutning vid vågräta mussvängar; styrka, inte utjämningsfart.", "Ingen extra kameralutning från musrörelser.", "Tydligast extra kameralutning vid svängar."},
 },
 /* look */ {
  {"Makes the weapon front follow looking up/down; does not alter the crosshair or bullets.", "No extra weapon pointing.", "Up to 10 degrees of extra pointing near vertical aim."},
  {"Fait suivre le haut/bas à l'avant de l'arme ; ne change ni viseur ni tirs.", "Aucun suivi de visée ajouté.", "Jusqu'à 10 degrés ajoutés près de la visée verticale."},
  {"Hace que la punta del arma siga la mirada arriba/abajo; no altera la mira ni los disparos.", "Sin orientación extra del arma.", "Hasta 10 grados de orientación extra al mirar casi verticalmente."},
  {"Lässt die Waffenspitze dem Blick auf/ab folgen; ändert weder Fadenkreuz noch Schüsse.", "Keine zusätzliche Waffenausrichtung.", "Bis zu 10 Grad zusätzlich bei fast senkrechtem Blick."},
  {"Fa seguire alla punta dell'arma lo sguardo su/giù; non cambia mirino o proiettili.", "Nessun orientamento extra dell'arma.", "Fino a 10 gradi extra mirando quasi in verticale."},
  {"Låter vapnets spets följa blicken upp/ned; ändrar inte sikte eller skott.", "Ingen extra vapenriktning.", "Upp till 10 graders extra riktning nära lodrätt sikte."},
 },
 /* side */ {
  {"Weapon moves left when strafing right, and right when strafing left.", "No sideways movement inertia.", "Largest sideways weapon travel, softened by movement response."},
  {"L'arme va à gauche en se déplaçant à droite, et inversement.", "Aucune inertie latérale de déplacement.", "Course latérale maximale, amortie par la réponse du mouvement."},
  {"El arma va a la izquierda al hacer strafe derecho, y viceversa.", "Sin desplazamiento lateral por inercia.", "Mayor recorrido lateral del arma, suavizado por la respuesta de movimiento."},
  {"Waffe bewegt sich beim Rechtslaufen nach links und umgekehrt.", "Keine seitliche Bewegungsträgheit.", "Größter seitlicher Waffenweg, durch Bewegungsreaktion geglättet."},
  {"L'arma va a sinistra spostandosi a destra, e viceversa.", "Nessuna inerzia laterale del movimento.", "Massima escursione laterale, smorzata dalla risposta del movimento."},
  {"Vapnet går åt vänster vid sidsteg höger och tvärtom.", "Ingen rörelsetröghet i sidled.", "Största sidoförskjutning, utjämnad av rörelseresponsen."},
 },
 /* depth */ {
  {"Weapon moves into the scene when going forward, and toward you when backing up.", "No forward/backward movement inertia.", "Largest travel into/out of the scene."},
  {"L'arme s'éloigne en avançant et se rapproche en reculant.", "Aucune inertie avant/arrière.", "Course maximale vers le fond et vers vous."},
  {"El arma entra en la escena al avanzar y se acerca a ti al retroceder.", "Sin desplazamiento por avance/retroceso.", "Mayor recorrido hacia adentro y hacia afuera."},
  {"Waffe geht beim Vorwärtslaufen in die Szene, beim Rückwärtslaufen zu dir.", "Keine Vor-/Rückwärtsbewegungsträgheit.", "Größter Weg in die Szene hinein und heraus."},
  {"L'arma entra nella scena avanzando e si avvicina arretrando.", "Nessuna inerzia avanti/indietro.", "Massima escursione verso l'interno e verso di te."},
  {"Vapnet går in i scenen framåt och mot dig när du backar.", "Ingen rörelsetröghet framåt/bakåt.", "Största rörelse inåt och utåt."},
 },
 /* moveroll */ {
  {"Adds weapon side tilt while strafing, opposite to movement.", "No added strafe tilt.", "Strongest added tilt during fast side movement."},
  {"Incline l'arme en déplacement latéral, à l'opposé du mouvement.", "Aucune inclinaison latérale ajoutée.", "Inclinaison maximale en déplacement latéral rapide."},
  {"Inclina lateralmente el arma al hacer strafe, en sentido opuesto al movimiento.", "Sin inclinación extra por strafe.", "Mayor inclinación extra al desplazarte rápido de lado."},
  {"Waffenseitenneigung beim Seitwärtslaufen, entgegen der Bewegung.", "Keine zusätzliche Seitwärtsneigung.", "Stärkste Neigung bei schnellem Seitwärtslaufen."},
  {"Inclina l'arma negli spostamenti laterali, in senso opposto.", "Nessuna inclinazione extra laterale.", "Massima inclinazione con spostamento laterale rapido."},
  {"Lutar vapnet i sidled vid sidsteg, motsatt rörelsen.", "Ingen extra sidstegslutning.", "Störst lutning vid snabb sidorörelse."},
 },
 /* stance */ {
  {"Weapon reacts to camera rise/fall when crouching; does not change crouch height.", "No added weapon tug when crouching/standing up.", "Stronger tug, still bounded by the crouch weapon limit."},
  {"L'arme réagit à la montée/descente caméra en s'accroupissant, sans changer la hauteur.", "Aucun mouvement ajouté pendant la transition.", "Mouvement plus fort, borné par la limite de l'arme accroupie."},
  {"El arma reacciona a la subida/bajada de cámara al agacharte; no cambia tu altura.", "Sin tirón añadido al agacharte o levantarte.", "Tirón más fuerte, sujeto al límite del arma al agacharse."},
  {"Waffe reagiert auf Kamerahöhenwechsel beim Ducken; ändert nicht die Duckhöhe.", "Kein zusätzlicher Ruck beim Ducken/Aufstehen.", "Stärkerer Ruck, durch das Duck-Waffenlimit begrenzt."},
  {"L'arma reagisce al cambio di altezza della telecamera accovacciandosi; non cambia l'altezza finale.", "Nessun impulso extra accovacciandosi/alzandosi.", "Impulso più forte, entro il limite dell'arma accovacciata."},
  {"Vapnet reagerar på kamerahöjdändring vid hukning; ändrar inte hukhöjden.", "Inget extra ryck vid hukning/uppresning.", "Starkare ryck, begränsat av vapnets hukgräns."},
 },
 /* stancelimit */ {
  {"Soft cap on vertical weapon tug when crouching/standing up.", "Removes this extra weapon tug.", "Allows up to 200 mm of extra weapon travel."},
  {"Limite souple du mouvement vertical de l'arme en s'accroupissant/se relevant.", "Supprime ce mouvement ajouté.", "Autorise jusqu'à 200 mm de course ajoutée."},
  {"Tope suave del tirón vertical del arma al agacharte o levantarte.", "Elimina ese tirón adicional del arma.", "Permite hasta 200 mm de recorrido extra."},
  {"Weiche Grenze des vertikalen Waffenrucks beim Ducken/Aufstehen.", "Entfernt diesen zusätzlichen Waffenruck.", "Bis zu 200 mm zusätzlicher Waffenweg."},
  {"Limite morbido del moto verticale dell'arma accovacciandosi/alzandosi.", "Elimina questo movimento extra.", "Fino a 200 mm di escursione extra."},
  {"Mjuk gräns för vapnets lodräta ryck vid hukning/uppresning.", "Tar bort detta extra vapenryck.", "Upp till 200 mm extra vapenrörelse."},
 },
 /* stairs */ {
  {"Largest grounded height change smoothed by the camera. Does not increase climbable obstacle height.", "Disables step-height smoothing.", "Smooths height changes up to 2000 mm while grounded."},
  {"Plus grand changement de hauteur au sol lissé par la caméra. N'augmente pas la hauteur franchissable.", "Désactive le lissage des marches.", "Lisse les changements jusqu'à 2000 mm au sol."},
  {"Mayor cambio de altura con los pies en suelo que suaviza la cámara. No aumenta la altura que puedes trepar.", "Desactiva el suavizado de desniveles.", "Suaviza cambios de hasta 2000 mm mientras estás en suelo."},
  {"Größter geglätteter Höhenwechsel am Boden. Erhöht nicht die überwindbare Hindernishöhe.", "Keine Glättung von Stufenhöhen.", "Glättet Höhenwechsel bis 2000 mm am Boden."},
  {"Massimo cambio di altezza a terra smussato dalla telecamera. Non aumenta l'altezza superabile.", "Disattiva la transizione morbida sui gradini.", "Smussa cambi fino a 2000 mm mentre sei a terra."},
  {"Största höjdändring på marken som kameran jämnar ut. Ökar inte klättringsbar hinderhöjd.", "Ingen utjämning av steghöjder.", "Jämnar ut höjdändringar upp till 2000 mm på marken."},
 },
 /* stairweapon */ {
  {"Adds vertical weapon reaction to smoothed stairs/low obstacles.", "Camera smoothing remains, without this extra weapon motion.", "Strongest extra weapon response on steps."},
  {"Réaction verticale de l'arme aux marches/petits obstacles lissés.", "Lissage caméra conservé, sans ce mouvement ajouté de l'arme.", "Réaction ajoutée de l'arme maximale sur les marches."},
  {"Añade una reacción vertical del arma al suavizar escaleras/obstáculos bajos.", "Mantiene el suavizado de cámara, sin este movimiento extra del arma.", "Mayor reacción extra del arma al cruzar desniveles."},
  {"Vertikale Waffenreaktion auf geglättete Stufen/niedrige Hindernisse.", "Kameraglättung bleibt, ohne diese Waffenbewegung.", "Stärkste zusätzliche Waffenreaktion auf Stufen."},
  {"Reazione verticale dell'arma su scale/piccoli ostacoli smussati.", "Resta la transizione della telecamera, senza questo moto extra.", "Massima reazione extra dell'arma sui gradini."},
  {"Lodrät vapenreaktion på utjämnade trappor/låga hinder.", "Kameran jämnas fortfarande ut, utan denna vapenrörelse.", "Starkaste extra vapenreaktion på trappsteg."},
 },
 /* impactresponse */ {
 {"How quickly the camera follows the jump/landing gesture; duration has separate controls.","Slow and soft, with more lag and a lower peak.","Fast and firm, following the gesture more closely."},
 {"Vitesse de suivi du geste saut/réception; durée réglée séparément.","Lent et doux, plus de retard et pic réduit.","Rapide et ferme, suit le geste plus fidèlement."},
 {"Qué tan rápido la cámara sigue el gesto de salto/aterrizaje; la duración se ajusta aparte.","Lenta y suave, con más retraso y menor pico.","Rápida y firme, sigue el gesto más de cerca."},
 {"Wie schnell die Kamera der Sprung-/Landebewegung folgt; Dauer separat.","Langsam und weich, mehr Verzögerung und kleinerer Ausschlag.","Schnell und fest, folgt der Bewegung genauer."},
 {"Rapidità con cui la telecamera segue il gesto; durata regolata a parte.","Lenta e morbida, più ritardo e picco ridotto.","Rapida e decisa, segue il gesto più da vicino."},
 {"Hur snabbt kameran följer hopp-/landningsrörelsen; längden justeras separat.","Långsam och mjuk, mer fördröjning och lägre topp.","Snabb och fast, följer rörelsen närmare."}
 },
 /* impulse */ {
  {"Cosmetic camera impulse at this event. Does not change jump height, gravity or damage.", "No added impulse at this event.", "Strongest impulse; response and impact limit still control the result."},
  {"Impulsion visuelle à cet événement. Ne change ni saut, ni gravité, ni dégâts.", "Aucune impulsion ajoutée à cet événement.", "Impulsion maximale ; réponse et limite contrôlent le résultat."},
  {"Impulso visual de cámara en este evento. No cambia altura de salto, gravedad ni daño.", "Sin impulso añadido en este evento.", "Impulso más fuerte; respuesta y límite siguen controlando el resultado."},
  {"Visueller Kameraimpuls bei diesem Ereignis. Ändert weder Sprunghöhe, Schwerkraft noch Schaden.", "Kein zusätzlicher Impuls bei diesem Ereignis.", "Stärkster Impuls; Reaktion und Grenze bestimmen das Ergebnis."},
  {"Impulso visivo in questo evento. Non cambia salto, gravità o danni.", "Nessun impulso extra in questo evento.", "Impulso massimo; risposta e limite controllano il risultato."},
  {"Visuell kameraimpuls vid händelsen. Ändrar inte hopphöjd, gravitation eller skada.", "Ingen extra impuls vid händelsen.", "Starkast impuls; respons och gräns styr resultatet."},
 },
 /* impactlimit */ {
  {"Soft cap on jump/landing camera travel; also bounds impact passed to weapon and tilt.", "Removes these added impact reactions.", "Allows a much larger bounce, depending on the impulse."},
  {"Limite du déplacement caméra au saut/à la réception, aussi transmise à l'arme et au roulis.", "Supprime ces réactions d'impact ajoutées.", "Permet un rebond bien plus ample selon l'impulsion."},
  {"Tope suave del recorrido de cámara al saltar/aterrizar; también limita el impacto enviado al arma y a la inclinación.", "Elimina estas reacciones extra al impacto.", "Permite un rebote mucho mayor, según el impulso."},
  {"Weiche Grenze des Kamerawegs bei Sprung/Landung; begrenzt auch Waffenimpuls und Neigung.", "Entfernt diese zusätzlichen Stoßreaktionen.", "Erlaubt deutlich größeres Nachfedern, abhängig vom Impuls."},
  {"Limite del moto telecamera al salto/atterraggio; limita anche impulso all'arma e inclinazione.", "Elimina queste reazioni extra all'impatto.", "Consente rimbalzi molto più ampi, secondo l'impulso."},
  {"Mjuk gräns för kamerarörelse vid hopp/landning; begränsar även impuls till vapen och lutning.", "Tar bort dessa extra stötreaktioner.", "Tillåter mycket större studs, beroende på impulsen."},
 },
 /* impactweapon */ {
  {"Share of camera impact added as vertical weapon motion.", "No extra weapon travel from impact.", "Full impact share, still capped by the weapon limit."},
  {"Part de l'impact caméra ajoutée au mouvement vertical de l'arme.", "Aucun déplacement ajouté de l'arme dû à l'impact.", "Impact complet, toujours borné par la limite de l'arme."},
  {"Porción del impacto de cámara añadida al movimiento vertical del arma.", "Sin recorrido extra del arma por el impacto.", "Porción completa del impacto, aún sujeta al límite del arma."},
  {"Anteil des Kamerastoßes als vertikale Waffenbewegung.", "Kein zusätzlicher Waffenweg durch den Stoß.", "Voller Stoßanteil, vom Waffenlimit begrenzt."},
  {"Quota dell'impatto telecamera aggiunta al movimento verticale dell'arma.", "Nessun moto extra dell'arma dall'impatto.", "Impatto completo, entro il limite dell'arma."},
  {"Andel kamerastöt som läggs till vapnets lodräta rörelse.", "Ingen extra vapenrörelse från stöten.", "Hela stöten, begränsad av vapengränsen."},
 },
 /* impactwlimit */ {
  {"Soft cap on weapon jump/landing travel, without removing camera bounce.", "No extra weapon impact travel.", "Allows up to 200 mm of extra weapon impact travel."},
  {"Limite du déplacement de l'arme au saut/à la réception, sans retirer le rebond caméra.", "Aucun déplacement d'impact ajouté à l'arme.", "Permet jusqu'à 200 mm de déplacement d'impact de l'arme."},
  {"Tope suave del recorrido del arma al saltar/aterrizar, sin quitar el rebote de cámara.", "Sin recorrido extra del arma por impacto.", "Permite hasta 200 mm de recorrido extra del arma por impacto."},
  {"Weiche Grenze des Waffenwegs bei Sprung/Landung, ohne das Kamerafedern zu entfernen.", "Kein zusätzlicher Stoßweg der Waffe.", "Bis zu 200 mm zusätzlicher Stoßweg der Waffe."},
  {"Limite del moto dell'arma al salto/atterraggio, senza eliminare il rimbalzo della telecamera.", "Nessuna corsa extra dell'arma da impatto.", "Fino a 200 mm di corsa extra dell'arma da impatto."},
  {"Mjuk gräns för vapnets hopp-/landningsrörelse utan att ta bort kamerans studs.", "Ingen extra vapenrörelse från stöt.", "Upp till 200 mm extra vapenrörelse från stöt."},
 },
 /* impactroll */ {
  {"Camera side tilt per millimetre of jump/landing bounce.", "No extra impact tilt.", "Strongest side tilt for the same bounce."},
  {"Roulis caméra par millimètre de rebond au saut/à la réception.", "Aucun roulis d'impact ajouté.", "Roulis maximal pour le même rebond."},
  {"Inclinación lateral de cámara por cada milímetro de rebote al saltar/aterrizar.", "Sin inclinación extra por impacto.", "Mayor inclinación lateral para un mismo rebote."},
  {"Kameraseitenneigung pro Millimeter Sprung-/Landungsfedern.", "Keine zusätzliche Stoßneigung.", "Stärkste Seitenneigung bei gleichem Nachfedern."},
  {"Inclinazione laterale telecamera per millimetro di rimbalzo al salto/atterraggio.", "Nessuna inclinazione extra da impatto.", "Massima inclinazione per lo stesso rimbalzo."},
  {"Kamerans sidolutning per millimeter hopp-/landningsstuds.", "Ingen extra stötlutning.", "Störst sidolutning för samma studs."},
 },
 /* reloadscale */ {
  {"Camera follows sampled reload motion in this direction; does not change animation speed.", "No added camera response in this direction.", "Strongest response in this direction, within reload limits."},
  {"La caméra suit le mouvement mesuré du rechargement dans cette direction, sans changer sa vitesse.", "Aucune réaction caméra ajoutée dans cette direction.", "Réaction maximale dans cette direction, selon les limites de rechargement."},
  {"La cámara sigue el movimiento medido de la recarga en esa dirección; no cambia su velocidad.", "Sin reacción extra de cámara en esa dirección.", "Reacción más fuerte en esa dirección, sujeta a los límites de recarga."},
  {"Kamera folgt gemessener Nachladebewegung in dieser Richtung; ändert nicht das Animationstempo.", "Keine zusätzliche Kamerareaktion in dieser Richtung.", "Stärkste Reaktion in dieser Richtung innerhalb der Nachladegrenzen."},
  {"La telecamera segue il moto misurato di ricarica in questa direzione, senza cambiarne la velocità.", "Nessuna reazione extra della telecamera in questa direzione.", "Massima reazione in questa direzione, entro i limiti di ricarica."},
  {"Kameran följer uppmätt omladdningsrörelse i denna riktning; ändrar inte animationsfarten.", "Ingen extra kamerareaktion i denna riktning.", "Starkast reaktion i denna riktning inom omladdningsgränserna."},
 },
 /* reloadlimit */ {
  {"Soft cap on extra camera and weapon translations during reload; angles have a separate limit.", "No extra reload translations; tilt may remain.", "Allows extra translations up to 100 mm per axis."},
  {"Limite des déplacements ajoutés caméra/arme au rechargement ; angles réglés séparément.", "Aucun déplacement ajouté ; l'inclinaison peut rester.", "Permet jusqu'à 100 mm de déplacement ajouté par axe."},
  {"Tope suave del desplazamiento extra de cámara y arma al recargar; los ángulos tienen otro límite.", "Sin desplazamientos extra de recarga; puede seguir la inclinación.", "Permite hasta 100 mm de desplazamiento extra por eje."},
  {"Weiche Grenze zusätzlicher Kamera-/Waffenverschiebung beim Nachladen; Winkel separat.", "Keine zusätzlichen Verschiebungen; Neigung kann bleiben.", "Bis zu 100 mm zusätzliche Verschiebung pro Achse."},
  {"Limite degli spostamenti extra di telecamera e arma ricaricando; angoli separati.", "Nessuno spostamento extra; può restare l'inclinazione.", "Fino a 100 mm di spostamento extra per asse."},
  {"Mjuk gräns för extra kamera-/vapenförskjutning vid omladdning; vinklar separat.", "Inga extra förskjutningar; lutning kan finnas kvar.", "Upp till 100 mm extra förskjutning per axel."},
 },
 /* reloadangle */ {
  {"Soft cap on added camera pitch and side tilt during reload.", "No extra camera reload tilt; translations remain.", "Allows up to 4 degrees of tilt on each axis."},
  {"Limite du tangage et roulis caméra ajoutés au rechargement.", "Aucune inclinaison caméra ajoutée ; déplacements conservés.", "Permet jusqu'à 4 degrés sur chaque axe."},
  {"Tope suave de la inclinación vertical y lateral extra de cámara al recargar.", "Sin inclinación extra de cámara al recargar; siguen los desplazamientos.", "Permite hasta 4 grados de inclinación por eje."},
  {"Weiche Grenze des Kameranickens/-rollens beim Nachladen.", "Keine zusätzliche Nachladeneigung; Verschiebungen bleiben.", "Bis zu 4 Grad Neigung pro Achse."},
  {"Limite delle inclinazioni extra della telecamera ricaricando.", "Nessuna inclinazione extra; restano gli spostamenti.", "Fino a 4 gradi di inclinazione per asse."},
  {"Mjuk gräns för extra kameranickning/sidolutning vid omladdning.", "Ingen extra omladdningslutning; förskjutningar kvarstår.", "Upp till 4 graders lutning per axel."},
 },
 /* reloadweapon */ {
  {"Adds weapon travel from sampled reload motion, on top of the original animation.", "Original reload animation without this added travel.", "Strongest added weapon travel, within the reload translation limit."},
  {"Ajoute du déplacement de l'arme à partir du rechargement mesuré, en plus de l'animation d'origine.", "Animation d'origine sans ce déplacement ajouté.", "Déplacement ajouté maximal, selon la limite de translation."},
  {"Añade recorrido al arma a partir de su recarga medida, además de su animación original.", "Animación original sin este recorrido extra.", "Mayor recorrido extra del arma, sujeto al límite de desplazamiento de recarga."},
  {"Zusätzlicher Waffenweg aus gemessener Nachladebewegung, neben der Originalanimation.", "Originalanimation ohne diesen zusätzlichen Weg.", "Größter zusätzlicher Waffenweg innerhalb der Nachlade-Verschiebungsgrenze."},
  {"Aggiunge corsa all'arma dal moto di ricarica misurato, oltre all'animazione originale.", "Animazione originale senza questa corsa extra.", "Massima corsa extra, entro il limite di spostamento della ricarica."},
  {"Extra vapenrörelse från uppmätt omladdning ovanpå originalanimationen.", "Originalanimation utan denna extra rörelse.", "Största extra vapenrörelse inom omladdningens förskjutningsgräns."},
 },
 /* reloadgun */ {
  {"Procedural reload strength for this weapon only; does not change original animation speed.", "No procedural reload reaction for this weapon.", "Triple input before soft limits; not necessarily triple visible motion."},
  {"Force procédurale de rechargement pour cette arme seule, sans changer sa vitesse d'animation.", "Aucune réaction procédurale de rechargement pour cette arme.", "Signal triplé avant limites souples, pas forcément le mouvement visible."},
  {"Intensidad de la recarga procedural solo para esta arma; no cambia la velocidad de su animación original.", "Sin reacción procedural de recarga para esta arma.", "Triplica la entrada antes de los topes suaves; no necesariamente el movimiento visible."},
  {"Prozedurale Nachladestärke nur dieser Waffe; ändert nicht das Originalanimationstempo.", "Keine prozedurale Nachladereaktion dieser Waffe.", "Dreifaches Eingangssignal vor weichen Grenzen, nicht zwingend dreifacher sichtbarer Weg."},
  {"Intensità procedurale di ricarica solo per quest'arma, senza cambiarne la velocità originale.", "Nessuna reazione procedurale di ricarica per quest'arma.", "Ingresso triplo prima dei limiti morbidi, non necessariamente moto visibile triplo."},
  {"Procedurstyrd omladdningsstyrka bara för detta vapen; ändrar inte originalanimationens fart.", "Ingen procedurstyrd omladdningsreaktion för detta vapen.", "Tredubbel insignal före mjuka gränser, inte alltid tredubbel synlig rörelse."},
 },
 /* steps */ {
  {"Enables new material-based footsteps; does not smooth stairs.", "New footsteps off; native water-sound path resumes.", "New footsteps on, according to detected surface."},
  {"Active les nouveaux pas selon le matériau ; ne lisse pas les escaliers.", "Nouveaux pas coupés ; sons d'eau d'origine rétablis.", "Nouveaux pas actifs selon la surface détectée."},
  {"Activa los nuevos pasos según material; no suaviza escaleras.", "Pasos nuevos apagados; vuelve la ruta original de sonidos de agua.", "Pasos nuevos activos según la superficie detectada."},
  {"Aktiviert neue materialabhängige Schritte; glättet keine Treppen.", "Neue Schritte aus; ursprüngliche Wassergeräusche aktiv.", "Neue Schritte an, je nach erkannter Oberfläche."},
  {"Attiva i nuovi passi secondo il materiale; non smussa le scale.", "Nuovi passi spenti; tornano i suoni d'acqua originali.", "Nuovi passi attivi secondo la superficie rilevata."},
  {"Aktiverar nya materialbaserade fotsteg; jämnar inte ut trappor.", "Nya fotsteg av; ursprungliga vattenljud återkommer.", "Nya fotsteg på, enligt upptäckt yta."},
 },
 /* stride */ {
  {"Distance between steps in this stance; also feeds bob rhythm before its rate settings.", "Short steps: more frequent sounds and faster base bob at equal speed.", "Long steps: less frequent sounds and slower base bob at equal speed."},
  {"Distance entre pas dans cette posture ; alimente aussi le rythme de base du balancement.", "Pas courts : sons plus fréquents et rythme plus rapide à vitesse égale.", "Pas longs : sons moins fréquents et rythme plus lent à vitesse égale."},
  {"Distancia entre pasos en esta postura; también alimenta el ritmo base del balanceo.", "Pasos cortos: sonidos más frecuentes y balanceo base más rápido a igual velocidad.", "Pasos largos: sonidos menos frecuentes y balanceo base más lento a igual velocidad."},
  {"Weg zwischen Schritten in dieser Haltung; liefert auch den Grundrhythmus des Wippens.", "Kurze Schritte: häufigere Töne und schnellerer Grundrhythmus bei gleichem Tempo.", "Lange Schritte: seltenere Töne und langsamerer Grundrhythmus bei gleichem Tempo."},
  {"Distanza fra passi in questa postura; alimenta anche il ritmo base dell'oscillazione.", "Passi corti: suoni più frequenti e ritmo base più rapido a pari velocità.", "Passi lunghi: suoni meno frequenti e ritmo base più lento a pari velocità."},
  {"Sträcka mellan steg i denna ställning; styr även gungningens grundrytm.", "Korta steg: tätare ljud och snabbare grundgung vid samma fart.", "Långa steg: glesare ljud och långsammare grundgung vid samma fart."},
 },
 /* runstride */ {
  {"Extra stride length as speed rises, while standing.", "No extra stride length when running.", "Adds up to 3000 mm; more distance between sounds and slower base bob at equal speed."},
  {"Longueur de pas ajoutée quand la vitesse augmente, debout.", "Aucune longueur ajoutée en course.", "Ajoute jusqu'à 3000 mm ; sons plus espacés et rythme de base plus lent à vitesse égale."},
  {"Longitud extra de paso al ganar velocidad estando de pie.", "Sin alargar el paso al correr.", "Añade hasta 3000 mm; más distancia entre sonidos y balanceo base más lento a igual velocidad."},
  {"Zusätzliche Schrittlänge bei steigendem Tempo im Stehen.", "Keine zusätzliche Laufschrittlänge.", "Bis zu 3000 mm mehr; größere Tonabstände und langsamerer Grundrhythmus bei gleichem Tempo."},
  {"Lunghezza extra del passo all'aumentare della velocità, in piedi.", "Nessun allungamento del passo correndo.", "Fino a 3000 mm extra; suoni più distanti e ritmo base più lento a pari velocità."},
  {"Extra steglängd när farten stiger i stående ställning.", "Ingen extra steglängd vid löpning.", "Upp till 3000 mm extra; glesare ljud och långsammare grundgung vid samma fart."},
 },
 /* volume */ {
  {"Footstep loudness on the indicated surface; still follows main SFX volume.", "Practically silent footsteps on this surface.", "Full footstep level on this surface, before main SFX volume."},
  {"Volume des pas sur la surface indiquée ; dépend du volume général des effets.", "Pas pratiquement silencieux sur cette surface.", "Niveau complet des pas, avant le volume général des effets."},
  {"Volumen de pasos en la superficie indicada; respeta el volumen general de SFX.", "Pasos prácticamente silenciosos en esa superficie.", "Nivel completo de pasos en esa superficie, antes del volumen general de SFX."},
  {"Schrittlautstärke auf angegebener Oberfläche; folgt der allgemeinen Effektlautstärke.", "Praktisch lautlose Schritte auf dieser Oberfläche.", "Voller Schrittpegel vor der allgemeinen Effektlautstärke."},
  {"Volume dei passi sulla superficie indicata; segue il volume generale degli effetti.", "Passi praticamente silenziosi su questa superficie.", "Livello pieno dei passi, prima del volume generale degli effetti."},
  {"Fotstegsvolym på angiven yta; följer huvudvolymen för ljudeffekter.", "Praktiskt taget tysta steg på denna yta.", "Full fotstegsnivå före huvudvolymen för ljudeffekter."},
 },
 /* crouchvol */ {
  {"Amount subtracted from footstep volume when crouched; not a percentage.", "Crouching does not reduce step volume.", "Subtracts 60 volume units; much quieter crouched steps."},
  {"Valeur soustraite au volume des pas accroupis ; pas un pourcentage.", "Aucune réduction du volume accroupi.", "Retire 60 unités ; pas accroupis bien plus discrets."},
  {"Cantidad que se resta al volumen de pasos al agacharte; no es un porcentaje.", "Agacharse no reduce el volumen de pasos.", "Resta 60 unidades de volumen; pasos agachados mucho más silenciosos."},
  {"Beim Ducken abgezogener Schrittlautstärkewert; kein Prozentsatz.", "Ducken senkt die Schrittlautstärke nicht.", "Zieht 60 Lautstärkeeinheiten ab; deutlich leisere Duckschritte."},
  {"Valore sottratto al volume dei passi accovacciati; non una percentuale.", "Accovacciarsi non riduce il volume dei passi.", "Sottrae 60 unità; passi accovacciati molto più silenziosi."},
  {"Belopp som dras från fotstegsvolymen i huk; inte procent.", "Hukning sänker inte stegvolymen.", "Drar av 60 volymenheter; mycket tystare steg i huk."},
 },
 /* pitch */ {
  {"Pitch of footsteps on this material, using the existing sample. Also changes sample playback speed.", "Deeper and slower footstep sound.", "Higher and faster footstep sound."},
  {"Tonalité des pas sur ce matériau avec le son existant. Change aussi sa vitesse de lecture.", "Pas plus graves et plus lents.", "Pas plus aigus et plus rapides."},
  {"Tono de pasos sobre este material usando el sonido existente. También cambia la velocidad de la muestra.", "Sonido de paso más grave y lento.", "Sonido de paso más agudo y rápido."},
  {"Schritttonhöhe auf diesem Material mit vorhandenem Klang; ändert auch die Abspielgeschwindigkeit.", "Tieferer und langsamerer Schrittklang.", "Höherer und schnellerer Schrittklang."},
  {"Tono dei passi su questo materiale usando il suono esistente; cambia anche la velocità del campione.", "Suono più grave e lento.", "Suono più acuto e rapido."},
  {"Fotstegens tonhöjd på materialet med befintligt ljud; ändrar även uppspelningsfarten.", "Djupare och långsammare stegljud.", "Högre och snabbare stegljud."},
 },
 /* pitchvar */ {
  {"Random pitch change around each material's base pitch to avoid identical steps.", "No random variation; same pitch for that material.", "Random change up to 256 units above/below the material's pitch."},
  {"Variation aléatoire autour de la tonalité du matériau pour éviter des pas identiques.", "Aucune variation aléatoire ; même tonalité pour ce matériau.", "Variation jusqu'à 256 unités au-dessus/en dessous du ton de base."},
  {"Variación aleatoria alrededor del tono de cada material para evitar pasos idénticos.", "Sin variación aleatoria; mismo tono para ese material.", "Cambio aleatorio de hasta 256 unidades arriba/abajo del tono del material."},
  {"Zufällige Tonänderung um den Materialgrundton gegen identische Schritte.", "Keine Zufallsvariation; gleicher Ton für dieses Material.", "Zufallsänderung bis 256 Einheiten über/unter dem Materialton."},
  {"Variazione casuale attorno al tono del materiale per evitare passi identici.", "Nessuna variazione casuale; stesso tono per quel materiale.", "Variazione fino a 256 unità sopra/sotto il tono del materiale."},
  {"Slumpad tonändring kring materialets grundton för att undvika identiska steg.", "Ingen slumpvariation; samma ton för materialet.", "Slumpändring upp till 256 enheter över/under materialets ton."},
 },
};
static const unsigned char MotionHelpStyle[MP_COUNT] = {
 [MP_SPEED] = 0,
 [MP_SPEED_RESPONSE] = 1,
 [MP_ZOOM_MIN] = 2,
 [MP_WB_RATE] = 3,
 [MP_WB_MAX_HZ] = 4,
 [MP_WB_X] = 5,
 [MP_WB_Y] = 5,
 [MP_WB_Z] = 5,
 [MP_WB_ROLL] = 5,
 [MP_WB_RESPONSE] = 1,
 [MP_BOB_CROUCH] = 6,
 [MP_HEAD_RATE] = 3,
 [MP_HEAD_X] = 5,
 [MP_HEAD_Y] = 5,
 [MP_HEAD_ROLL] = 5,
 [MP_SWAY_RESPONSE] = 1,
 [MP_SWAY_GAIN] = 7,
 [MP_SWAY_LIMIT] = 8,
 [MP_SWAY_X] = 9,
 [MP_SWAY_Y] = 9,
 [MP_SWAY_PITCH] = 9,
 [MP_SWAY_YAW] = 9,
 [MP_SWAY_ROLL] = 9,
 [MP_SWAY_CAMERA] = 10,
 [MP_LOOK_GAIN] = 11,
 [MP_LOOK_RESPONSE] = 1,
 [MP_MOVE_RESPONSE] = 1,
 [MP_MOVE_SIDE] = 12,
 [MP_MOVE_DEPTH] = 13,
 [MP_MOVE_ROLL] = 14,
 [MP_EYE_RESPONSE] = 1,
 [MP_STANCE_GAIN] = 15,
 [MP_STANCE_LIMIT] = 16,
 [MP_STANCE_RESPONSE] = 1,
 [MP_STAIR_RESPONSE] = 1,
 [MP_STAIR_HEIGHT] = 17,
 [MP_STAIR_WEAPON] = 18,
 [MP_IMPACT_RESPONSE] = 19,
 [MP_JUMP_IMPULSE] = 20,
 [MP_LAND_IMPULSE] = 20,
 [MP_IMPACT_LIMIT] = 21,
 [MP_IMPACT_WEAPON] = 22,
 [MP_IMPACT_WLIMIT] = 23,
 [MP_IMPACT_ROLL] = 24,
 [MP_RELOAD_RESPONSE] = 1,
 [MP_RELOAD_X] = 25,
 [MP_RELOAD_Y] = 25,
 [MP_RELOAD_Z] = 25,
 [MP_RELOAD_PITCH] = 25,
 [MP_RELOAD_ROLL] = 25,
 [MP_RELOAD_LIMIT] = 26,
 [MP_RELOAD_ANGLE] = 27,
 [MP_RELOAD_WEAPON] = 28,
 [MP_RELOAD_GUN_0] = 29,
 [MP_RELOAD_GUN_1] = 29,
 [MP_RELOAD_GUN_2] = 29,
 [MP_RELOAD_GUN_3] = 29,
 [MP_RELOAD_GUN_4] = 29,
 [MP_RELOAD_GUN_5] = 29,
 [MP_RELOAD_GUN_6] = 29,
 [MP_RELOAD_GUN_7] = 29,
 [MP_RELOAD_GUN_8] = 29,
 [MP_STEPS_ENABLE] = 30,
 [MP_STEP_LENGTH] = 31,
 [MP_RUN_LENGTH] = 32,
 [MP_CROUCH_LENGTH] = 31,
 [MP_VOL_GENERIC] = 33,
 [MP_VOL_METAL] = 33,
 [MP_VOL_SOFT] = 33,
 [MP_VOL_WOOD] = 33,
 [MP_VOL_WATER] = 33,
 [MP_VOL_CROUCH] = 34,
 [MP_PITCH_METAL] = 35,
 [MP_PITCH_SOFT] = 35,
 [MP_PITCH_WOOD] = 35,
 [MP_PITCH_VARIATION] = 36,
};
static const char *MotionResponseDescription(int id,int language)
{
 switch(id){
 case MP_SPEED_RESPONSE: { static const char *const t[6]={"Smooths changes in walking/running effect intensity as speed changes.","Lisse les changements d'intensité des effets quand la vitesse change.","Suaviza los cambios de intensidad al ganar o perder velocidad caminando/corriendo.","Glättet Änderungen der Bewegungsstärke bei Tempowechseln.","Smussa i cambi di intensità degli effetti quando cambia la velocità.","Jämnar ut ändringar i rörelseintensitet när farten ändras."}; return t[language]; }
 case MP_WB_RESPONSE: { static const char *const t[6]={"Smooths the three weapon bob translations; does not smooth its side tilt.","Lisse les trois déplacements du balancement de l'arme, pas son roulis.","Suaviza los tres desplazamientos del balanceo del arma; no suaviza su inclinación lateral.","Glättet die drei Waffenwipp-Verschiebungen, nicht die Seitenneigung.","Smussa i tre spostamenti dell'oscillazione dell'arma, non la sua inclinazione laterale.","Jämnar ut vapnets tre gungförskjutningar, inte sidolutningen."}; return t[language]; }
 case MP_SWAY_RESPONSE: { static const char *const t[6]={"Smooths how weapon sway follows changes in mouse turning speed.","Lisse la réaction de l'arme aux changements de vitesse de rotation souris.","Suaviza cómo el sway del arma sigue los cambios de velocidad de giro del mouse.","Glättet den Waffen-Nachlauf bei Änderungen der Mausdrehgeschwindigkeit.","Smussa l'inerzia dell'arma ai cambi di velocità di rotazione del mouse.","Jämnar ut vapentrögheten när musens vridhastighet ändras."}; return t[language]; }
 case MP_LOOK_RESPONSE: { static const char *const t[6]={"Smooths weapon pointing when looking up or down, including its return.","Lisse l'orientation de l'arme en regardant en haut/bas, y compris son retour.","Suaviza la orientación del arma al mirar arriba/abajo, incluido su regreso.","Glättet die Waffenausrichtung beim Blick auf/ab, auch die Rückkehr.","Smussa l'orientamento dell'arma guardando su/giù, compreso il ritorno.","Jämnar ut vapenriktningen vid blick upp/ned, även återgången."}; return t[language]; }
 case MP_MOVE_RESPONSE: { static const char *const t[6]={"Smooths weapon inertia when starting, stopping or changing movement direction.","Lisse l'inertie de l'arme au départ, à l'arrêt ou au changement de direction.","Suaviza la inercia del arma al iniciar, frenar o cambiar la dirección de desplazamiento.","Glättet Waffenträgheit beim Anlaufen, Stoppen und Richtungswechsel.","Smussa l'inerzia dell'arma iniziando, fermandosi o cambiando direzione.","Jämnar ut vapentröghet vid start, stopp och riktningsbyte."}; return t[language]; }
 case MP_EYE_RESPONSE: { static const char *const t[6]={"Smooths camera height when crouching and standing; keeps the final height.","Lisse la hauteur caméra en s'accroupissant/se relevant, sans changer la hauteur finale.","Suaviza la altura de cámara al agacharte y levantarte; conserva la altura final.","Glättet die Kamerahöhe beim Ducken/Aufstehen; die Endhöhe bleibt gleich.","Smussa l'altezza della telecamera accovacciandosi/alzandosi, senza cambiare l'altezza finale.","Jämnar ut kamerahöjden vid hukning/uppresning; sluthöjden ändras inte."}; return t[language]; }
 case MP_STANCE_RESPONSE: { static const char *const t[6]={"Smooths the weapon tug when crouching/standing, separately from camera height smoothing.","Lisse le mouvement de l'arme en s'accroupissant, séparément du lissage caméra.","Suaviza el tirón del arma al agacharte/levantarte, separado del suavizado de altura de cámara.","Glättet den Waffenruck beim Ducken/Aufstehen, getrennt von der Kamerahöhe.","Smussa l'impulso dell'arma accovacciandosi/alzandosi, separato dall'altezza telecamera.","Jämnar ut vapenryck vid hukning/uppresning, separat från kamerahöjden."}; return t[language]; }
 case MP_STAIR_RESPONSE: { static const char *const t[6]={"How quickly the camera catches up to a new step height after absorbing the initial jolt.","Vitesse de retour caméra à la hauteur d'une marche après absorption du choc initial.","Qué tan pronto la cámara alcanza la nueva altura del escalón tras absorber el golpe inicial.","Wie schnell die Kamera nach dem Abfangen des Stoßes die neue Stufenhöhe erreicht.","Quanto rapidamente la telecamera raggiunge l'altezza del gradino dopo aver assorbito lo scatto.","Hur snabbt kameran når den nya steghöjden efter att ha dämpat rycket."}; return t[language]; }
 case MP_RELOAD_RESPONSE: { static const char *const t[6]={"Smooths sampled reload movement before applying the camera and weapon multipliers.","Lisse le mouvement mesuré du rechargement avant les multiplicateurs caméra/arme.","Suaviza el movimiento medido de recarga antes de aplicar sus multiplicadores de cámara y arma.","Glättet gemessene Nachladebewegung vor den Kamera- und Waffenfaktoren.","Smussa il movimento misurato di ricarica prima dei moltiplicatori di telecamera e arma.","Jämnar ut uppmätt omladdningsrörelse före kamera- och vapenfaktorerna."}; return t[language]; }
 default: return NULL;
 }
}
static const char *const MotionJumpHelp[6][6][3]={
 {
  {"Smooths the start and return of this layer only. Duration is controlled separately.","Very gradual response with more delay; the peak may be smaller.","Faster, firmer tracking of the pulse, while remaining continuous."},
  {"Lisse le début et le retour de cette couche seule. La durée se règle séparément.","Réaction très progressive avec plus de retard ; le pic peut diminuer.","Suit l'impulsion plus vite et fermement, sans rupture."},
  {"Suaviza el inicio y el regreso solo de esta capa. La duración se ajusta por separado.","Respuesta muy gradual y con más retraso; el pico puede ser menor.","Sigue el impulso más rápido y firme, manteniendo la continuidad."},
  {"Glättet Start und Rückkehr nur dieser Ebene. Die Dauer ist separat einstellbar.","Sehr allmähliche Reaktion mit mehr Verzögerung; die Spitze kann kleiner sein.","Folgt dem Impuls schneller und straffer, aber weiterhin kontinuierlich."},
  {"Smussa inizio e ritorno solo di questo livello. La durata si regola separatamente.","Risposta molto graduale e ritardata; il picco può ridursi.","Segue l'impulso più rapidamente e con fermezza, senza scatti."},
  {"Jämnar ut start och återgång enbart för detta lager. Tiden ställs in separat.","Mycket gradvis respons med mer fördröjning; toppen kan bli mindre.","Följer impulsen snabbare och fastare, men fortfarande kontinuerligt."},
 },
 {
  {"Strength of this cosmetic jump or landing pulse. Independent of the other layer; no change to jump physics.","No pulse from this event.","Strongest pulse; the travel limit and smoothing still shape the result."},
  {"Force de cette impulsion visuelle de saut ou réception. Indépendante de l'autre couche ; physique inchangée.","Aucune impulsion pour cet événement.","Impulsion maximale, toujours réglée par la limite et le lissage."},
  {"Fuerza del gesto visual de salto o aterrizaje. Independiente de la otra capa; no cambia la física del salto.","Este evento no genera impulso.","Impulso más fuerte; el límite de recorrido y el suavizado aún definen el resultado."},
  {"Stärke dieses visuellen Sprung- oder Landungsimpulses. Unabhängig von der anderen Ebene; keine Änderung der Physik.","Kein Impuls bei diesem Ereignis.","Stärkster Impuls; Weggrenze und Glättung bestimmen weiterhin das Ergebnis."},
  {"Forza di questo impulso visivo di salto o atterraggio. Indipendente dall'altro livello; fisica invariata.","Nessun impulso per questo evento.","Impulso massimo, sempre regolato dal limite e dallo smussamento."},
  {"Styrkan på denna visuella hopp- eller landningsimpuls. Oberoende av det andra lagret; fysiken ändras inte.","Ingen impuls vid denna händelse.","Starkaste impulsen; rörelsegräns och utjämning formar fortfarande resultatet."},
 },
 {
  {"Duration of this layer's pulse, including its gradual rise and fall. Does not change time spent in the air.","A brief pulse lasting 0.2 seconds.","A broad pulse lasting 2 seconds, with a slower rise and fall."},
  {"Durée de l'impulsion de cette couche, montée et descente progressives comprises. Ne change pas le temps en l'air.","Impulsion brève de 0,2 seconde.","Impulsion ample de 2 secondes, montée et descente plus lentes."},
  {"Duración del impulso de esta capa, incluida su subida y bajada gradual. No cambia el tiempo que pasas en el aire.","Gesto breve de 0,2 segundos.","Gesto prolongado de 2 segundos, con subida y bajada más lentas."},
  {"Dauer des Impulses dieser Ebene samt sanftem Anstieg und Abfall. Ändert nicht die Flugzeit.","Kurzer Impuls von 0,2 Sekunden.","Breiter Impuls von 2 Sekunden mit langsamerem Anstieg und Abfall."},
  {"Durata dell'impulso di questo livello, inclusi salita e ritorno graduali. Non cambia il tempo in aria.","Impulso breve di 0,2 secondi.","Impulso ampio di 2 secondi con salita e ritorno più lenti."},
  {"Impulsens längd för detta lager, inklusive mjuk uppgång och nedgång. Ändrar inte tiden i luften.","Kort impuls på 0,2 sekunder.","Lång impuls på 2 sekunder med långsammare uppgång och nedgång."},
 },
 {
  {"Soft cap on jump/landing travel for this layer only. It also bounds the side tilt driven by that travel.","Removes this layer's added travel and associated tilt; the other layer remains active.","Allows up to 200 mm (20 cm) of travel, depending on the impulse."},
  {"Limite douce du déplacement de saut/réception de cette couche seule. Borne aussi le roulis lié au déplacement.","Retire ce déplacement et son roulis ; l'autre couche reste active.","Autorise jusqu'à 200 mm (20 cm) selon l'impulsion."},
  {"Tope suave del recorrido de salto/aterrizaje solo de esta capa. También limita la inclinación que nace de ese recorrido.","Quita el desplazamiento extra y su inclinación de esta capa; la otra sigue activa.","Permite hasta 200 mm (20 cm) de recorrido, según el impulso."},
  {"Weiche Weggrenze beim Springen/Landen nur für diese Ebene. Begrenzt auch die daraus folgende Seitenneigung.","Entfernt diesen Zusatzweg und seine Neigung; die andere Ebene bleibt aktiv.","Erlaubt bis zu 200 mm (20 cm) Weg, abhängig vom Impuls."},
  {"Limite morbido del movimento di salto/atterraggio solo di questo livello. Limita anche l'inclinazione legata al movimento.","Toglie questo movimento e la sua inclinazione; l'altro livello resta attivo.","Consente fino a 200 mm (20 cm) di movimento, secondo l'impulso."},
  {"Mjuk rörelsegräns vid hopp/landning enbart för detta lager. Begränsar även sidolutningen som följer rörelsen.","Tar bort lagrets extra rörelse och lutning; det andra lagret är aktivt.","Tillåter upp till 200 mm (20 cm) rörelse beroende på impulsen."},
 },
 {
  {"Overall multiplier for the weapon's own jump/landing pulses. It no longer copies camera movement.","No added weapon jump/landing reaction; camera settings are unaffected.","Uses the full weapon pulse, subject only to weapon limit and smoothing."},
  {"Multiplicateur des impulsions de saut/réception propres à l'arme. Ne copie plus le mouvement caméra.","Aucune réaction ajoutée de l'arme ; caméra inchangée.","Impulsion complète de l'arme, bornée par ses propres limite et lissage."},
  {"Multiplicador de los impulsos propios del arma al saltar/aterrizar. Ya no copia el movimiento de la cámara.","Sin reacción extra de salto/aterrizaje del arma; no afecta la cámara.","Usa todo el impulso del arma, sujeto solo a su límite y suavizado."},
  {"Gesamtfaktor für die eigenen Sprung-/Landungsimpulse der Waffe. Kopiert nicht mehr die Kamerabewegung.","Keine zusätzliche Waffenreaktion; Kameraeinstellungen bleiben unverändert.","Nutzt den vollen Waffenimpuls mit eigener Weggrenze und Glättung."},
  {"Moltiplicatore degli impulsi propri dell'arma al salto/atterraggio. Non copia più il movimento della telecamera.","Nessuna reazione extra dell'arma; telecamera invariata.","Usa tutto l'impulso dell'arma, con il proprio limite e smussamento."},
  {"Faktor för vapnets egna hopp-/landningsimpulser. Kopierar inte längre kamerarörelsen.","Ingen extra vapenreaktion; kamerans inställningar påverkas inte.","Använder hela vapenimpulsen med vapnets egen gräns och utjämning."},
 },
 {
  {"Side tilt per millimetre of this layer's jump/landing travel. Jump and landing tilt in opposite directions.","No extra side tilt.","Strongest side tilt for the same travel; requires a nonzero travel limit."},
  {"Roulis par millimètre du déplacement de saut/réception de cette couche. Saut et réception penchent en sens opposés.","Aucun roulis ajouté.","Roulis maximal pour le même déplacement ; nécessite une limite non nulle."},
  {"Inclinación lateral por milímetro de recorrido de esta capa. Salto y aterrizaje inclinan hacia lados opuestos.","Sin inclinación lateral extra.","Mayor inclinación para el mismo recorrido; necesita un límite de recorrido mayor que cero."},
  {"Seitenneigung pro Millimeter Sprung-/Landungsweg dieser Ebene. Sprung und Landung neigen in Gegenrichtungen.","Keine zusätzliche Seitenneigung.","Stärkste Neigung bei gleichem Weg; benötigt eine Weggrenze über null."},
  {"Inclinazione laterale per millimetro di movimento di questo livello. Salto e atterraggio inclinano in direzioni opposte.","Nessuna inclinazione laterale extra.","Massima inclinazione a parità di movimento; richiede un limite maggiore di zero."},
  {"Sidolutning per millimeter hopp-/landningsrörelse i detta lager. Hopp och landning lutar åt motsatta håll.","Ingen extra sidolutning.","Största lutning för samma rörelse; kräver en rörelsegräns över noll."},
 },
};
const char *MotionParamHelp(int id,int language)
{
 static char text[1536];
 static const char *const lo[6]={"Min","Min","Mín","Min","Min","Min"};
 static const char *const hi[6]={"Max","Max","Máx","Max","Max","Max"};
 const char *const *parts;
 const MOTION_PARAM *param;
 const char *description;
 if(id<0||id>=MP_COUNT)return "";
 if(language<0||language>=6)language=0;
 param=&MotionParams[id];parts=MotionHelpPatterns[MotionHelpStyle[id]][language];
 if(id>=MP_IMPACT_WRESPONSE){
  int style=id==MP_IMPACT_WRESPONSE?0:id==MP_WEAPON_NEAR?1:id==MP_EQUIP_RESPONSE?3:id==MP_EQUIP_LIMIT?4:id==MP_EQUIP_ANGLE?5:2;
  parts=MotionExtraHelp[style][language];
 }
 if(id>=MP_JUMP_DURATION){
  int style=id<=MP_LAND_DURATION?0:id==MP_EQUIP_CAMERA?1:id==MP_EQUIP_CRESPONSE?2:id==MP_EQUIP_CLIMIT?3:4;
  parts=MotionNewHelp[style][language];
 }
 switch(id){
 case MP_IMPACT_RESPONSE:case MP_IMPACT_WRESPONSE:parts=MotionJumpHelp[0][language];break;
 case MP_JUMP_IMPULSE:case MP_LAND_IMPULSE:case MP_W_JUMP_IMPULSE:case MP_W_LAND_IMPULSE:parts=MotionJumpHelp[1][language];break;
 case MP_JUMP_DURATION:case MP_LAND_DURATION:case MP_W_JUMP_DURATION:case MP_W_LAND_DURATION:parts=MotionJumpHelp[2][language];break;
 case MP_IMPACT_LIMIT:case MP_IMPACT_WLIMIT:parts=MotionJumpHelp[3][language];break;
 case MP_IMPACT_WEAPON:parts=MotionJumpHelp[4][language];break;
 case MP_IMPACT_ROLL:case MP_W_IMPACT_ROLL:parts=MotionJumpHelp[5][language];break;
 default:break;
 }
 description=MotionResponseDescription(id,language);
 if(!description)description=parts[0];
 snprintf(text,sizeof text,"%s. %s %s %.4g %s: %s %s %.4g %s: %s",
  param->label[language],description,lo[language],param->min,param->unit,parts[1],hi[language],param->max,param->unit,parts[2]);
 return text;
}
