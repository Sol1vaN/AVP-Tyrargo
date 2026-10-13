<p align="center"><img src="docs/media/banner.png" alt="Aliens vs Predator Tyrargo" width="100%"></p>
<h1 align="center">Aliens vs Predator Tyrargo</h1>
<p align="center"><b>English</b> · <a href="README.es.md">Español</a></p>

Tyrargo improves the classic Aliens versus Predator engine through precise input, richer audiovisual presentation, expanded single-player systems and coordinated NPC behavior. It is derived from [atsb's NakedAVP 1.2.3](https://github.com/atsb/NakedAVP), building on the original Rebellion code and earlier community ports.

This is an independent, non-commercial fan project, unaffiliated with Rebellion or the franchise's rights holders. Rebellion created and owns the original game/source; its [non-commercial license and required credits](LICENSE) remain in force. Original game data must be supplied separately.

![Halos and lighting — the selected Tyrargo showcase screenshot](docs/media/hero.png)

![Marine gameplay](docs/media/hero-marine.jpg)

**Installation instructions are on the [How to install and play](docs/en/install-and-play.md) page.** Developers: [build from source](docs/en/build.md).

Current release candidate: **{{VERSION}}**, Windows x64. **[Download binaries and release notes](https://github.com/Sol1vaN/AVP-Tyrargo/releases/tag/v6.252-rc1)**. Supply your own Gold CD or Classic Steam/GOG game data. The same release includes source, dependency sources and relinking materials. Broader player testing continues.

## Showcase

<p align="center"><a href="https://youtu.be/p9oYk3mTuwo"><img src="https://img.youtube.com/vi/p9oYk3mTuwo/hqdefault.jpg" alt="Play the Tyrargo showcase on YouTube" width="640"></a><br><a href="https://youtu.be/p9oYk3mTuwo">▶ Watch the showcase</a></p>

GitHub READMEs do not embed a YouTube player. This preview opens the video with one click.

![Same supplied CRT comparison scene, alternating off and on](docs/media/crt-comparison.gif)

*CRT off/on in Tyrargo, using the supplied comparison pair. GIF color quantization is for documentation only; it does not represent the renderer's color precision.* [Static comparisons and complete gallery](docs/en/gallery.md).

## Goals

- Preserve the atmosphere, assets and visual detail of the original while making input and presentation comfortable on modern systems.
- Improve expensive rendering, collision and NPC work without secretly removing effects or reducing enemy populations.
- Make single-player resources, squad commands and interface options explicit and configurable.
- Keep changes testable: compare behavior, pixels, frame times and save/load lifetime before accepting an optimization.

## Highlights

{{COUNTS}}

These are **consolidated implemented changes**, classified once each; counts are not promises of bug-free behavior. Repeated experiments are tracked in the [development history](docs/en/history.md), not counted again as new features.

{{HIGHLIGHTS}}

[Complete categorized catalog](docs/en/features.md) · [Technical explanations, code, flows and examples](docs/en/technical.md) · [Performance evidence](docs/en/performance.md).

## Main differences and ancestry

| Area | NakedAVP 1.2.3 baseline | Tyrargo |
|---|---|---|
| Input and precision | Community SDL3/OpenGL port of the original engine | Direct fractional mouse angles, improved projection and viewmodel precision |
| Presentation | Classic engine presentation | Per-species FOV/UI controls, live previews, CRT, modern water and wide-screen framing |
| Single player | Native campaign and difficulty behavior | Campaign persistence, ammunition options and Molecular Acid tiers |
| NPC support | Native species behavior | Follow/patrol commands, shared supply jobs, inventories, sentry tasks and navigation recovery |
| Performance | Legacy submission and collision paths | Ordered buffers, spatial collision filtering and bounded shared support work |

AvPx is a separate community enhancement referenced during historical comparisons. Tyrargo is **not an AvPx fork**. This table compares our inspected NakedAVP baseline with our own changes; it is not a feature-completeness or performance ranking against other ports. See [credits and references](CREDITS.md).

## Compilation and development

The maintained validation target is **Windows x64, MSYS2 UCRT64 GCC, SDL3, OpenAL and desktop OpenGL**. Smacker/Bink and screenshot helpers are included in the source. Other platforms and the inherited GLES path need independent validation before being advertised as supported.

See [compilation, diagnostics and versioning](docs/en/build.md) and [manual local/GitHub workflow](docs/en/workflow.md). Changes remain local until you explicitly push. No workflow publishes builds or releases automatically.

## Thanks

- **Rebellion**, original creators and source copyright holders; the earlier port contributors and the AvP community.
- **[atsb / NakedAVP](https://github.com/atsb/NakedAVP)**, the direct project base.
- **[XProger / OpenLara](https://github.com/XProger/OpenLara)** and **[FluffyQuack / OpenLara](https://github.com/FluffyQuack/OpenLara)**, water references and their community work.
- **[HelelSingh / CRT-Guest-ReShade](https://github.com/HelelSingh/CRT-Guest-ReShade)** and **[Mattias Gustavsson / newpixie](https://github.com/mattiasgustavsson/newpixie)**, CRT shader references and work.
- Decoder, SDL, OpenAL, stb and toolchain authors. Full attribution and component licenses: [CREDITS](CREDITS.md) and [third-party notices](THIRD_PARTY_NOTICES.md).

## TODO

{{ROADMAP}}

[Categorized roadmap and status](docs/en/roadmap.md). Planned and conceptual entries are not implemented features.
