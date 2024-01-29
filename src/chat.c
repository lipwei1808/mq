#include <stdio.h>

#include "mq/queue.h"
#include "mq/client.h"

#include <string.h>

// Global variables
MessageQueue* mq;
char chat[BUFSIZ] = "";

#define DEFAULT_PORT "9002"
#define DEFAULT_HOST "0.0.0.0"
#define SPACE " "

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

struct cli_args {
  char name[NI_MAXHOST];
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
};

void display_help() {
  fprintf(stderr, "Usage: [options]\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -name NAME            Name of the user chatting\n");
  fprintf(stderr, "    -host HOST            Host of the server to connect to\n");
  fprintf(stderr, "    -port PORT            Port number of the server\n");
  fprintf(stderr, "    -help                 Print this help message\n");
}

bool parse_args(int argc, char* argv[], struct cli_args* result) {
  unsigned int idx = 1;
  while (idx < argc && strlen(argv[idx]) > 1 && argv[idx][0] == '-') {
    char* flag = argv[idx++] + 1;

    if (strcmp(flag, "help") == 0) {
      display_help();
      return false;
    } 
    if (idx >= argc) {
      error("invalid args\n");
      display_help();
      return false;
    }

    if (strcmp(flag, "hos") == 0) {
      strncpy(result->host, argv[idx++], NI_MAXHOST);
      result->host[NI_MAXHOST - 1] = '\0';
    } else if (strcmp(flag, "name") == 0) {
      strncpy(result->name, argv[idx++], NI_MAXHOST);
      result->name[NI_MAXHOST - 1] = '\0';
    } else if (strcmp(flag, "port") == 0) {
      strncpy(result->port, argv[idx++], NI_MAXSERV);
      result->port[NI_MAXSERV - 1] = '\0';
    } else {
      display_help();
      return false;
    }
  }

  // check if port and name is provided
  if (strlen(result->name) == 0) {
    error("missing name\n");
    display_help();
    return false;
  }

  if (strlen(result->port) == 0) {
    strcpy(result->port, DEFAULT_PORT);
  }

  if (strlen(result->host) == 0) {
    strcpy(result->host, DEFAULT_HOST);
  }
  return true;
}

int main(int argc, char* argv[]) {
  // parse command line arguments
  struct cli_args result;
  memset(result.host, 0, NI_MAXHOST);
  memset(result.name, 0, NI_MAXHOST);
  memset(result.port, 0, NI_MAXSERV);
  bool success = parse_args(argc, argv, &result);
  if (!success) {
    exit(1);
  }

  // create client
  mq = mq_create(result.name, result.host, result.port);
  if (mq == NULL) {
    fprintf(stderr, "error creating mq\n");
    exit(1);
  }

  // start the application and subscriptions
  mq_start(mq);
  mq_subscribe(mq, "SHUTDOWN");
  
  // Start puller
  pthread_t puller;
  pthread_create(&puller, NULL, worker, NULL);

  // Start event loop
  while (!feof(stdin)) {
    // Get user input
    char input[BUFSIZ];
    while (fgets(input, BUFSIZ, stdin) == NULL && !feof(stdin));
    // replace ending whitespace with null terminator
    size_t len = strlen(input);
    input[len - 1] = '\0';

    // Parse user input
    char copyInput[BUFSIZ];
    strcpy(copyInput, input);
    char command[BUFSIZ] = "";
    char argument[BUFSIZ] = "";
    char* token = strtok(copyInput, " ");

    if (token != NULL) {
      strcat(command, token);
    }
    token = strtok(NULL, " ");
    while (token != NULL) {
      strcat(argument, token);
      token = strtok(NULL, " ");
      if (token != NULL) {
        strcat(argument, SPACE);
      }
    }

    if (strcmp(command, "/exit") == 0 || strcmp(command, "/quit") == 0) {
      mq_stop(mq);
      info("peaceful exit by user\n");
      break;
    } else if (strcmp(command, "/subscribe") == 0) {
      if (strcmp(argument, "") == 0) {
        error("invalid arg\n");
      }
      if (strcmp(chat, "") != 0) {
        error("already subscribe to a chat, /unsubscribe %s first before subscribing\n", chat);
        continue;
      }
      strcpy(chat, argument);

      char* token = strchr(argument, ' ');
      if (token != NULL) {
        error("invalid topic to subscribe\n");
        continue;
      }
      mq_subscribe(mq, argument);
    } else if (strcmp(command, "/unsubscribe") == 0) {
      if (strcmp(argument, "") == 0) {
        error("invalid arg\n");
      }
      char* token = strchr(argument, ' ');
      if (token != NULL) {
        error("invalid topic to unsubscribe\n");
        continue;
      }

      if (strcmp(chat, "") == 0) {
        error("not subscribed to a chat yet, /subscribe %s first", argument);
        continue;
      }
      strcpy(chat, "");
      mq_unsubscribe(mq, argument);
    } else {
      if (strcmp(chat, "") == 0) {
        error("subscribe to a chat first with /subscribe <chat>\n");
      } else {
        mq_publish(mq, "chat", input);
      }
    }
  }

  // Cleanup
  pthread_join(puller, NULL);
  mq_delete(mq);
}