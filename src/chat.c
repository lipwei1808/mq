#include <stdio.h>

#include "mq/queue.h"


int main() {
  printf("Hello World\n");
  Queue* q = queue_create();
  if (q == NULL) {
    printf("Error init\n");
    exit(1);
  }
  queue_status(q);
  queue_delete(q);
}