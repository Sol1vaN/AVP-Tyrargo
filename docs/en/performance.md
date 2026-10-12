[English](../../README.md) · [Español](../../README.es.md)

# Performance evidence

Probe 249 measured two New Derelict saves at 2560×1440 with the same diagnostic executable, camera path, seed and fixed simulation step (455/65536 seconds). Optimizations were toggled within that build; the previous-path control was not a separately compiled older binary. The player was kept alive to observe the scene; NPCs received normal damage. Presentation was hidden and uncapped. These are local scenario measurements, not guaranteed gameplay FPS on every machine.

| Scenario | Original paths mean / p99 | Optimized mean / p99 | Mean reduction |
|---|---|---|---:|
| A | 15.828 / 27.074 ms | 10.371 / 19.372 ms | 34.48% |
| B | 18.692 / 28.942 ms | 9.460 / 17.084 ms | 49.39% |

The measured changes are ordered vertex/index buffer streaming and the immutable-mesh collision broad phase. No graphics/effect/population reduction was used. The original dynamic/morph/animated collision fallback remains. This is CPU/driver work reduction, not a new Vulkan backend or a rewritten multithreaded engine.

Validation compared 173,786 actor and 618,247 particle queries without polygon/normal/order differences. Fourteen blend/filter/specular combinations produced identical pixels. Save/load and two native restarts passed the local tests. Moving Predator acquisition samples changed from 81 frozen samples to 0; walk/run clips were observed. This validates those tests, not every map/state.

Earlier optimization areas are indexed in the catalog: CRT fragment work, GPU water grid/reflection traversal, fire batching, bounded shared AI/support planning, buffered audio/event logging and video-context lifetime. Each requires its own reproducible measurement; their gains must not be summed as one universal percentage.

Imported production candidate compiled: **2026-10-11 20:21:56.200870 −03:00**. SHA256: `059f77bbbe9e5b36dd0e6a4d06c591463806b6e94ee0054be0dffec2ebd545fb`. Diagnostic executable hashes differ from this production candidate.

Private saves, camera traces, test executables and original assets are retained locally; they are not distributed here. [Technical guide](technical.md) · [Catalog](features.md).
