/* Immutable, unanimated module meshes only. A BVH rejects polygons outside
   the exact swept particle/actor box; accepted polygons are replayed in native order.
   Moving objects, animated/morphed meshes and allocation failures use legacy.
   All entries are released before their owning level pool is destroyed. */
#include <stdlib.h>
#include <string.h>
#define PARTICLE_MESH_LIMIT 64
#define PARTICLE_CACHE_BYTES (32u*1024u*1024u)
typedef struct {struct ColPolyTag poly;int flag,min[3],max[3];} PARTICLE_MESH_POLY;
typedef struct {int min[3],max[3],left,right,start,count;} PARTICLE_MESH_NODE;
typedef struct {void*shape;PARTICLE_MESH_POLY*poly;PARTICLE_MESH_NODE*nodes;int*order,*hits;int count,nodeCount;} PARTICLE_MESH_CACHE;
static PARTICLE_MESH_CACHE particleMesh[PARTICLE_MESH_LIMIT];
static int particleMeshCount;static size_t particleMeshBytes;
static PARTICLE_MESH_CACHE*particleSortMesh;static int particleSortAxis;
void ParticleCollisionCacheReset(void){
 for(int i=0;i<particleMeshCount;i++){free(particleMesh[i].poly);free(particleMesh[i].nodes);free(particleMesh[i].order);free(particleMesh[i].hits);}
 memset(particleMesh,0,sizeof particleMesh);particleMeshCount=0;particleMeshBytes=0;particleSortMesh=NULL;
}
static int ParticleMeshCompare(const void*aa,const void*bb){
 int a=*(const int*)aa,b=*(const int*)bb;PARTICLE_MESH_POLY*x=&particleSortMesh->poly[a],*y=&particleSortMesh->poly[b];
 int64_t cx=(int64_t)x->min[particleSortAxis]+x->max[particleSortAxis],cy=(int64_t)y->min[particleSortAxis]+y->max[particleSortAxis];
 return cx==cy?(a>b)-(a<b):cx>cy?1:-1;
}
static int ParticleMeshIndexCompare(const void*aa,const void*bb){int a=*(const int*)aa,b=*(const int*)bb;return (a>b)-(a<b);}
static int ParticleMeshBuildNode(PARTICLE_MESH_CACHE*c,int start,int count){
 int index=c->nodeCount++;PARTICLE_MESH_NODE*n=&c->nodes[index];n->left=n->right=-1;n->start=start;n->count=count;
 for(int axis=0;axis<3;axis++){n->min[axis]=INT_MAX;n->max[axis]=INT_MIN;}
 for(int i=start;i<start+count;i++){PARTICLE_MESH_POLY*p=&c->poly[c->order[i]];for(int axis=0;axis<3;axis++){if(p->min[axis]<n->min[axis])n->min[axis]=p->min[axis];if(p->max[axis]>n->max[axis])n->max[axis]=p->max[axis];}}
 if(count<=8)return index;
 int axis=0;for(int j=1;j<3;j++)if((int64_t)n->max[j]-n->min[j]>(int64_t)n->max[axis]-n->min[axis])axis=j;
 particleSortMesh=c;particleSortAxis=axis;qsort(c->order+start,count,sizeof(int),ParticleMeshCompare);
 int half=count/2;n->left=ParticleMeshBuildNode(c,start,half);n->right=ParticleMeshBuildNode(c,start+half,count-half);n->count=0;return index;
}
static PARTICLE_MESH_CACHE*ParticleMeshGet(DISPLAYBLOCK*object){
 if(!object->ObMyModule || !object->ObShapeData || object->ObMorphCtrl || object->ShapeAnimControlBlock || object->HModelControlBlock || (object->ObStrategyBlock && object->ObStrategyBlock->DynPtr))return NULL;
 for(int i=0;i<particleMeshCount;i++)if(particleMesh[i].shape==object->ObShapeData)return particleMesh[i].count>=0?&particleMesh[i]:NULL;
 if(particleMeshCount==PARTICLE_MESH_LIMIT)return NULL;
 int count=SetupPolygonAccess(object);if(count<=0 || count>131072)return NULL;
 size_t bytes=(size_t)count*(sizeof(PARTICLE_MESH_POLY)+2*sizeof(PARTICLE_MESH_NODE)+2*sizeof(int));
 PARTICLE_MESH_CACHE*c=&particleMesh[particleMeshCount++];c->shape=object->ObShapeData;c->count=-1;
 if(bytes>PARTICLE_CACHE_BYTES-particleMeshBytes)return NULL;
 c->poly=calloc(count,sizeof(*c->poly));c->nodes=calloc((size_t)count*2,sizeof(*c->nodes));c->order=malloc((size_t)count*sizeof(int));c->hits=malloc((size_t)count*sizeof(int));
 if(!c->poly || !c->nodes || !c->order || !c->hits){free(c->poly);free(c->nodes);free(c->order);free(c->hits);c->poly=NULL;c->nodes=NULL;c->order=c->hits=NULL;return NULL;}
 particleMeshBytes+=bytes;c->count=0;
 while(count--){AccessNextPolygon();
  PARTICLE_MESH_POLY*q=&c->poly[c->count];q->flag=PolygonFlag;GetPolygonVertices(&q->poly);GetPolygonNormal(&q->poly);
  for(int axis=0;axis<3;axis++){q->min[axis]=INT_MAX;q->max[axis]=INT_MIN;}
  for(int v=0;v<q->poly.NumberOfVertices;v++){int a[3]={q->poly.PolyPoint[v].vx,q->poly.PolyPoint[v].vy,q->poly.PolyPoint[v].vz};for(int axis=0;axis<3;axis++){if(a[axis]<q->min[axis])q->min[axis]=a[axis];if(a[axis]>q->max[axis])q->max[axis]=a[axis];}}
  c->order[c->count]=c->count;c->count++;
 }
 if(c->count)ParticleMeshBuildNode(c,0,c->count);return c;
}
static int ParticleMeshOverlaps(const int min[3],const int max[3],const int lo[3],const int hi[3]){
 for(int a=0;a<3;a++)if(max[a]<lo[a] || min[a]>hi[a])return 0;return 1;
}
static void ParticleMeshQuery(PARTICLE_MESH_CACHE*c,int node,const int lo[3],const int hi[3],int*count){
 PARTICLE_MESH_NODE*n=&c->nodes[node];if(!ParticleMeshOverlaps(n->min,n->max,lo,hi))return;
 if(n->count){for(int i=n->start;i<n->start+n->count;i++){int index=c->order[i];PARTICLE_MESH_POLY*p=&c->poly[index];if(ParticleMeshOverlaps(p->min,p->max,lo,hi))c->hits[(*count)++]=index;}return;}
 ParticleMeshQuery(c,n->left,lo,hi,count);ParticleMeshQuery(c,n->right,lo,hi,count);
}
static int ParticleMeshAppend(DISPLAYBLOCK*object,int visibilityPolicy){
 PARTICLE_MESH_CACHE*c=ParticleMeshGet(object);if(!c)return 0;
 int lo[3]={DBBMinX,DBBMinY,DBBMinZ},hi[3]={DBBMaxX,DBBMaxY,DBBMaxZ},count=0;
 if(c->count)ParticleMeshQuery(c,0,lo,hi,&count);qsort(c->hits,count,sizeof(int),ParticleMeshIndexCompare);
 for(int k=0;k<count;k++){
  PARTICLE_MESH_POLY*q=&c->poly[c->hits[k]];
  if((q->flag & iflag_notvis) && (visibilityPolicy==1 || (visibilityPolicy==2 && !(q->flag & iflag_mirror))))continue;
  struct ColPolyTag*out=CollisionPolysPtr;
  *out=q->poly;out->ParentObject=object;
  for(int v=0;v<out->NumberOfVertices;v++){out->PolyPoint[v].vx+=object->ObWorld.vx;out->PolyPoint[v].vy+=object->ObWorld.vy;out->PolyPoint[v].vz+=object->ObWorld.vz;}
  CollisionPolysPtr++;NumberOfCollisionPolys++;LOCALASSERT(NumberOfCollisionPolys<MAXIMUM_NUMBER_OF_COLLISIONPOLYS);
  if(q->flag & iflag_no_bfc){struct ColPolyTag*reverse=CollisionPolysPtr;*reverse=*out;for(int v=0;v<out->NumberOfVertices;v++)reverse->PolyPoint[v]=out->PolyPoint[out->NumberOfVertices-1-v];reverse->PolyNormal.vx=-out->PolyNormal.vx;reverse->PolyNormal.vy=-out->PolyNormal.vy;reverse->PolyNormal.vz=-out->PolyNormal.vz;CollisionPolysPtr++;NumberOfCollisionPolys++;LOCALASSERT(NumberOfCollisionPolys<MAXIMUM_NUMBER_OF_COLLISIONPOLYS);}
 }
 return 1;
}
