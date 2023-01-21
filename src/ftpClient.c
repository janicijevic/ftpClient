

/*
    TODO:
    !!! - get wsl ip (ipconfig vEthernet)  172.22.176.1                 

    - Separate response by lines (readline ??)
    - have a set of recognized commands and possible responses
    - state machine moving between with known responses and commands
    - Figure out states (sending recieving)
    - Sort out preset commands
    - give prompt to change server and port (default)

    PROMPT
    - tab autocomplete
    - arrow keys for history
    - reject command if not one of recognized commands

*/
// ======= My supported commands
// pwd      ls      cd      mkdir
// delete   put     get     lcd
// open     close   user    pass
// exit     help



#define COMMANDNUM 14
char* commands[COMMANDNUM] = {
    "pwd",      "ls",       "cd",       "mkdir",
    "delete",   "put",      "get",      "lcd",
    "open",     "close",    "user",     "pass",
    "exit",     "help"
};


#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "socketManager.h"


// Test telnet server
// #define SERVERNAME "clients.us.code42.com"      // Server of connection if client, ex: "www.google.com" or "127.43.55.1"
// #define MYPORT "4287"                           // PORT of connection

#define SERVERNAME "172.29.240.1"    // Server of connection of client, ex: "www.google.com" or "127.43.55.1"
#define MYPORT "21"                // PORT of connection


/*
Returns index of command cmd in array of commands, returns -1 if not found
*/
int getCommandIndex(char cmd[]){

    for(int i = 0; i<COMMANDNUM; i++){
        if(strcmp(cmd, commands[i]) == 0){
            return i;
        }
    }

    return -1;
}

/*
Connects and communicates on the data connection using ip and port
Returns 0 if successful, < 0 if error
*/
int dataConnection(char ip[], char port[]){

    int sockfd = sockInit(ip, port, CONNECT);
    if(sockfd < 0){
        fprintf(stderr, "error: socketInit\n");
        return -1;
    }

    // Message recieve buffer
    char recvBuf[MAXDATASIZE];
    recvBuf[MAXDATASIZE-1] = '\0';

    // Message send buffer
    size_t size;
    char sendBuf[MAXDATASIZE];
    sendBuf[MAXDATASIZE-1] = '\0';      // Terminate the buffer


    while(1){
        int numbytes;
        if((numbytes = recv(sockfd, recvBuf, MAXDATASIZE-1, 0)) == -1){
            perror("recv");
            return -2;
        }
        if(numbytes == 0){
            printf("Client closed connection\n");
            close(sockfd);
            return 0;          // End of message
        }
        
        recvBuf[numbytes] = '\0';   // Terminate buffer

        printf("client: received \n'%s'\n",recvBuf);
    }

    close(sockfd);
    return 0;
}



/* 
Extracts ip and port from msg
Arguments:
    msg - string containing an ftp response to PASV command of the format:
          227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
          where h1.h2.h3.h4 is the ip address and p1*256+p2 is the port to connect to
    size - size of msg buffer
    ip - buffer to write extracted ip to        (at least 16 bytes)
    port - buffer to write extracted port to    (at least 6 bytes)
Returns:
    0 if successful, < 0 if an error occured
*/
int getIPandPort(char* msg, size_t size, char ip[], char port[]){
   
    int h1, h2, h3, h4, p1, p2;

    msg = strchr(msg, '(');     // Move to start of parentheses
    if(msg == NULL){
        return -1;
    }

    int scanret = sscanf(msg, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    if (scanret == 0 || scanret == EOF){
        return -2;
    }
    int p = p1*256 + p2;        // Calculate port
    if(p < 0 || p > 65535){     // Maximum tcp port is 65535
        return -3;
    }

    if(sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4) < 0) { return -4; }
    if(sprintf(port, "%d", p) < 0) { return -5; }

    return 0;
}


#include <sys/prctl.h>
#include <signal.h>


/*
Parses ftp response message and acts accordingly
Arguments:
    msg - string containing an ftp response
    size - size of msg buffer
*/
void handleReply(char* msg, size_t size){

    char reply[4];
    memcpy(reply, msg, 3);  // extract reply code
    reply[3] = '\0';


    printf("%s: ", reply);
    switch(reply[0]){
        case '1':
            printf("Positive Preliminary reply\n");
            break;
        case '2':
            printf("Positive Completion reply\n");
            break;
        case '3':
            printf("Positive Intermediate reply\n");
            break;
        case '4':
            printf("Transient Negative Completion reply\n");
            break;
        case '5':
            printf("Permanent Negative Completion reply\n");
            break;
        case '6':
            printf("Protected reply\n");
            break;
        default:
            printf("Reply code not recognized\n");
            return;
    }

    int code = atoi(reply); 

    // 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
    if(code == 227){
        char ip[16], port[6];
        
        int ipret = getIPandPort(msg, size, ip, port);  // Extract ip and port from 
        
        if(ipret < 0){
            printf("error extracting ip and port\n");
            return;
        }

        pid_t ppid = getpid();
        pid_t dpid = fork();
        if(dpid < 0){                  // !!!!!! REPLACE WITH <
            perror("data fork");
        }
        if(dpid == 0){
            // Set child to get signal upon parent death
            if(prctl(PR_SET_PDEATHSIG, SIGHUP) < 0) {perror("prctl"); exit(1);};
            // If parent got killed before calling prctl
            if (getppid() != ppid)
                exit(1);

            int dataret = dataConnection(ip, port);
            if(dataret < 0){
                printf("error in dataConnection\n");
                exit(1);
            }
            exit(0);
        }
        return;
    }



}

void controlConnection(char ip[], char port[]){

    int sockfd = sockInit(ip, port, CONNECT);
    if(sockfd < 0){
        fprintf(stderr, "error: socketInit\n");
        exit(1);
    }

    // Message recieve buffer
    char recvBuf[MAXDATASIZE];
    recvBuf[MAXDATASIZE-1] = '\0';

    // Message send buffer
    size_t size;
    char sendBuf[MAXDATASIZE];
    sendBuf[MAXDATASIZE-1] = '\0';


    while(1){        
        int numbytes;
        if((numbytes = recv(sockfd, recvBuf, MAXDATASIZE-1, 0)) == -1){
            perror("recv");
            exit(1);
        }
        if(numbytes == 0){
            printf("Client closed connection\n");
            close(sockfd);
            exit(1);
        }
        
        recvBuf[numbytes] = '\0';   // Terminate buffer

        printf("client: received \n'%s'\n",recvBuf);


        handleReply(recvBuf, numbytes);


        size = MAXDATASIZE-1;
        char* msgptr = (char*)sendBuf;
        getline(&msgptr, &size, stdin);
        size = strlen(sendBuf);
        sendBuf[size-1] = '\r';
        sendBuf[size] = '\n';           // Replace \n with \r\n
        sendBuf[size+1] = '\0';         // Terminate string
        size = size+1;
        // sendBuf is the captured message from command line

        printf("Sending: \ns'%s'\n", sendBuf);
        if (send(sockfd, sendBuf, strlen(sendBuf), 0) == -1)
            perror("send");


    }

    close(sockfd);

}

int main(){

    controlConnection(SERVERNAME, MYPORT);

    return 0;
}

