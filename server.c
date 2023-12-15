#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "http.h"

#define PORT             "8080"
#define MAX_NUM_CONNS    12
#define MAX_BUF_LEN      1000

/*********************************************************************
 *                                                                   *
 *                        struct definitions                         *
 *                                                                   *
 *********************************************************************/

/********
 * conn *
 ********/

/* data associated with a connection */

struct conn {
    pthread_t thr;
    struct sockaddr addr;
    int fd, addrlen;
};

/*********************************************************************
 *                                                                   *
 *                           global data                             *
 *                                                                   *
 *********************************************************************/

// global state for server, use unix socket struct as addr field for conn

int n_conns;
struct conn conns[MAX_NUM_CONNS];

int server_fd;
struct sockaddr server;
socklen_t server_len;

/*********************************************************************
 *                                                                   *
 *                         initialize data                           *
 *                                                                   *
 *********************************************************************/

/***************
 * server_init *
 ***************/

/* initialize (global) server context */

void
server_init()
{
    n_conns = 0;
    memset(conns, 0, MAX_NUM_CONNS * sizeof(struct conn));
    server_fd = 0;
    memset(&server, 0, sizeof(struct sockaddr));
    server_len = 0;
}

/**************
 * server_gai *
 **************/

void
server_gai()
{
    int status;
    struct addrinfo hints, *res, *p;
    
    memset(&hints, 0, sizeof(struct addrinfo)); 
    hints.ai_family = AF_INET;     
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, PORT, &hints, &res);

    if (status < 0) {
        fprintf(stderr, "[ERROR] getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    /* take the very last matching addrinfo */

    for (p = res; p != NULL; p = p->ai_next) {
        
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_fd == -1)
            continue;
        
        status = bind(server_fd, p->ai_addr, p->ai_addrlen);
        if (status < 0) {
            close(server_fd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        freeaddrinfo(res);
        fprintf(stderr, "[ERROR]  Could not find matching address!\n");
        exit(EXIT_FAILURE);
    }

    memcpy(&server, p->ai_addr, p->ai_addrlen);
    server_len = p->ai_addrlen;

    freeaddrinfo(res);

    printf("[SERVER] Starting ... OK\n");
}

/*************
 * conn_init *
 *************/

/* initialize connection data */

void
conn_init(struct conn* conn, int fd, struct sockaddr* addr, socklen_t addrlen) {
    conn->thr = 0;
    conn->fd = fd;
    memcpy(&conn->addr, addr, addrlen);
    conn->addrlen = addrlen;
}

/*********************************************************************
 *                                                                   *
 *                       concurrent functions                        *
 *                                                                   *
 *********************************************************************/

/***************
 * handle_conn *
 ***************/

/* handles the connection in a loop */

void*
handle_conn(void* arg)
{
    int n_bytes, status;
    struct conn* conn;
    char buf[MAX_BUF_LEN];
    struct request req;
    struct response resp;

    conn = arg;

    while (1) {
        char* msg;
        int msg_len;

        n_bytes = recv(conn->fd, buf, MAX_BUF_LEN, 0);
        
        if (n_bytes == 0) {
            /* client connection ended */
            return NULL;
        }

        if (n_bytes < 0) {
            fprintf(stderr, "[ERROR] client %d, recv: %s\n", 
                    conn->fd, strerror(errno));
            return (void*)EXIT_FAILURE;
        }

        /* ASSUMING n_bytes < MAX_BUF_LEN */
        buf[n_bytes] = 0;

        printf("\nrecieved from client %d:\n%s\n\n", conn->fd, buf);

        /* create request*/
        request_init(&req);
        status = parse_request(&req, buf);
        
        /* create a response */

        response_init(&resp);

        if (status == 0) {
            
            if (req.method == POST) {
                char *html, *res;
                html = view[0].file.data;
                handle_post(&req, html, &res);
                free(html);
                view[0].file.data = res;
                printf("result is: %s\n", res);
            }
            
            route_response(&resp, &req);
        } else {
            route_error(&resp, status);           
        }
        
        /* serialize response into text */
        make_response(&resp, &msg, &msg_len);

        n_bytes = send(conn->fd, msg, msg_len, 0);

        if (n_bytes < 0) {
            fprintf(stderr, "[ERROR] client %d, send: %s\n", 
                    conn->fd, strerror(errno));
            return (void*)EXIT_FAILURE;
        }

        free(msg);
    }   
}


/*********************************************************************
 *                                                                   *
 *                               main                                *
 *                                                                   *
 *********************************************************************/

/********
 * main *
 ********/

int
main()
{
    int status, conn_fd;
    struct sockaddr_storage* client;
    socklen_t client_len;

    /* initialize data */

    server_init();
    status = view_init();
    if (status < 0) {
        fprintf(stderr, "[ERROR] view_init");
        exit(EXIT_FAILURE);
    }
    server_gai();

    client_len = 0;
    client = NULL;

    /* listen */

    status = listen(server_fd, SOMAXCONN);

    if (status < 0) {
        close(server_fd);
        fprintf(stderr, "[ERROR] listen: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] listening ... OK\n");

    /* connection accept loop */

    while (1) {
        struct conn* conn;

        if (n_conns >= MAX_NUM_CONNS)
            break;

        conn_fd = accept(server_fd, (struct sockaddr*)&client, &server_len);

        if (conn_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;

            fprintf(stderr, "[ERROR] accept: %s\n", strerror(errno));
            break;
        }

        printf("[SERVER] connected with a client!\n");

        conn = conns + n_conns;

        conn_init(conn, conn_fd, (struct sockaddr*)&client, client_len);
    
        status = pthread_create(&conn->thr, NULL, handle_conn, (void*)conn);

        if (status < 0) {
            fprintf(stderr, "[ERROR] pthread_create: %s\n", strerror(status));
            break;
        }

        n_conns++;
    }

    view_free();
    close(server_fd);
}