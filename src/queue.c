#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <time.h>

#include "queue.h"

#define QUEUE_SEMAPHORE_TIMEOUT_SECS                        5

typedef struct timespec TIME_TIMEOUT;

typedef struct {
    void *      data;

    uint32_t    id;
}
QUEUE_ITEM;

struct _queue {
    QUEUE_ITEM *    items;

    sem_t *         mutexSemaphore;
    TIME_TIMEOUT *  tm;

    uint32_t        capacity;
    uint32_t        currentSize;

    uint32_t        front;
    uint32_t        back;

    uint32_t        nextId;
};

inline TIME_TIMEOUT * _getSemaphoreTimeOut(HQUEUE q)
{
    clock_gettime(CLOCK_REALTIME, q->tm);

    q->tm->tv_sec += QUEUE_SEMAPHORE_TIMEOUT_SECS;

    return q->tm;
}

void q_dump(HQUEUE q)
{
    uint32_t        i;
    uint32_t        f;

    f = q->front;

    printf("q capacity: %u\n", q->capacity);
    printf("q currentSise: %u\n", q->currentSize);
    printf("q front: %u\n", q->front);
    printf("q back: %u\n", q->back);
    printf("q nextId: %u\n", q->nextId);

    for (i = 0;i < q->capacity;i++) {
        QUEUE_ITEM item = q->items[f++];

        if (f == q->currentSize) {
            break;
        }
        if (f == q->back) {
            break;
        }

        printf("Item id: %u\n", item.id);
    }
}

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

    queue->mutexSemaphore = (sem_t *)malloc(sizeof(sem_t));

    queue->tm = (TIME_TIMEOUT *)malloc(sizeof(TIME_TIMEOUT));

    if (sem_init(queue->mutexSemaphore, 0, 1)) {
        fprintf(stderr, "Failed to initialise mutex sempahore: %s\n", strerror(errno));
        exit(-1);
    }

    queue->capacity = capacity;
    queue->currentSize = 0;
    queue->front = 0;
    queue->back = 0;

    queue->nextId = 0x00000001;

    return queue;
}

void q_destroy(HQUEUE q)
{
    sem_destroy(q->mutexSemaphore);

    free(q->mutexSemaphore);
    free(q->tm);
    free(q->items);
    free(q);
}

uint32_t q_getCapacity(HQUEUE q)
{
    uint32_t    capacity;
    int         semRtn;

    semRtn = sem_timedwait(q->mutexSemaphore, _getSemaphoreTimeOut(q));

    if (semRtn < 0 && errno == ETIMEDOUT) {
        fprintf(stderr, "FATAL ERROR: Timed out waiting for sempahore lock in function q_getCapacity()\n");
        exit(-1);
    }
    else if (semRtn < 0) {
        fprintf(stderr, "FATAL ERROR: Error waiting for semephore lock in function q_getCapacity(): %s\n", strerror(errno));
        exit(-1);
    }

    capacity = q->capacity;
    sem_post(q->mutexSemaphore);

    return capacity;
}

uint32_t q_getCurrentSize(HQUEUE q)
{
    uint32_t    currentSize;
    int         semRtn;

    semRtn = sem_timedwait(q->mutexSemaphore, _getSemaphoreTimeOut(q));

    if (semRtn < 0 && errno == ETIMEDOUT) {
        fprintf(stderr, "FATAL ERROR: Timed out waiting for sempahore lock in function q_getCurrentSize()\n");
        exit(-1);
    }
    else if (semRtn < 0) {
        fprintf(stderr, "FATAL ERROR: Error waiting for semephore lock in function q_getCurrentSize(): %s\n", strerror(errno));
        exit(-1);
    }

    currentSize = q->currentSize;
    sem_post(q->mutexSemaphore);

    return currentSize;
}

int q_addItem(HQUEUE q, void * item)
{
    int         semRtn;

    semRtn = sem_timedwait(q->mutexSemaphore, _getSemaphoreTimeOut(q));

    if (semRtn < 0 && errno == ETIMEDOUT) {
        fprintf(stderr, "FATAL ERROR: Timed out waiting for sempahore lock in function q_addItem()\n");
        exit(-1);
    }
    else if (semRtn < 0) {
        fprintf(stderr, "FATAL ERROR: Error waiting for semephore lock in function q_addItem(): %s\n", strerror(errno));
        exit(-1);
    }

    /*
    ** Add the item to the back of the queue...
    */
    q->items[q->back].data = item;
    q->items[q->back].id = q->nextId++;

    q->currentSize++;

    if (q->currentSize == QUEUE_MAX_SIZE) {
        sem_post(q->mutexSemaphore);
        return QERR_QUEUE_FULL;
    }

    q->back++;

    if (q->back == q->capacity) {
        q->back = 0;
    }

    sem_post(q->mutexSemaphore);

    return QERR_OK;
}

void * q_takeItem(HQUEUE q)
{
    QUEUE_ITEM  qItem;
    void *      item;
    int         semRtn;

    semRtn = sem_timedwait(q->mutexSemaphore, _getSemaphoreTimeOut(q));

    if (semRtn < 0 && errno == ETIMEDOUT) {
        fprintf(stderr, "FATAL ERROR: Timed out waiting for sempahore lock in function q_takeItem()\n");
        exit(-1);
    }
    else if (semRtn < 0) {
        fprintf(stderr, "FATAL ERROR: Error waiting for semephore lock in function q_takeItem(): %s\n", strerror(errno));
        exit(-1);
    }

    /*
    ** Check if the queue is empty...
    */
    if (q->front == q->back) {
        sem_post(q->mutexSemaphore);
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

        sem_post(q->mutexSemaphore);

        return NULL;
    }

    item = qItem.data;

    q->currentSize--;

    if (q->front == q->capacity) {
        q->front = 0;
    }

    sem_post(q->mutexSemaphore);

    return item;
}
