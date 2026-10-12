# AVP: Tyrargo v6.251 RC1 — first public Windows x64 release

Tyrargo is an independent, non-commercial fan port derived from **atsb's NakedAVP 1.2.3**, built on the original Rebellion Aliens versus Predator source. **Rebellion created and owns the original game/source.** Its license and the component licenses remain applicable.

This is a **release candidate**, with compiled binaries, matching production source, dependency sources and relinking materials. Original game data is supplied separately.

## Download and install

Download **AVP-Tyrargo-v6.251-rc1-Windows-x64.zip** for playing. The other archives are development/license materials.

Compatible game-data editions:

- **Aliens versus Predator Gold Edition — retail CD**.
- **Aliens versus Predator Classic 2000 — Steam or GOG**.

Extract the Windows ZIP into the installed game directory, retaining directories and notices, then run **avpt.exe**. Keep its runtime DLLs beside it. The executable has a different name from the stock executables, so those can remain installed. Back up profiles/saves before replacing an earlier Tyrargo build. A copy of the game directory can be used for an independent installation.

[Full installation instructions](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/en/install-and-play.md) · [Feature catalog](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/en/features.md) · [Gallery](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/en/gallery.md)

## Changes in this candidate

- **Assisted targeting:** enemies outside the sight's capture rectangle are rejected before nearest-target selection. A nearer off-axis enemy can no longer mask another aimable enemy. Ally exclusions remain active, and free/assisted smartgun modes retain their separate behavior.
- **Alien route recovery:** a stalled approach retains its target instead of being forced into wandering. During the bounded recovery interval it can retry a visible target direction through ordinary collision/dynamics. Wall climbing and authored jumping remain available; no teleport or collision bypass is used.
- **Marine weapon cadence:** persistent per-weapon/channel timing prevents animation resets, target changes or weapon swaps from bypassing shot recovery. Grenade launcher spacing respects the player's firing/recoil definitions; a controlled combat replay at 60 FPS measured about 1.35 seconds between launches against a 1.33-second minimum.
- **Reloads and ammunition:** SADAR/disc launchers no longer refill instantly through the squad controller. Reload clocks and animations use player definitions; projectile ammunition is debited after successful creation. Native automatic weapons use the corresponding player fire rates and ammunition types. Flamethrower consumption carries its fractional remainder instead of treating a complete tank as three seconds of fuel.
- **Simulation outside camera:** firing/reload animation events advance even when an allied NPC is not being drawn. Weapon clocks are saved in the new optional squad extension; older 245/246 squad payloads remain accepted.
- **Documentation and credits:** instrumentation is rewritten coherently in English with a separate Spanish page. Installation editions are explicit. The gallery contains **160 screenshots in 21 categories**, with CRT Before/After naming and the additional Marine screenshot on the homepage. Shader/runtime notices and visible decoder/CRT credits are included.

## What the public build includes

The existing Tyrargo foundation provides fractional mouse/render precision; per-species FOV, HUD, crosshair and text scaling; live option previews; windowed/borderless/exclusive presentation; screenshot options; optional CRT GLSL/image controls and modern water; spatial audio/environment effects; campaign persistence and optimized ammunition; and coordinated single-player NPC support.

Allied marines can acknowledge Use commands, follow or patrol, collect two distinct additional weapons, manage private ammunition, use nearby sentries and leave recoverable firearms on death. Supply jobs coordinate access and give the human player priority. Health/armor pickups use the below-60% threshold and ammunition the below-30% threshold. Exploration and combat retain native collision constraints. Predator locomotion and weapon acquisition include the prior animation corrections.

Molecular Acid menu labels **x1 / x1.5 / x2** use enemy factors **1.5 / 2 / 2.5** and resource factors **1.5 / 2 / 3**. Pulse Rifle magazines, SADAR rockets and marine discs use total pickup quantities **3 / 4 / 5**. Predator energy scales with resources, and Acid armor uses a 15-point base multiplied by that resource factor. Authored triggers and supported generators still matter; every custom scripted wave is not guaranteed to have a simple multiplied count.

Performance work preserves draw order, collision results and visual options. The prior two New Derelict comparisons reduced measured mean frame times from **15.828 to 10.371 ms** and **18.692 to 9.460 ms** in their documented local fixtures. Those are scenario results, not a promise of constant FPS on every map/machine. [Measurement method and limits](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/en/performance.md).

## Validation and limits

This iteration exercised nine supported marine weapons at multiple simulated frame steps; reload capacities/reserves; fractional fuel; all four grenade variants with native gravity/velocity; friendly-target filtering; blocked sight lines; per-weapon save/load state; the tunnel save; delayed alien arrival/assisted targeting; and a fresh Steam-data start. The release's decoder rebuild/relink script was also exercised. Laboratory invulnerability/scene setup/replay hooks are excluded from production.

Broader campaign, custom-map and multiplayer testing continues. This build is not labeled stable, and it does not introduce a new map loader, a Vulkan renderer or the planned camouflage system. Use saves made against the corresponding map data; restore the same custom-map installation when investigating a save.

## Build and source materials

