# Instrumentación y diagnóstico

[English](../../INSTRUMENTATION.md)

Este documento describe los interruptores actuales y los diagnósticos de la build. Los editores de calibración y las escenas de laboratorio se mantienen separados del gameplay normal. El [historial](history.md) recoge los hitos de desarrollo.

## Interruptores de compilación

| Interruptor | Valor normal | Uso |
|---|---:|---|
| `TYRARGO_DEVELOPMENT_TOOLS` en `src/tyrargo_instrumentation.h` | 0 | Editor de armas/flamethrower (`-dev`) y herramientas de desarrollo. Cambiar a 1 y recompilar; los cheats nativos no requieren este switch. |
| `SHAKE_CALIBRATION_AVAILABLE` en `src/shake_params.h` | 0 | Calibración HUD (`-dev2`), disponible solo al habilitarla y recompilar. |
| `TYRARGO_MOTION_CALIBRATION` | 0 | Calibración de cámara; también requiere restaurar el acceso al menú de desarrollo. |
| `TYRARGO_RENDER_TRACE` | 0 | Trazas antiguas de render/contexto; revisar destinos antes de habilitarlas. |
| `TYRARGO_MEASURE_LATENCY` en `src/event_latency.h` | 1 | Registro acotado de latencias y checkpoints de carga. Cambiar a 0 y recompilar para desactivarlo. |

Los argumentos de editores deshabilitados no abren menús ocultos ni importan CSV antiguos. Los valores calibrados del código y los controles públicos de intensidad permanecen activos.

## Cheats nativos de consola

Inicia el juego con `-debug` (o `-d` / `--debug`) para habilitar los cheats originales en una build pública. Abre la consola dentro del juego y escribe `GOD` para alternar la inmortalidad o `GIVEALLWEAPONS` para obtener las armas/munición disponibles. Sin ese parámetro permanecen desactivados. Los editores de calibración son independientes; las reglas nativas de debug excluyen esos runs de los récords normales de campaña.


## Registros de la build

El resolver de configuración selecciona la carpeta `.avp`. Los registros se acumulan entre sesiones; no se borran perfiles, partidas ni registros anteriores.

- `event_latency.log`: frames lentos y eventos marcados, versión/sesión, duraciones de etapas CPU, posición/arma, foco/menú y contadores del proceso.
- `sfx_latency.log`: operaciones de audio lentas y contadores agregados de rechazos por muestras, instancias, voces o backend.
- `load_progress.log`: checkpoints de cargas confirmadas, mapas, reinicios y restauración de partidas. Cada checkpoint se cierra inmediatamente para conservarlo si después ocurre un cierre anormal.

El historial usa memoria fija: 128 eventos por frame y 8192 eventos retenidos. El gameplay con foco se registra desde 12,5 ms; frontend y frames sin foco usan 25 ms. Los eventos marcados pueden registrarse por debajo de esos umbrales. Los contadores de omisiones y sobrescrituras indican los límites del buffer.

El historial de eventos/audio se exporta en transiciones y al salir normalmente, no por NPC ni por frame ordinario. Los checkpoints síncronos de carga son la excepción explícita. Un cierre forzado puede perder el historial aún en memoria. La sesión identifica una ejecución; los tiempos relativos SDL de ejecuciones distintas no comparten un reloj.

Las duraciones son tiempo de pared CPU inclusivo, no tiempo de ejecución GPU. VSync y limitador se identifican aparte. Los fallos de página incluyen soft y hard y no prueban por sí solos acceso a disco ni fragmentación. La medición no fuerza sincronización GPU.

## Calibración

Los imports de desarrollo son `logs/dev_viewmodel_z.csv`, `logs/dev_flamethrower.csv`, `logs/marine_camera_motion.csv` y `logs/marine_hud_shake.csv`. Requieren habilitar sus editores; la importación automática del CSV de cámara sigue desactivada en la build normal. Los parámetros incorporados se conservan en `src/dev_viewmodel.c`, `src/motion_params.def` y `src/shake_params.def`.

## Laboratorios y límites

Las probes locales usan copias independientes de código, perfiles y saves, tiempo simulado fijo y duración limitada. Ventanas ocultas, invulnerabilidad, escenas controladas, capturas de píxeles, trazas NPC y variables `AI_*`, `LAB_*` o `PERF_REPLAY` pertenecen a esas copias diagnósticas. No están en el código de producción ni en el ejecutable publicado.

El paso fijo no garantiza rutas idénticas si cambia el consumo aleatorio o el muestreo. Comparar el mismo save, duración y escena e indicar si fue natural o controlada. Partidas privadas, registros de laboratorio y recursos comerciales no forman parte de la publicación.

Los diagnósticos antiguos OpenAL, Pentium, generadores/red y música permanecen apagados. Se mantienen errores ordinarios, consola visible, notificaciones y FPS opcional. La restauración OpenGL, el rollback de vídeo y las protecciones de vida de recursos son funciones normales.

[Evidencia de rendimiento](performance.md) · [Compilación](build.md) · [Instalación y reportes](install-and-play.md).
