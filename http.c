#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>
#include "http.h"

/*********************************************************************
 *                                                                   *
 *                           global data                             *
 *                                                                   *
 *********************************************************************/

extern struct page view[] = {
    { "/index.html" },
    { "/webserver.png" }
};

char* view_loc = "pages";

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
}

/********************************************************************
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
make_response(struct response* resp, char** data, int* len)
{
    return 0;
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


