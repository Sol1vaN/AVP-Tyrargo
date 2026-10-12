# Instrumentaciones conservadas â€” AVP: Tyrargo 6.249 RC1



La probe 248 corrige el movimiento del escuadrón, amplía la exploración conjunta y agrega armas recuperables de marines muertos. Conserva las mediciones de latencias/eventos/audio de la base 247. Las trazas y comprobaciones del laboratorio de esta probe se compilan únicamente en las copias de diagnóstico; no están en el ejecutable portable. Los editores de desarrollo siguen desactivados. Sus implementaciones permanecen disponibles para una futura compilaciÃ³n de calibraciÃ³n. Los argumentos antiguos se aceptan sin activar herramientas; no restauran CSV ni muestran menÃºs ocultos en esta entrega.



| Sistema | Estado de entrega / ubicaciÃ³n | ReactivaciÃ³n deliberada |

|---|---|---|

| Viewmodels y origen/orientaciÃ³n de flamethrower (`-dev`) | `TYRARGO_DEVELOPMENT_TOOLS=0`, `src/tyrargo_instrumentation.h`; cÃ³digo en `src/dev_viewmodel.c` | Cambiar a 1, recompilar y ejecutar `avpt.exe -dev`. MenÃº Controles; sliders y export. |

| HUD shake (`-dev2`, experimental feats 2) | `SHAKE_CALIBRATION_AVAILABLE=0`, `src/shake_params.h` | Cambiar a 1, recompilar y ejecutar `avpt.exe -dev2`. |

| Experimental feats original, cÃ¡mara marine | `TYRARGO_MOTION_CALIBRATION=0`; `src/motion_params.c`, menÃºs `avp_menus.c`/`avp_menudata.c` | Cambiar a 1, restaurar el acceso al menÃº original en Controles y recompilar. |

| Comandos de depuraciÃ³n y consola Windows (`-debug`, `-windebug`) | `TYRARGO_DEVELOPMENT_TOOLS=0`, `src/main.c` | CompilaciÃ³n de desarrollo con el indicador a 1; usar el argumento correspondiente. |

| Trazas de render, contexto y crash de probes 113/114/124 | `TYRARGO_RENDER_TRACE=0`, `src/main.c` | Activar el indicador y recompilar. Revisar destinos de las trazas antes de distribuir. |

| MediciÃ³n de latencias/eventos/audio | `TYRARGO_MEASURE_LATENCY=1`, `src/event_latency.h` | Activa automÃ¡ticamente en esta probe; `src/event_latency.c` y `src/audio_latency.c`. Restaurar a 0 en `src/event_latency.h` para desactivarla. |

| Contadores de consultas MCI/CD | Condicionados a `TYRARGO_RENDER_TRACE` en `src/cdplayer.c` | Activar trazas y recompilar. ReproducciÃ³n musical no depende de estos contadores. |

| Traza musical antigua Probe140I11 | FunciÃ³n sin llamadores, ahora ademÃ¡s excluida con `#if 0` en `src/avp/cdtrackselection.cpp` | Restaurar bloque y llamadas de diagnÃ³stico explÃ­citas si se necesita. |



## CSV y parÃ¡metros calibrados



- `logs/dev_viewmodel_z.csv`: posiciones de armas; sÃ³lo se lee con el editor habilitado.

- `logs/dev_flamethrower.csv`: X/Y/Z y tres Ã¡ngulos; sÃ³lo con editor habilitado. Controles W/S adelante/atrÃ¡s, A/D izquierda/derecha, Q/E abajo/arriba. Export conserva ambos CSV del editor.

- `logs/marine_camera_motion.csv`: cÃ¡mara y armas. Su lectura automÃ¡tica ahora estÃ¡ desactivada aunque exista un CSV antiguo.

- `logs/marine_hud_shake.csv`: calibraciÃ³n del HUD; sÃ³lo con `-dev2` habilitado en una compilaciÃ³n de desarrollo.



