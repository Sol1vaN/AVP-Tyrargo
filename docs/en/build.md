[English](../../README.md) · [Español](../es/build.md)

# Build, diagnostics and versioning

## Validated target

Windows x64, desktop OpenGL, MSYS2 UCRT64 GCC, CMake and Ninja. The import baseline was built using GCC 16.2.0, SDL3 3.4.14 and OpenAL Soft 1.25.2. Package versions may advance; record versions for each new build. Linux/macOS/GLES are inherited code paths, not currently certified Tyrargo targets.

## Dependencies and compilation

Install [MSYS2](https://www.msys2.org/), update it according to its instructions and open **UCRT64**, then install:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-openal
```

[SDL3 package](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-sdl3) · [OpenAL package](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-openal).

In Windows PowerShell, from the repository:

```powershell
.\tools\build-windows.ps1 -CMake C:\msys64\ucrt64\bin\cmake.exe -Jobs 4
```

`-MsysRoot`, `-CMake`, `-BuildDir` and `-Jobs` are configurable. The helper explicitly selects UCRT64 and desktop OpenGL, fails on configuration/build errors, and writes **compiled_timestamps.csv** for objects and `avpt.exe`. It never copies to a game install, pushes or publishes.

Equivalent configure/build from an environment with the UCRT64 compiler and libraries on its paths:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENGL_TYPE=OPENGL
cmake --build build --parallel 4
```

Use the same UCRT64 runtime libraries with your executable. [Installation](install-and-play.md) explains original data and the working directory. Original data is needed to play, not to compile. Bundled Smacker/Bink decoder sources mean this snapshot does not require a separate FFmpeg package.

## Source precision and encoding

Legacy C/C++ source retains its existing byte encoding and line endings; `.gitattributes` prevents unintended normalization of those files. Documentation and Python utilities use UTF-8. Avoid applying whole-tree formatters or encoding conversions to the import.

## Version and documentation

`src/tyrargo_version.h` is the source of C/C++, CMake and Windows resource version values. The initial import is **v6.249 RC1**, PE version `6.249.0.1`. Major 6 is Tyrargo's chosen version convention; it is not NakedAVP's upstream major version. RC means release candidate, not stable.

After a deliberate new iteration, update the header and the relevant records in `docs/data/features.json`, `history.json` and `roadmap.json`. Then:

```sh
python tools/update_docs.py
python tools/update_docs.py --check
python tools/check_docs.py
```

The generator derives bilingual counts and code excerpts from the current catalog/source. GitHub Markdown is a stored document, so its visible counts change when refreshed docs are committed and pushed; there is no automatic remote publication.

## Instrumentation and validation

[INSTRUMENTATION](../../INSTRUMENTATION.md) describes opt-in development switches and production latency measurement. For the latest changes, inspect [technical flows](technical.md) and [performance evidence](performance.md).

Regression coverage should include: original and custom maps; all species/vision modes; display changes and native restart; save/load and squad inventory/planner state; mixed particle/glass/luminous rendering; input at several frame caps; NPC follow, supply, patrol and combat. Documentation validation is not a replacement for gameplay testing.

## Packaging

Package only a player-tested build with matching runtime DLLs, license/source/relink materials and a SHA256/timestamp manifest. Keep game assets, profiles, saves and diagnostic builds out. Releases remain manual and can start as drafts; [distribution-review items](../../THIRD_PARTY_NOTICES.md) must be resolved first.

Prefer a short ASCII checkout path such as `C:/src/AVP-Tyrargo`. Some windres builds fail when long/non-ASCII include paths make their preprocessor command unusable; a temporary drive alias is another local option. This does not require changing engine code.
