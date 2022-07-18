#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define PORT_NUM 54789
#define PORT_STRING "54789"
#define BACKLOG 4

typedef struct serverData {
    struct sockaddr_in servaddr, clientaddr;
    int serverfd, clientfd;
    int len;
} serverdata;

typedef struct clientData {
    struct addrinfo hints, *actualdata;
    int serverfd;
} clientdata;

// sockets.c methods
void initServer(serverdata*);
void startServer(serverdata*);
int connectClient(clientdata*, char*, char*);

// clientcommands.c methods
void printHelp(int);
int handleServerResponse(int);
void rcd(int, char*);
void ls();
void dcConnect(clientdata*, int, char*);
void readDataConnection(clientdata*);
int crls(clientdata*);
int show(clientdata*, int, char*);
int get(clientdata*, int, char*);
int put(clientdata*, int, char*);

// servercommands.c methods
void acknowledge(serverdata*, char*);
void sendError(int, char*);
void swrite(int, char*, int);
int dataConnection(serverdata*);
int srls(serverdata*);
int sget(serverdata*, char*);
int sput(serverdata*, char*);

// miscfunctions.c methods
int getWordCount(char*);
void getWordList(char*, char**);
void ewrite(int, char*, int);
int copyLineRead(int, char*, int);
void execWithPipe(char**, char**);
void execWithSockets(char**, char**, int, int);