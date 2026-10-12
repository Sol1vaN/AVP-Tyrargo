#ifndef TYRARGO_COPLANAR_DEPTH_H
#define TYRARGO_COPLANAR_DEPTH_H
/* Rasterizer depth bias covers the depth slope of grazing/near clipped faces.
 * Only load-time classified coplanar trim batches use it, in both colour passes.
 * Positions, projection, UVs, MSAA coverage and collision geometry stay unchanged.
 * https://wikis.khronos.org/opengl/Parameters_of_Polygon_Offset */
#define COPLANAR_TRIM_OFFSET_FACTOR (-1.0f)
#define COPLANAR_TRIM_OFFSET_UNITS (-8.0f)
#endif
