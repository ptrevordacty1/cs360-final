#include "mftp.h"

void acknowledge(serverdata *server, char *portnum) {
    /*
    Sends an acknowledgement message to the client.
    */
    char acknowledgement[8] = "A"; // max port is 5, will never need more than 8 bytes for this string
    if(strlen(portnum) > 0) strcat(acknowledgement, portnum);
    strcat(acknowledgement, "\n");
    ewrite(server->clientfd, acknowledgement, strlen(acknowledgement));
}

void sendError(int clientfd, char *errmessage) {
    /*
    Sends an error message to the client.
    */
    char message[120] = "E";
    strcat(message, errmessage);
    strcat(message, "\n");
    ewrite(clientfd, message, strlen(message));
}

void swrite(int clientfd, char *string, int stringlen) {
    /*
    write() with error checking, error message output to client
    */
    if(write(clientfd, string, stringlen) < 0) {
        sendError(clientfd, strerror(errno));
    }
}

int dataConnection(serverdata *dc) {
    /*
    Creates a socket and binds it to port 0. This acts as a wildcard,
    giving it a random open ephemeral port. Stores the port number and
    other information in 'dc'. Returns -1 on error, or 0 on success.
    */
    dc->len = sizeof(struct sockaddr_in);

    if((dc->serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if(setsockopt(dc->serverfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        return -1;
    }

    memset(&dc->servaddr, 0, sizeof(dc->servaddr));
    dc->servaddr.sin_family = AF_INET;
    dc->servaddr.sin_port = 0;
    dc->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(dc->serverfd, (struct sockaddr*) &dc->servaddr, sizeof(struct sockaddr_in)) < 0) {
        return -1;
    }

    int len = sizeof(struct sockaddr_in);
    if(getsockname(dc->serverfd, (struct sockaddr*) &dc->servaddr, &dc->len) < 0) {
    	return -1;
    }

    listen(dc->serverfd, 1);

    return 0;
}

int srls(serverdata *server) {
    /*
    Execute 'ls -l' command from the server, sending its output to the client
    */
    // fork process before calling exec()
    if(fork() == 0) {
        char *args1[3];
        args1[0] = "ls";
        args1[1] = "-l";
        args1[2] = NULL; 
        close(server->serverfd);
        close(STDOUT_FILENO);
        dup(server->clientfd);
        close(server->clientfd);
        execvp(args1[0], args1);
        return -1;
    }
    // wait for child process if necessary
    waitpid(-1, NULL, WNOHANG);
    return 0;
}

int sget(serverdata *dc, char *filename) {
    // checking if filename is a normal, readable file
    struct stat st;
    if(lstat(filename, &st) < 0) {
		return -1;
	}
	if(!S_ISREG(st.st_mode)) {
        // sets errno to 'operation not permitted'. Couldn't find something more appropriate.
        errno = 1;
        return -1;
    }
    if(access(filename, R_OK) != 0) {
        return -1;
    }

    // open filename for reading
    int fd = open(filename, O_RDONLY);
    if(fd < 0) {
        return -1;
    }
    char buffer[120];
    int numread = 0;

    // write file contents through data connection to the client
    while((numread = read(fd, buffer, 120)) > 0) {
        ewrite(dc->clientfd, buffer, numread);
    }
    return 0;
}

int sput(serverdata *dc, char *pathname) {
    
    int fd = open(pathname, O_CREAT | O_RDWR, 0700);
    if(fd < 0) {
        return -1;
    }

    char buffer[120];
    int numread;
    while((numread = read(dc->clientfd, buffer, 120)) > 0) {
        write(fd, buffer, numread);
    }

    return 0;
}