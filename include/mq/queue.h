/* queue.h: Concurrent Queue of Requests */

#ifndef QUEUE_H
#define QUEUE_H

#include "mq/request.h"
#include "mq/thread.h"

/* Structures */

typedef struct Queue Queue;
struct Queue {
    Request *head;
    Request *tail;
    size_t   size;

    /* TODO: Add any necessary thread and synchronization primitives */
    pthread_mutex_t mutex;      // allows single access to the queue
    pthread_cond_t notEmpty;    // condition to keep track when queue is not empty
    pthread_cond_t empty;       // condition to track when queue is empty
};

/* Functions */

Queue *	    queue_create();
void        queue_delete(Queue *q);

void	    queue_push(Queue *q, Request *r);
Request *   queue_pop(Queue *q);

void        queue_status(Queue* q);

#endif

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
