#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_MAX_SIZE                                      32768

typedef struct _queue {
    void *      data;

    uint32_t    size;

    uint32_t    top;
    uint32_t    bottom;

    uint32_t    pointer;
};

typedef struct _queue * HQUEUE;

HQUEUE q_create(uint32_t numItems)
{
    HQUEUE      queue;

    if (numItems > QUEUE_MAX_SIZE) {
        fprintf(
            stderr, 
            "FATAL ERROR: Maximum queue size is %u, requested size %u\n", 
            QUEUE_MAX_SIZE, 
            numItems);
        exit(-1);
    }

    queue = (HQUEUE)malloc(sizeof(struct _queue));

    if (queue == NULL) {
        fprintf(
            stderr, 
            "FATAL ERROR: Failed to allocate memory for HQUEUE object\n");
        exit(-1);
    }

    queue->data = (void *)malloc(numItems * sizeof(void *));

    if (queue->data == NULL) {
        fprintf(
            stderr, 
            "FATAL ERROR: Failed to allocate memory for queue data\n");
        exit(-1);
    }

    queue->size = numItems;
    queue->bottom = 0;
    queue->top = 0;
    queue->pointer = 0;

    return queue;
}

void q_destroy(HQUEUE q)
{
    free(q->data);
    free(q);
}

