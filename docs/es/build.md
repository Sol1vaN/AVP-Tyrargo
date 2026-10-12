[English](../en/build.md) · [Español](../../README.es.md)

# Compilar, diagnosticar y mantener versiones

## Plataforma validada

Windows x64, OpenGL de escritorio, GCC UCRT64 de MSYS2, CMake y Ninja. La base importada usó GCC 16.2.0, SDL3 3.4.14 y OpenAL Soft 1.25.2. Las versiones de paquetes pueden avanzar: registrar las utilizadas en cada build. Linux/macOS/GLES son rutas heredadas todavía sin certificación Tyrargo.

## Dependencias y compilación

Instalar/actualizar [MSYS2](https://www.msys2.org/) siguiendo sus instrucciones, abrir **UCRT64** e instalar:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-sdl3 mingw-w64-ucrt-x86_64-openal
```

[Paquete SDL3](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-sdl3) · [Paquete OpenAL](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-openal).

Desde la carpeta del repositorio, en PowerShell:

```powershell
.\tools\build-windows.ps1 -CMake C:\msys64\ucrt64\bin\cmake.exe -Jobs 4
```

Admite `-MsysRoot`, `-CMake`, `-BuildDir` y `-Jobs`. Selecciona UCRT64/OpenGL, falla si hay error y genera **compiled_timestamps.csv** para objetos y ejecutable. No instala, sube ni publica nada.

Alternativa desde entorno con compilador y bibliotecas UCRT64 en sus rutas:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENGL_TYPE=OPENGL
cmake --build build --parallel 4
```

Usar DLLs de la misma toolchain. [Instalación](install-and-play.md) explica recursos y directorio de trabajo. Se necesitan datos comerciales para jugar, no para compilar. Los decodificadores incluidos evitan requerir un paquete FFmpeg aparte en este snapshot.

## Codificación y versiones

Conservar bytes/codificación/saltos de línea del código legacy; `.gitattributes` impide normalizaciones involuntarias. Documentación/Python usan UTF-8. Evitar formateadores o conversiones globales.

`src/tyrargo_version.h` alimenta versión C/C++, CMake y recursos Windows. Importación inicial **v6.249 RC1**, PE `6.249.0.1`. Major 6 es una convención Tyrargo; no es la versión mayor upstream. RC significa candidato, no estable.

Actualizar deliberadamente header y registros `docs/data/features.json`, `history.json`, `roadmap.json`, luego:

```sh
python tools/update_docs.py
python tools/update_docs.py --check
python tools/check_docs.py
```

El generador toma conteos bilingües y extractos del catálogo/source actual. GitHub muestra Markdown guardado: cifras cambian al regenerar, hacer commit y push. No existe publicación remota automática.

## Instrumentación y pruebas

[INSTRUMENTATION](../../INSTRUMENTATION.md) documenta flags de desarrollo y medición de latencia en producción. Revisar [flujos técnicos](technical.md) y [rendimiento](performance.md).

Probar mapas originales/custom, especies y visiones, cambios de vídeo/reinicio, guardar/cargar e inventario/planner, partículas/vidrios/luces, input a varios límites y NPC follow/suministro/patrulla/combate. Validar documentos no reemplaza pruebas gameplay.

## Paquetes

Empaquetar solo una build probada por jugadores, DLLs correspondientes, licencias/materiales source/relink y manifiesto SHA256/fechas. Excluir recursos comerciales, perfiles, saves y diagnósticos. Releases son manuales y pueden comenzar como borrador; resolver antes [revisión de distribución](../../THIRD_PARTY_NOTICES.md).

Preferir una ruta corta ASCII, por ejemplo `C:/src/AVP-Tyrargo`. Algunos windres fallan con comandos largos/rutas no ASCII; un alias temporal de unidad es otra opción local. No requiere modificar el motor.
