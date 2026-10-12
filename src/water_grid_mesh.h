/* Persistent world-space grid. No per-frame vertex transforms or ripple scans.
 * Terrain rays see static world modules only. Retry unresolved clearances with
 * a fixed budget, instead of thousands of simultaneous misses every 0.5 s. */
#if !defined(USE_OPENGL_ES)
typedef struct CPU_WATER_GRID {
 int x,y,z,sx,sz,count,indexCount,cursor;unsigned generation,used,revision;
 WATER_VERTEX *vertices;unsigned short *indices;
} CPU_WATER_GRID;
static CPU_WATER_GRID modernGrids[16];
static unsigned modernGridSerial,modernGridRevision;
static int DrawGPUWaterPatch(int x,int y,int z){
 if(!WaterRenderGridSupported())return 0;
 unsigned generation=WaterGeneration();CPU_WATER_GRID *g=0;
 for(int i=0;i<16;i++){
  CPU_WATER_GRID *q=&modernGrids[i];
  if(q->generation!=generation){free(q->vertices);free(q->indices);memset(q,0,sizeof(*q));}
  if(q->vertices&&q->x==x&&q->y==y&&q->z==z&&q->sx==MeshXScale&&q->sz==MeshZScale)g=q;
 }
 if(!g){
  g=&modernGrids[0];for(int i=1;i<16;i++)if(modernGrids[i].used<g->used)g=&modernGrids[i];
  free(g->vertices);free(g->indices);memset(g,0,sizeof(*g));
  int nx=WaterMeshSegments(MeshXScale)+1,nz=WaterMeshSegments(MeshZScale)+1;
  g->count=nx*nz;g->indexCount=6*(nx-1)*(nz-1);
  g->vertices=(WATER_VERTEX*)calloc(g->count,sizeof(WATER_VERTEX));g->indices=(unsigned short*)malloc(g->indexCount*sizeof(unsigned short));
  if(!g->vertices||!g->indices){free(g->vertices);free(g->indices);memset(g,0,sizeof(*g));return 0;}
  g->x=x;g->y=y;g->z=z;g->sx=MeshXScale;g->sz=MeshZScale;g->generation=generation;
  int n=0;
  for(int a=0;a<nx;a++)for(int b=0;b<nz;b++){
   int i=a*nz+b;WATER_VERTEX *v=&g->vertices[i];
   v->world[0]=x+a*(float)MeshXScale/(nx-1);v->world[1]=y;v->world[2]=z+b*(float)MeshZScale/(nz-1);
   v->clip[0]=WaterModernFloorOffset((int)lroundf(v->world[0]),y,(int)lroundf(v->world[2]),256);
   if(a<nx-1&&b<nz-1){g->indices[n++]=i;g->indices[n++]=i+1;g->indices[n++]=i+nz;g->indices[n++]=i+1;g->indices[n++]=i+nz+1;g->indices[n++]=i+nz;}
  }
  g->revision=++modernGridRevision;
 }
 g->used=++modernGridSerial;
 for(int j=0;j<16;j++){
  WATER_VERTEX *v=&g->vertices[g->cursor++];if(g->cursor==g->count)g->cursor=0;
  if(v->clip[0]>=256){
   float limit=WaterModernFloorOffset((int)lroundf(v->world[0]),y,(int)lroundf(v->world[2]),256);
   if(limit!=v->clip[0]){v->clip[0]=limit;g->revision=++modernGridRevision;}
  }
 }
 static float sines[4096];static int haveSines;
 if(!haveSines){for(int i=0;i<4096;i++)sines[i]=(float)GetSin(i);haveSines=1;}
 WATER_GRID_STATE state={0};state.sines=sines;state.revision=g->revision;state.phase=CloakingPhase&8191; /* Both legacy wave periods, without long-session float loss. */
 state.count=WaterRippleSnapshot(state.ripples,state.inverseRadius,100);
 state.uv[0]=WaterXOrigin;state.uv[1]=WaterZOrigin;state.uv[2]=WaterUScale;state.uv[3]=WaterVScale;
 state.color[0]=state.color[1]=state.color[2]=1;
 if(CurrentVisionMode==VISION_MODE_IMAGEINTENSIFIER){state.color[0]=state.color[2]=0;state.color[1]=51/255.0f;}
 else if(CurrentVisionMode==VISION_MODE_PRED_THERMAL||CurrentVisionMode==VISION_MODE_PRED_SEEALIENS||CurrentVisionMode==VISION_MODE_PRED_SEEPREDTECH){state.color[0]=state.color[1]=0;state.color[2]=28/255.0f;}
 FlushTriangleBuffers(1);WaterPatchLights(x+MeshXScale*.5f,y,z+MeshZScale*.5f);
 return WaterRenderDrawGrid(g->vertices,g->count,g->indices,g->indexCount,CurrTextureHandle?CurrTextureHandle->id:0,y,ViewMatrix,
  (float)AvpProjectionX(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreX,
  (float)AvpProjectionY(Global_VDB_Ptr)/ScreenDescriptorBlock.SDB_CentreY,Global_VDB_Ptr->VDB_ClipZ*GlobalScale,&state);
}
#endif
