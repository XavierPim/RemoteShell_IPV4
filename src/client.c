
#include "../include/client.h"

void start_client(const char *address, uint16_t port)
{
    int                client_socket;
    struct sockaddr_in server_addr;
    int                flags;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    flags = fcntl(client_socket, F_GETFD);
    if(flags == -1)
    {
        close(client_socket);
        perror("Error getting flags on socket");
        exit(EXIT_FAILURE);
    }

    flags |= FD_CLOEXEC;
    if(fcntl(client_socket, F_SETFD, flags) == -1)
    {
        close(client_socket);
        perror("Error setting FD_CLOEXEC on socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port        = htons(port);

    if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server. Type your messages and press Enter to send.\n");

    while(1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if(select(client_socket + 1, &readfds, NULL, NULL, NULL) == -1)
        {
            perror("Select error");
            break;
        }

        if(FD_ISSET(client_socket, &readfds))
        {
            char    buffer[BUFFER_SIZE];
            ssize_t bytes_received;
            bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if(bytes_received <= 0)
            {
                printf("Server closed the connection.\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("Received: %s", buffer);
        }

        if(FD_ISSET(STDIN_FILENO, &readfds))
        {
            char buffer[BUFFER_SIZE];
            if(fgets(buffer, sizeof(buffer), stdin) == NULL)
            {
                printf("EOF detected. Closing connection.\n");
                break;
            }
            if(send(client_socket, buffer, strlen(buffer), 0) == -1)
            {
                perror("Error sending message");
                break;
            }
        }
    }

    close(client_socket);
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

    port = strtol(argv[2], &endptr, TENNER);    // Use base 10 for decimal numbers
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

    start_client(argv[1], (uint16_t)port);
    return 0;
}