Los valores importados siguen en el source (`dev_viewmodel.c`, `motion_params.def`, `shake_params.def`). Desactivar los editores no desactiva los efectos ni los sliders pÃºblicos de intensidad. Flamethrower conserva posiciÃ³n (-44, 6, 0) y Ã¡ngulos (0, 0, 0).



## DiagnÃ³sticos heredados ya apagados



`OPENAL_DEBUG` estÃ¡ dentro de `#if 0` en `src/openal.c`. `PROFILING_ON` y `PENTIUM_PROFILING_ON` estÃ¡n a 0. `logGenData` estÃ¡ a 0 en `src/avp/bh_gener.c`. El registro `net.log`, el espejo `ConsoleLog.txt`, `TimeStampedMessage` y la traza `CDDA_RandomMusicTracef` ya estaban desactivados o eran no-op. No se afirma que estuvieran midiendo en la entrega anterior.



Se conservan los mensajes ordinarios de errores (incluido `dx_error.log` en rutas de error), la consola visible, las notificaciones y el contador FPS solicitado por el jugador. TambiÃ©n permanecen las protecciones funcionales para restaurar el contexto OpenGL y cambiar modo de vÃ­deo: no son instrumentaciÃ³n. No se borran logs/CSV existentes, perfiles, configuraciones ni partidas.



## ValidaciÃ³n



Las pruebas `05_tools/02_tests.py` y `05_tools/03_tests.py` compilan los mÃ³dulos reales de latencia. Verifican lÃ­mites, aislamiento del hilo principal, conservaciÃ³n entre ejecuciones y ausencia de escrituras del logger durante frames. Las herramientas de calibraciÃ³n siguen desactivadas como en las probes anteriores. La documentaciÃ³n describe reactivaciÃ³n por cambios deliberados de source, no opciones disponibles en el ejecutable de entrega.





## Probe 225: diagnÃ³stico de tirones



Se mide la partida, el frontend, entradas/cambios de menÃºs, guardado completo de perfiles (snapshot, serializaciÃ³n y cierre), configuraciÃ³n de vÃ­deo y aplicaciÃ³n de modo. Se mantienen zona/PVS, triggers, AI, colisiones, cargas de archivos/archivos empaquetados, uploads de texturas, audio y presentaciÃ³n/limitador ya instrumentados.



Los cambios de foco/minimizaciÃ³n/restauraciÃ³n permanecen en el historial aunque el frame sea rÃ¡pido. Cada frame registrado lleva contexto de menÃº/foco, memoria residente/privada, fallos de pÃ¡gina y deltas de I/O del proceso. Los fallos de pÃ¡gina incluyen tanto soft como hard; estos contadores por sÃ­ solos no prueban fragmentaciÃ³n ni lecturas del disco. Las duraciones son inclusivas y de CPU wall time, no tiempo GPU. Las esperas del limitador/VSync y frames sin foco se identifican por separado.



Destino: `event_latency.log` y, si hay audio lento, `sfx_latency.log` en el directorio local `.avp` configurado por el juego. Buffer fijo: 128 eventos por frame y 8192 de historial. Sobreescrituras/omisiones se reportan en el encabezado. Sin allocaciones o escrituras del logger durante los frames: exporta al salir del frontend, terminar/reiniciar una misiÃ³n o salir normalmente. La salida normal SDL_QUIT tambiÃ©n termina y exporta el frame activo antes de cerrar SDL. Un cierre forzado puede perder el tramo aÃºn almacenado en memoria.



No se reactivan `-dev`, `-dev2`, calibraciÃ³n de cÃ¡mara ni trazas antiguas de render/contexto/crash que escribÃ­an durante el recorrido. No se alteran saves, preferencias, fÃ­sica, dificultad, LOD o calidad visual. La probe es de diagnÃ³stico, no afirma reparar ni identificar todavÃ­a las pausas observadas.





## Probe 226: sesiones y cobertura de pausas



Los dos archivos siguen usando FILEMODE_APPEND. No se truncan al arrancar. Cada export incluye versiÃ³n y una sesiÃ³n compartida entre event_latency.log y sfx_latency.log: fecha UTC con milisegundos, PID e identificador preciso de inicio. Varias exportaciones de una misma ejecuciÃ³n conservan el mismo identificador. Las horas t= siguen siendo relativas a SDL; no comparar esos tiempos entre ejecuciones distintas sin mirar la sesiÃ³n.



