#include "mftp.h"

void initServer(serverdata *server) {
    server->len = sizeof(struct sockaddr_in);

    // creating a socket
    if((server->serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    // initializing servaddr
    memset(&server->servaddr, 0, sizeof(server->servaddr));
    server->servaddr.sin_family = AF_INET;
    server->servaddr.sin_port = htons(PORT_NUM);
    server->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // setsockopt
    if(setsockopt(server->serverfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    // bind
    if(bind(server->serverfd, (struct sockaddr*) &server->servaddr, sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }

    // listen
    listen(server->serverfd, BACKLOG);
}

void startServer(serverdata *server) {
    int processCount = 0;
    // server loop
    while(1) {
        // accepting client
        if((server->clientfd = accept(server->serverfd, (struct sockaddr*) &server->clientaddr, &server->len)) < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            return;
        }

        // getting host name
        char hostname[NI_MAXHOST];
        int hostentry;
        hostentry = getnameinfo((struct sockaddr*) &server->clientaddr, sizeof(server->clientaddr), hostname, sizeof(hostname), NULL, 0, NI_NUMERICSERV);
        if(hostentry != 0) {
            fprintf(stderr, "%s\n", gai_strerror(hostentry));
            return;
        }

        printf("Connection accepted from host %s\n", hostname);
        fflush(stdout);

        processCount++;

        // fork connections
        if(fork() == 0) {
            int numread;
            char buffer[120];
            char message[120];
            serverdata dc;
            int hasDataConnection = 0;

            while(copyLineRead(server->clientfd, message, 120) > 0) {
                // read client output into 'message' until newline
                message[strlen(message)-1] = '\0'; // remove newline from end of message
                switch(message[0]) {
                    case 'D':
                        if(dataConnection(&dc) < 0) {
                            fprintf(stderr, "Failed to establish data connection with error %s\n", strerror(errno));
                            sendError(server->clientfd, strerror(errno));
                            break;
                        }
                        else {
                            char portnum[6];
                            sprintf(portnum, "%d", ntohs(dc.servaddr.sin_port));
                            acknowledge(server, portnum);
                            if((dc.clientfd = accept(dc.serverfd, (struct sockaddr *) &dc.clientaddr, &dc.len)) < 0) {
                                fprintf(stderr, "Failed to establish data connection with error %s\n", strerror(errno));
                                sendError(server->clientfd, strerror(errno));
                                break;
                            }
                            hasDataConnection = 1;
                            printf("Data connection established on port %d.\n", ntohs(dc.servaddr.sin_port));
                            fflush(stdout);
                        }
                        break;

                    case 'C':
                        // change directory
                        if(chdir(&message[1]) != 0) {
                            fprintf(stderr, "cd to %s failed with error '%s'\n", &message[1], strerror(errno));
                            sendError(server->clientfd, strerror(errno));
                        }
                        else {
                            printf("Changed directory to %s\n", &message[1]);
                            fflush(stdout);
                            acknowledge(server, "");
                        }
                        break;

                    case 'L':
                        // server side ls command
                        if(!hasDataConnection) {
                            fprintf(stderr, "rls failed, no data connection available.\n");
                            sendError(server->clientfd, "No data connection availablen.\n");
                            break;
                        }
                        if(srls(&dc) < 0) {
                            fprintf(stderr, "rls failed with error %s\n", strerror(errno));
                            sendError(server->clientfd, strerror(errno));
                            close(dc.serverfd);
                            close(dc.clientfd);
                            hasDataConnection = 0;
                            break;
                        }
                        acknowledge(server, "");
                        // close data connection
                        close(dc.serverfd);
                        close(dc.clientfd);
                        hasDataConnection = 0;
                        printf("rls executed successfully\n");
                        break;

                    case 'G':
                        // server side of get command
                        if(!hasDataConnection) {
                            fprintf(stderr, "get failed, no dataConnection available.\n");
                            sendError(server->clientfd, "No data connection availablen.\n");
                            break;
                        }
                        if(sget(&dc, &message[1]) < 0) {
                            fprintf(stderr, "get failed with error %s\n", strerror(errno));
                            sendError(server->clientfd, strerror(errno));
                            close(dc.serverfd);
                            close(dc.clientfd);
                            hasDataConnection = 0;
                            break;
                        }
                        acknowledge(server, "");
                        close(dc.serverfd);
                        close(dc.clientfd);
                        hasDataConnection = 0;
                        printf("get on file %s executed successfully\n", &message[1]);
                        break;

                    case 'P':
                        // server side of put command
                        if(!hasDataConnection) {
                            fprintf(stderr, "put failed, no dataConnection available.\n");
                            sendError(server->clientfd, "No data connection availablen.\n");
                            break;
                        }
                        acknowledge(server, "");
                        if(sput(&dc, &message[1]) < 0) {
                            fprintf(stderr, "put failed with error %s\n", strerror(errno));
                            sendError(server->clientfd, strerror(errno));
                            close(dc.serverfd);
                            close(dc.clientfd);
                            hasDataConnection = 0;
                            break;
                        }
                        close(dc.serverfd);
                        close(dc.clientfd);
                        hasDataConnection = 0;
                        printf("put on file executed successfully\n", &message[1]);
                        fflush(stdout);
                        break;

                    case 'Q':
                        acknowledge(server, "");
                        close(server->serverfd);
                        printf("Quitting\n");
                        fflush(stdout);
                        exit(0);
                }
                strcpy(message, "");
            }
        }

        // clean up child processes when backlog is full
        if(processCount % BACKLOG == 0) {
            printf("waiting for child processes...\n");
            fflush(stdout);
            // make sure not to modify errno when calling waitpid
            int errnocpy = errno;
            while(waitpid(-1, NULL, WNOHANG) > 0);
            errno = errnocpy;
        }
    }
}

int connectClient(clientdata *client, char *address, char *port) {
    /*
    Connects a client program to the given address.
    */
    if((client->serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }

    memset(&client->hints, 0, sizeof(client->hints));
    client->hints.ai_socktype = SOCK_STREAM;
    client->hints.ai_family = AF_INET;

    int err;
    err = getaddrinfo(address, port, &client->hints, &client->actualdata);
    if(err < 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        return -1;
    }

    // connect to server
    if(connect(client->serverfd, client->actualdata->ai_addr, client->actualdata->ai_addrlen) < 0) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}