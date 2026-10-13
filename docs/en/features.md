[English](../../README.md) · [Español](../../README.es.md)

# Detailed feature catalog

Consolidated implemented behavior; source-review evidence is linked below. Individual entries are not separate test certifications.

<a id="input"></a>

## Mouse, keyboard and bindings

SDL float deltas follow a direct angular path with signed rounding, fractional residuals and zoom/FOV scaling. Physical keyboard and extended mouse actions preserve species-specific bindings. This corrects tested conversion errors, not every possible hardware issue.

[src/avp/win95/usr_io.c](../../src/avp/win95/usr_io.c) · [Code and examples](technical.md#input)

| ID | Type | Change |
|---|---|---|

| `input-01` | fix | Direct mouse-to-camera angles |

| `input-02` | fix | Sub-angular yaw/pitch residuals |

| `input-03` | fix | Symmetric signed mouse conversion |

| `input-04` | QoL | FOV-aware zoom sensitivity |

| `input-05` | feature | Extended mouse buttons and wheel bindings |

| `input-06` | fix | Physical keyboard layout input |

| `input-07` | QoL | Persistent weapon bindings per species |

| `input-08` | feature | Manual reload action and localized label |

| `input-09` | QoL | Screenshot and save/load bindings |

<a id="display"></a>

## Display modes and presentation

SDL window, drawable, framebuffer and logical canvas are coordinated through guarded changes and fallback. Texture restoration supports context recreation. Modal window movement services the game/menu and audio paths. VSync and caps are options; high-refresh simulation and the 1000-FPS setting remain audit tasks.

[src/main.c](../../src/main.c) · [Code and examples](technical.md#display)

| ID | Type | Change |
|---|---|---|

| `display-01` | feature | Windowed, borderless and exclusive modes |

| `display-02` | fix | Transactional video-mode switching |

| `display-03` | fix | Texture rehydration after context recreation |

| `display-04` | fix | Resolution and refresh synchronization |

| `display-05` | QoL | Letterboxed and widescreen canvases |

| `display-06` | feature | VSync and frame-cap options |

| `display-07` | fix | Loop/audio continuity during window dragging |

| `display-08` | fix | GL state restoration across menus/gameplay |

<a id="geometry"></a>

## Projection and geometric precision

Fractional render coordinates survive transforms and clipping. Dedicated world/viewmodel projections synchronize camera pose, frustum and active visibility descriptors without replacing the native gameplay representation.

[src/render_geometry.c](../../src/render_geometry.c) · [Code and examples](technical.md#geometry)

| ID | Type | Change |
|---|---|---|

| `geometry-01` | fix | Float render transforms and clipping |

| `geometry-02` | fix | Hierarchical moving-model section precision |

| `geometry-03` | fix | Current-pose camera-space viewmodels |

| `geometry-04` | feature | World and weapon FOV per species |

| `geometry-05` | fix | Hor-plus projection and frustum synchronization |

| `geometry-06` | fix | Geometry seams and near-plane handling |

| `geometry-07` | QoL | Calibrated weapon viewmodel positions |

<a id="render"></a>

## Renderer, lighting and effects

Texture cutouts also constrain the additional luminous/depth pass. Shared UV clocks fix luminous-layer drift. Glass/effect ordering preserves visibility. Ordered buffer streaming and fire batching reduce submission work with original draw/blend/filter behavior and fallbacks.

[src/opengl.c](../../src/opengl.c) · [Code and examples](technical.md#render)

| ID | Type | Change |
|---|---|---|

| `render-01` | fix | Alpha-aware luminous/specular depth pass |

| `render-02` | fix | Animated luminous UV synchronization |

| `render-03` | fix | Particles and halos visible through glass |

| `render-04` | feature | Ordered VBO/IBO streaming with fallback |

| `render-05` | fix | Fire/particle rendering work reduction |

| `render-06` | feature | Extended model-detail and draw-distance controls |

| `render-07` | QoL | Texture and HUD filtering controls |

<a id="water"></a>

## Water simulation and rendering

Time-based wave propagation and damping avoid integer loss and uncontrolled per-frame disturbances. Modern optics add depth-aware refraction, Fresnel, scene reflections, native-light specular and caustics. Screen-space effects cannot reflect unseen geometry.

[src/water_effect.c](../../src/water_effect.c) · [Code and examples](technical.md#water)

| ID | Type | Change |
|---|---|---|

| `water-01` | fix | Time-based ripple propagation and damping |

| `water-02` | feature | Selectable classic and modern water |

| `water-03` | feature | Depth-aware refraction with foreground protection |

| `water-04` | feature | Fresnel and screen-space reflections |

| `water-05` | feature | Native-light specular and refracted-ray caustics |

| `water-06` | fix | GPU water grid and bounded reflection traversal |

<a id="postfx"></a>

## CRT, noise and image controls

Native GLSL CRT, image controls and monitor noise are selectable. Constant or interior-region work is kept out of expensive fragment paths where equivalent. CRT provenance includes Mattias/newpixie and CRT-Guest-ReShade; credits do not substitute license notices.

[src/crt_filter.c](../../src/crt_filter.c) · [Code and examples](technical.md#postfx)

| ID | Type | Change |
|---|---|---|

| `postfx-01` | feature | Native GLSL CRT filter |

| `postfx-02` | feature | Brightness, contrast and saturation controls |

| `postfx-03` | feature | RGB/CRT monitor no-signal modes |

| `postfx-04` | QoL | Noise and grain scale options |

| `postfx-05` | fix | CRT shader work reduction |

<a id="ui"></a>

## Main menu, in-game menu and UX

Menu presentation uses explicit logical spaces, atlas sampling guards and supported Latin composition. Species profiles retain scales. Ordinary settings apply with Escape; video-mode confirmation is deliberately separate. Supported save/load slots remain discoverable in carousels.

[src/avp/win95/frontend/avp_menus.c](../../src/avp/win95/frontend/avp_menus.c) · [Code and examples](technical.md#ui)

| ID | Type | Change |
|---|---|---|

| `ui-01` | QoL | Tyrargo identity and shared version labels |

| `ui-02` | QoL | Widescreen Starfield and restored menu backgrounds |

| `ui-03` | fix | Glyph/HUD atlas bleeding and frame borders |

| `ui-04` | fix | Accented Latin glyph composition |

| `ui-05` | feature | Independent HUD, text, console and crosshair scales |

| `ui-06` | QoL | In-game HUD/text/movie/image previews |

| `ui-07` | QoL | Escape applies settings and closes menus/console |

| `ui-08` | QoL | 50 save slots plus dedicated quicksave/quickload |

| `ui-09` | QoL | Notification duration and adaptive FPS placement |

| `ui-10` | fix | Profile, startup fade and menu-transition cleanup |

| `ui-11` | fix | Native console cheats enabled by -debug in public builds |

<a id="media"></a>

## Movies and screenshots

Bundled Smacker/Bink decoders and buffered audio drive playback. Physical movie viewports support wide modes. Original commercial media is supplied by the player. BMP/PNG/JPEG captures exclude their own notifications.

[src/fmv_backend.cpp](../../src/fmv_backend.cpp) · [Code and examples](technical.md#media)

| ID | Type | Change |
|---|---|---|

| `media-01` | feature | Native Smacker and Bink playback |

| `media-02` | fix | Movie audio clock, RGB and end synchronization |

| `media-03` | feature | Native, scaled, wide and ultrawide movies |

| `media-04` | feature | Optional IntroSound99 menu-music choice |

| `media-05` | feature | BMP, PNG and JPEG menu/game screenshots |

| `media-06` | fix | Clean repeated screenshots |

<a id="audio"></a>

## Audio, music and latency

OpenAL emitters follow world positions and environmental routing. Weapon overlap and feeding paths include sound handling. CD-DA support includes drive detection and volume; repeat modes include random silence. Bounded diagnostics defer writes away from frames.

[src/openal.c](../../src/openal.c) · [Code and examples](technical.md#audio)

| ID | Type | Change |
|---|---|---|

| `audio-01` | feature | Environmental EFX and positional NPC effects |

| `audio-02` | fix | Overlapping primary/secondary weapon audio |

| `audio-03` | fix | Alien feeding and combat SFX handling |

| `audio-04` | feature | CD-DA/Red Book music support |

| `audio-05` | feature | One-shot, looping and random-silence music |

| `audio-06` | fix | Bounded event/audio latency handling |

<a id="player"></a>

## Player movement and weapons

Wall contacts, climbing and moving support have dedicated corrections while slope friction remains. Input options include run/crouch and smart zoom/jetpack. Fire-input queues retain valid taps; interaction/cooldown paths avoid frame-driven bursts.

[src/avp/pmove.c](../../src/avp/pmove.c) · [Code and examples](technical.md#player)

| ID | Type | Change |
|---|---|---|

| `player-01` | fix | Wall sliding and curved-surface contact |

| `player-02` | fix | Moving-platform support and jump contact |

| `player-03` | fix | Alien stairs, wall turns and climbing transitions |

| `player-04` | feature | Smart jetpack and Predator zoom controls |

| `player-05` | QoL | Always-run, crouch toggle and expanded pitch |

| `player-06` | fix | Semiautomatic edge/latch/queue input |

| `player-07` | fix | Controlled repeated flare interval |

| `player-08` | fix | Reachable pickup and door interaction |

<a id="motion"></a>

## Camera, weapon and HUD motion

Marine camera/viewmodel/HUD layers cover movement and equipment events with configurable channels. Transitions include stance, stairs and landing. Development calibration is opt-in source code; production retains imported values and public intensity controls.

[src/camera_motion.c](../../src/camera_motion.c) · [Code and examples](technical.md#motion)

| ID | Type | Change |
|---|---|---|

| `motion-01` | feature | Procedural weapon walk/run bob and sway |

| `motion-02` | feature | Smooth crouch, stairs, jump and landing feedback |

| `motion-03` | feature | Weapon-specific HUD shake profiles |

| `motion-04` | QoL | Separate motion intensity sliders |

| `motion-05` | fix | Motion lifetime and run/pitch recovery |

<a id="campaign"></a>

## Campaign, resources and Molecular Acid

Acid is a single-player modifier over Hard, not an extra index into native difficulty arrays. UI labels x1/x1.5/x2 map to enemy factors 1.5/2/2.5 and resource factors 1.5/2/3. Exceptions are quantities, not multipliers. Campaign state and optional ammo economy have separate lifetimes.

[src/molecular_acid.c](../../src/molecular_acid.c) · [Code and examples](technical.md#campaign)

| ID | Type | Change |
|---|---|---|

| `campaign-01` | feature | Optional player-state persistence between episodes |

| `campaign-02` | feature | Optimized ammunition campaign mode |

| `campaign-03` | feature | Molecular Acid tiers and population pressure |

| `campaign-04` | feature | Acid storage and magazine capacity rules |

| `campaign-05` | feature | Pulse Rifle/SADAR/marine-disc pickups of 3/4/5 |

| `campaign-06` | feature | Predator energy scaling and 15-point base armor |

| `campaign-07` | fix | Bonus/custom-map Acid handling |

| `campaign-08` | fix | Dormant visibility and wakeup recovery |

| `campaign-09` | QoL | Briefing spacing, episode memory and retry screens |

<a id="aliens"></a>

## Alien NPC behavior and navigation

Fractional timing, local steering and support/contact recovery address tested vents, hatches and edges. Leap authorization is bounded: not every large leap is caused by FPS. Corpse feeding includes its animation and audio paths. Remaining arbitrary-map recovery needs playtesting.

[src/avp/bh_alien.c](../../src/avp/bh_alien.c) · [Code and examples](technical.md#aliens)

| ID | Type | Change |
|---|---|---|

| `aliens-01` | fix | Fractional NPC movement timing |

| `aliens-02` | fix | Vent lips, square hatches and thin-edge recovery |

| `aliens-03` | fix | Bounded vertical leap authorization and support |

| `aliens-04` | feature | Player-corpse feeding behavior |

| `aliens-05` | fix | Progress-aware, committed recovery at vent edges without discarding the target |

<a id="predators"></a>

## Predator NPC combat and patrol

Private weapon state, target acquisition and patrol extend native combat. Moving shoulder-cannon acquisition uses locomotion while active shots retain native completion timing. Level navigation and cloak visibility still constrain pursuit.

[src/avp/bh_pred.c](../../src/avp/bh_pred.c) · [Code and examples](technical.md#predators)

| ID | Type | Change |
|---|---|---|

| `predators-01` | feature | Private NPC weapon inventory and selection |

| `predators-02` | feature | Predator kill taunts |

| `predators-03` | fix | Weapon-specific acquisition and aiming |

| `predators-04` | feature | Patrol and pursuit planning |

| `predators-05` | fix | Walk/run animations during moving cannon acquisition |

<a id="marines"></a>

## Marine NPC squad, inventory and tactics

Use commands toggle follow/patrol with acknowledgment. Shared jobs, staged patrol and pacing hysteresis reduce conflicting decisions. Marines collect supported extra weapons/resources, select weapons, assist with sentries and leave recoverable drops. Saved inventories/planning have reset/load guards.

[src/avp/npc_support.c](../../src/avp/npc_support.c) · [Code and examples](technical.md#marines)

| ID | Type | Change |
|---|---|---|

| `marines-01` | feature | Use-to-command follow/patrol with acknowledgment |

| `marines-02` | feature | Coordinated expanding exploration planner |

| `marines-03` | fix | Follow pacing, stopping distance and separation |

| `marines-04` | fix | APC access, exit and slope handling |

| `marines-05` | feature | Shared randomized supply jobs with human priority |

| `marines-06` | feature | Two distinct additional weapon pickups |

| `marines-07` | feature | Resource thresholds: health/armor 60%, ammo 30% |

| `marines-08` | feature | Player-defined weapon cadence, reloads, fuel and private inventory |

| `marines-09` | fix | Smartgun convergence, ally exclusion and valid capture-area selection |

| `marines-10` | fix | Weapon-specific aiming and committed projectile ammunition |

| `marines-11` | feature | Nearby sentry support task |

| `marines-12` | feature | Marine kill celebrations |

| `marines-13` | feature | Recoverable weapon drops on death |

| `marines-14` | fix | Persisted squad, planner and per-weapon timing state |

<a id="kernel"></a>

## Engine lifetime, performance and diagnostics

A bounded immutable-mesh BVH selects collision candidates while retaining native polygon math/order. Dynamic/morph/animated meshes fall back. Cache lifetime precedes level-pool teardown. Staged AI/support work and investigated crash guards retain full effects and NPC populations; results are scenario-specific.

[src/avp/particle_mesh_cache.inc](../../src/avp/particle_mesh_cache.inc) · [Code and examples](technical.md#kernel)

| ID | Type | Change |
|---|---|---|

| `kernel-01` | feature | Immutable-mesh BVH for actor/particle collisions |

| `kernel-02` | fix | Dynamic-mesh fallback and cache teardown |

| `kernel-03` | fix | Shared/staged AI and support work |

| `kernel-04` | fix | Particle, NPC and level-state crash guards |

| `kernel-05` | QoL | Separate diagnostic and production builds |