Cada frame de partida retenido incluye posiciÃ³n del jugador en milÃ­metros y arma seleccionada. Se desglosan las operaciones antes no medidas de UpdateGame (daemon/consola, feromonas, inventario/mantenimiento del jugador, eliminaciÃ³n de objetos, etc.), manejo de pickups, preparaciÃ³n/transformaciÃ³n/pipeline de shapes y cambios de estado de texturas OpenGL. PostLandscapeRendering incluye espejos/lluvia/agua, no solamente partÃ­culas: ahora sus operaciones de Derelict se miden por separado. No se fuerza sincronizaciÃ³n GPU para medir.



El logger continÃºa acotado en memoria y exportando entre recorridos o al cerrar normalmente. El cierre forzado puede perder eventos aÃºn no exportados; overwritten y omitted siguen describiendo pÃ©rdida por lÃ­mites del buffer. Mantener logs histÃ³ricos en disco no significa registrar cada frame normal.



Esta entrega aplica ademÃ¡s dos cambios funcionales: detecciÃ³n vertical del cuerpo del jugador en sensores de puertas de proximidad, conservando alcance horizontal y bloqueos; fila de carga rÃ¡pida celeste translÃºcida y textos rojos, sin cambiar otros slots. No se afirma haber reparado las pausas cuya operaciÃ³n interna aÃºn no se distinguÃ­a en Probe 225.





## Probe 228: correcciÃ³n de crash en la instrumentaciÃ³n



El volcado Windows de Probe 227 (03-10-2026 17:24:35, PID 25136) confirma una lectura invÃ¡lida de 0xFE en EventLatencyEnd.part.0, RVA 0x20fb60, desde HandleParticleSystem. La llamada de efectos utilizaba DISPLAYBLOCK.name; los actores creados desde MODULE temporales podÃ­an heredar un name sin inicializar. Ahora se inicializan los 16 MODULE temporales y esa mediciÃ³n utiliza exclusivamente el nombre del mÃ³dulo real ObMyModule si existe, ademÃ¡s del ID binario SBname. Los actores dinÃ¡micos se identifican por tipo e ID, sin interpretar su name como texto.



El logger sigue activo, acotado y acumulando sesiones como antes. No se aÃ±ade escritura por frame, sondeo de memoria ni captura de excepciones para ocultar el problema.





## Probe 229: checkpoints persistentes de carga



`EventLatencyLoadCheckpoint` en `src/event_latency.c` escribe `.avp/load_progress.log` con FILEMODE_APPEND, versiÃ³n y sesiÃ³n, cerrando cada checkpoint. SÃ³lo lo llaman selecciÃ³n confirmada de carga, carga de mapas, RestartLevel y LoadSavedGame. ExcepciÃ³n explÃ­cita a la polÃ­tica de mantener el logger en memoria: son transiciones sÃ­ncronas de carga, no frames ordinarios de gameplay. No hay escritura por actor ni por bloque del save. No se instala un watchdog ni un manejador de excepciones.



Para apagarlo junto con las otras mediciones, poner TYRARGO_MEASURE_LATENCY=0 en src/event_latency.h y recompilar. -dev y -dev2 siguen deshabilitados. Los logs anteriores siguen acumulÃ¡ndose sin truncarse; un cierre forzado todavÃ­a puede perder el historial de rendimiento en memoria, pero no los checkpoints ya cerrados.



AppHang 6.227 del 03-10-2026 20:47:10: causa exacta pendiente, sin volcado de ese bloqueo. Se aÃ±ade cobertura para distinguir selecciÃ³n, carga del mapa, destrucciÃ³n de objetos, reconstrucciÃ³n, FMVs, lectura, descompresiÃ³n y restauraciÃ³n. Las protecciones de longitud de bloque evitan avances nulos/negativos; no se atribuyen al quicksave inspeccionado, cuyos lÃ­mites resultaron vÃ¡lidos.





