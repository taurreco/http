#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h>
#include "http.h"

#define MAX_DATE_LEN        200
#define MAX_BUF_LEN         500
#define MAX_POST_ENTRIES    20

/*********************************************************************
 *                                                                   *
 *                           data struct                             *
 *                                                                   *
 *********************************************************************/

struct post_entry {
    char key[MAX_BUF_LEN];
    char val[MAX_BUF_LEN];
};

/*********************************************************************
 *                                                                   *
 *                           global data                             *
 *                                                                   *
 *********************************************************************/

struct route view[] = {
    { .resource = "/",              .page = "/index.html",    .file = { }, .type = TEXT_HTML },
    { .resource = "/login.html",    .page = "/login.html",    .file = { }, .type = TEXT_HTML },
    { .resource = "/webserver.png", .page = "/webserver.png", .file = { }, .type = IMAGE_PNG },
    { .resource = "/style/background.css", .page = "/style/background.css", .file = { }, .type = TEXT_CSS },
    { .resource = "/4xx.html",      .page = "/4xx.html",      .file = {},  .type = TEXT_HTML }
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

/***************
 * parse_entry *
 ***************/

void
parse_entry(char** data, char* buf)
{
    while (**data != '&' && **data != 0) {
        *buf = **data;
        (*data)++;
        buf++;
    }

    *buf = 0;
}

/*****************
 * parse_key_val *
 *****************/

void
parse_key_val(char* entry, char* key, char* val)
{
    while (*entry != '=') {
        *key = *entry;
        entry++;
        key++;
    }

    entry++;

    while (*entry != 0 && *entry != ':' && *entry != ' ') {
        *val = *entry;
        entry++;
        val++;
    }

    *key = 0;
    *val = 0;
}

/*****************
 * status_to_str *
 *****************/

char*
status_to_str(enum status_code status)
{
    switch (status) {
        case OK:
            return "OK";
        case BAD_REQUEST: 
            return "Bad Request";
        case NOT_FOUND:
            return "Not Found";
        default: 
    }

    return NULL;
}

/***************
 * mime_to_str *
 ***************/

char*
mime_to_str(enum mime_type type)
{
    switch (type) {
        case TEXT_HTML: 
            return "text/html;charset=utf-8";
        case TEXT_CSS:
            return "text/css;charset=utf-8";
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
    
    if (strcmp(str, "text/css;charset=utf-8") == 0)
        return TEXT_CSS;

    if (strcmp(str, "image/png") == 0)
        return IMAGE_PNG;

    if (strcmp(str, "application/x-www-form-urlencoded") == 0)
        return APP_XFORM;
    
    return 0;
}

/*********
 * split *
 *********/

void
split(char* start, int mid, char** left, char** right)
{
    *left = malloc(mid + 1);
    memcpy(*left, start, mid);
    (*left)[mid] = 0;
    *right = strdup(start + mid);
}

/**********************
 * next_angle_bracket *
 **********************/

void
next_angle_bracket(char** data)
{
    while (**data != '>')
        (*data)++;
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
    int n_pages, status, size;
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

        data = malloc(size + 1);
        fread(data, size, 1, fp);
        data[size] = 0;

        file->fp = fp;
        file->data = data;
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
        req->content = malloc(req->content_len + 1);
        memcpy(req->content, data, req->content_len);
        req->content[req->content_len] = 0;
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

    resp->status = OK;
    view_find(req->uri, NULL, &file, &resp->content_type);
    resp->content = file.data;
    resp->content_len = file.size;
}

/***************
 * handle_post *
 ***************/

void
handle_post(struct request* req, char* html, char** res)
{
    struct post_entry entries[MAX_POST_ENTRIES];
    char entry[MAX_BUF_LEN], key[MAX_BUF_LEN], val[MAX_BUF_LEN];
    char buf[MAX_BUF_LEN];
    char *content, *start, *cur;
    int len;

    /* parse entries into a table */

    len = 0;
    content = (char*)req->content;
    while (*content != 0 && len < MAX_POST_ENTRIES) {
        parse_entry(&content, entry);
        if (*content)
            content++;
        parse_key_val(entry, key, val);
        strcpy(entries[len].key, key);
        strcpy(entries[len].val, val);
        len++;
    }

    /* ignore status */

    start = strdup(html);

    for (int i = 0; i < len; i++) {
        int mid;
        char *key, *val, *target, *left, *right;
        
        mid = 0;
        key = entries[i].key;
        val = entries[i].val;
        asprintf(&target, "id=\"%s\"", key);

        cur = start;

        while (*cur != 0) {
            parse_word(&cur, buf);
            skip(&cur);
            if (strcmp(target, buf) == 0) {
                next_angle_bracket(&cur);
                mid = cur - start + 1;
                split(start, mid, &left, &right);
                free(start);
                asprintf(&start, "%s%s%s", left, val, right);
                cur = start + mid + 1;
                free(left);
                free(right);
            }
        }

        free(target);
    }

    *res = start;
}

/***************
 * route_error *
 ***************/

void
route_error(struct response* resp, enum status_code status)
{
    struct file file;

    resp->status = status;
    view_find("/4xx.html", NULL, &file, NULL);
    resp->content_len = asprintf((char**)&resp->content, (char*)file.data, status, status_to_str(status));
    resp->content_type = TEXT_HTML; 

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
        free(file->data);
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


