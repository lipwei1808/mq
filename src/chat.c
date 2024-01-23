#include <stdio.h>

#include "mq/queue.h"
Queue* q;

void* worker(void* arg) {
  long k = (long) arg;
  for (int i = 0; i < 1000000; i++) {
    // printf("WORKER %d\n", k);
    Request* req = request_create(NULL, NULL, NULL);
    queue_push(q, req);
  }
  return NULL;
}

int main() {
  q = queue_create();
  if (q == NULL) {
    printf("Error init\n");
    exit(1);
  }
  queue_status(q);

  pthread_t t1;
  pthread_t t2;
  long x = 1;
  long y = 2;
  pthread_create(&t1, NULL, worker, (void*) x);
  pthread_create(&t2, NULL, worker, (void*) y);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  
  queue_status(q);
  queue_delete(q);
}