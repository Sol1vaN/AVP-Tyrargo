[English](../../README.md) · [Español](../es/install-and-play.md)

# How to install and play

## What you need

A legal installation of Aliens versus Predator Gold/Classic and a Windows x64 Tyrargo package approved by the maintainer. The source repository does not include original maps, textures, movies, audio or custom maps. This repository currently contains a release-candidate source snapshot; there is no public binary download yet. Developers can [compile it](build.md).

## Install a reviewed Windows package

1. Close the game. Back up its existing executable/runtime DLLs, `.avp`, profile folders and saves. If the engine uses a user configuration directory, back up that directory too.
2. Extract the Tyrargo package into a copy of your existing game installation. Keep the package's runtime DLLs next to `avpt.exe`; keep the included license notices.
3. Verify that `avp_rifs/temple.rif` and `fastfile/ffinfo.txt` exist. Preserve the game's `avp_huds`, `graphics`, `Text`, `FMVs` and other supplied data directories.
4. Run `avpt.exe` with the game folder as its working directory. If data is stored elsewhere, point `AVP_DATA` at the folder containing `avp_rifs` and `fastfile`.
5. Create/select your profile, open Audio/Video and choose display mode, resolution and pacing. Configure species controls and FOV/interface scales to taste.

A folder containing only the executable and DLLs is not a complete game installation.

```powershell
$env:AVP_DATA = 'D:\Games\Aliens versus Predator Classic'
.\avpt.exe
```

The engine's configuration resolver may use the local `.avp` folder or a user `.avp` folder according to the environment. Do not delete an existing profile just to change executables.

## Basic Tyrargo options

| Option | Purpose |
|---|---|
| World/weapon FOV and species profile | Configure scene and weapon framing separately |
| HUD/crosshair/text scales | Adjust interface readability independently |
| CRT, image and water controls | Select the intended presentation; modern water is optional |
| Screenshot format and capture bind | BMP, PNG or JPEG from gameplay and menus |
| Manual reload and species weapon binds | Set your own controls |
| Smart jetpack/zoom and run/crouch toggles | Choose hold/toggle interaction |
| Quicksave/quickload | Dedicated quick slot, in addition to normal save slots |
| Persistence/optimized ammunition | Optional single-player campaign/resource modes |

Use the in-game binding menu as the source of truth for your keys; saved profiles can differ from defaults. Escape applies ordinary settings and returns; resolution changes use explicit confirmation.

## Molecular Acid

Acid modifies Hard/Director's Cut behavior in single player. Labels intentionally differ from internal resource factors:

| Menu label | Enemy factor | Resource factor | Pulse Rifle magazines / SADAR rockets / marine discs per pickup |
|---|---:|---:|---:|
| x1 | 1.5 | 1.5 | 3 |
| x1.5 | 2 | 2 | 4 |
| x2 | 2.5 | 3 | 5 |

Supported magazines/tanks and reserves expand; the listed exceptions use total pickup quantities. Predator energy uses the resource factor and Acid armor uses a 15-point base × that factor. Population handling respects supported generators/actors and authored map triggers; it is not a promise that every scripted wave always contains exactly the same multiplied count.

## Allied marines

Use an allied marine to toggle follow/patrol and receive an acknowledgment. Follow returns after combat; patrol uses coordinated expanding exploration. Marines can collect two distinct extra weapon types, reload/switch supported weapons, use nearby sentries and leave recoverable weapons when killed. Their resource pickup thresholds are below 60% health/armor and below 30% ammunition. Cramped supply jobs give the human player priority.

## Optional original menu music

If you own the original 1999 movie asset, supply it as `IntroSound99.smk` in the movie-data location used by your installation. Keep the Gold asset separately. The option appears only when the additional asset is available. No commercial movie is included in this repository.

## Custom maps and Steam

Follow each map author's installation instructions and back up the replaced level. A replacement Derelict map still occupies its original episode slot; there is no built-in friendly mod/map loader yet.

Steam launch setup is currently manual; automatic Steam configuration is a roadmap item. In Steam's game properties, an optional launch command is:

```text
"D:\Games\Aliens versus Predator Classic\avpt.exe" %command%
```

Replace the path with your installation. Tyrargo does not currently rewrite Steam's configuration on startup.

## Troubleshooting and reporting

Missing-data errors: check the working directory and the two validation files above. Missing-DLL errors: use DLLs from the same reviewed package/toolchain. For display problems, start from a supported windowed mode and report the requested and actual resolution. For crashes, poor frame times or stuck NPCs, report version, map, display settings, weapon/event and a reproducible save if available. Saves/logs can contain personal data; do not upload them publicly without review.

[Feature catalog](features.md) · [Known/planned work](roadmap.md) · [Compilation](build.md).
