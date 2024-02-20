#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define TENNER 10
#define EPOLL_MAX_EVENTS 10
#define BUFFER_SIZE 1024

struct ClientInfo
{
    int client_socket;
    int client_index;
    int clients[MAX_CLIENTS];
};

void  start_server(const char *address, uint16_t port);
void *handle_client(void *arg);
void  setup_signal_handler(void);
int   create_server_socket(const char *address, uint16_t port);
int   accept_client(int server_socket, struct sockaddr_in *client_addr);

#endif /* SERVER_H */
