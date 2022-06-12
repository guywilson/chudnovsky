#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"


typedef struct {
    void *      data;

    uint32_t    id;
}
QUEUE_ITEM;

struct _queue {
    QUEUE_ITEM *    items;

    uint32_t        capacity;
    uint32_t        currentSize;

    uint32_t        front;
    uint32_t        back;

    uint32_t        nextId;
};

HQUEUE q_create(uint32_t capacity)
{
    HQUEUE      queue;

    if (capacity > QUEUE_MAX_SIZE) {
        fprintf(
            stderr, 
            "FATAL ERROR: Maximum queue size is %u, requested size %u\n", 
            QUEUE_MAX_SIZE, 
            capacity);
        exit(-1);
    }

    queue = (HQUEUE)malloc(sizeof(struct _queue));

    if (queue == NULL) {
        fprintf(
            stderr, 
            "FATAL ERROR: Failed to allocate memory for HQUEUE object\n");
        exit(-1);
    }

    queue->items = (QUEUE_ITEM *)malloc(capacity * sizeof(QUEUE_ITEM));

    if (queue->items == NULL) {
        fprintf(
            stderr, 
            "FATAL ERROR: Failed to allocate memory for queue data\n");
        exit(-1);
    }

    memset(queue->items, 0, (capacity * sizeof(QUEUE_ITEM)));

    queue->capacity = capacity;
    queue->currentSize = 0;
    queue->front = 0;
    queue->back = 0;

    queue->nextId = 0x00000001;

    return queue;
}

void q_destroy(HQUEUE q)
{
    free(q->items);
    free(q);
}

uint32_t q_getCapacity(HQUEUE q)
{
    return q->capacity;
}

uint32_t q_getCurrentSize(HQUEUE q)
{
    return q->currentSize;
}

int q_addItem(HQUEUE q, void * item)
{
    /*
    ** Add the item to the back of the queue...
    */
    q->items[q->back].data = item;
    q->items[q->back].id = q->nextId++;

    q->currentSize++;

    if (q->currentSize == QUEUE_MAX_SIZE) {
        return QERR_QUEUE_FULL;
    }

    q->back++;

    if (q->back == QUEUE_MAX_SIZE) {
        q->back = 0;
    }

    return QERR_OK;
}

void * q_takeItem(HQUEUE q)
{
    QUEUE_ITEM          qItem;
    void *              item;

    /*
    ** Check if the queue is empty...
    */
    if (q->front == q->back) {
        return NULL;
    }

    qItem = q->items[q->front++];

    if (qItem.data == NULL) {
        fprintf(
            stderr, 
            "FATAL ERROR: Item with ID %d in queue is NULL. f: %d, b: %d, s: %d", 
            qItem.id, 
            q->front, 
            q->back, 
            q->currentSize);

        return NULL;
    }

    q->currentSize--;

    if (q->front == QUEUE_MAX_SIZE) {
        q->front = 0;
    }

    return item;
}
