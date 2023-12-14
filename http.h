#ifndef HTTP_H
#define HTTP_H

#include <stdint.h>

#define MAX_URI_LEN 200

/*********************************************************************
 *                                                                   *
 *                     enum & struct definitions                     *
 *                                                                   *
 *********************************************************************/

/*************
 * mime_type *
 *************/

enum mime_type {
    TEXT_HTML    = 0x00000000A,
    IMAGE_PNG    = 0x000000024
};

/***************
 * method_type *
 ***************/

enum method_type {
    GET,
    POST
};

/***************
 * status_code *
 ***************/

enum status_code {
    OK              = 200,
    BAD_REQUEST     = 400,
    NOT_FOUND       = 404
};

/***********
 * request *
 ***********/

/* a parsed HTTP request */

struct request {
    enum method_type method;                /* request line */
    char uri[MAX_URI_LEN];

    int content_len;                           /* body */
    enum mime_type content_type;
    uint8_t* content;
};

/************
 * response *
 ************/

/* relevant content for an http response */

struct response {
    enum status_code status;

    int content_len;
    enum mime_type content_type;
    uint8_t* content;
};

/********
 * file *
 ********/

struct file {
    FILE* fp;
    uint8_t* data;
    int fd;
    int size;
};

/********
 * page *
 ********/

struct page {
    char* uri;
    struct file file;
};

/*********************************************************************
 *                                                                   *
 *                               view                                *
 *                                                                   *
 *********************************************************************/

/* holds file data associated with pages in the website */

extern struct page view[];

/*********************************************************************
 *                                                                   *
 *                            functions                              *
 *                                                                   *
 *********************************************************************/

void request_init(struct request* req);
enum status_code parse_request(struct request* req, char* data, int len);

void response_init(struct response* resp);
void make_response(struct response* resp, char** data, int* len);

void request_free(struct request* req);
void response_free(struct response* resp);

void route_response(struct response* resp, struct request* req);
void route_error(struct response* resp, enum status_code status);

int view_init();
void view_free();

#endif    /* HTTP_H */