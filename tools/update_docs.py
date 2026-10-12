#!/usr/bin/env python3
"""Generate bilingual GitHub docs locally. --check validates, never writes/pushes."""
from pathlib import Path
from collections import Counter
import argparse,json,re,sys
ROOT=Path(__file__).resolve().parents[1]
def load(name):return json.loads((ROOT/'docs/data'/name).read_text('utf-8'))
def version():
 s=(ROOT/'src/tyrargo_version.h').read_text('latin1')
 parts={k:int(re.search(r'#define TYRARGO_VERSION_'+k+r' (\d+)',s)[1]) for k in ('MAJOR','MINOR','PATCH','RC')}
 return f"v{parts['MAJOR']}.{parts['MINOR']} RC{parts['RC']}"
def excerpt(path,symbol):
 lines=(ROOT/path).read_text('latin1').splitlines()
 matches=[i for i,l in enumerate(lines) if symbol in l]
 candidates=[i for i in matches if ';' not in lines[i] and (lines[i].rstrip().endswith(')') or '{' in lines[i])]
 index=next(iter(candidates or matches),None)
 if index is None:raise ValueError(f'Missing source symbol: {path}:{symbol}')
 return index+1,'\n'.join(lines[max(0,index-1):index+17])+'\n/* ... excerpt continues in the linked source ... */'
