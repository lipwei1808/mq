#include <stdio.h>

#include "mq/queue.h"
#include "mq/client.h"

#include <string.h>

MessageQueue* mq;

int main() {
  // Initialize
  char name[] = "chatx";
  char host[] = "localhost";
  char port[] = "9000";
  mq = mq_create(name, host, port);
  if (mq == NULL) {
    fprintf(stderr, "error creating mq\n");
    exit(1);
  }

  // Start event loop
  while (!feof(stdin)) {
    printf(">CHAT ");
    char input[BUFSIZ];

    fgets(input, BUFSIZ, stdin);
    chomp(input);
    printf("INPUT: [%s]\n", input);
  }

  // Cleanup
  mq_delete(mq);
}