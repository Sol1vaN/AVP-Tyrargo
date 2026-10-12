# Third-party notices

The original Rebellion engine and its derivatives retain the non-commercial [LICENSE](LICENSE). Independently licensed libraries, shader adaptations and build helpers retain their own terms; the repository and executable are not uniformly MIT or GPL. The core restriction does not replace those component grants.

| Component | Use and license |
|---|---|
| Rebellion AvP / NakedAVP | Inherited engine; original notices and [non-commercial license](LICENSE) retained |
| OpenLara water adaptation | XProger; [BSD 2-Clause](LICENSES/OpenLara-BSD-2-Clause.txt) |
| libsmacker | Static decoder; Greg Kennedy; [LGPL 2.1-or-later](src/third_party/libsmacker/COPYING) |
| libbinkdec / FFmpeg transforms | Static decoder/transforms; Barry Duncan, Konstantin Shishkov, Peter Ross and other retained header authors; [LGPL 2.1-or-later](LICENSES/LGPL-2.1.txt) |
| stb_image_write | [MIT/public-domain alternatives](src/third_party/stb/LICENSE.txt) |
| Integrated MattiasCRT adaptation | Iñigo Quilez postprocessing reference through the supplied MattiasCRT/expanded ReShade shader; GLSL translation and adaptations identified in `src/crt_filter.c`; [CC BY-NC-SA 3.0](LICENSES/CC-BY-NC-SA-3.0.txt) |
| newpixie reference | Mattias Gustavsson; [MIT notice](LICENSES/newpixie-MIT.txt) retained for the reference; it does not replace the supplied CRT adaptation's CC notice |
| CRT-Guest-ReShade reference | HelelSingh / DevilSingh, guest(r), Dr. Venom and Libretro; the separate GPL Guest shader is credited but not bundled in Tyrargo |
| SDL3 3.4.14 | Dynamic DLL; [zlib notice](LICENSES/runtime/SDL3/LICENSE.txt) |
| OpenAL Soft 1.25.2 | Dynamic library; Chris Robinson and retained upstream authors; [Library GPL 2-or-later](LICENSES/OpenAL-LGPL-2.0.txt). The MSYS2 package also builds utilities; its aggregate GPL metadata is not the library file's notice. |
| GCC 16.2.0 runtimes | Dynamic libgcc/libstdc++; [GPL 3](LICENSES/runtime/gcc-libs/COPYING3) with [GCC Runtime Library Exception](LICENSES/runtime/gcc-libs/COPYING.RUNTIME); retained [LGPL notices](LICENSES/runtime/gcc-libs/COPYING.LIB) where applicable |
| libiconv 1.19 | Dynamic runtime; [LGPL 2.1-or-later](LICENSES/runtime/libiconv/COPYING.LIB); documentation has separate notices |
| winpthreads 14.0.0 development a7cb47123 | Dynamic runtime; [MIT/BSD notices](LICENSES/runtime/libwinpthread/COPYING) |
| FindSDL3/CMake helpers | Retained BSD notices in the helper source; author Eric Wing and listed contributors |

## Source and relinking materials

The same release page supplies the production source, Windows binary package, dependency sources/build recipes, and relinkable object files with a rebuild/relink script. Static decoder sources and their notices are included in the production source. Dependency archives preserve their upstream licenses and include the matching MSYS2 build recipes. Exact packaged DLL hashes/versions are recorded in the Windows package manifest.

You may modify the LGPL libraries for your own use and reverse engineer the linked program to debug those modifications, as permitted by their licenses. Replace dynamically linked libraries with compatible builds or use the supplied relinking materials to replace the static decoders. No restriction in this project's notices is intended to remove those LGPL permissions. Component modifications and source headers remain identifiable.

Commercial game data, local language assets, maps, movies, audio, profiles and saves are not distributed. Showcase images are illustrative user-supplied game captures; underlying game art and franchise rights remain with their owners. They are not granted the engine's code license by appearing here.

## Español

El motor original conserva la licencia no comercial de Rebellion. Librerías independientes y adaptaciones del shader conservan sus propias licencias; no se presentan como una licencia MIT/GPL uniforme. La release incluye código, fuentes de dependencias y objetos/script para recompilar y volver a enlazar decodificadores. Se permiten las modificaciones y la ingeniería inversa necesarias para depurar cambios de las librerías LGPL conforme a sus licencias. No se incluyen recursos comerciales ni datos personales.