DETAILS={
'input':('src/avp/win95/usr_io.c','MouseCountsToAngleFixed','SDL float counts → sensitivity/zoom → 16.16 angles → pmove residuals → fractional render pose','Deltas float SDL → sensibilidad/zoom → ángulos 16.16 → restos pmove → pose fraccional de render','Compare equal accumulated counts at several frame caps and in opposite directions; inspect tiny movements under zoom. Preserve legacy strafe/movement handling separately.','Comparar cuentas acumuladas iguales con distintos límites y sentidos; revisar movimientos pequeños con zoom. Conservar aparte strafe/movimiento legacy.'),
'display':('src/main.c','SDL_CreateWindow','requested mode → guarded SDL/GL transaction → drawable/framebuffer synchronization → texture/state restore or fallback','modo solicitado → transacción protegida SDL/GL → sincronizar drawable/framebuffer → restaurar texturas/estado o fallback','Switch between all three modes, change resolution/AA, drag the window and return through menus. Check the actual drawable, not only the requested mode.','Alternar los tres modos, resolución/AA, mover ventana y volver por menús. Comprobar drawable real y no solo modo solicitado.'),
'geometry':('src/playeryaw.c','PlayerYawRadians','native gameplay pose + fractional render pose → camera/model transforms → clipping → OpenGL floats','pose nativa + pose fraccional render → transformar cámara/modelo → clipping → floats OpenGL','Observe pistol barrel/slide during idle and zoom; compare FOV/aspect transitions. Render-only precision changes must not silently change collision shapes.','Observar cañón/corredera en idle y zoom; comparar FOV/aspecto. Precisión visual no debe alterar silenciosamente formas de colisión.'),
'render':('src/opengl.c','glBufferData','ordered vertices/indices → compatible streamed storage → original draw sequence → luminous/effect passes','vértices/índices ordenados → almacenamiento renovado → secuencia original → pasadas luminosas/efectos','Compare alpha cutouts, all blend/filter modes, glass, halos and vision modes. Reordering translucent geometry is not an acceptable optimization.','Comparar recortes alpha, mezclas/filtros, vidrio, halos y modos de visión. Reordenar transparencia no es una optimización aceptable.'),
'water':('src/water_effect.c','WaterAdvance','elapsed simulation time → wave integration → fractional height/gradient → water grid → depth/color optics','tiempo simulado → integrar ondas → altura/gradiente fraccional → malla → óptica color/profundidad','Compare an isolated ripple over equal elapsed time at multiple frame caps; test foreground silhouettes and screen-edge reflection fallback.','Comparar onda aislada en tiempo igual con distintos límites; probar siluetas foreground y límites de reflejo en bordes.'),
'postfx':('src/crt_filter.c','CRTFilterVertexSource','scene framebuffer → GLSL image/CRT pass → selected presentation canvas','framebuffer escena → pasada GLSL imagen/CRT → lienzo seleccionado','Use the supplied off/on pair for appearance. Measure the same scene at the same resolution for cost; a GIF is not a precision benchmark.','Usar par desactivado/activado para aspecto. Medir misma escena/resolución para coste; GIF no es prueba de precisión.'),
'ui':('src/avp/win95/frontend/avp_menus.c','static void RenderMenu(void)','profile settings → logical layout/font atlas → live preview → guarded apply/persist','perfil → layout lógico/atlas → vista previa → aplicar/guardar con protección','Check accented text, control frame edges, death statistics and live previews in borderless and letterboxed modes.','Comprobar acentos, marcos, estadísticas de muerte y vistas previas en sin bordes y letterboxed.'),
'media':('src/fmv_backend.cpp','smk','movie stream → native decoder → buffered audio clock → physical viewport → end/transition cleanup','stream vídeo → decodificador nativo → reloj audio en buffer → viewport físico → limpieza final/transición','Test movie end, skip, audio-only menu music and wide modes; capture repeatedly without including notification text.','Probar final, omitir, música de menú solo audio y modos anchos; capturar repetidamente sin incluir avisos.'),
'audio':('src/openal.c','alSourcefv','event + emitter position → OpenAL source/environment → playback; diagnostics → bounded buffers → deferred writes','evento + posición → fuente/entorno OpenAL → reproducir; diagnóstico → buffer acotado → escritura diferida','Move away from an NPC pickup, listen through geometry, overlap primary/secondary weapon sounds and compare feeding emitters.','Alejarse de pickup NPC, escuchar tras geometría, solapar armas primarias/secundarias y comparar alimentación.'),
'player':('src/avp/pmove.c','ConsumeRawMouseAngle','input requests → native movement/contact → supported platform or climbing transitions → camera feedback','solicitudes entrada → movimiento/contacto nativo → apoyo móvil/trepado → feedback cámara','Test slopes without removing friction, descending lifts, wall turns, repeated fire taps and the flare cooldown.','Probar pendientes sin quitar fricción, lifts descendentes, giros, taps de disparo y cooldown de bengalas.'),
'motion':('src/camera_motion.c','spring','movement/equipment events → independent bounded motion channels → camera/viewmodel/HUD outputs','eventos movimiento/equipo → canales independientes acotados → salidas cámara/arma/HUD','Compare walking, running, crouching, reload and landing with each public intensity channel disabled/enabled. This is Marine motion; Predator/Alien expansion remains planned.','Comparar caminar, correr, agacharse, recarga y aterrizaje con canales desactivados/activados. Movimiento Marine; expansión Predator/Alien sigue planificada.'),
'campaign':('src/molecular_acid.c','resourceTenths','selected Acid tier → native Hard behavior + population/resources → bounded capacities/pickup remainders → save state','nivel Acid → Hard nativo + población/recursos → capacidades/restos acotados → guardado','Compare identical level starts across tiers and reload saved games. Pulse Rifle/SADAR/disc 3/4/5 are total quantities per pickup; Predator armor is 15 × resource factor.','Comparar inicios idénticos por nivel y recargar partidas. 3/4/5 de Pulse Rifle/SADAR/disco son cantidades totales; armadura Predator es 15 × factor recursos.'),
'aliens':('src/avp/npc_local_nav.c','NPC','native goal/behavior → progress/support checks → local route/contact recovery → bounded leap or locomotion','objetivo nativo → comprobar avance/apoyo → recuperar ruta/contacto → salto acotado o locomoción','Use a repeatable vent/hatch save and record contacts, requested velocity and actual displacement. Separate authored high leaps from invalid repeated impulses.','Usar save de ducto/escotilla y registrar contactos, velocidad solicitada y avance real. Separar saltos altos deliberados de impulsos repetidos inválidos.'),
'predators':('src/avp/bh_pred.c','NPCSupport','target eligibility → acquisition/weapon state → movement + locomotion clip → native shot completion','elegibilidad → adquisición/arma → movimiento + clip locomoción → finalización nativa de disparo','Record moving shoulder-cannon acquisition and actual animation names; also test melee, taunts and weapon changes.','Registrar adquisición móvil del cañón y nombres de clips; probar melee, taunts y cambios de arma.'),
'marines':('src/avp/marine_squad_impl.inc','MarineSupportControl','command/patrol task → shared job/planner → local navigation + pacing → combat/resupply/sentry → persisted state','orden/patrulla → tarea/planner compartido → navegación + ritmo → combate/suministro/torreta → estado persistido','Check Use acknowledgments, two distinct extra weapons, resource thresholds, APC courtesy, slopes and return from combat to follow/patrol. NPC inventory is private, not shared with the human player.','Comprobar confirmaciones usar, dos armas distintas, umbrales, cortesía APC, pendientes y retorno tras combate. Inventario NPC es privado, no compartido con jugador.'),
'kernel':('src/avp/particle_mesh_cache.inc','ParticleMeshAppend','immutable-mesh bounds → BVH candidates → sort in native polygon order → original collision math; dynamic mesh → original fallback','bounds inmutables → candidatos BVH → ordenar según polígonos nativos → matemática original; dinámicas → fallback','Compare exact polygon lists/normals/order and lifetime across load/restart. Cache is bounded to 32 MiB/64 meshes; it is cleared before freeing map pools.','Comparar polígonos/normales/orden y vida al cargar/reiniciar. Caché acotada a 32 MiB/64 mallas; se limpia antes de liberar pools.')}
def outputs():
 data=load('features.json');items=data['items'];cats=data['categories'];hist=load('history.json');todo=load('roadmap.json');media=load('media.json');out={}
 ids=[i['id'] for i in items]
 assert len(ids)==len(set(ids));assert all(i['type'] in ('fix','QoL','feature') and (ROOT/i['source']).is_file() for i in items)
 counts=Counter(i['type'] for i in items)
 for lang in ('en','es'):
  en=lang=='en';folder=f'docs/{lang}/';nav='[English](../../README.md) · [Español](../../README.es.md)\n\n'
  table=('| Fixes | Quality of life | Features | Total |' if en else '| Correcciones | Calidad de vida | Funciones | Total |')+f"\n|---:|---:|---:|---:|\n| {counts['fix']} | {counts['QoL']} | {counts['feature']} | {len(items)} |"
  highlights=[];features=[nav+'# '+('Detailed feature catalog' if en else 'Catálogo detallado de mejoras'),('Consolidated implemented behavior; source-review evidence is linked below. Individual entries are not separate test certifications.' if en else 'Comportamiento implementado consolidado; hay enlaces de evidencia del source. Cada entrada no equivale a una certificación de prueba individual.')]
  technical=[nav+'# '+('Technical guide: code, flows and examples' if en else 'Guía técnica: código, flujos y ejemplos'),('Excerpts are generated from the current source snapshot. Links open the complete implementation. Flows are schematic; preserve the native simulation interfaces.' if en else 'Extractos generados del source actual. Los enlaces abren implementación completa. Flujos son esquemáticos; conservar interfaces nativas de simulación.')]
  for key,c in cats.items():
   members=[i for i in items if i['category']==key]
   highlights.append(f"- **[{c[lang]}]({folder}features.md#{key})** — "+' · '.join(i['title'][lang] for i in members[:2])+f' ({len(members)}).')
   features+= [f'<a id="{key}"></a>\n\n## {c[lang]}',c['description'][lang],f"[{c['source']}](../../{c['source']}) · ["+('Code and examples' if en else 'Código y ejemplos')+f'](technical.md#{key})','| ID | '+('Type | Change' if en else 'Tipo | Cambio')+' |\n|---|---|---|']
   features += [f"| `{i['id']}` | {i['type']} | {i['title'][lang]} |" for i in members]
   path,sym,flowen,flowes,testen,testes=DETAILS[key];line,code=excerpt(path,sym)
   technical += [f'<a id="{key}"></a>\n\n## {c[lang]}',c['description'][lang],'```text\n'+(flowen if en else flowes)+'\n```',f'[{path}:{line}](../../{path}#L{line})\n\n```c\n{code}\n```',('**Example and verification:** ' if en else '**Ejemplo y comprobación:** ')+(testen if en else testes)]
  road=[nav+'# '+('Roadmap' if en else 'Hoja de ruta'),('Future work, not shipped functionality. Status is explicit; no release-date promises.' if en else 'Trabajo futuro, no funciones entregadas. Estado explícito, sin promesas de fecha.')]
  for cat in dict.fromkeys(x['category'] for x in todo):
   road += ['## '+cat,'| '+('Task | Status' if en else 'Tarea | Estado')+' |\n|---|---|']+[f"| {x['title'][lang]} | `{x['status']}` |" for x in todo if x['category']==cat]
  short='\n'.join('- '+x['title'][lang] for x in todo[:6])
  tmpl=(ROOT/f'docs/templates/README.{lang}.md').read_text('utf-8')
  for token,value in {'VERSION':version(),'COUNTS':table,'HIGHLIGHTS':'\n'.join(highlights),'ROADMAP':short}.items():tmpl=tmpl.replace('{{'+token+'}}',value)
  out['README.md' if en else 'README.es.md']=tmpl
  out[folder+'features.md']='\n\n'.join(features)+'\n'
  out[folder+'technical.md']='\n\n'.join(technical)+'\n'
  out[folder+'roadmap.md']='\n\n'.join(road)+'\n'
  intro=nav+'# '+('Development milestones' if en else 'Hitos del desarrollo')+'\n\n'+('Inventory of local probe names. A probe can be a diagnostic experiment or revision of an existing feature; this is not 249 releases, nor 249 independent features. Original probe folders and private saves remain outside this repository. Git ancestry retains upstream history; the first Tyrargo commit imports the current snapshot.' if en else 'Inventario de nombres de probes locales. Una probe puede ser diagnóstico o revisión de una función; no equivale a 249 releases ni 249 funciones independientes. Probes originales y saves privados quedan fuera del repositorio. Git conserva historia upstream; el primer commit Tyrargo importa el snapshot actual.')+'\n\n| Probe | Milestone |\n|---:|---|\n'
  out[folder+'history.md']=intro+'\n'.join(f"| {x['probe']:03} | {x['title']} |" for x in hist)+'\n'
  gallery=[nav+'# '+('Showcase gallery' if en else 'Galería showcase'),('All supplied screenshots, resized into documentation copies; original game assets/maps are not bundled. Images may depict earlier iterations. CRT off/on is a matched pair; water/map scenes are illustrations, not matched performance comparisons.' if en else 'Todas las capturas suministradas, en copias para documentación; no incluye recursos/mapas originales. Imágenes pueden mostrar iteraciones previas. CRT off/on es un par comparable; agua/mapas son ejemplos, no comparaciones de rendimiento.')]

  categories=list(dict.fromkeys(x['category'] for x in media if 'category' in x))
  gallery += [' · '.join(f"[{next(x['title'][lang] for x in media if x.get('category')==cat)}](#{cat})" for cat in categories)]
  for cat in categories:
   members=[x for x in media if x.get('category')==cat]
   gallery += [f'<a id="{cat}"></a>\n\n## {members[0]["title"][lang]}']
   last_group=None
   for x in members:
    sub='/'.join(x['original'].split('/')[2:-1])
    if sub and sub!=last_group:gallery += ['### '+sub.replace('new delerectic','New Derelict').replace('marine','Marine').replace('predator','Predator').replace('alien','Alien')];last_group=sub
    caption=x['original'].split('/')[-1]
    if caption.lower() in ('before.png','after.png'):caption=('Before — CRT disabled' if caption.lower()=='before.png' else 'After — CRT enabled') if en else ('Antes — CRT desactivado' if caption.lower()=='before.png' else 'Después — CRT activado')
    gallery += [f"<details><summary>{caption}</summary>\n\n![{caption}](../media/{x['file']})\n\n</details>"]
  out[folder+'gallery.md']='\n\n'.join(gallery)+'\n'
 out['README_TYRARGO.md']=f'# AVP: Tyrargo {version()}\n\nThis legacy entry point follows the current version header. See [English README](README.md), [README español](README.es.md), [build/versioning guide](docs/en/build.md) and [milestones](docs/en/history.md).\n'
 return out
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args();changed=[]
 for name,text in outputs().items():
  f=ROOT/name;text=text.rstrip()+'\n'
  if not f.exists() or f.read_text('utf-8')!=text:
   changed.append(name)
   if not args.check:f.parent.mkdir(parents=True,exist_ok=True);f.write_text(text,encoding='utf-8',newline='\n')
 if args.check and changed:print('Stale generated docs: '+', '.join(changed));sys.exit(1)
 print('Documentation current' if args.check else f'Generated {len(changed)} documents; no Git/remote operations.')
