
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "csapp.h"
#include "sbuf.h"
#include "logbuf.h"
#define NTHREADS  20
#define SBUFSIZE  15

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct entry {
    char* key;
    char* value;
    int size;
    struct entry *next;
} cache;

void *logThread(void *vargp);
void *thread(void *vargp);

sbuf_t sbuf; /* Shared buffer of connected descriptors */
logbuf_t logbuf; /* Shared buffer of things to log */
cache * head;
sem_t cache_mutex; // initialized to 1; protects cache
sem_t readers_mutex; // initialized to 1; protects readerscount
int readers_count; // initialized to 0
int sizeOfCache;

void sigint_handler(int signum) {
    // Free all allocated memory
    cache *entry = head;
    cache *entryToFree;
    while (entry->key != NULL) {
        free(entry->key);
        free(entry->value);
        entryToFree = entry;
        entry = entry->next;
        free(entryToFree);
    }
    free(entry);
    sbuf_deinit(&sbuf);
    logbuf_deinit(&logbuf, SBUFSIZE);

    exit(0);
}

int main(int argc, char *argv[]){
    signal(SIGINT, sigint_handler);
    head = malloc(sizeof(cache));
    sem_init(&cache_mutex, 0, 1);
    sem_init(&readers_mutex, 0, 1);
    readers_count = 0;
    sizeOfCache = 0;

    ////////// Read in request
    struct sockaddr_in ip4addr;
    int sfd;

    // Ignore SISPIPE signal
    signal(SIGPIPE, SIG_IGN);

    //create socket
    ip4addr.sin_family = AF_INET;
    printf("port: %s\n", argv[1]);
    ip4addr.sin_port = htons(atoi(argv[1]));
    ip4addr.sin_addr.s_addr = INADDR_ANY;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    //bind it
    if (bind(sfd, (struct sockaddr *)&ip4addr, sizeof(struct sockaddr_in)) < 0) {
        close(sfd);
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    //listen
    if (listen(sfd, 100) < 0) {
        close(sfd);
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    //init buffers
    int cfd, i;
    pthread_t tid;
    logbuf_init(&logbuf, SBUFSIZE);
    sbuf_init(&sbuf, SBUFSIZE); 

    // Create logger thread
    pthread_create(&tid, NULL, logThread, NULL);

    // Create proxy threads
    for (i = 0; i < NTHREADS; i++)  
	    pthread_create(&tid, NULL, thread, NULL);    

    while(1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len;

        cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
        sbuf_insert(&sbuf, cfd); /* Insert cfd in buffer */
    }

    return 0;
}


void *logThread(void *vargp) {
    pthread_detach(pthread_self());

    /* File pointer to hold reference of input file */
    FILE *fPtr;

    // Have thread loop and write when it has logs available
    while(1) {
        // Collect log
        char* log = logbuf_remove(&logbuf);
        printf("logger thread: %s\n\n", log);

        /* Append log to file */
        fPtr = fopen("logFile.txt", "a");
        if (fPtr == NULL) {
            /* Unable to open file hence exit */
            printf("\nUnable to open file.\n");
            printf("Please check whether file exists and you have write privilege.\n");
            exit(EXIT_FAILURE);
        }
        fputs(log, fPtr);
        fputs("\n", fPtr);
        fclose(fPtr);
        free(log);
    }

}


void *thread(void *vargp) {
    pthread_detach(pthread_self());
    int cfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */

    int nread = 0;
    int total_bytes_read = 0;
    char buf[MAX_OBJECT_SIZE];
    memset(buf, 0, MAX_OBJECT_SIZE);

    while(1) {
        nread = read(cfd, buf + total_bytes_read, MAX_OBJECT_SIZE);
        total_bytes_read += nread;
        
        if (nread == 0) //if end of request, exit loop
            break;  

        if (total_bytes_read < 4) // continue if more data to read
            continue;

        if (!memcmp(buf + total_bytes_read - 4, "\r\n\r\n", 4)) {
            break;
        }
        
    }

    char request[4000];


    ////////////// Extract URL

    // This extracts 'GET' and copies it to the request we're building
    char host[400];
    char directory[400];
    char port[40];
    char *token = strtok(buf, " ");
    strcpy(request, token);
    // extract url
    char *url = calloc(400, sizeof(char));
    char* preURL = strtok(NULL, " ");
    strcpy(url, preURL);

    //write to buffer to write resp to client
    char *outputBuffer = calloc(MAX_OBJECT_SIZE, sizeof(char));
    int bytes_read = 0;
    int cacheHit = 0;


    /////////////// Read from cache

    sem_wait( &readers_mutex );
    if( readers_count == 0 ) {
        sem_wait( &cache_mutex );
    }
    ++readers_count;
    sem_post( &readers_mutex );

    // Check if the url is in the cache
    cache * entry = head;
    while (entry->key != NULL) {
        if (!strcmp(entry->key, url)) {
            bytes_read = entry->size;
            memcpy(outputBuffer, entry->value, bytes_read);
            cacheHit = 1;
            break;
        }
        entry = entry->next;
    }

    sem_wait( &readers_mutex );
    --readers_count;
    if( readers_count == 0 ) {
        sem_post( &cache_mutex );
    }
    sem_post( &readers_mutex );

    if(cacheHit) {
        goto forwardResponse; //TODO CHANGE TO FUNCTION
    }


    //////////// Continue parsing and build proxy request

    char strToParse[800];
    strcpy(strToParse, url);
    char* rest = strToParse;
    // If the website starts with https://, strtok it again
    // to get the host
    token = strtok_r(rest, "/", &rest);
    rest = rest + 1;
    if(strstr(token, "http:") != NULL) {
        token = strtok_r(rest, " ", &rest); 
    }

    char firstAttempt[400];
    char plainHost[400];
    strcpy(firstAttempt, token);
    rest = firstAttempt;
    if (strstr(token, ":")) {
        token = strtok_r(rest, "/", &rest);
        strcpy(host, token);
        token = strtok_r(rest, "\0", &rest);
        strcpy(directory, token);

        char hostToParse[400];
        strcpy(hostToParse, host);
        rest = hostToParse;
        token = strtok_r(rest, ":", &rest);
        strcpy(plainHost, token);
        strcpy(port, rest);
    }
    else {
        token = strtok_r(rest, "/", &rest);
        strcpy(host, token);
        strcpy(directory, rest);
        strcpy(port, "80");
    }

    if (port[strlen(port) - 1] == '/') {
        port[strlen(port) - 1] = '\0';
    }

    // Add the directory to the request we're building
    int length = strlen(request);
    request[length] = ' ';
    request[length + 1] = '/';
    strcpy(request + length + 2, directory);
    // Add 'HTTP/1.0' to the request
    length = strlen(request);
    request[length] = ' ';
    strcpy(request + length + 1, "HTTP/1.0\r\n");
    // Find the host line if it exists
    token = strtok(NULL, "Host:");
    if (token != NULL) {
        token = strtok(NULL, " ");
        token = strtok(NULL, "\r\n");
        strcpy(host, token);
    }
    // Add host name to the request
    strcpy(request + strlen(request), "Host: ");
    strcpy(request + strlen(request), host);
    strcpy(request + strlen(request), "\r\n");
    // Add the other things to the request
    strcpy(request + strlen(request), user_agent_hdr);
    strcpy(request + strlen(request), "Connection: close\r\n");
    strcpy(request + strlen(request), "Proxy-Connection: close\r\n");

    // Add on any additional request headers
    while ((token = strtok(NULL, "\n")) != NULL) {
        // Don't add any of these
        if (strstr(token, "Proxy-Connection:") != NULL
            || strstr(token, "Connection:") != NULL)
            continue;
            
        strcpy(request + strlen(request), token);
        strcpy(request + strlen(request), "\n");
    }
    // Terminate request
    strcpy(request + strlen(request), "\r\n");


    //////////////// Forward request to server

    struct addrinfo aInfo;
    struct addrinfo *result, *rp;
    int s, fd;

    /* Obtain address(es) matching host/port */
    memset(&aInfo, 0, sizeof(struct addrinfo));
    aInfo.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    aInfo.ai_socktype = SOCK_STREAM; /* TCP socket */
    aInfo.ai_flags = 0;
    aInfo.ai_protocol = 0; /* Any protocol */
    if (port == NULL || strstr(port, " ") != NULL) {
        strcpy(port, "80");
    }

    // If the host has been changed to have the port appended to it,
    // change it back to how it is without the port appended to it
    if (plainHost != NULL && strstr(plainHost, " ") == NULL) {
        strcpy(host, plainHost);
    }
    s = getaddrinfo(host, port, &aInfo, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    /* getaddrinfo() returns a list of address structures.
    Try each address until we successfully connect(2).
    If socket(2) (or connect(2)) fails, we (close the socket
    and) try the next address. */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1)
            continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */
        close(fd);
    }
    if (rp == NULL) { /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
    }
    freeaddrinfo(result); /* No longer needed */


    // Write to server and read back response
    bytes_read = strlen(request);
    int bytes_sent = 0;
    int current_bytes_received = 0;
    do {
        current_bytes_received = write(fd, request + bytes_sent, bytes_read - bytes_sent);
        if (current_bytes_received < 0) {
            fprintf(stderr, "write error to server\n");
            return NULL;
        }
        bytes_sent += current_bytes_received;

    } while( bytes_sent < bytes_read );

    bytes_read = 0;

    do {
        nread = read(fd, outputBuffer + bytes_read, MAX_OBJECT_SIZE);
        if (nread == -1) {
            perror("read");
            return NULL;
        }
        bytes_read += nread;
    } while( nread != 0 );


    ///////////////// Add response to cache (write)

    sem_wait( &cache_mutex );

    // access and modify cache
    cache * newHead = NULL;
    newHead = malloc(sizeof(cache));
    newHead->key = calloc(400, sizeof(char));
    newHead->value = calloc(MAX_OBJECT_SIZE, sizeof(char));
    strcpy(newHead->key, url);
    memcpy(newHead->value, outputBuffer, bytes_read);
    newHead->size = bytes_read;
    for (int i = 0; i < bytes_read; i++) {
        printf("%c", newHead->value[i]);
    }
        
    newHead->next = head;
    head = newHead;

    sem_post( &cache_mutex );


    //////////// Forward response back to client
    forwardResponse: //TODO: pull into function
    
    bytes_sent = 0;
    current_bytes_received = 0;
    do {
        current_bytes_received = write(cfd, outputBuffer + bytes_sent, bytes_read - bytes_sent);
        if (current_bytes_received < 0) {
            fprintf(stderr, "write error to client\n");
            return NULL;
        }
        bytes_sent += current_bytes_received;
    } while( bytes_sent < bytes_read );


    // Insert url and response to log file
    logbuf_insert(&logbuf, url);

    close(fd);
    close(cfd);
    free(outputBuffer);
    return 0;
}

