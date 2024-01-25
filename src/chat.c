#include <stdio.h>

#include "mq/queue.h"
#include "mq/client.h"

#include <string.h>

MessageQueue* mq;

int main() {
  // Initialize
  char name[] = "chatx";
  char host[] = "0.0.0.0";
  char port[] = "9002";
  mq = mq_create(name, host, port);
  if (mq == NULL) {
    fprintf(stderr, "error creating mq\n");
    exit(1);
  }

  // start the application and subscriptions
  mq_start(mq);
  mq_subscribe(mq, "chat");

  // Start event loop
  while (!feof(stdin)) {
    printf(">CHAT ");
    char input[BUFSIZ];

    fgets(input, BUFSIZ, stdin);
    size_t len = strlen(input);
    input[len - 1] = '\0';
    info("len: %zu, Input: [%s]\n", len, input);
    if (strcmp(input, "/exit") == 0 || strcmp(input, "/quit") == 0) {
      info("peaceful exit by user\n");
      break;
    }

    mq_publish(mq, "chat", input);

  }

  // Cleanup
  mq_delete(mq);
}