# Credits / Créditos

## Original game and source / Juego y código originales

**Rebellion Developments**, creators and owners of the original Aliens versus Predator source, copyright 1999–2000. The original release was published by Fox Interactive. The Rebellion [license](LICENSE) is retained verbatim: non-commercial use, visible attribution and preservation of its conditions. This fan project is not affiliated with Rebellion or the franchise rights holders.

## Direct base and community ports / Base directa y ports comunitarios

- **[atsb / NakedAVP](https://github.com/atsb/NakedAVP)**: direct Tyrargo base, specifically tag **1.2.3**, commit `a360b0b2c80196345ba3e2553476f4f649e7e54f`. Upstream Git history is preserved as ancestry of the Tyrargo import. SDL3 work, cleanups and fixes belong to their original contributors.
- **[icculus.org AvP port](https://icculus.org/avp/)** and its contributors. The inherited README specifically credits **Tim Beckmann** for cleanup fixes and **Chuck Mason** for the initial OpenAL implementation.
- **Sol1vaN / RaideR**: Tyrargo direction, calibration, supplied screenshots, save-based reproduction and the maintained project.

## Water / Agua

- **[XProger / OpenLara](https://github.com/XProger/OpenLara)**: water-wave approach/reference, with its [BSD 2-Clause notice](LICENSES/OpenLara-BSD-2-Clause.txt) retained. The Tyrargo water module identifies its AvP-specific integration.
- **[FluffyQuack / OpenLara](https://github.com/FluffyQuack/OpenLara)**: community work and water reference requested by the maintainer. The original XProger authorship is preserved alongside this credit.

## CRT shaders

- **[Mattias Gustavsson / newpixie](https://github.com/mattiasgustavsson/newpixie)**: CRT emulation work and GLSL reference. `crtemu.h` provides MIT/public-domain alternatives; the [MIT notice](LICENSES/newpixie-MIT.txt) is preserved here.
- **[HelelSingh / CRT-Guest-ReShade](https://github.com/HelelSingh/CRT-Guest-ReShade)**: CRT shader reference and community port. Its `CRT-Guest-Advanced.fx` credits **guest(r)**, **Dr. Venom**, the **Libretro community**, and the ReShade port by **DevilSingh with guest(r)**. That shader declares GPL-2.0-or-later. Attribution here does not relabel GPL code as MIT or as the Rebellion license.
- **Iñigo Quilez**: postprocessing shader credited by the supplied MattiasCRT ReShade source. The integrated `src/crt_filter.c` translates the expanded `MattiasCRT_Unlocked_Ex.fx` into GLSL and retains its **CC BY-NC-SA 3.0 Unported** notice. Changes include GLSL translation, fixed profile values, previews/image controls and rounded-border optimization. [License](LICENSES/CC-BY-NC-SA-3.0.txt). The newpixie MIT reference and the separate Guest GPL shader are not used to relabel this adaptation.

## Libraries / Bibliotecas

- **[SDL](https://github.com/libsdl-org/SDL)** contributors: window/input/platform facilities.
- **[OpenAL Soft](https://github.com/kcat/openal-soft)** contributors: audio and environmental effects.
- **libsmacker** authors, credited in `src/third_party/libsmacker`; **Barry Duncan**, **Konstantin Shishkov**, **Peter Ross** and FFmpeg contributors for libbinkdec and its inherited decoder/transform code.
- **[Sean Barrett and stb contributors](https://github.com/nothings/stb)**: image writing, with the bundled notice retained.
- CMake/FindSDL module contributors, including **Eric Wing**, whose inherited notices remain in source; MSYS2, GCC and Ninja contributors for the Windows build environment.

## Community knowledge and presentation / Conocimiento y presentación

- **[AvP community at Steam](https://steamcommunity.com/app/3730/discussions/)**: historical behavior reports, custom maps and source-port discussion.
- **[PCGamingWiki: Aliens versus Predator](https://www.pcgamingwiki.com/wiki/Aliens_versus_Predator)**: historical configuration/input references. Community observations provide reproduction leads, not proof of the cause of every symptom.
- **AvPx and earlier AvP source-port projects**: historical comparison references; Tyrargo is not an AvPx code fork.
- README/documentation layout references: **[OpenXRay](https://github.com/OpenXRay/xray-16)**, **[Quake III Arena R](https://github.com/fknfilewalker/Quake-III-Arena-R)**, **[TRX](https://github.com/LostArtefacts/TRX)** and **[CroftEngine](https://github.com/stohrendorf/CroftEngine)**. Layout inspiration does not mean their engines are bundled here.

Captured game artwork, screenshots and the supplied banner retain their underlying rights. Original commercial assets and custom-map packages are not redistributed in the source repository.

Los créditos no sustituyen las condiciones de licencia. Se conserva autoría y avisos originales; las referencias visuales/comunitarias no se presentan como código incluido. La revisión previa a releases está detallada en [avisos de terceros](THIRD_PARTY_NOTICES.md).
