<p align="center"><img src="docs/media/banner.png" alt="Aliens vs Predator Tyrargo" width="100%"></p>
<h1 align="center">Aliens vs Predator Tyrargo</h1>
<p align="center"><a href="README.md">English</a> · <b>Español</b></p>

Tyrargo mejora el motor clásico de Aliens versus Predator mediante entrada precisa, una presentación audiovisual ampliada, nuevos sistemas singleplayer y comportamiento NPC coordinado. Deriva de [NakedAVP 1.2.3 de atsb](https://github.com/atsb/NakedAVP), sobre el código original de Rebellion y ports anteriores de la comunidad.

Es un proyecto independiente de fans, no comercial, sin afiliación con Rebellion ni los titulares de la franquicia. Rebellion creó y posee el juego/código original; se conserva su [licencia no comercial y créditos obligatorios](LICENSE). Los datos originales del juego se suministran por separado.

![Halos e iluminación — captura elegida de Tyrargo](docs/media/hero.png)

**Las instrucciones están en [Cómo instalar y jugar](docs/es/install-and-play.md).** Desarrolladores: [compilar desde el código](docs/es/build.md).

Código actual: **{{VERSION}}**, candidato Windows x64. Publicar binarios es un paso manual posterior a pruebas de jugadores y revisión de licencias. Aún no hay un binario publicado.

## Showcase

<p align="center"><a href="https://youtu.be/p9oYk3mTuwo"><img src="https://img.youtube.com/vi/p9oYk3mTuwo/hqdefault.jpg" alt="Reproducir showcase de Tyrargo en YouTube" width="640"></a><br><a href="https://youtu.be/p9oYk3mTuwo">▶ Ver el showcase</a></p>

GitHub no permite incrustar un reproductor YouTube en el README. Esta vista previa abre el vídeo con un clic.

![Comparación CRT de la misma escena, alternando desactivado y activado](docs/media/crt-comparison.gif)

*CRT desactivado/activado en Tyrargo, usando el par suministrado. La cuantización de colores del GIF pertenece a la documentación, no al render del juego.* [Comparaciones estáticas y galería completa](docs/es/gallery.md).

## Objetivos

- Conservar atmósfera, recursos y detalle visual mientras entrada y presentación se adaptan a sistemas modernos.
- Reducir trabajo caro de render, colisiones e IA sin eliminar efectos ni reducir población a escondidas.
- Hacer configurables y explícitos los recursos singleplayer, órdenes de escuadrón y opciones de interfaz.
- Comparar comportamiento, píxeles, tiempo de frame y guardar/cargar antes de aceptar optimizaciones.

## Mejoras destacadas

{{COUNTS}}

Son **cambios implementados consolidados**, contados una sola vez; no prometen ausencia de bugs. Experimentos repetidos se conservan en el [historial de 249 hitos](docs/es/history.md), sin volver a contarlos como funciones nuevas.

{{HIGHLIGHTS}}

[Catálogo completo por categoría](docs/es/features.md) · [Explicaciones técnicas, código, flujos y ejemplos](docs/es/technical.md) · [Evidencia de rendimiento](docs/es/performance.md).

## Diferencias y procedencia

| Área | Base NakedAVP 1.2.3 | Tyrargo |
|---|---|---|
| Entrada y precisión | Port SDL3/OpenGL comunitario del motor original | Ángulos fraccionales del mouse y precisión de proyección/viewmodel |
| Presentación | Presentación clásica | FOV/interfaz por especie, vistas previas, CRT, agua moderna y encuadres panorámicos |
| Singleplayer | Campaña y dificultades nativas | Persistencia, opciones de ammo y niveles Molecular Acid |
| Apoyo NPC | Comportamiento nativo por especie | Follow/patrol, suministros compartidos, inventarios, torretas y recuperación de navegación |
| Rendimiento | Envío y consultas de colisión legacy | Buffers ordenados, filtro espacial y trabajo de apoyo compartido acotado |

AvPx es una mejora comunitaria independiente usada como referencia histórica. Tyrargo **no deriva de AvPx**. La tabla compara la base NakedAVP inspeccionada con nuestros cambios; no clasifica completitud ni rendimiento frente a otros ports. [Créditos y referencias](CREDITS.md).

## Compilación y desarrollo

La plataforma de validación mantenida es **Windows x64, GCC UCRT64 de MSYS2, SDL3, OpenAL y OpenGL de escritorio**. Decodificadores Smacker/Bink y utilidades de captura están incluidos. Otras plataformas y la ruta GLES heredada necesitan validación independiente.

Consultar [compilación, diagnóstico y versiones](docs/es/build.md) y [flujo manual local/GitHub](docs/es/workflow.md). Los cambios permanecen locales hasta hacer push explícito. Ningún workflow publica builds ni releases automáticamente.

## Gracias

- **Rebellion**, creadores originales y titulares del código; autores de ports anteriores y comunidad AvP.
- **[atsb / NakedAVP](https://github.com/atsb/NakedAVP)**, base directa.
- **[XProger / OpenLara](https://github.com/XProger/OpenLara)** y **[FluffyQuack / OpenLara](https://github.com/FluffyQuack/OpenLara)**, referencias del agua y trabajo comunitario.
- **[HelelSingh / CRT-Guest-ReShade](https://github.com/HelelSingh/CRT-Guest-ReShade)** y **[Mattias Gustavsson / newpixie](https://github.com/mattiasgustavsson/newpixie)**, trabajo y referencias del shader CRT.
- Autores de decodificadores, SDL, OpenAL, stb y compiladores. [Créditos completos](CREDITS.md) y [licencias de terceros](THIRD_PARTY_NOTICES.md).

## TODO

{{ROADMAP}}

[Hoja de ruta categorizada y estados](docs/es/roadmap.md). Propuestas y conceptos no se cuentan como implementaciones.
