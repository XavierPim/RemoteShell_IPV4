#include "../include/server.h"
#include <wait.h>

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

    pid_t pid = fork();
    if(pid == -1)
    {
        perror("fork failed");
        close(client_socket);
        clients[client_info->client_index] = 0;
        free(client_info);
        return NULL;
    }
    if(pid == 0)
    {
        // Child process
        while(1)
        {
            char    buffer[BUFFER_SIZE];
            ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if(bytes_received <= 0)
            {
                printf("Client %d disconnected.\n", client_info->client_index);
                break;
            }
            buffer[bytes_received] = '\0';
            printf("Received command from Client %d: %s\n", client_info->client_index, buffer);

            // Execute the command
            executor(buffer);
        }

        close(client_socket);
        exit(0);    // Terminate the child process
    }
    else
    {
        // Parent process
        close(client_socket);    // Close the client socket in the parent process
        clients[client_info->client_index] = 0;
        free(client_info);
        wait(NULL);    // Optionally wait for the child process to finish
        return NULL;
    }
}

// Function to start the server
void start_server(const char *address, uint16_t port)
{
    int                server_socket;
    int                client_socket;
    fd_set             readfds;
    struct sockaddr_in client_addr;

    server_socket = create_server_socket(address, port);
    printf("Server listening on %s:%d\n", address, port);

    while(!exit_flag)
    {
        int       activity;
        int       max_sd = server_socket;
        pthread_t tid;

        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        for(int i = 0; i < MAX_CLIENTS; ++i)
        {
            if(clients[i] > 0)
            {
                FD_SET(clients[i], &readfds);
                if(clients[i] > max_sd)
                {
                    max_sd = clients[i];
                }
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if(activity < 0 && !exit_flag)
        {
            perror("select failed");
            break;
        }

        if(FD_ISSET(server_socket, &readfds))
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

            // Initialize the client_info structure
            client_info->client_socket = client_socket;
            client_info->client_index  = client_index;

            if(pthread_create(&tid, NULL, handle_client, (void *)client_info) != 0)
            {
                perror("Thread creation failed");
                close(client_socket);
                free(client_info);
                continue;
            }

            pthread_detach(tid);
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

    // Close the server socket
    close(server_socket);

    printf("Server shutdown complete.\n");
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

void executor(char *command)
{
    char  *token;
    char  *saveptr;    // For strtok_r
    char **commandArgs;
    int    arg_count;

    commandArgs = (char **)malloc(sizeof(char *) * SIXTYFO);
    if(commandArgs == NULL)
    {
        perror("malloc");
        exit(1);
    }
    arg_count = 0;

    // Split the command and following options into tokens using strtok_r.
    token = strtok_r(command, " ", &saveptr);
    while(token != NULL)
    {
        commandArgs[arg_count] = strdup(token);
        if(commandArgs[arg_count] == NULL)
        {
            perror("strdup");
            // Free previously allocated memory before exiting
            for(int i = 0; i < arg_count; i++)
            {
                free(commandArgs[i]);
            }
            free(commandArgs);
            exit(1);
        }
        arg_count++;
        token = strtok_r(NULL, " ", &saveptr);
    }
    commandArgs[arg_count] = NULL;    // Null-terminate the array.

    // Execute the command
        execvp(commandArgs[0], commandArgs);
    printf("commandArgs in executor %s\n", commandArgs[0]);
    // If execvp returns, it must have failed.
        perror("execvp");

    // Free allocated memory
    for(int i = 0; i < arg_count; i++)
    {
        free(commandArgs[i]);
    }
    free(commandArgs);
    //    _exit(1);    // Use _exit in child to prevent flushing stdio buffers
}