## Probe 230: preferencias visuales del menÃº



No se aÃ±ade instrumentaciÃ³n de juego. Se conserva el logger y los checkpoints de carga de Probe 229, con sesiones acumuladas y los mismos interruptores; -dev y -dev2 siguen deshabilitados.



Las preferencias globales de vÃ­deo conservan sus primeros 30 campos y agregan tipo de fondo, selecciÃ³n de mÃºsica y marcador 230. No se cambian carpetas, perfiles, binds ni saves. IntroSoundc.smk se comprueba al cargar preferencias y al entrar al frontend, sin sondeo por frame. Una selecciÃ³n 99 guardada no se sobrescribe si falta el asset: se usa Golden y se oculta la opciÃ³n hasta el siguiente acceso al frontend con el archivo presente.



Los ejecutables de pruebas bajo 06_tests trabajan con copias privadas de los FMVs y una configuraciÃ³n temporal; no se incluyen en el portÃ¡til. La prueba del vÃ­deo usa el decodificador real y verifica cachÃ©, veinte reinicios, framing y liberaciÃ³n. Las pruebas de composiciÃ³n usan las funciones extraÃ­das del source, no una implementaciÃ³n alternativa. La validaciÃ³n no sustituye una prueba interactiva completa de menÃºs en el juego.



Probe 231: sin nuevas instrumentaciones activadas. La clasificaciÃ³n de marcos coplanares se ejecuta una vez al cargar el mapa y no genera logs. Pruebas aisladas y auditorÃ­a de Sulaco en 06_tests y 04_docs de la probe. Se mantiene el logger solicitado para investigar parones y crashes; -dev y -dev2 siguen deshabilitados.

Probe 232: el margen de profundidad de marcos se aplica Ãºnicamente durante sus batches, conservando/restaurando los estados del render. Sin nuevas instrumentaciones activadas.

Probe 233: encuadre centrado del Bink dentro del marco 4:3 y curva de presentacion 99 acelerada 30/38 s. Sin nuevas instrumentaciones activadas.

Probe 234: fade de logo 99 interpolado por frame, composicion de logo GPU en precision de 8 bits por canal y pulso luminoso 110%. Sin nuevas instrumentaciones; recurso temporal liberado al salir de la introduccion.

Probe 235: subida musical 29/37 s, pulso 115% a 37 s con retorno hasta 39 s, fade out 40/44 s. Sin nuevas instrumentaciones ni cambios a la reproduccion de audio.

## Probe 236 RC2: alcance del callback modal

SetFrontendModalFramesActive se habilita exclusivamente alrededor del bombeo de eventos del bucle de menÃº ya inicializado. Intro, carga de grÃ¡ficos, crÃ©ditos y salida mantienen esa autorizaciÃ³n deshabilitada. No es un editor ni una nueva opciÃ³n: evita que los EXPOSED de SDL consulten MenuHelpText antes de SetupNewMenu. El callback de partida conserva los controles de hilo y reentrada.


## Probe237: cachÃ©s de precisiÃ³n de render

Las poses de SECTION_DATA conservan traslaciones, cuaterniones y matrices en double exclusivamente para dibujar armas y modelos jerÃ¡rquicos del mundo. No son instrumentaciones ni se guardan en las partidas; se invalidan al cargar. La fÃ­sica, eventos y temporizadores conservan su ruta existente. El mantenimiento de mÃºsica/audio durante el arrastre de ventana es cÃ³digo normal, no un logger. No se habilitaron nuevos editores ni trazas.


## Probe240 / 6.240 RC1

Geometry compatibility rounding now uses SSE2 on supported targets; precise doubles and the lighting equations are unchanged. Scalar lrint remains the fallback.
Focused gameplay frames at or above12.5ms are now captured by event_latency.log, covering 50-72FPS drops previously below25ms. Frontend and unfocused capture retain25ms. Recording remains bounded in memory; ordinary gameplay recording does not write files.
Diagnostic sources under06_tests additionally capture all stages in performance.csv, use deterministic RNG/fixed dt and diagnostic immortality, and may sweep the camera. These hooks are NOT in01_source or the portable executable.


