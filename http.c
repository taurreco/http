#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h>
#include "http.h"

#define MAX_DATE_LEN 200
#define MAX_BUF_LEN 500

/*********************************************************************
 *                                                                   *
 *                           global data                             *
 *                                                                   *
 *********************************************************************/

struct route view[] = {
    { .resource = "/",              .page = "/index.html",    .file = { }, .type = TEXT_HTML },
    { .resource = "/login.html",    .page = "/login.html",    .file = { }, .type = TEXT_HTML },
    { .resource = "/webserver.png", .page = "/webserver.png", .file = { }, .type = IMAGE_PNG }
};

const char* view_loc  = "pages";
const char* date_fmt  = "%a, %d %b %Y %H:%M:%S %Z";
const char* resp_fmt  = "HTTP/1.0 %d %s\r\n"          /* status line */
                        "Content-Type: %s\r\n"        /* headers */
                        "Content-Length: %zu\r\n"
                        "Date: %s\r\n"
                        "\r\n";                       /* CRLF */

/*********************************************************************
 *                                                                   *
 *                              utility                              *
 *                                                                   *
 *********************************************************************/

/***********
 * is_crlf *
 ***********/

int
is_crlf(char* a)
{
    return a[0] == '\r' && a[1] == '\n';
}

/********
 * skip *
 ********/

void
skip(char** data)
{
    while (**data == ' ' && **data != 0)
        (*data)++;
}

/***************
 * end_of_line *
 ***************/

/* skips until the end of the line */

void
end_of_line(char** data)
{
    while (**data != '\r' && **data != 0)
        (*data)++;
}

/**************
 * parse_word *
 **************/

/* fills buf with a keyword, moves data to whitespace after */

void
parse_word(char** data, char* buf)
{
    while (**data != ' ' && **data != '\r' && **data != 0) {
        *buf = **data;
        (*data)++;
        buf++;
    }

    *buf = 0;
}

/**************
 * status_to_str *
 **************/

char*
status_to_str(enum status_code status)
{
    switch (status) {
        case OK:
            return "OK";
        case BAD_REQUEST: 
            return "Bad Request";
        default: 
    }

    return NULL;
}

/************
 * mime_to_str *
 ************/

char*
mime_to_str(enum mime_type type)
{
    switch (type) {
        case TEXT_HTML: 
            return  "text/html;charset=utf-8";
        case IMAGE_PNG: 
            return "image/png";
        case APP_XFORM:
            return "application/x-www-form-urlencoded";
        default: 
    }

    return NULL;
}

/************
 * str_mime *
 ************/

enum mime_type
str_to_mime(char* str)
{
    if (strcmp(str, "text/html;charset=utf-8") == 0)
        return TEXT_HTML;


    if (strcmp(str, "image/png") == 0)
        return IMAGE_PNG;


    if (strcmp(str, "application/x-www-form-urlencoded") == 0)
        return APP_XFORM;
    
    return 0;
}

/*************
 * view_find *
 *************/

int
view_find(char* resource, char* page, struct file* file, enum mime_type* type)
{
    int n_pages;

    n_pages = sizeof(view) / sizeof(struct route);

    for (int i = 0; i < n_pages; i++) {
        if (strcmp(view[i].resource, resource) == 0) {
            if (page)
                strcpy(page, view[i].page);
            if (file)
                memcpy(file, &view[i].file, sizeof(struct file));
            if (type)
                *type = view[i].type;
            return 0;
        }
    }

    return -1;
}

/*********************************************************************
 *                                                                   *
 *                            initializers                           *
 *                                                                   *
 *********************************************************************/

/*************
 * view_init *
 *************/

int
view_init()
{
    FILE* fp;
    int n_pages, status, fd, size;
    uint8_t* data;
    struct file* file;

    n_pages = sizeof(view) / sizeof(struct route);

    for (int i = 0; i < n_pages; i++) {
        char* path;

        file = &view[i].file;

        status = asprintf(&path, "%s%s", view_loc, view[i].page);

        if (status < 0)
            return -1;

        fp = fopen(path, "r");

        if (fp == NULL) {
            fprintf(stderr, "[ERROR] fopen %s\n", strerror(errno));
            free(path);
            return -1;
        }

        /* get file size */

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        /* map file */

        fd = fileno(fp);
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

        if (data == MAP_FAILED) {
            fprintf(stderr, "[ERROR] mmap %s\n", strerror(errno));
            fclose(fp);
            view_free();
            free(path);
            return -1;
        }

        file->fp = fp;
        file->data = data;
        file->fd = fd;
        file->size = size;
        
        free(path);
    }

    return 0;
}

