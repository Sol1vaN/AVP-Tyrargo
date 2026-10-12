[English](../en/install-and-play.md) · [Español](../../README.es.md)

# Cómo instalar y jugar

## Requisitos

Una instalación legal de Aliens versus Predator Gold/Classic y un paquete Tyrargo Windows x64 aprobado por el mantenedor. El repositorio no incluye mapas, texturas, vídeos, audio ni mapas custom originales. Actualmente contiene el código de un candidato; aún no hay descarga binaria pública. [Cómo compilar](build.md).

## Instalar un paquete revisado

1. Cerrar el juego. Respaldar ejecutable, DLLs, `.avp`, perfiles y partidas. Si utiliza una carpeta de configuración de usuario, respaldarla también.
2. Extraer el paquete en una copia de la instalación existente. Mantener DLLs junto a `avpt.exe` y conservar los avisos de licencia.
3. Comprobar `avp_rifs/temple.rif` y `fastfile/ffinfo.txt`. Conservar `avp_huds`, `graphics`, `Text`, `FMVs` y demás recursos suministrados.
4. Ejecutar `avpt.exe` con la carpeta del juego como directorio de trabajo. Si los recursos están aparte, indicar con `AVP_DATA` la carpeta que contiene `avp_rifs` y `fastfile`.
5. Seleccionar/crear perfil; ajustar modo, resolución y sincronización en Audio/Video; configurar controles, FOV y escalas por especie.

Ejecutable y DLLs por sí solos no forman una instalación completa.

```powershell
$env:AVP_DATA = 'D:\Juegos\Aliens versus Predator Classic'
.\avpt.exe
```

La configuración puede resolverse en `.avp` local o en una carpeta `.avp` de usuario según el entorno. No borrar perfiles para cambiar de ejecutable.

## Opciones principales

| Opción | Uso |
|---|---|
| FOV mundo/arma y perfil por especie | Encuadres independientes |
| Escalas HUD/crosshair/texto | Legibilidad independiente |
| CRT, imagen y agua | Elegir presentación; agua moderna opcional |
| Formato y bind de captura | BMP, PNG o JPEG en juego y menús |
| Recarga manual y binds por especie | Personalizar controles |
| Jetpack/zoom inteligentes, correr/agachar | Elegir mantener o alternar |
| Quicksave/quickload | Slot dedicado además de slots normales |
| Persistencia/munición optimizada | Opciones de campaña y recursos singleplayer |

Consultar los binds dentro del juego: perfiles guardados pueden diferir de valores por defecto. Escape aplica ajustes ordinarios; resoluciones conservan confirmación explícita.

## Molecular Acid

Modifica Hard/Director's Cut en singleplayer. Las etiquetas difieren deliberadamente de factores internos:

| Etiqueta | Enemigos | Recursos | Cargadores Pulse Rifle / cohetes SADAR / discos Marine por pickup |
|---|---:|---:|---:|
| x1 | 1.5 | 1.5 | 3 |
| x1.5 | 2 | 2 | 4 |
| x2 | 2.5 | 3 | 5 |

Aumentan reservas y cargadores/depósitos soportados; excepciones son cantidades totales por pickup. Energía Predator usa factor de recursos y armadura Acid parte de 15 puntos × factor. Población respeta generadores/actores soportados y triggers del mapa; no garantiza multiplicar exactamente cada oleada guionizada.

## Marines aliados

Usar sobre un Marine alterna follow/patrol con confirmación. Follow se retoma tras combate; patrol usa exploración coordinada expansiva. Recogen dos tipos adicionales distintos, recargan/cambian armas soportadas, ayudan con torretas y dejan armas recuperables al morir. Recogen salud/armadura bajo 60% y ammo bajo 30%. En zonas estrechas de suministro el humano tiene prioridad.

## Música de menú opcional

Si posees el recurso del juego de 1999, suministrarlo como `IntroSound99.smk` en la ubicación de vídeos de tu instalación, conservando aparte el recurso Gold. La opción aparece al detectar el archivo adicional. No se distribuye dicho vídeo aquí.

## Mapas custom y Steam

Seguir instrucciones del autor y respaldar el nivel sustituido. Un reemplazo Derelict utiliza su slot original; aún no hay cargador amigable de mapas/mods integrado.

Steam se configura manualmente; integración automática sigue en la hoja de ruta. En propiedades del juego, una opción de lanzamiento es:

```text
"D:\Juegos\Aliens versus Predator Classic\avpt.exe" %command%
```

Adaptar a tu ruta. Tyrargo no modifica automáticamente la configuración Steam al iniciar.

## Problemas y reportes

Error de recursos: comprobar directorio de trabajo y los dos archivos de validación. DLL faltante: usar bibliotecas del mismo paquete/toolchain. Problemas de vídeo: comenzar en ventana compatible y reportar resolución pedida/real. Para crash, rendimiento o NPC atascado: indicar versión, mapa, ajustes, arma/evento y save reproducible si existe. Revisar datos personales antes de subir saves/logs públicamente.

[Catálogo](features.md) · [Hoja de ruta](roadmap.md) · [Compilación](build.md).
