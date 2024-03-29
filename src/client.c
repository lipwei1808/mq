/* client.c: Message Queue Client */

#include "mq/client.h"
#include "mq/logging.h"
#include "mq/socket.h"
#include "mq/string.h"

#include <unistd.h>
#include <errno.h>

/* Internal Constants */

#define SENTINEL "SHUTDOWN"

/* Internal Prototypes */

void * mq_pusher(void *);
void * mq_puller(void *);

/* External Functions */

/**
 * Create Message Queue withs specified name, host, and port.
 * @param   name        Name of client's queue.
 * @param   host        Address of server.
 * @param   port        Port of server.
 * @return  Newly allocated Message Queue structure.
 */
MessageQueue * mq_create(const char *name, const char *host, const char *port) {
    MessageQueue* mq = malloc(sizeof(MessageQueue));
    if (mq == NULL) {
        fprintf(stderr, "Error in mq_create malloc\n");
    }

    // Initialize name, host and port
    strncpy(mq->name, name, NI_MAXHOST - 1);
    mq->name[NI_MAXHOST - 1] = '\0';
    strncpy(mq->host, host, NI_MAXHOST - 1);
    mq->host[NI_MAXHOST - 1] = '\0';
    strncpy(mq->port, port, NI_MAXSERV - 1);
    mq->port[NI_MAXSERV - 1] = '\0';

    // Initialize outgoing
    Queue* outgoing = queue_create();
    if (outgoing == NULL) {
        free(mq);
        return NULL;
    }
    mq->outgoing = outgoing;

    // Initialize incoming
    Queue* incoming = queue_create();
    if (incoming == NULL) {
        queue_delete(outgoing);
        free(mq);
        return NULL;
    }
    mq->incoming = incoming;
    mq->shutdown = false;
    return mq;
}

/**
 * Delete Message Queue structure (and internal resources).
 * @param   mq      Message Queue structure.
 */
void mq_delete(MessageQueue *mq) {
    queue_delete(mq->outgoing);
    queue_delete(mq->incoming);
    free(mq);
}

/**
 * Publish one message to topic (by placing new Request in outgoing queue).
 * @param   mq      Message Queue structure.
 * @param   topic   Topic to publish to.
 * @param   body    Message body to publish.
 */
void mq_publish(MessageQueue *mq, const char *topic, const char *body) {
    // get parameters
    char* method = mq_get_method(PUT);
    char fmt_string[] = "/topic/%s";
    int size = snprintf(NULL, 0, fmt_string, topic);
    char uri[size + 1];
    sprintf(uri, fmt_string, topic);

    // insert request
    Request* req = request_create(method, uri, body);
    queue_push(mq->outgoing, req);
    free(method);
}

/**
 * Retrieve one message (by taking Request from incoming queue).
 * @param   mq      Message Queue structure.
 * @return  Newly allocated message body (must be freed).
 */
char * mq_retrieve(MessageQueue *mq) {
    Request* req = queue_pop(mq->incoming);
    // TODO: relook at this req memory (need to free - currently not freed)
    if (strcmp(req->body, SENTINEL) == 0) {
        request_delete(req);
        return NULL;
    }
    char* body = malloc(sizeof(char) * (strlen(req->body) + 1));
    strcpy(body, req->body);
    request_delete(req);
    return body;
}

/**
 * Subscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to subscribe to.
 **/
void mq_subscribe(MessageQueue *mq, const char *topic) {
    char* method = mq_get_method(PUT);
    char fmt_string[] = "/subscription/%s/%s";
    int size = snprintf(NULL, 0, fmt_string, mq->name, topic);
    char uri[size + 1];
    sprintf(uri, fmt_string, mq->name, topic);

    Request* req = request_create(method, uri, NULL);
    queue_push(mq->outgoing, req);
    free(method);
}

/**
 * Unubscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to unsubscribe from.
 **/
void mq_unsubscribe(MessageQueue *mq, const char *topic) {
    char* method = mq_get_method(DELETE);
    char fmt_string[] = "/subscription/%s/%s";
    int size = snprintf(NULL, 0, fmt_string, mq->name, topic);
    char uri[size + 1];
    sprintf(uri, fmt_string, mq->name, topic);

    Request* req = request_create(method, uri, NULL);
    queue_push(mq->outgoing, req);
    free(method);
}

