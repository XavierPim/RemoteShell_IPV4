#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <ndbm.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static void      setup_signal_handler(void);
static void      sigint_handler(int signum);
static void      parse_arguments(int argc, char *argv[], char **ip_address, char **port);
static void      handle_arguments(const char *ip_address, const char *port_str, in_port_t *port);
static in_port_t parse_in_port_t(const char *port_str);
static void      convert_address(const char *address, struct sockaddr_storage *addr);
static int       socket_create(int domain, int type, int protocol);
static void      socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
static void      start_listening(int server_fd, int backlog);
static int       socket_accept_connection(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len);
static void      socket_close(int sockfd);

#define BASE_TEN 10
// #define BUFFER 2048
#define MAX_CLIENTS 10
#define OPTIONS 0666
#define DB_NAME "data.db"
// #define AVG 10
//  #define AGENT 15
//   #define STORAGE 1000

// init the mutexes
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)
static pthread_mutex_t read_mutex = PTHREAD_MUTEX_INITIALIZER;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs
{
    int client_socket;
};

typedef struct
{
    pthread_t *threads;
    int        size;
} ThreadPool;

void thread_pool_init(ThreadPool *pool, int size);
void thread_pool_destroy(ThreadPool *pool);
void thread_pool_enqueue(ThreadPool *pool, void *(*func)(void *), void *arg);

static void *handle_connection(void *arg);

void *thread_function(void *arg);    // dummy

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static volatile sig_atomic_t exit_flag = 0;

int main(int argc, char *argv[])
{
    char                   *address;
    char                   *port_str;
    in_port_t               port;
    int                     backlog = BASE_TEN;
    int                     sockfd;
    struct sockaddr_storage addr;
    ThreadPool              thread_pool;

    address  = NULL;
    port_str = NULL;

    printf("--------------------------------------------\n");

    parse_arguments(argc, argv, &address, &port_str);
    handle_arguments(address, port_str, &port);
    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    socket_bind(sockfd, &addr, port);
    start_listening(sockfd, backlog);
    setup_signal_handler();

    thread_pool_init(&thread_pool, MAX_CLIENTS);

    while(!exit_flag)
    {
        int                     client_sockfd;
        struct sockaddr_storage client_addr;
        socklen_t               client_addr_len;
        struct ThreadArgs      *args;

        client_addr_len = sizeof(client_addr);
        client_sockfd   = socket_accept_connection(sockfd, &client_addr, &client_addr_len);

        args     = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
        args->db = db;

        if(client_sockfd == -1)
        {
            if(exit_flag)
            {
                free(args);
                break;
            }
        }

        if(exit_flag)
        {
            free(args);
            break;
        }

        args->client_socket = client_sockfd;

        thread_pool_enqueue(&thread_pool, handle_connection, args);

        free(args);
    }

    thread_pool_destroy(&thread_pool);

    shutdown(sockfd, SHUT_RDWR);

    socket_close(sockfd);

    dbm_close(db);

    return EXIT_SUCCESS;
}

static void parse_arguments(int argc, char *argv[], char **ip_address, char **port)
{
    if(argc == 3)
    {
        *ip_address = argv[1];
        *port       = argv[2];
    }
    else
    {
        printf("invalid num args\n");
        printf("usage: ./server [ip addr] [port]\n");
        exit(EXIT_FAILURE);
    }
}

static void handle_arguments(const char *ip_address, const char *port_str, in_port_t *port)
{
    if(ip_address == NULL)
    {
        printf("ip is null\n");
        exit(EXIT_FAILURE);
    }

    if(port_str == NULL)
    {
        printf("port str is null\n");
        exit(EXIT_FAILURE);
    }

    *port = parse_in_port_t(port_str);
}