/****************
 * request_init *
 ****************/

void 
request_init(struct request* req)
{
    memset(req, 0, sizeof(struct request));
}

/*****************
 * response_init *
 *****************/

void 
response_init(struct response* resp) 
{
    memset(resp, 0, sizeof(struct response));
}

/*********************************************************************
 *                                                                   *
 *                             conversion                            *
 *                                                                   *
 *********************************************************************/

/*****************
 * parse_request *
 *****************/

enum status_code
parse_request(struct request* req, char* data)
{
    char buf[MAX_BUF_LEN];
    int status;

    status = 0;

    /* request method type */

    req->method = -1;
    parse_word(&data, buf);
    skip(&data);
    
    if (strcmp(buf, "GET") == 0) {
        req->method = GET;
    }

    if (strcmp(buf, "POST") == 0) {
        req->method = POST;
    }

    if ((int)req->method == -1)
        return BAD_REQUEST;

    /* uri */

    parse_word(&data, buf);
    skip(&data);

    status = view_find(buf, NULL, NULL, NULL);
    if (status < 0)
        return NOT_FOUND;

    strcpy(req->uri, buf);
 
    /* version */
    
    parse_word(&data, buf);
    skip(&data);
    
    /* headers */

    end_of_line(&data);
    data += 2;    /* skip \r\n */
    while (!is_crlf(data)) {
        skip(&data);
        parse_word(&data, buf);
        skip(&data);
        
        if (strcmp("Content-Type:", buf) == 0) {
            parse_word(&data, buf);
            skip(&data);
            req->content_type = str_to_mime(buf); 
            if (req->content_type == 0)
                return BAD_REQUEST;
        }

        if (strcmp("Content-Length:", buf) == 0) {
            parse_word(&data, buf);
            skip(&data);
            req->content_len = atoi(buf);
            if (req->content_len == 0)
                return BAD_REQUEST;
        }

        end_of_line(&data);
        data += 2;
    }
    data += 2;
    skip(&data);

    /* body */

    if (req->content_len != 0) {
        req->content = malloc(req->content_len);
        memcpy(req->content, data, req->content_len);
    }

    return 0;
}

/*****************
 * make_response *
 *****************/

void 
make_response(struct response* resp, char** data, int* data_len)
{
    time_t now;
    struct tm* tm;
    int len, full_len;
    char *buf, *full_buf, *status_msg, *content_type;
    char date[MAX_DATE_LEN];

    status_msg = status_to_str(resp->status);
    content_type = mime_to_str(resp->content_type);

    now = time(NULL);
    tm = gmtime(&now);

    strftime(date, MAX_DATE_LEN, date_fmt, tm);
    len = asprintf(&buf, resp_fmt, resp->status, status_msg, 
                   content_type, resp->content_len, date);

    /* pray to god len > 0 */

    full_len = len + resp->content_len;

    full_buf = malloc(full_len);
    memcpy(full_buf, buf, len);
    memcpy(full_buf + len, resp->content, resp->content_len);

    free(buf);

    *data = full_buf;
    *data_len = full_len;
}

/*********************************************************************
 *                                                                   *
 *                              routing                              *
 *                                                                   *
 *********************************************************************/

/******************
 * route_response *
 ******************/

void
route_response(struct response* resp, struct request* req)
{
    struct file file;

    switch (req->method) {
        case GET:
            resp->status = OK;
            view_find(req->uri, NULL, &file, &resp->content_type);
            resp->content = file.data;
            resp->content_len = file.size;
            break;
        default:
    }
    return;
}

/***************
 * route_error *
 ***************/

void
route_error(struct response* resp, enum status_code status)
{
   // struct file file;
/*
    resp->status = status;
    view_find(status_to_str(status), &file);
    resp->content = file.data;
    resp->content_len = file.size;
    resp->content_type = TEXT_HTML; 
*/
}

/*********************************************************************
 *                                                                   *
 *                            destructors                            *
 *                                                                   *
 *********************************************************************/

/*************
 * view_free *
 *************/

void
view_free()
{
    int n_pages;
    struct file* file;

    n_pages = sizeof(view) / sizeof(struct route);

    for (int i = 0; i < n_pages; i++) {
        file = &view[i].file;
        
        if (file == NULL)
            continue;

        fclose(file->fp);
        munmap(file->data, file->size);
    }
}

/****************
 * request_free *
 ****************/

void 
request_free(struct request* req)
{

}

/*****************
 * response_free *
 *****************/

void 
response_free(struct response* resp)
{

}


