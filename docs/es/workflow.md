[English](../en/workflow.md) · [Español](../../README.es.md)

# Trabajar localmente y publicar manualmente

**Sol1vaN/AVP-Tyrargo** es un repositorio público e independiente en GitHub. Conserva la historia Git de NakedAVP 1.2.3 y el remote `upstream` de referencia. Se preparó de forma privada antes de la primera publicación; no pertenece a la red de forks de GitHub.

Proyecto original, probes e instalación del juego siguen separados de la copia de publicación. Git/repo remota no borran esas carpetas. Git conserva archivos confirmados; no respalda automáticamente saves, assets o probes ignorados. Mantener sus respaldos habituales.

| Acción | Efecto |
|---|---|
| Editar / compilar / probar | Solo local |
| `git add` | Seleccionar archivos para commit |
| `git commit` | Guardar checkpoint local |
| `git push` | Enviar commits explícitamente a GitHub |
| Release borrador | Preparar entrada sin publicarla |
| Publicar release / cambiar privacidad | Decisión separada después de revisar |

## Iteración normal

1. Seguir desarrollo y pruebas locales. Revisar cambios antes del commit.
2. Si trabajaste en otra probe, copiar solo source elegido a esta repo conservando documentos y probes originales. Excluir datos/saves/builds.
3. Actualizar versión y catálogo/TODO, regenerar y verificar:

```sh
python tools/update_docs.py
python tools/update_docs.py --check
python tools/check_docs.py
git status --short
git diff --stat
```

4. Seleccionar cambios y crear checkpoint local:

```sh
git add src CMakeLists.txt docs README.md README.es.md README_TYRARGO.md
git diff --cached --stat
git commit -m "Describir el cambio probado"
```

5. Cuando decidas enviarlo:

```sh
git push origin main
```

No hay pushes, merges ni releases automáticos. Verificación documental local bajo demanda. No se instala workflow GitHub Actions de builds, push o releases.

## Revisar una regresión

Ver checkpoints con `git log --oneline`. Para revisar una versión previa sin sobrescribir trabajo actual:

```sh
git worktree add --detach ../Tyrargo-regression <commit>
```

Usar hash existente y revisado. Para deshacer algo ya compartido, preferir commit revert antes que reescribir historia remota. No usar reset/clean destructivos sin saber qué descartan.

Las 249 probes no se inventan como 249 commits: quedan locales y tienen un inventario de hitos rastreado. La historia upstream sí se conserva como antecesora.

## Revisión y publicación

Revisar presentación/procedencia, probar gameplay y completar avisos/materiales de distribución. Después decidir explícitamente hacer pública la repo y publicar un release. Mientras tanto solo colaboradores autorizados ven el contenido. Nada modifica la repo de atsb.

## Avisar a atsb

Issue/discussion para un aviso breve si su política lo admite; PR para una contribución de código concreta; Projects organiza planificación. Hay un [borrador local](../../MAINTAINER_MESSAGE_DRAFT.md), todavía sin enviar. El borrador ya puede incluir los enlaces públicos a la repo y la release. NakedAVP tiene Issues habilitados y Discussions deshabilitados; revisar su política antes de publicar un aviso.
