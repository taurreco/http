#ifndef HTTP_H
#define HTTP_H

#define MAX_URI_LEN 200
#define MAX_HEADER_LEN 200

/*********************************************************************
 *                                                                   *
 *                     enum & struct definitions                     *
 *                                                                   *
 *********************************************************************/

/*************
 * mime_type *
 *************/

enum mime_type {
    TEXT,
    PNG
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
    OK = 200,
    BAD_REQUEST = 400
};

/*******
 * uri *
 *******/

struct uri {
    char raw[MAX_URI_LEN];
};

/***************
 * header_host *
 ***************/

struct header_host {
    char raw[MAX_HEADER_LEN];
    int len;
    char* host;
    char* host_end;
    char* port;
    char* port_end;
};

/*********************
 * header_user_agent *
 *********************/

struct header_user_agent {
    char raw[MAX_HEADER_LEN];
};

/***********
 * request *
 ***********/

/* a parsed HTTP request */

struct request {
    enum method_type method;                /* request line */
    struct uri uri;

    struct header_host host;                /* headers */
    struct header_user_agent user_agent;

    int data_len;                           /* body */
    enum mime_type data_type;
    uint8_t* data;
};

/************
 * response *
 ************/

/* relevant data for an http response */

struct response {
    enum status_code status;
    char* status_msg;

    int is_data_mmapped;
    int data_len;
    enum mime_type data_type;
    uint8_t* data;
};

/*********************************************************************
 *                                                                   *
 *                            functions                              *
 *                                                                   *
 *********************************************************************/

void init_request(struct request* req);
int parse_request(struct request* req, char* data, int len);

int init_response(struct response* resp);
void make_response(struct response* resp, char** data, int* len);

void free_request(struct request* req);
void free_response(struct response* resp);

#endif    /* HTTP_H */