/**
 * Start running the background threads:
 *  1. First thread should continuously send requests from outgoing queue.
 *  2. Second thread should continuously receive reqeusts to incoming queue.
 * @param   mq      Message Queue structure.
 */
void mq_start(MessageQueue *mq) {
    mq_subscribe(mq, "SHUTDOWN");
    pthread_create(&mq->pusher, NULL, mq_pusher, (void*) mq);
    pthread_create(&mq->puller, NULL, mq_puller, (void*) mq);
}

/**
 * Stop the message queue client by setting shutdown attribute and sending
 * sentinel messages
 * @param   mq      Message Queue structure.
 */
void mq_stop(MessageQueue *mq) {
    mq->shutdown = true;

    // Send sentinel message
    mq_publish(mq, "SHUTDOWN", "SHUTDOWN");
    pthread_join(mq->pusher, NULL);
    pthread_join(mq->puller, NULL);
}

/**
 * Returns whether or not the message queue should be shutdown.
 * @param   mq      Message Queue structure.
 */
bool mq_shutdown(MessageQueue *mq) {
    return mq->shutdown;
}

/* Internal Functions */

/**
 * Pusher thread takes messages from outgoing queue and sends them to server.
 * @param   arg     Message Queue structure.
 **/
void * mq_pusher(void *arg) {
    // Producer
    MessageQueue* mq = (MessageQueue*) arg;
    while (!mq_shutdown(mq)) {
        // Send message to server
        Request* req = queue_pop(mq->outgoing);
        FILE* socket = socket_connect(mq->host, mq->port);
        if (socket == NULL) {
            error("socket is null!\n");
        }
        request_write(req, socket);
        fflush(socket);
        request_delete(req);
    }
    return NULL;
}

/**
 * Puller thread requests new messages from server and then puts them in
 * incoming queue.
 * @param   arg     Message Queue structure.
 **/
void * mq_puller(void *arg) {
    // Consumer
    MessageQueue* mq = (MessageQueue*) arg;
    while (!mq_shutdown(mq)) {
        char* method = mq_get_method(GET);
        char fmt_string[] = "/queue/%s";
        int size = snprintf(NULL, 0, fmt_string, mq->name);
        char uri[size + 1];
        sprintf(uri, fmt_string, mq->name);
        Request* req = request_create(method, uri, NULL);

        FILE* socket = socket_connect(mq->host, mq->port);
        request_write(req, socket);
        fflush(socket);
        char response[BUFSIZ];
        int x = read(fileno(socket), response, BUFSIZ);
        if (x == -1) {
            error("THERE IS AN ERROR from reading by puller %d\n", errno);
        }
        response[x] = '\0';
        // Parse response into response code and body
        int response_code;
        if (sscanf(response, "HTTP/1.%*d %d", &response_code) == -1) {
            error("Error parsing response code\n");
        }

        if (response_code == 200) {
            int content_length;
            char* content_length_start = strstr(response, "Content-Length:");
            if (sscanf(content_length_start, "Content-Length: %d", &content_length) == -1) {
                error("error parsing content length\n");
            }

            char* body = strstr(response, "\r\n\r\n");
            body += 4; // move past the whitespaces
            body[content_length] = '\0';
            // Create the Request   
            Request* r = request_create(NULL, NULL, body);

            // Put into incoming queue
            queue_push(mq->incoming, r);
        }

        // cleanup resources
        request_delete(req);
        free(method);
    }
    return NULL;
}

/**
 * Retrieves a string HTTP method.
 * @param   method    HTTP_METHOD enum.
 * @return  char* HTTP method (must be freed).
 **/
char*       mq_get_method(enum HTTP_METHOD method) {
    char* method_ptr = NULL;
    char str[10] = "";
    switch (method) {
        case GET: {
            strcpy(str, "GET");
            break;
        }
        case PUT: {
            strcpy(str, "PUT");
            break;
        }
        case DELETE: {
            strcpy(str, "DELETE");
            break;
        }
        default: {
            return NULL;
        }
    }
    method_ptr = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(method_ptr, str);
    return method_ptr;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