## Probe241 / 6.241 RC1

Alien launch and feeding event latches are runtime state only and do not change the savegame layout. Waypoint support-plane arrival does not modify collision geometry. Grenade sound is emitted at the committed shot callback, independently of the primary sound handle.

Audio playback rejection counts (unloaded sample, instance limit, voice limit, backend failure) are aggregated in bounded memory and appended to the existing sfx_latency.log at its loading/shutdown flush boundaries. Free-slot allocation makes no additional backend queries until a voice pool is exhausted. Paused and virtualized voices are retained.

Sources under06_tests use a copied quicksave/profile, fixed simulated time, hidden window and diagnostic immortality. Candidate replay records60 simulated seconds; unit harnesses extract production functions and replace only external systems. Audio replay additionally verifies four simultaneously active OpenAL sources. None of these test hooks are present in01_source or the portable executable.

Feeding integration replay uses the normal PlayerIsDead path and a real corpse, places an NPC nearby in the diagnostic process only, and verifies three animation-marker events (flags=2) with paired jaw/flesh playback. Production does not contain this scene setup.


## Probe242 / 6.242 RC1

Marine/Predator motion sampling, acceleration/travel fractional carry and avoidance progress are runtime caches only, reset on creation/loading. Serialized NPC/save blocks are unchanged. Production has no new loggers or test scenes.

06_tests replays use isolated copies of profiles/savegames, a hidden window, fixed simulated timestep and diagnostic immortality. AI_FRAMES bounds the diagnostic duration; AI_SAVE_ROUNDTRIP saves and reloads slot49 in the isolated test profile. NPC/pickup traces exist only in diagnostic sources. Fixed timestep does not guarantee identical AI paths: random consumption and frame sampling differ.


Probe242 reserve overflow fix: runtime magazine counts are 16-bit. Player strategy data retains the legacy byte-based wire structures and sizes; the existing optional MolecularAcid block now has an expanded payload for wide counts. New loaders accept both old and expanded Acid payloads. Older versions keep the saturated byte counts and ignore the larger Acid payload. Diagnostic AI_WIDE_RESERVES injects 297/286 reserves only in an isolated save/reload test.


## Probe 244: laboratorio de escuadra e inventario

Las comprobaciones controladas de órdenes, pickups, guardado/carga, daño Smartgun y saltos viven en 05_tools/instrument_244.py y se aplican exclusivamente a 06_tests/candidate_source. El ejecutable de 03_portable_release conserva únicamente las mediciones generales previas. Las pruebas usan copias de perfiles y partidas; no escriben en la instalación del juego. Los resultados y timestamps se registran en 04_docs y 06_tests.


## Laboratorio 245

Las repeticiones instrumentadas se generan exclusivamente en `06_tests/candidate_source` mediante `05_tools/prepare_diagnostics.py`. `PERF_REPLAY`, invulnerabilidad de laboratorio, avance fijo, loterías repetidas, eliminación de atacantes y snapshots de actores no existen en el ejecutable de entrega. Las pruebas usan copias locales de los perfiles y saves, y los datos del juego sólo como recursos.

Pruebas finales de 245: los laboratorios de rendimiento conservan los NPC y ataques del save; solamente el player de diagnóstico permanece inmortal. APC_LAB elimina los atacantes iniciales para medir exclusivamente recogida/turnos. APC_SUITE comprueba la lotería en un depósito desocupado y restaura los contactos físicos después de la prueba. Las trazas se muestrean cada 120 frames. Los resultados son mediciones CPU en esta máquina, sin pacing, y no equivalen a una garantía de FPS universales.


## Laboratorio 248 / v6.248 RC1

Las fuentes de entrega usan formación estable con recuperación de destinos bloqueados, contacto de apoyo normal a la pendiente, patrullaje coordinado y pickups persistentes del arma activa de los marines muertos. Los campos de movimiento, destino alternativo y trayectoria son cachés de ejecución; el progreso de exploración usa un bloque de guardado opcional y separado, schema 248.

