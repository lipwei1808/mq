/* queue.c: Concurrent Queue of Requests */

#include "mq/queue.h"

#include <assert.h>

/**
 * Create queue structure.
 * @return  Newly allocated queue structure.
 */
Queue * queue_create() {
    Queue* q = malloc(sizeof(Queue));
    if (q == NULL) {
        return NULL;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    int res = pthread_mutex_init(&q->mutex, NULL);
    if (res != 0) {
        fprintf(stderr, "Something went wrong with mutex init err=%d\n", res);
        free(q);
        return NULL;
    }
    return q;
}

/**
 * Delete queue structure.
 * @param   q       Queue structure.
 */
void queue_delete(Queue *q) {
    Request* cur = q->head;
    while (cur) {
        Request* next = cur->next;
        request_delete(cur);
        cur = next;
    }
    free(q);
}

/**
 * Push request to the back of queue.
 * @param   q       Queue structure.
 * @param   r       Request structure.
 */
void queue_push(Queue *q, Request *r) {
    if (q->size == 0) {
        q->head = r;
        q->tail = r;
        q->size = 1;
    } else {
        q->tail->next = r;
        q->tail = q->tail->next;
    }
}

/**
 * Pop request to the front of queue (block until there is something to return).
 * @param   q       Queue structure.
 * @return  Request structure.
 */
Request * queue_pop(Queue *q) {
    if (q->size == 0) {
        return NULL;
    }

    if (q->size == 1) {
        Request* req = q->head;
        q->head = NULL;
        q->tail = NULL;
        return req;
    }

    Request* req = q->head;
    q->head = req->next;
    return req;
}

void queue_status(Queue* q) {
    assert(q != NULL);
    printf("Queue size: %zu\n", q->size);
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
