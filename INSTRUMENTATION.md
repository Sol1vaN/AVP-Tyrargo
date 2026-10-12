# Instrumentation and diagnostics

[Español](docs/es/instrumentation.md)

This document describes the current source switches and shipped diagnostics. Calibration editors and laboratory fixtures are separate from normal gameplay. The changelog is in [development milestones](docs/en/history.md).

## Build switches

| Switch | Default | Purpose and deliberate reactivation |
|---|---:|---|
| `TYRARGO_DEVELOPMENT_TOOLS` in `src/tyrargo_instrumentation.h` | 0 | Viewmodel/flamethrower editor (`-dev`), development console/debug tools. Set to 1 and rebuild. |
| `SHAKE_CALIBRATION_AVAILABLE` in `src/shake_params.h` | 0 | HUD calibration (`-dev2`). Enable and rebuild. |
| `TYRARGO_MOTION_CALIBRATION` | 0 | Camera calibration. Enable and restore the development menu entry before rebuilding. |
| `TYRARGO_RENDER_TRACE` | 0 | Legacy context/render traces. Review output paths before enabling and rebuilding. |
| `TYRARGO_MEASURE_LATENCY` in `src/event_latency.h` | 1 | Bounded event/audio latency recording and load checkpoints. Set to 0 and rebuild to disable. |

Disabled editor arguments do not reopen hidden menus or import old calibration CSV files. Public effect-intensity controls and the calibrated values embedded in source remain active.

## Shipped logs

The game's configuration resolver chooses the `.avp` configuration directory. Logs are appended across sessions; existing profiles, saves and log files are not deleted.

- `event_latency.log`: retained slow frames and marked events, with version/session identity, CPU stage durations, player position/weapon, focus/menu context and process counters.
- `sfx_latency.log`: slow audio operations and aggregated sample/instance/voice/backend rejection counts.
- `load_progress.log`: synchronous checkpoints for selected loads, map transitions, restart and save restoration. Each checkpoint is closed immediately so a later abnormal exit does not erase that checkpoint.

Ordinary event capture uses fixed memory: 128 events per frame and an 8192-event history. Focused gameplay frames are retained from 12.5 ms; frontend/unfocused frames use 25 ms. Marked transitions can be retained below those thresholds. Omitted/overwritten counts identify buffer limits.

Event/audio history is exported at transition/shutdown boundaries, not once per actor or gameplay frame. Load checkpoints are an explicit exception during synchronous loading. A forced termination may lose history still in memory. Session identity links exports from one run; SDL-relative timestamps from different runs must not be compared as one clock.

Durations measure inclusive CPU wall time, not GPU execution. VSync/limiter waits are recorded separately. Page-fault counts include soft and hard faults and do not alone prove disk stalls or fragmentation. No forced GPU synchronization is introduced for measurement.

## Calibration files

Development-only imports include `logs/dev_viewmodel_z.csv`, `logs/dev_flamethrower.csv`, `logs/marine_camera_motion.csv` and `logs/marine_hud_shake.csv`. Their corresponding editors must be enabled deliberately; camera CSV auto-import remains disabled in normal builds. Embedded parameters are preserved in `src/dev_viewmodel.c`, `src/motion_params.def` and `src/shake_params.def`.

## Laboratory builds

Local probes use separate source copies, copied saves/profiles, fixed simulated time, bounded replay length and, where explicitly selected, hidden windows or invulnerability. Per-NPC traces, controlled scene setup, pixel captures and `AI_*`/`LAB_*`/`PERF_REPLAY` environment hooks belong to those diagnostic copies. They are not present in the published production source or executable.

A fixed timestep controls elapsed simulation time; it does not guarantee identical paths when random consumption or frame sampling changes. Compare the same save, duration and fixture, and report whether the scene was natural or controlled. Private saves, laboratory logs and commercial map/audio data are excluded from the public repository and release.

## Disabled legacy diagnostics and retained safeguards

Legacy OpenAL debug, Pentium profiling, generator/network logging and old music traces remain disabled. Ordinary errors, the visible console, notifications and the optional FPS display remain available. OpenGL context restoration, display-mode rollback and resource-lifetime guards are functional protections, not diagnostic editors.

See [performance evidence](docs/en/performance.md), [building](docs/en/build.md) and [reporting problems](docs/en/install-and-play.md).
