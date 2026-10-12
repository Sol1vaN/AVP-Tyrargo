/* If this is set to a non-zero value then kzsort.c hijacks the pipeline */
#define KZSORT_ON 1

/* A visible module consumes a display block. Match the engine's real upper
 * bound instead of the old estimate of 400, including extended resident PVS. */
#define MAX_NUMBER_OF_VISIBLE_MODULES maxobjects

struct KItem
{
	POLYHEADER *PolyPtr;

	int SortKey;
};

struct KObject
{
	DISPLAYBLOCK *DispPtr;

	int SortKey;

	int DrawBeforeEnvironment;
};

/* render with new z-sort */
extern void KRenderItems(VIEWDESCRIPTORBLOCK *VDBPtr);

/* generic item shape function */
extern void KShapeItemsInstr(SHAPEINSTR *shapeinstrptr);
extern void OutputKItem(int *shapeitemptr);
							
extern void RenderThisDisplayblock(DISPLAYBLOCK *dbPtr);