in_port_t parse_in_port_t(const char *str)
{
    char     *endptr;
    uintmax_t parsed_value;

    errno        = 0;
    parsed_value = strtoumax(str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        perror("Error parsing in_port_t\n");
        exit(EXIT_FAILURE);
    }

    if(*endptr != '\0')
    {
        printf("non-numerics inside port\n");
        exit(EXIT_FAILURE);
    }

    if(parsed_value > UINT16_MAX)
    {
        printf("port out of range\n");
        exit(EXIT_FAILURE);
    }

    return (in_port_t)parsed_value;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void sigint_handler(int signum)
{
    exit_flag = 1;
}

#pragma GCC diagnostic pop

static void convert_address(const char *address, struct sockaddr_storage *addr)
{
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
    }
    else
    {
        fprintf(stderr, "%s is not an IPv4 or an IPv6 address\n", address);
        exit(EXIT_FAILURE);
    }
}

static int socket_create(int domain, int type, int protocol)
{
    int sockfd;
    int opt = 1;

    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        perror("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

static void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        fprintf(stderr,
                "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: "
                "%d\n",
                addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop\n");
        exit(EXIT_FAILURE);
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);
}

static void start_listening(int server_fd, int backlog)
{
    if(listen(server_fd, backlog) == -1)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Listening for incoming connections...\n");
}

static int socket_accept_connection(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len)
{
    int  client_fd;
    char client_host[NI_MAXHOST];
    char client_service[NI_MAXSERV];

    errno     = 0;
    client_fd = accept(server_fd, (struct sockaddr *)client_addr, client_addr_len);

    if(client_fd == -1)
    {
        if(errno != EINTR)
        {
            perror("accept failed\n");
        }

        return -1;
    }

    if(getnameinfo((struct sockaddr *)client_addr, *client_addr_len, client_host, NI_MAXHOST, client_service, NI_MAXSERV, 0) == 0)
    {
        printf("Received new request from -> %s:%s\n\n", client_host, client_service);
    }
    else
    {
        printf("Unable to get client information\n");
    }

    return client_fd;
}

static void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sigint_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void *handle_connection(void *arg)
{
    const struct ThreadArgs *args;
    int                      client_sockfd;

    args = (struct ThreadArgs *)arg;
    //
    client_sockfd = args->client_socket;

    printf("handling");
    pthread_mutex_lock(&read_mutex);

    pthread_mutex_unlock(&read_mutex);

    pthread_mutex_lock(&write_mutex);
    pthread_mutex_unlock(&write_mutex);

    socket_close(client_sockfd);

    return NULL;
}

#pragma GCC diagnostic pop

static void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }
}

void thread_pool_init(ThreadPool *pool, int size)
{
    pthread_attr_t thread_attr;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * (unsigned long)size);
    pool->size    = size;

    pthread_attr_init(&thread_attr);

    for(int i = 0; i < size; i++)
    {
        pthread_create(&(pool->threads[i]), NULL, thread_function, NULL);
    }
}

void thread_pool_destroy(ThreadPool *pool)
{
    for(int i = 0; i < pool->size; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
}

void thread_pool_enqueue(ThreadPool *pool, void *(*func)(void *), void *arg)
{
    for(int i = 0; i < pool->size; i++)
    {
        struct ThreadArgs *thread_args = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
        if(thread_args == NULL)
        {
            free(arg);
            perror("Error allocating memory for thread arguments");
            exit(EXIT_FAILURE);
        }
        memcpy(thread_args, arg, sizeof(struct ThreadArgs));

        if(pthread_create(&(pool->threads[i]), NULL, func, thread_args) == 0)
        {
            // Thread creation successful, return immediately
            return;
        }
    }

    // All threads busy, wait for one to finish and retry
    for(int i = 0; i < pool->size; i++)
    {
        struct ThreadArgs *thread_args;

        pthread_join(pool->threads[i], NULL);

        // Allocate new memory for each retry
        thread_args = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
        if(thread_args == NULL)
        {
            free(arg);
            perror("Error allocating memory for thread arguments");
            exit(EXIT_FAILURE);
        }
        memcpy(thread_args, arg, sizeof(struct ThreadArgs));

        if(pthread_create(&(pool->threads[i]), NULL, func, thread_args) == 0)
        {
            // Thread creation successful, return immediately
            return;
        }
    }
}

// DUMMY FUNCTION
void *thread_function(void *arg)
{
    (void)arg;
    // Your thread logic goes here
    return NULL;
}
