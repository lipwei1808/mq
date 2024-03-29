/* request.c: Request structure */

#include "mq/request.h"
#include "mq/logging.h"

#include <stdlib.h>
#include <string.h>

/**
 * Create Request structure.
 * @param   method      Request method string.
 * @param   uri         Request uri string.
 * @param   body        Request body string.
 * @return  Newly allocated Request structure.
 */
Request * request_create(const char *method, const char *uri, const char *body) {
    Request* req = malloc(sizeof(Request));
    if (method == NULL) {
        req->method = NULL;
    } else {
        int size = strlen(method);
        req->method = malloc(sizeof(char) * ++size);
        strcpy(req->method, method);
    }

    if (uri == NULL) {
        req->uri = NULL;
    } else {
        int size = strlen(uri);
        req->uri = malloc(sizeof(char) * ++size);
        strcpy(req->uri, uri);
    }

    if (body == NULL) {
        req->body = NULL;
    } else {
        int size = strlen(body);
        req->body = malloc(sizeof(char) * ++size);
        strcpy(req->body, body);
    }

    req->next = NULL;
    return req;
}

/**
 * Delete Request structure.
 * @param   r           Request structure.
 */
void request_delete(Request *r) {
    free(r->method);
    free(r->uri);
    free(r->body);
    free(r);
}

/**
 * Write HTTP Request to stream:
 *  
 *  $METHOD $URI HTTP/1.0\r\n
 *  Content-Length: Length($BODY)\r\n
 *  \r\n
 *  $BODY
 *      
 * @param   r           Request structure.
 * @param   fs          Socket file stream.
 */
void request_write(Request *r, FILE *fs) {
    if (r->body == NULL) {
        fprintf(fs, "%s %s HTTP/1.0\r\n\r\n",
                r->method, r->uri);
    } else {
        fprintf(fs, "%s %s HTTP/1.0\r\nContent-Length: %zu\r\n\r\n%s",
                r->method, r->uri, strlen(r->body), r->body);
    }
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */ 
