#ifndef AVP_FIRE_BATCH_H
#define AVP_FIRE_BATCH_H
/* Clip-space vertices; texcoord z chooses additive (0), inverse-colour (1), or alpha blend (2). */
typedef struct FireBatchVertex { float position[4], uvMode[3]; unsigned char colour[4]; } FireBatchVertex;
int FireBatchAvailable(void);
int FireBatchPending(void);
void FireBatchAppend(const FireBatchVertex *vertices, int count, unsigned int texture);
void FireBatchFlush(void);
void FireBatchRelease(void);
#endif
