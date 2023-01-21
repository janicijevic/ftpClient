/*
    Functions to help initialize and connect sockets

*/


#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define MAXDATASIZE BUFSIZ  // How many bytes we can read at once


// Flags for socketInit
#define CONNECT     0x1     // socketInit connects the socket to specified host
#define BIND        0x2     // socketInit binds socket to specified port

#define ALLFLAGS    (CONNECT|BIND)


void *get_in_addr(struct sockaddr *sa);

int sockInit(char* serverName, char* port, int flags);