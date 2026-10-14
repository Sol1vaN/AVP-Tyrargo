[English](../../README.md) · [Español](../../README.es.md)

# Catálogo detallado de mejoras

Comportamiento implementado consolidado; hay enlaces de evidencia del source. Cada entrada no equivale a una certificación de prueba individual.

<a id="input"></a>

## Mouse, teclado y binds

Deltas float de SDL siguen una ruta angular directa con redondeo simétrico, restos fraccionales y escala zoom/FOV. Teclado físico y mouse extendido conservan binds por especie. Corrige errores de conversión ensayados, sin prometer resolver cualquier problema de hardware.

[src/avp/win95/usr_io.c](../../src/avp/win95/usr_io.c) · [Código y ejemplos](technical.md#input)

| ID | Tipo | Cambio |
|---|---|---|

| `input-01` | fix | Ángulos directos del mouse a la cámara |

| `input-02` | fix | Restos subangulares yaw/pitch |

| `input-03` | fix | Conversión simétrica de mouse con signo |

| `input-04` | QoL | Sensibilidad del zoom según FOV |

| `input-05` | feature | Binds de botones adicionales y rueda |

| `input-06` | fix | Entrada física según teclado |

| `input-07` | QoL | Binds persistentes de armas por especie |

| `input-08` | feature | Recarga manual y etiqueta localizada |

| `input-09` | QoL | Binds de captura y guardar/cargar |

<a id="display"></a>

## Modos de pantalla y presentación

Ventana SDL, drawable, framebuffer y lienzo lógico se coordinan con cambios protegidos y fallback. Se restauran texturas al recrear contexto. Mover ventana atiende juego/menú y audio. VSync y límite son opciones; simulación de alto refresco y límite 1000 FPS siguen en auditoría.

[src/main.c](../../src/main.c) · [Código y ejemplos](technical.md#display)

| ID | Tipo | Cambio |
|---|---|---|

| `display-01` | feature | Modos ventana, sin bordes y exclusivo |

| `display-02` | fix | Cambios transaccionales de modo de vídeo |

| `display-03` | fix | Restauración de texturas al recrear contexto |

| `display-04` | fix | Sincronización de resolución y refresco |

| `display-05` | QoL | Lienzos letterboxed y panorámicos |

| `display-06` | feature | Opciones VSync y límite de FPS |

| `display-07` | fix | Continuidad del loop/audio al mover ventana |

| `display-08` | fix | Restauración de estado GL entre menú y juego |

<a id="geometry"></a>

## Proyección y precisión geométrica

Coordenadas fraccionales sobreviven transformación y clipping. Proyecciones de mundo/viewmodel sincronizan cámara, frustum y visibilidad sin sustituir la representación nativa de gameplay.

[src/render_geometry.c](../../src/render_geometry.c) · [Código y ejemplos](technical.md#geometry)

| ID | Tipo | Cambio |
|---|---|---|

| `geometry-01` | fix | Transformación y clipping float |

| `geometry-02` | fix | Precisión de secciones móviles jerárquicas |

| `geometry-03` | fix | Viewmodels según pose actual de cámara |

| `geometry-04` | feature | FOV del mundo y arma por especie |

| `geometry-05` | fix | Proyección Hor-plus y frustum sincronizados |

| `geometry-06` | fix | Juntas geométricas y plano cercano |

| `geometry-07` | QoL | Posiciones calibradas de armas |

<a id="render"></a>

## Render, iluminación y efectos

Los recortes de textura también limitan la pasada luminosa/profundidad. Relojes UV compartidos corrigen desfases. Orden de vidrio/efectos conserva visibilidad. Buffers ordenados y agrupación de fuego reducen trabajo conservando orden, mezclas y filtros originales.

[src/opengl.c](../../src/opengl.c) · [Código y ejemplos](technical.md#render)

| ID | Tipo | Cambio |
|---|---|---|

| `render-01` | fix | Pasada luminosa/especular respetando alpha |

| `render-02` | fix | Sincronización UV de capas luminosas |

| `render-03` | fix | Partículas y halos visibles tras vidrio |

| `render-04` | feature | Streaming VBO/IBO ordenado con fallback |

| `render-05` | fix | Reducción de trabajo de fuego/partículas |

| `render-06` | feature | Controles ampliados de detalle y distancia |

| `render-07` | QoL | Controles de filtros de texturas y HUD |

<a id="water"></a>

## Simulación y render de agua

Propagación y amortiguación por tiempo evitan pérdida por enteros y perturbaciones descontroladas por frame. Óptica moderna incorpora refracción con profundidad, Fresnel, reflejos, especular con luces nativas y cáusticas. Screen-space no refleja geometría fuera de pantalla.

[src/water_effect.c](../../src/water_effect.c) · [Código y ejemplos](technical.md#water)

| ID | Tipo | Cambio |
|---|---|---|

| `water-01` | fix | Propagación y amortiguación de ondas por tiempo |

| `water-02` | feature | Agua clásica y moderna seleccionable |

| `water-03` | feature | Refracción con protección de foreground |

| `water-04` | feature | Fresnel y reflejos screen-space |

| `water-05` | feature | Especular nativo y cáusticas de refracción |

| `water-06` | fix | Malla GPU y recorrido acotado de reflejos |

<a id="postfx"></a>

## CRT, ruido y ajustes de imagen

CRT GLSL, ajustes de imagen y ruido de monitores son seleccionables. Trabajo constante o interior evita rutas caras por fragmento cuando es equivalente. Procedencia CRT incluye Mattias/newpixie y CRT-Guest-ReShade; créditos no sustituyen licencias.

[src/crt_filter.c](../../src/crt_filter.c) · [Código y ejemplos](technical.md#postfx)

| ID | Tipo | Cambio |
|---|---|---|

| `postfx-01` | feature | Filtro CRT GLSL nativo |

| `postfx-02` | feature | Controles de brillo, contraste y saturación |

| `postfx-03` | feature | Modos sin señal RGB/CRT |

| `postfx-04` | QoL | Opciones de escala de ruido y grano |

| `postfx-05` | fix | Reducción de trabajo del shader CRT |

<a id="ui"></a>

## Menú principal, menú ingame y UX

Menús usan espacios lógicos explícitos, protección de muestreo y composición latina. Perfiles por especie conservan escalas. Escape aplica ajustes ordinarios; confirmación de vídeo es explícita. Slots guardar/cargar permanecen accesibles en carruseles.

[src/avp/win95/frontend/avp_menus.c](../../src/avp/win95/frontend/avp_menus.c) · [Código y ejemplos](technical.md#ui)

| ID | Tipo | Cambio |
|---|---|---|

| `ui-01` | QoL | Identidad Tyrargo y versioning |

| `ui-02` | QoL | Starfield panorámico y fondos recuperados |

| `ui-03` | fix | Sangrado de atlas glifos/HUD y marcos |

| `ui-04` | fix | Composición de glifos latinos acentuados |

| `ui-05` | feature | Escalas independientes HUD, texto, consola y crosshair |

| `ui-06` | QoL | Vistas previas HUD/texto/vídeo/imagen ingame |

| `ui-07` | QoL | Escape aplica ajustes y cierra menús/consola |

| `ui-08` | QoL | 50 slots y quicksave/quickload dedicado |

| `ui-09` | QoL | Duración de avisos y posición adaptable de FPS |

| `ui-10` | fix | Limpieza de perfiles, fades y transiciones |

| `ui-11` | fix | Cheats nativos de consola habilitados con -debug en builds públicas |

<a id="media"></a>

## Vídeos y capturas

Decodificadores Smacker/Bink y audio en buffer reproducen vídeos. Viewports físicos soportan modos panorámicos. El jugador suministra medios comerciales. Capturas BMP/PNG/JPEG excluyen sus notificaciones.

[src/fmv_backend.cpp](../../src/fmv_backend.cpp) · [Código y ejemplos](technical.md#media)

| ID | Tipo | Cambio |
|---|---|---|

| `media-01` | feature | Reproducción nativa Smacker y Bink |

| `media-02` | fix | Reloj de audio, RGB y finalización sincronizados |

| `media-03` | feature | Vídeos nativos, escalados y ultrapanorámicos |

| `media-04` | feature | Música IntroSound99 opcional de menú |

| `media-05` | feature | Capturas BMP, PNG y JPEG en juego/menú |

| `media-06` | fix | Capturas repetidas limpias |

<a id="audio"></a>

## Audio, música y latencia

Emisores OpenAL siguen posiciones y entorno. Solapamiento de armas y alimentación consideran audio. CD-DA incluye unidad y volumen; bucles incluyen silencio aleatorio. Diagnósticos acotados difieren escrituras fuera de frames.

[src/openal.c](../../src/openal.c) · [Código y ejemplos](technical.md#audio)

| ID | Tipo | Cambio |
|---|---|---|

| `audio-01` | feature | EFX ambiental y efectos NPC posicionales |

| `audio-02` | fix | Audio solapado de armas primarias/secundarias |

| `audio-03` | fix | SFX de alimentación y combate Alien |

| `audio-04` | feature | Soporte musical CD-DA/Red Book |

| `audio-05` | feature | Música una vez, bucle y silencio aleatorio |

| `audio-06` | fix | Gestión acotada de latencia eventos/audio |

<a id="player"></a>

## Movimiento y armas del jugador

Contacto, trepado y apoyo móvil tienen correcciones conservando fricción de pendientes. Hay correr/agachar y zoom/jetpack inteligentes. Colas de disparo conservan taps; interacción y cooldown evitan ráfagas por frame.

[src/avp/pmove.c](../../src/avp/pmove.c) · [Código y ejemplos](technical.md#player)

| ID | Tipo | Cambio |
|---|---|---|

| `player-01` | fix | Deslizamiento y contacto con superficies curvas |

| `player-02` | fix | Apoyo de plataforma móvil y contacto al saltar |

| `player-03` | fix | Escaleras, giros y trepado Alien |

| `player-04` | feature | Jetpack y zoom Predator inteligentes |

| `player-05` | QoL | Correr siempre, agachar alternado y pitch ampliado |

| `player-06` | fix | Entrada semiautomática edge/latch/queue |

| `player-07` | fix | Intervalo controlado de bengalas |

| `player-08` | fix | Interacción con pickups y puertas alcanzables |

<a id="motion"></a>

## Movimiento de cámara, armas y HUD

Capas Marine de cámara/arma/HUD cubren movimiento y equipo con canales configurables. Transiciones incluyen postura, escaleras y aterrizaje. Calibración requiere source de desarrollo; producción conserva valores y controles públicos.

[src/camera_motion.c](../../src/camera_motion.c) · [Código y ejemplos](technical.md#motion)

| ID | Tipo | Cambio |
|---|---|---|

| `motion-01` | feature | Bobbing y sway procedural caminar/correr |

| `motion-02` | feature | Feedback suave agacharse, escaleras, salto y aterrizaje |

| `motion-03` | feature | Perfiles HUD shake por arma |

| `motion-04` | QoL | Sliders separados de intensidad |

| `motion-05` | fix | Vida de estados y recuperación correr/pitch |

<a id="campaign"></a>

## Campaña, recursos y Molecular Acid

Acid modifica Hard en singleplayer y no agrega índices a arrays nativos. x1/x1.5/x2 corresponden a enemigos 1.5/2/2.5 y recursos 1.5/2/3. Excepciones son cantidades. Estado de campaña y economía de ammo tienen vidas separadas.

[src/molecular_acid.c](../../src/molecular_acid.c) · [Código y ejemplos](technical.md#campaign)

| ID | Tipo | Cambio |
|---|---|---|

| `campaign-01` | feature | Persistencia opcional entre episodios |

| `campaign-02` | feature | Modo campaña de munición optimizada |

| `campaign-03` | feature | Niveles Molecular Acid y presión de población |

| `campaign-04` | feature | Reglas Acid de reservas y cargadores |

| `campaign-05` | feature | Pickups Pulse Rifle/SADAR/disco Marine de 3/4/5 |

| `campaign-06` | feature | Energía Predator escalada y armadura base 15 |

| `campaign-07` | fix | Gestión Acid bonus/custom |

| `campaign-08` | fix | Recuperación de visibilidad y activación dormida |

| `campaign-09` | QoL | Espaciado briefings, memoria episodio y reintentos |

<a id="aliens"></a>

## Comportamiento y navegación de Alien NPC

Tiempo fraccional, dirección local y recuperación de contacto corrigen casos ensayados de ductos, escotillas y bordes. Salto tiene autorización acotada: no todo salto grande es por FPS. Alimentación incluye animación/audio. Otros mapas requieren pruebas.

[src/avp/bh_alien.c](../../src/avp/bh_alien.c) · [Código y ejemplos](technical.md#aliens)

| ID | Tipo | Cambio |
|---|---|---|

| `aliens-01` | fix | Tiempo fraccional de movimiento NPC |

| `aliens-02` | fix | Recuperación en ductos, escotillas y bordes |

| `aliens-03` | fix | Autorización acotada de salto vertical y apoyo |

| `aliens-04` | feature | Alimentación del cadáver del jugador |

| `aliens-05` | fix | Recuperación comprometida en bordes de ductos, midiendo progreso y conservando el objetivo |

<a id="predators"></a>

## Combate y patrulla de Predator NPC

Armas privadas, adquisición y patrulla amplían combate nativo. Adquisición del cañón en movimiento usa locomoción; disparo activo conserva tiempo de finalización. Navegación y camuflaje siguen condicionando persecución.

[src/avp/bh_pred.c](../../src/avp/bh_pred.c) · [Código y ejemplos](technical.md#predators)

| ID | Tipo | Cambio |
|---|---|---|

| `predators-01` | feature | Inventario NPC privado y elección de arma |

| `predators-02` | feature | Taunts Predator al matar |

| `predators-03` | fix | Adquisición y apuntado por arma |

| `predators-04` | feature | Planificación de patrulla y persecución |

| `predators-05` | fix | Caminar/correr al adquirir disparo en movimiento |

<a id="marines"></a>

## Escuadrón, inventario y tácticas Marine NPC

Usar alterna follow/patrol con confirmación. Tareas compartidas, patrulla por etapas e histéresis reducen decisiones en conflicto. Marines recogen armas/recursos, eligen armas, ayudan con torretas y dejan drops. Inventarios/planner guardados tienen protecciones de cargar/reiniciar.

[src/avp/npc_support.c](../../src/avp/npc_support.c) · [Código y ejemplos](technical.md#marines)

| ID | Tipo | Cambio |
|---|---|---|

| `marines-01` | feature | Órdenes usar follow/patrol con confirmación |

| `marines-02` | feature | Planner de exploración expansiva coordinada |

| `marines-03` | fix | Ritmo, distancia de parada y separación al seguir |

| `marines-04` | fix | Acceso/salida APC y manejo de pendientes |

| `marines-05` | feature | Reabastecimiento compartido aleatorio con prioridad humana |

| `marines-06` | feature | Recogida de dos armas adicionales distintas |

| `marines-07` | feature | Umbrales: salud/armadura 60%, ammo 30% |

| `marines-08` | feature | Cadencia, recargas y combustible definidos como el jugador, con inventario privado |

| `marines-09` | fix | Convergencia smartgun, exclusión de aliados y selección dentro del área válida |

| `marines-10` | fix | Apuntado por arma y consumo de munición al crear el proyectil |

| `marines-11` | feature | Tarea de apoyo con torretas cercanas |

| `marines-12` | feature | Celebraciones Marine al matar |

| `marines-13` | feature | Armas recuperables al morir |

| `marines-14` | fix | Estado persistente de escuadrón, planner y temporizadores por arma |

<a id="kernel"></a>

## Vida útil, rendimiento y diagnóstico del motor

BVH acotado de mallas inmutables selecciona candidatos conservando matemática/orden nativos. Mallas dinámicas/morph/animadas usan fallback. Caché se limpia antes del pool. IA por etapas y protecciones investigadas conservan efectos/población; resultados dependen del escenario.

[src/avp/particle_mesh_cache.inc](../../src/avp/particle_mesh_cache.inc) · [Código y ejemplos](technical.md#kernel)

| ID | Tipo | Cambio |
|---|---|---|

| `kernel-01` | feature | BVH de mallas inmutables para actores/partículas |

| `kernel-02` | fix | Fallback dinámico y limpieza de caché |

| `kernel-03` | fix | Trabajo IA/apoyo compartido por etapas |

| `kernel-04` | fix | Protecciones crash de partículas, NPC y nivel |

| `kernel-05` | QoL | Compilaciones diagnósticas y de producción separadas |
