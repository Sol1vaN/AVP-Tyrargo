# Third-party notices / Avisos de terceros

The project is not uniformly MIT/GPL licensed. Rebellion's original source is governed by [LICENSE](LICENSE); component notices remain applicable to their own code. The current publication is a private source review, not a binary redistribution certification.

| Component | Use in this snapshot | Notice |
|---|---|---|
| Rebellion AvP source | Core inherited engine | Original non-commercial [LICENSE](LICENSE) |
| OpenLara water reference | Identified water-wave adaptation | [BSD 2-Clause](LICENSES/OpenLara-BSD-2-Clause.txt), original author XProger |
| libsmacker | Bundled decoder compiled into the executable | [LGPL 2.1](src/third_party/libsmacker/COPYING); preserve source headers |
| libbinkdec / inherited FFmpeg transforms | Bundled decoder/transform sources compiled into the executable | LGPL 2.1-or-later notices in the component headers; [license text](LICENSES/LGPL-2.1.txt) |
| stb_image_write | Bundled image writer | [MIT/public-domain alternatives](src/third_party/stb/LICENSE.txt) |
| newpixie CRT reference | CRT reference, intermediate port identified in crt_filter.c | [Mattias Gustavsson MIT notice](LICENSES/newpixie-MIT.txt) |
| CRT-Guest-ReShade | Credited reference; original .fx is not bundled here | Referenced Guest shader declares GPL-2.0-or-later; do not apply the core's non-commercial license to it |
| SDL3 | External dynamic runtime dependency | [SDL upstream license](https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt) |
| OpenAL Soft | External dynamic runtime dependency | [Upstream component notices](https://github.com/kcat/openal-soft) |
| GCC runtime / libiconv / winpthreads | Windows runtime dependencies as needed | Preserve the matching toolchain/package notices when packaging |
| FindSDL3/CMake helpers | Build discovery | Inherited notice in each helper |

## Before binary/public release

- Complete the mapping from `MattiasCRT_Unlocked_Ex.fx` to its original/intermediate shader notices. Adding a credit does not resolve a missing license or incompatible copied component.
- For static LGPL decoders, prepare the applicable corresponding source/build materials and a usable relinking route; inspect how those obligations interact with the inherited non-commercial core before public binary distribution. A source ZIP alone is not automatically the complete relinking material.
- Include notices and corresponding-source references for the exact dynamic DLL versions packaged, including dependent runtime libraries. Do not assume a package's single metadata label covers all of its files.
- Preserve existing source notices and resolve any directly copied third-party implementation's conditions. Omitting a product name from presentation is not a license change.
- Exclude commercial game/movie/audio/map data; document where users supply their own installation. Review screenshot/banner rights separately from engine-source rights.

En la revisión privada se conserva el código y sus avisos. Antes de distribuir binarios públicamente, completar procedencia del CRT, materiales para relink de decodificadores LGPL y licencias de las DLL exactas. Los créditos, por sí solos, no resuelven obligaciones de distribución.
