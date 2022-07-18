#include "mftp.h"

void printHelp(int fd) {
    /*
    Writes a help message to the given file descriptor.
    */
    char helpmessage[573] = ""; // 573 is the number of characters in the help message + 1 for null terminator.
    strcat(helpmessage, "Help: \n");
    strcat(helpmessage, "\t- exit:\t\t\tExit the server process corresponding to this client.\n");
    strcat(helpmessage, "\t- cd <pathname>:\tChanges the client's directory to <pathname>.\n");
    strcat(helpmessage, "\t- rcd <pathname>:\tChanges the server's directory to <pathname>.\n");
    strcat(helpmessage, "\t- ls:\t\t\tLists the contents of the client's current directory.\n");
    strcat(helpmessage, "\t- rls:\t\t\tLists the contents of the server's current directory.\n");
    strcat(helpmessage, "\t- get <pathname>:\tGet the file at <pathname> from the server and store it locally.\n");
    strcat(helpmessage, "\t- show <pathname>:\tPrint the contents of the file at <pathname> from the server.\n");
    strcat(helpmessage, "\t- put <pathname>:\tCopy the contents of the local file at <pathname> to the server.\n");
    write(fd, helpmessage, strlen(helpmessage));
}

int handleServerResponse(int serverfd) {
    /*
    Reads response message from the server.
    If server responds with Error, print the error message and return -1,
    if it responds with Accept followed by a port number, return the port number,
    otherwise, server Accepted, return -1.
    */
    int numread;
    char buffer[120];
    char message[120] = "";
    // read entire response into 'message'
    if((numread = copyLineRead(serverfd, message, 120)) <= 0) return numread;
    message[numread-1] = '\0'; // remove newline
    fflush(stdout);

    if(message[0] == 'E') {
        // response contains error message
        fprintf(stderr, "%s", &message[1]); // &message[1] removes the 'E' at the beginning of the response.
        return -1;
    }

    else if(message[0] == 'A' && strlen(message) > 2) {
        // server accepted AND sent port number
        char *ptr;
        long int strnum = strtol(&message[1], &ptr, 10);
        return (int) strnum;
    }
    // server accepted
    return 0;
}

void rcd(int serverfd, char *pathname) {
    /*
    Signal the server to change directory to 'pathname'
    */
    char message[4099] = "C";
    strcat(message, pathname);
    strcat(message, "\n");
    ewrite(serverfd, message, strlen(message));
}

void ls() {
    /*
    Execute 'ls -l' command locally, piped into 'more -20'
    */
    // fork process before calling exec()
    if(fork() == 0) {
        char *args1[3];
        args1[0] = "ls";
        args1[1] = "-l";
        args1[2] = NULL; 
        char *args2[3];
        args2[0] = "more";
        args2[1] = "-20";
        args2[2] = NULL;
        execWithPipe(args1, args2);
    }
    // wait for child process
    wait(NULL);
}

int crls(clientdata *dc) {
    /*
    The other end of srls; reads from the server, sending it's output to 'more -20'
    */
    // fork process before calling exec()
    if(fork() == 0) {
        char *args1[3];
        args1[0] = "more";
        args1[1] = "-20";
        args1[2] = NULL; 
        close(STDIN_FILENO);
        dup(dc->serverfd);
        close(dc->serverfd);
        execvp(args1[0], args1);
        return -1;
    }
    // wait for child process
    wait(NULL);
    return 0;
}

int show(clientdata *dc, int serverfd, char *pathname) {

    char message[4099] = "G";
    strcat(message, pathname);
    strcat(message, "\n");
    ewrite(serverfd, message, strlen(message));
    handleServerResponse(serverfd);
    
    if(fork() == 0) {
        char *args1[3];
        args1[0] = "more";
        args1[1] = "-20";
        args1[2] = NULL; 
        close(STDIN_FILENO);
        dup(dc->serverfd);
        close(dc->serverfd);
        execvp(args1[0], args1);
        return -1;
    }
    // wait for child process
    wait(NULL);
    return 0;
}

int get(clientdata *dc, int serverfd, char *pathname) {

    char message[4099] = "G";
    strcat(message, pathname);
    strcat(message, "\n");
    ewrite(serverfd, message, strlen(message));
    handleServerResponse(serverfd);
    
    int fd = open(pathname, O_CREAT | O_RDWR, 0700);
    if(fd < 0) {
        return -1;
    }

    char buffer[120];
    int numread;
    while((numread = read(dc->serverfd, buffer, 120)) > 0) {
        write(fd, buffer, numread);
    }

    return 0;
}

int put(clientdata *dc, int serverfd, char *pathname) {
   
    int fd = open(pathname, O_RDONLY);
    if(fd < 0) {
        return -1;
    }

    char message[4099] = "P";
    strcat(message, pathname);
    strcat(message, "\n");
    ewrite(serverfd, message, strlen(message));
    handleServerResponse(serverfd);

    char buffer[120];
    int numread;
    while((numread = read(fd, buffer, 120)) > 0) {
        write(dc->serverfd, buffer, numread);
    }
    close(dc->serverfd);

    return 0;
}

void dcConnect(clientdata *dc, int serverfd, char *address) {
    // signal for server to create a data connection
    ewrite(serverfd, "D\n", 2);
    int port = handleServerResponse(serverfd);
    if(port <= 0) {
        return;
    }
    // connect to the data connection
    char portnum[6];
    sprintf(portnum, "%d", port);
    connectClient(dc, address, portnum);
}

void readDataConnection(clientdata *dc) {
    char buffer[120];
    int numread = 0;
    while((numread = read(dc->serverfd, buffer, 120)) > 0) {
        ewrite(1, buffer, numread);
    }
}