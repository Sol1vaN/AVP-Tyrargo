[English](../../README.md) · [Español](../../README.es.md)

# Evidencia de rendimiento

Probe 249 midió dos saves New Derelict a 2560×1440, con el mismo ejecutable diagnóstico, cámara, semilla y paso fijo 455/65536 segundos. Se alternaron optimizaciones en esa build; control no es otro binario anterior. Jugador protegido para observar; NPC reciben daño normal. Presentación oculta sin límite. Son mediciones locales, no garantía de FPS en todo equipo.

| Scenario | Original paths mean / p99 | Optimized mean / p99 | Mean reduction |
|---|---|---|---:|
| A | 15.828 / 27.074 ms | 10.371 / 19.372 ms | 34.48% |
| B | 18.692 / 28.942 ms | 9.460 / 17.084 ms | 49.39% |

Cambios medidos: buffers de vértices/índices ordenados y filtro espacial de colisiones. Sin reducir gráficos, efectos o población. Mallas dinámicas/morph/animadas conservan fallback. Reduce trabajo CPU/driver; no es backend Vulkan ni reescritura multihilo.

Se compararon 173.786 consultas de actores y 618.247 de partículas sin diferencias de polígonos/normales/orden. Catorce combinaciones de mezcla/filtro/especular produjeron píxeles idénticos. Guardar/cargar y dos reinicios nativos pasaron pruebas. Adquisición Predator móvil pasó de 81 muestras congeladas a 0, con clips caminar/correr. Valida esos ensayos, no todos los mapas/estados.

Áreas previas en catálogo: fragmentos CRT, malla GPU/reflejos agua, agrupación fuego, planificación compartida acotada, registros audio/eventos en buffer y vida contexto. Cada una requiere medición propia; sus ganancias no se suman en un porcentaje universal.

Imported production candidate compiled: **2026-10-11 20:21:56.200870 −03:00**. SHA256: `059f77bbbe9e5b36dd0e6a4d06c591463806b6e94ee0054be0dffec2ebd545fb`. Diagnostic executable hashes differ from this production candidate.

Private saves, camera traces, test executables and original assets are retained locally; they are not distributed here. [Technical guide](technical.md) · [Catalog](features.md).
