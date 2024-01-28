#include <stdio.h>

#include "mq/queue.h"
#include "mq/client.h"

#include <string.h>

MessageQueue* mq;

void* worker(void* arg) {
  while (!mq_shutdown(mq)) {
    pthread_mutex_lock(&mq->incoming->mutex);
    while (mq->incoming->size == 0) {
      pthread_cond_wait(&mq->incoming->notEmpty, &mq->incoming->mutex);
    }

    Request* res = queue_pop(mq->incoming);
    printf("\n[FROM SERVER] %s\n", res->body);
    pthread_mutex_unlock(&mq->incoming->mutex);
  }
  return NULL;
}

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
  mq_subscribe(mq, "SHUTDOWN");
  
  // Start puller
  pthread_t puller;
  pthread_create(&puller, NULL, worker, NULL);

  // Start event loop
  while (!feof(stdin)) {
    char input[BUFSIZ];

    fgets(input, BUFSIZ, stdin);
    size_t len = strlen(input);
    input[len - 1] = '\0';
    if (strcmp(input, "/exit") == 0 || strcmp(input, "/quit") == 0) {
      mq_stop(mq);
      info("peaceful exit by user\n");
      break;
    }

    mq_publish(mq, "chat", input);

  }

  // Cleanup
  pthread_join(puller, NULL);
  mq_delete(mq);
}