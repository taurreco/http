#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h>
#include "http.h"

#define MAX_DATE_LEN 200

/*********************************************************************
 *                                                                   *
 *                           global data                             *
 *                                                                   *
 *********************************************************************/

extern struct page view[] = {
    { "/index.html" },
    { "/webserver.png" }
};

const char* view_loc  = "pages";
const char* date_fmt  = "%a, %d %b %Y %H:%M:%S %Z";
const char* resp_fmt  = "HTTP/1.0 %d %s\r\n"          /* status line */
                        "Content-Type: %s\r\n"        /* headers */
                        "Content-Length: %zu\r\n"
                        "Date: %s\r\n"
                        "\r\n";

/*********************************************************************
 *                                                                   *
 *                              utility                              *
 *                                                                   *
 *********************************************************************/

/**************
 * status_str *
 **************/

char*
status_str(enum status_code status)
{
    switch (status) {
        case OK: 
            return "OK";
        case BAD_REQUEST: 
            return "Bad Request";
        default: 
            return NULL;
    }
}

/************
 * mime_str *
 ************/

char*
mime_str(enum mime_type type)
{
    switch (type) {
        case TEXT_HTML: 
            return  "text/html;charset=utf-8";
        case IMAGE_PNG: 
            return "image/png";
        default: 
            return NULL;
    }
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

    n_pages = sizeof(view) / sizeof(struct page);

    for (int i = 0; i < n_pages; i++) {
        char* path;

        file = &view[i].file;

        status = asprintf(&path, "%s%s", view_loc, view[i].uri);

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

        if (data < 0) {
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

int
parse_request(struct request* req, char* data, int len)
{
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

    status_msg = status_str(resp->status);
    content_type = mime_str(resp->content_type);

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
 *                               misc                                *
 *                                                                   *
 *********************************************************************/

/*************
 * view_find *
 *************/

int
view_find(char* uri, struct file* file)
{
    int n_pages;

    n_pages = sizeof(view) / sizeof(struct page);

    for (int i = 0; i < n_pages; i++) {
        if (strcmp(view[i].uri, uri) == 0) {
            memcpy(file, &view[i].file, sizeof(struct file));
            return 0;
        }
    }

    return -1;
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

    n_pages = sizeof(view) / sizeof(struct page);

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


