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



#define MAX_CONNS   12
#define PORT        "8080"

/*********************************************************************
 *                                                                   *
 *                        struct definitions                         *
 *                                                                   *
 *********************************************************************/

struct conn {
    struct sockaddr addr;
    int fd, addrlen;
    /* immutable address & port */
};

/*********************************************************************
 *                                                                   *
 *                           global data                             *
 *                                                                   *
 *********************************************************************/

// global state for server, use unix socket struct as addr field for conn

int n_conns;
struct conn conns[MAX_CONNS];

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
    memset(conns, 0, MAX_CONNS * sizeof(struct conn));
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

    /* take the very last matching sockhintsaddr */

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
    conn->fd = fd;
    memcpy(&conn->addr, addr, addrlen);
    conn->addrlen = addrlen;
}

/*********************************************************************
 *                                                                   *
 *                               main                                *
 *                                                                   *
 *********************************************************************/

/********
 * main *
 ********/

/* ./server <ip-addr> <port> */

int
main()
{
    int status, conn_fd;
    struct sockaddr_storage* client;
    socklen_t client_len;

    server_init();
    server_gai();

    client_len = 0;
    client = NULL;

    status = listen(server_fd, SOMAXCONN);

    if (status < 0) {
        close(server_fd);
        fprintf(stderr, "[ERROR] listen: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] listening ... OK\n");

    while (1) {
        conn_fd = accept(server_fd, (struct sockaddr*)&client, &server_len);

        if (conn_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;

            close(server_fd);
            fprintf(stderr, "[ERROR] accept: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        printf("connected with a client!\n");

        conn_init(conns + n_conns, conn_fd, (struct sockaddr*)&client, client_len);
    }
}