`05_tools/prepare_diagnostics.py` crea las fuentes de laboratorio por separado. Las repeticiones usan copias de RaideR_51.sav, RaideR_1.sav y RaideR_3.sav, pasos de simulación 455/65536 y 1092/65536 s, y protección de salud del humano/aliados. Los atacantes permanecen en las pruebas completas. Los casos APC_RETURN y FORMATION_WALL recrean exclusivamente en diagnóstico dos posiciones capturadas durante esas repeticiones; permiten comprobar las colisiones y los destinos alternativos sin esperar otra vez todo el combate. Las pruebas de armas ejecutan muerte, recogida nativa y guardado/carga.

Las colocaciones de prueba, protección, trazas de actores y modificación del paso de simulación no existen en las fuentes ni en el ejecutable portable. La prueba del portable se limita al arranque durante cinco segundos con audio; el proceso se termina deliberadamente al finalizar. El informe distingue esa comprobación de las mediciones sin audio ni espera de cuadros.

`04_docs/validation.json` conserva las métricas de las regresiones completas; `final_audit.json` verifica fuentes, portable y originales de las partidas. `compiled_timestamps.csv` inventaría los artefactos compilados disponibles, incluidos los ejecutables archivados y las DLL reutilizadas. `build_history.jsonl` conserva el historial de compilación. No se certifica con estas pruebas la cobertura de cada rincón de todos los mapas personalizados.


En la validacion 248, AI_END_COMBAT es exclusivo del ejecutable instrumentado: las fixtures de formacion en pendiente eliminan hostiles con dano nativo a partir del frame 12; el ensayo de regreso tras combate los elimina desde el frame 12000 para disponer de una fase de recuperacion definida. Los ensayos naturales de combate, patrullaje y rendimiento no activan este hook. La proteccion de aliados hace que un combate natural pueda prolongarse; se conserva tambien su captura sin interrupcion para inspeccion. Ninguno de estos hooks se incluye en el portable.

La reagrupacion 248 usa histeresis de 6,5/4 m. Al cambiar de exploracion a reagrupacion, tanto el lider como su acompanante descartan la ruta local anterior una vez y recalculan hacia su companero. Tras cinco segundos de espera, el lider vuelve mediante navegacion nativa; el retorno a exploracion tambien invalida ese destino temporal. Esto impide reutilizar un tramo historico durante una reunion.


248 RC2: fixes the Windows dump-confirmed stale patrol destination after a
level unload/new session. Destinations use validated module indices; map
ownership is checked before using cached graph state. Restart and environment
destruction reset squad runtime state. Optional schema 248 saves remain compatible.
No laboratory hooks are present in this production source.

Audio-enabled unload testing also exposed SoundSys_StopAll writing through
external handles after level-pool release. Restart and environment destruction
now stop sounds while their owners are alive, before strategy/pool teardown.


## Probe 249 -- saved-world performance and predator gait

Ordered GL 1.5 streaming replaces mutable client-array submission with orphaned
vertex/index buffers. All vertices, formats, specular passes, blend state, samplers
and draw order are preserved. Buffer resources are deleted before context teardown.
The original client-array path remains the fallback when buffers are unavailable.

Immutable module meshes use a bounded 32 MiB BVH for swept particle and actor boxes. The
native polygon order, equality boundaries, normals and double-sided copies remain
unchanged, including each caller's invisible/mirror polygon policy. Dynamic, morphed and animated objects use the original routine. Cache
entries are reset before clearing the map pool and when resetting NPC support.

Shoulder-cannon acquisition uses a playing walk/run clip while actually moving.
A shot already in progress retains its original completion clock. Native authored
attack clips are preferred; models without moving attack clips use their existing
walk/run clip. Melee, taunts, death and weapon swaps retain their own sequences.

Diagnostic binaries only contain deterministic seeds, GPU pixel comparisons,
collision cross-checks, frozen-scene tests and alternative-path controls. None of
these environment controls or invulnerability helpers are included in production.
