#include "../include/server.h"

// Global array to keep track of client sockets
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int clients[MAX_CLIENTS] = {0};

// Global flag to indicate when to exit the server loop
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static volatile sig_atomic_t exit_flag = 0;

// Signal handler for SIGINT
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static void sigint_handler(int signum)
{
    (void)signum;
    exit_flag = 1;
}

// Set up the signal handler for SIGINT
void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
    sa.sa_handler = sigint_handler;
#pragma clang diagnostic pop
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

// Function to handle communication with a client
void *handle_client(void *arg)
{
    struct ClientInfo *client_info   = (struct ClientInfo *)arg;
    int                client_socket = client_info->client_socket;
    int                client_index  = client_info->client_index;
    char               buffer[BUFFER_SIZE];

    while(1)
    {
        ssize_t bytes_received;
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if(bytes_received <= 0)
        {
            printf("Client %d disconnected.\n", client_index);
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Received from Client %d: %s", client_index, buffer);
        send(client_socket, buffer, (size_t)bytes_received, 0);
    }

    close(client_socket);
    clients[client_index] = 0;
    free(client_info);
    return NULL;
}

// Function to start the server
void start_server(const char *address, uint16_t port)
{
    int                server_socket;
    int                client_socket;
    int                epoll_fd;
    struct epoll_event event;
    struct epoll_event events[EPOLL_MAX_EVENTS];

    // Set up the signal handler for SIGINT
    setup_signal_handler();

    server_socket = create_server_socket(address, port);
    printf("Server listening on %s:%d\n", address, port);

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if(epoll_fd == -1)
    {
        perror("epoll_create1 failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    event.events  = EPOLLIN;
    event.data.fd = server_socket;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1)
    {
        perror("epoll_ctl failed");
        close(server_socket);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while(!exit_flag)
    {
        int num_events = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, -1);
        if(num_events == -1)
        {
            perror("epoll_wait failed");
            close(server_socket);
            close(epoll_fd);
            exit(EXIT_FAILURE);    //    struct sockaddr_in client_addr;
        }

        for(int i = 0; i < num_events; ++i)
        {
            struct sockaddr_in client_addr;
            socklen_t          client_len;
            pthread_t          tid;
            client_len = sizeof(client_addr);
            memset(&client_addr, 0, client_len);    // Initialize the entire struct to zero
            client_addr.sin_family = AF_INET;       // Set the address family

            if(events[i].data.fd == server_socket)
            {
                struct ClientInfo *client_info;
                int                client_index;
                client_socket = accept_client(server_socket, &client_addr);
                if(client_socket == -1)
                {
                    continue;
                }

                printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                client_index = -1;
                for(int j = 0; j < MAX_CLIENTS; ++j)
                {
                    if(clients[j] == 0)
                    {
                        client_index = j;
                        clients[j]   = client_socket;
                        break;
                    }
                }

                if(client_index == -1)
                {
                    fprintf(stderr, "Too many clients. Connection rejected.\n");
                    close(client_socket);
                    continue;
                }

                client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
                if(client_info == NULL)
                {
                    perror("Memory allocation failed");
                    close(client_socket);
                    continue;
                }

                client_info->client_socket = client_socket;
                client_info->client_index  = client_index;
                memcpy(client_info->clients, clients, sizeof(clients));

                if(pthread_create(&tid, NULL, handle_client, (void *)client_info) != 0)
                {
                    perror("Thread creation failed");
                    close(client_socket);
                    free(client_info);
                    continue;
                }

                pthread_detach(tid);

                event.events  = EPOLLIN;
                event.data.fd = client_socket;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
                {
                    perror("epoll_ctl failed");
                    close(client_socket);
                    continue;
                }
            }
            else
            {
                // Handle data from client
            }
        }
    }

    // Close all client sockets before shutting down
    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(clients[i] != 0)
        {
            close(clients[i]);
        }
    }

    // Close the server socket and epoll file descriptor
    close(server_socket);
    close(epoll_fd);

    printf("Server shutdown complete.\n");
    // No need for exit(EXIT_SUCCESS) or exit(EXIT_FAILURE) here
}

int main(int argc, const char *argv[])
{
    char *endptr;
    long  port;
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port = strtol(argv[2], &endptr, TENNER);
    if(*endptr != '\0')
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    if(port < 0 || port > UINT16_MAX)
    {
        fprintf(stderr, "Port number out of range: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    setup_signal_handler();
    start_server(argv[1], (uint16_t)port);
    return 0;
}

int create_server_socket(const char *address, uint16_t port)
{
    int                server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port        = htons(port);

    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket, MAX_CLIENTS) == -1)
    {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

int accept_client(int server_socket, struct sockaddr_in *client_addr)
{
    int       client_socket;
    socklen_t client_len = sizeof(*client_addr);

    client_socket = accept(server_socket, (struct sockaddr *)client_addr, &client_len);
    if(client_socket == -1)
    {
        perror("Accept failed");
        return -1;
    }

    return client_socket;
}
