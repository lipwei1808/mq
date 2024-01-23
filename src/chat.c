#include <stdio.h>

#include "mq/queue.h"


int main() {
  printf("Hello World\n");
  Queue* q = queue_create();
  queue_status(q);
  queue_delete(q);
}