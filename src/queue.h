#include <stdint.h>

#ifndef __INCL_QUEUE
#define __INCL_QUEUE

#define QUEUE_MAX_SIZE                                      65536

#define QERR_OK                                             0
#define QERR_QUEUE_FULL                                     0x0001

struct _queue;
typedef struct _queue * HQUEUE;

HQUEUE q_create(uint32_t capacity);
void q_destroy(HQUEUE q);
uint32_t q_getCapacity(HQUEUE q);
uint32_t q_getCurrentSize(HQUEUE q);
int q_addItem(HQUEUE q, void * item);
void * q_takeItem(HQUEUE q);

#endif
