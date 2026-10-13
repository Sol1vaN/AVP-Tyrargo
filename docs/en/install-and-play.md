[English](../../README.md) · [Español](../es/install-and-play.md)

# How to install and play

## What you need

Tyrargo uses data from these editions:

- **Aliens versus Predator Gold Edition — retail CD** (installed game data).
- **Aliens versus Predator Classic 2000 — Steam or GOG**.

You must own and install the game. The package supplies the port and its runtime libraries; it does not supply commercial maps, textures, audio, movies or custom maps. Windows x64 is the supported binary target. [Downloads](https://github.com/Sol1vaN/AVP-Tyrargo/releases) · [Compile from source](build.md).

## Install and play

1. Close the game and back up your profiles/saves. Extract the Windows x64 ZIP **into the existing game directory**, keeping the package's directories and license notices. You may instead use a separate copy of the game directory.
2. The supplied launcher is **`avpt.exe`**. It has a distinct name from the stock game executables; keep those original executables. Keep the supplied runtime DLLs next to `avpt.exe`. If the destination already contains a previous Tyrargo build, replace that build's matching files together.
3. Check that `avp_rifs/temple.rif` and `fastfile/ffinfo.txt` exist. Preserve all installed game-data directories. Some editions package graphics/audio in `fastfile`; a separate `graphics` directory is not required in that case.
4. Start `avpt.exe` from that game directory. Select/create your profile and configure Audio/Video and species controls. The stock launcher can still start the original game.

For Steam, a typical destination is `Steam/steamapps/common/Aliens versus Predator Classic`. For Gold CD and GOG, use the directory you selected during installation. Steam/GOG availability does not mean the port runs through the original executable automatically.

Alternatively, keep the port in its own directory and select an existing data installation:

```powershell
.\avpt.exe -p 'D:\Games\Aliens versus Predator Classic'
```

`-p` selects the data directory. Configuration/profile location is resolved separately by the engine and environment; preserve `.avp` and existing user profiles when upgrading. `AVP_DATA` is another supported way to select data. A directory containing only the port and DLLs is not a complete game.

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

## Smartgun tracking

The smartgun starts in Tracking mode in a new game. Secondary fire switches between Tracking and Free modes. In Tracking, the targeting sight follows detected enemies and turns red, while the gun converges towards the target. A saved game restores its saved mode, which can be Free; use secondary fire to re-enable Tracking. Allied targets remain excluded.

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