- **Windows-x64.zip:** avpt.exe, the six matching runtime DLLs, installation notes, notices and a file/version/hash manifest.
- **source.zip:** the matching production source and bilingual documentation.
- **relink.zip:** production objects, decoder source/compile recipes, import libraries, object timestamps and a tested rebuild/relink script.
- **dependency-sources.zip:** SDL3 3.4.14, OpenAL Soft 1.25.2, libiconv 1.19, GCC 16.2.0 and the matching winpthreads source, plus pinned MSYS2 build recipes and licenses.
- **SHA256SUMS.txt:** hashes of the downloadable archives.

The original engine remains non-commercial. Independently licensed LGPL, BSD, zlib, MIT and CRT components retain their own notices and permissions. The supplied MattiasCRT adaptation carries **CC BY-NC-SA 3.0**; newpixie and CRT-Guest-ReShade are credited references. No commercial game assets, movie/audio data, profiles or saves are included.

---

## Español

Esta es la primera publicación pública de **AVP: Tyrargo v6.251 RC1**, un port independiente y no comercial basado en **NakedAVP 1.2.3 de atsb** y el código original de **Rebellion**, creadores y propietarios del juego/código. Se conservan su licencia y las licencias propias de los componentes.

Para jugar descarga **AVP-Tyrargo-v6.251-rc1-Windows-x64.zip**, extráelo en el directorio instalado y ejecuta **avpt.exe**. Sirven **Gold Edition CD retail** y **Classic 2000 Steam/GOG**. Conserva las DLL junto al ejecutable y los avisos de licencia. Los ejecutables originales tienen otro nombre y permanecen disponibles. Respalda perfiles/partidas al actualizar un Tyrargo anterior. El paquete no contiene el juego ni recursos comerciales.

[Instalación completa](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/es/install-and-play.md) · [Catálogo](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/es/features.md) · [Galería](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/es/gallery.md)

### Correcciones de este candidato

- La selección asistida descarta primero blancos fuera del área válida de la mira. Un enemigo cercano fuera de esa área ya no oculta a otro apuntable. Se mantienen la exclusión de aliados y los modos libre/asistido.
- El alien atascado conserva su objetivo al recuperar la ruta, en lugar de entrar obligadamente en merodeo. Puede probar la dirección de un objetivo visible durante una recuperación acotada, manteniendo colisiones, trepado y saltos nativos.
- La cadencia NPC usa temporizadores persistentes por arma/canal. Cambiar objetivo, arma o animación no evita la recuperación. Las granadas respetan aproximadamente 1,33 segundos como mínimo; el combate controlado a 60 FPS midió unos 1,35 segundos entre lanzamientos.
- SADAR/lanzadiscos dejan de recargar instantáneamente. Recargas, cadencias, munición y consumo de combustible se vinculan a las definiciones del jugador. Solo se descuenta un proyectil cuando su creación tiene éxito. El combustible conserva sus restos fraccionales.
- Las animaciones de disparo/recarga avanzan fuera de cámara. Los relojes de arma se conservan al guardar/cargar; se aceptan extensiones antiguas de escuadrón 245/246.
- La instrumentación tiene páginas coherentes separadas por idioma. La galería reúne **160 capturas en 21 categorías**, pares CRT Before/After y la nueva captura Marine bajo Halos en la portada. Se incluyen créditos y licencias identificadas del shader, librerías y decodificadores.

### Base disponible y comprobaciones

Se mantienen precisión de mouse/geometría, escalas y FOV por especie, vistas previas, modos de pantalla, capturas, CRT GLSL, ajustes de imagen, agua moderna opcional, audio espacial, persistencia, munición optimizada y comportamiento coordinado del escuadrón. Los marines pueden recibir órdenes, recoger dos armas adicionales, administrar inventario privado, usar torretas y dejar armas recuperables. Los umbrales de pickup son menos del 60% de salud/armadura y menos del 30% de munición.

Acid **x1 / x1.5 / x2** usa enemigos **1,5 / 2 / 2,5** y recursos **1,5 / 2 / 3**. Las excepciones Pulse Rifle/SADAR/discos entregan cantidades totales **3 / 4 / 5**. Energía Predator y armadura base de 15 puntos usan el factor de recursos. Los scripts y generadores del mapa conservan sus condiciones.

Las mediciones previas de New Derelict bajaron el tiempo medio de frame de **15,828 a 10,371 ms** y de **18,692 a 9,460 ms** en las escenas documentadas. No equivalen a una garantía de FPS constantes. [Método y límites](https://github.com/Sol1vaN/AVP-Tyrargo/blob/main/docs/es/performance.md).

Se probaron nueve armas con distintos pasos simulados, recargas/reservas, combustible, cuatro granadas y sus físicas, aliados/obstrucciones, guardar/cargar temporizadores, el atasco del túnel, la llegada posterior de aliens y el arranque con datos Steam. También se probó recompilar y volver a enlazar los decodificadores. Las escenas y variables de laboratorio no forman parte del ejecutable publicado.

Los archivos de código, dependencias y relink acompañan a los binarios en esta misma release, con hashes SHA-256. Continúan las pruebas más amplias; **RC1 es un candidato**, no una versión estable. No se implementan aquí el futuro cloaking, Vulkan ni un cargador de mapas nuevo. Para reproducir un save debe usarse su mismo mapa custom. Los recursos comerciales y las partidas personales se suministran por separado.