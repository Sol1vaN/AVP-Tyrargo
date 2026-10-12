[English](../en/install-and-play.md) · [Español](../../README.es.md)

# Cómo instalar y jugar

## Versiones compatibles y requisitos

- **Aliens versus Predator Gold Edition — CD retail**, con sus datos instalados.
- **Aliens versus Predator Classic 2000 — Steam o GOG**.

Debes poseer e instalar el juego. El paquete Windows x64 contiene el port y sus librerías; no contiene mapas, texturas, audio, vídeos ni mapas custom comerciales. [Descargas](https://github.com/Sol1vaN/AVP-Tyrargo/releases) · [Compilar desde el código](build.md).

## Instalar y jugar

1. Cierra el juego y respalda perfiles/partidas. Extrae el ZIP Windows x64 **en el directorio del juego instalado**, conservando carpetas y avisos de licencia. También puedes usar una copia independiente de la instalación.
2. El port se inicia con **`avpt.exe`**, cuyo nombre es distinto de los ejecutables originales. Conserva esos ejecutables y coloca las DLL del paquete junto a `avpt.exe`. Al actualizar una versión anterior de Tyrargo, sustituye juntos los archivos coincidentes de ese port.
3. Comprueba `avp_rifs/temple.rif` y `fastfile/ffinfo.txt`. Conserva los directorios originales de datos. Algunas ediciones empaquetan gráficos/audio en `fastfile`; en ese caso no requieren una carpeta `graphics` separada.
4. Inicia `avpt.exe` desde el directorio del juego y configura perfil, Audio/Vídeo y controles por especie. El launcher original sigue disponible para iniciar la versión original.

En Steam, el destino habitual es `Steam/steamapps/common/Aliens versus Predator Classic`. En Gold CD y GOG, utiliza el directorio elegido al instalar. El port no reemplaza automáticamente el ejecutable que abre Steam/GOG.

También puedes mantener el port separado y seleccionar los datos existentes:

```powershell
.\avpt.exe -p 'D:\Games\Aliens versus Predator Classic'
```

`-p` selecciona los datos; la configuración y los perfiles se resuelven aparte según el entorno del motor. Conserva `.avp` y los perfiles existentes al actualizar. `AVP_DATA` también permite seleccionar datos. El ejecutable y las DLL por sí solos no constituyen el juego completo.

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
