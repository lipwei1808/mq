#include <stdio.h>

#include "mq/queue.h"
#include "mq/client.h"
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
  char* put = mq_get_method(PUT);
  char* get = mq_get_method(GET);
  char* del = mq_get_method(DELETE);
  printf("Method: %s, %s, %s\n", get, put, del);
  printf("Method: %p, %p, %p\n", &get, &put, &del);
}