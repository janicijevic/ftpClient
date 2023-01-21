#include "socketManager.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>




void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int sockInit(char* serverName, char* port, int flags){

    if(flags <= 0 || flags > ALLFLAGS){
        printf("error: Invalid flags: %d", flags);
        return -1;
    }

    int sockfd = 0;                 // Socket descriptor 
    int status = 0;                 // For errors
    int yes = 1;                    // For reusing ports

    char s[INET6_ADDRSTRLEN];       // For ip address

    // Setup server info and get list of addrinfo structs
    struct addrinfo serverInfo;     // Server address information

    memset(&serverInfo, 0, sizeof(serverInfo)); // Clear serverInfo
    serverInfo.ai_family = AF_INET;             // IPv4
    serverInfo.ai_socktype = SOCK_STREAM;       // TCP
    serverInfo.ai_flags = AI_PASSIVE;           // Fill in IP

    struct addrinfo* head;                      // Head of linked list returned by getaddrinfo
    if((status = getaddrinfo(serverName, port, &serverInfo, &head) < 0)){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Iterate through list of addinfo structs and try to connect
    struct addrinfo* res;
    for(res = head; res != NULL; res = res->ai_next) {
        if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
            perror("socket");
            continue;
        }
       
        if(flags&BIND){
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                perror("setsockopt");
                exit(1);
            }
        
            if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
                close(sockfd);
                perror("bind");
                continue;
            }
        }
        if(flags&CONNECT){
            if(connect(sockfd, res->ai_addr, res->ai_addrlen) == -1){
                close(sockfd);
                perror("connect");
                continue;
            }
        }

        break;      // Success
    }
    
    if(res == NULL){
        fprintf(stderr, "Couldn't connect\n");
        exit(1);
    }

    inet_ntop(res->ai_family, get_in_addr((struct sockaddr *)res->ai_addr), s, sizeof s);

    freeaddrinfo(head); // Done with the list


    return sockfd;
}