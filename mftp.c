#include "mftp.h"

int main(int argc, char **argv) {
    if(argc < 2) {
        fprintf(stderr, "Error: not enough arguments\n");
        return -1;
    }

    char *address = argv[1];

    clientdata client;
    printf("Waiting to connect to server...\n");
    if(connectClient(&client, address, PORT_STRING) < 0) exit(0);
    printf("Connected to server\n");
    fflush(stdout);

    int numread = 0;
    char message[120] = "";
    clientdata dc;
    int messageTokenCount = 0;

    while(1) {
        // getting input from user
        printf("> ");
        fflush(stdout);
        strcpy(message, "");

        if(copyLineRead(0, message, 120) <= 0) break;

        // splitting the message into tokens, storing tokens in an array
        messageTokenCount = getWordCount(message);
        char *messageTokens[messageTokenCount];
        getWordList(message, messageTokens);

        // empty input
        if(messageTokenCount == 0) {
            continue;
        }

        // exit
        else if(strncmp(messageTokens[0], "exit", 4) == 0) {
            ewrite(client.serverfd, "Q\n", 2);
            handleServerResponse(client.serverfd);
            break;
        }

        // cd
        else if(strncmp(messageTokens[0], "cd", 2) == 0 && messageTokenCount > 1) {
            if(chdir(messageTokens[1]) < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
            }
        }

        // rcd
        else if(strncmp(messageTokens[0], "rcd", 3) == 0 && messageTokenCount > 1) {
            rcd(client.serverfd, messageTokens[1]);
            if(handleServerResponse(client.serverfd) < 0) {
                continue;
            }
        }

        // ls
        else if(strncmp(messageTokens[0], "ls", 2) == 0) {
            ls();
            fflush(stdout);
        }

        // rls
        else if(strncmp(messageTokens[0], "rls", 3) == 0) {
            dcConnect(&dc, client.serverfd, address);
            ewrite(client.serverfd, "L\n", 2);
            // signal ls -l command from server
            handleServerResponse(client.serverfd);
            // handle ls -l output from server data connection
            if(crls(&dc) < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                continue;
            }
            fflush(stdout);
        }

        // get
        else if(strncmp(messageTokens[0], "get", 3) == 0 && messageTokenCount > 1) {
            dcConnect(&dc, client.serverfd, address);
            // signal the server to send the contents of the filename in messageTokens[1]
            if(get(&dc, client.serverfd, messageTokens[1]) < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
            }
        }

        // show
        else if(strncmp(messageTokens[0], "show", 4) == 0 && messageTokenCount > 1) {
            dcConnect(&dc, client.serverfd, address);
            // signal the server to send the contents of the filename in messageTokens[1]
            if(show(&dc, client.serverfd, messageTokens[1]) < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
            }
        }

        // put
        else if(strncmp(messageTokens[0], "put", 3) == 0 && messageTokenCount > 1) {
            dcConnect(&dc, client.serverfd, address);
            // signal the server to receive the contents of the filename in messageTokens[1]
            if(put(&dc, client.serverfd, messageTokens[1]) < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
            }
        }

        // help
        else if(strncmp(messageTokens[0], "help", 4) == 0) {
            printHelp(1);
            fflush(stdout);
        }

        // invalid command,
        else {
            printf("Unrecognized command. Try 'help'.\n");
            fflush(stdout);
        }
    }

    return 0;
}