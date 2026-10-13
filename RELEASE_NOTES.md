# AVP: Tyrargo v6.252 RC1 — Windows x64

This update improves autonomous alien recovery at vent edges and restores the original console cheats in normal builds. It includes all changes from v6.251 RC1.

## Alien vent-edge recovery

- Detects a stalled approach by measuring progress toward the target. Sideways oscillation at a vent lip no longer counts as useful forward movement.
- Starts a bounded recovery sooner and keeps a direct approach when the enemy is physically visible, instead of repeatedly abandoning it for waiting or avoidance.
- Uses current targeting positions for navigation visibility and preserves the shared line-of-sight state.
- Retains native hull collision, wall climbing, movement speed, gravity and leap limits. Patrol, retreat and inter-room detours retain their normal progress checks.

Nine controlled save replays with the player stationary reached a visible combat position in approximately 1.7–2.5 simulated seconds across tested timesteps of roughly 60, 144, 240 and 1000 FPS. The previous build also eventually recovered in this case, but more slowly. Repeated save loading and a separate oscillation/progress test passed. These are targeted checks, not a claim that every map edge is resolved.

## Original cheats available again

Launch the executable with:

```text
avpt.exe -debug
```

Open the in-game console and enter `GOD` to toggle immortality, or `GIVEALLWEAPONS` for equipment. Normal builds now honor the startup flag instead of depending on development calibration tools. Commands remain disabled without the flag. The original cheat restrictions on normal saves/records still apply. `-d` and `--debug` remain compatible aliases; `-debug` is sufficient.

Native `GOD` on/off and `GIVEALLWEAPONS` were checked with the debug flag and without it.

## Smartgun behavior preserved

No smartgun targeting-mode change is included in this update. A new game starts in Tracking, secondary fire switches Tracking / Free, and loading restores the mode saved in that game. Rendered tests confirmed enemy acquisition, the red tracking sight, moving crosshair and weapon convergence after switching the supplied Free-mode save to Tracking. Allied filtering and previous targeting fixes remain in place.

## Download and install

Download **AVP-Tyrargo-v6.252-rc1-Windows-x64.zip**. Extract it into your installed **Aliens versus Predator Gold Edition (retail CD)** or **Aliens versus Predator Classic 2000 (Steam/GOG)** directory, preserving folders. Run `avpt.exe` and keep the included DLLs beside it. Back up previous Tyrargo profiles and saves before updating.

Original commercial game data must be supplied separately. This release contains no game maps, movies, audio, profiles, saves or diagnostic hooks. RC1 remains a release candidate for broader player testing.

The same release supplies matching production source, dependency sources/build recipes, relinkable objects and `SHA256SUMS.txt`. Original Rebellion, NakedAVP and third-party notices remain included. Installation details: [English](https://github.com/Sol1vaN/AVP-Tyrargo/blob/v6.252-rc1/docs/en/install-and-play.md) / [Español](https://github.com/Sol1vaN/AVP-Tyrargo/blob/v6.252-rc1/docs/es/install-and-play.md).

## Español

### Alien atascado en ventilaciones

La recuperación detecta la falta de avance hacia el objetivo, comienza antes y mantiene el intento de salida cuando existe visibilidad física. Así evita que el movimiento lateral en el borde reinicie la detección del atasco o que el alien abandone continuamente el intento. Se conservan las colisiones, el trepado y los límites de movimiento originales.

En nueve pruebas con el jugador inmóvil, el alien alcanzó una posición visible de combate en unos **1,7–2,5 segundos de simulación**. También se comprobó la recuperación al volver a cargar la partida. La versión anterior terminaba saliendo en este caso, pero tardaba más; la validación se limita a los escenarios probados.

### Cheats disponibles nuevamente

Inicia con **`avpt.exe -debug`**. En la consola del juego, **`GOD`** activa/desactiva la inmortalidad y **`GIVEALLWEAPONS`** entrega equipamiento. No necesitas `--debug`; ese alias sigue siendo compatible. Sin el parámetro, los cheats permanecen desactivados. Se conservan las restricciones originales de guardado y registros cuando se usan cheats.

### Smartgun e instalación

El cambio Tracking / Free permanece intacto: disparo secundario alterna el modo y al cargar se restaura el modo guardado. La build no fuerza el seguimiento.

Descarga el ZIP Windows x64, extráelo en tu instalación de Gold CD o Classic Steam/GOG y ejecuta `avpt.exe`, conservando las DLL incluidas. Respalda tus perfiles y partidas anteriores. Los datos comerciales del juego no se incluyen. RC1 continúa disponible para pruebas más amplias.
