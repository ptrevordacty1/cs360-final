#include "mftp.h"

int getWordCount(char *sentence) {
    /*
    Returns the number of words in a string 'sentence'.
    */
    int wordCount = 0;
    int i = 0, j = 0;
    while(1) {
        while(isspace(sentence[i]) && sentence[i] != '\0') i++;
        j = i;
        while(!isspace(sentence[j]) && sentence[j] != '\0') j++;
        if(i == j) break;
        wordCount++;
        i = j;
    }
    return wordCount;
}

void getWordList(char *sentence, char **wordList) {
    /*
    Updates an array of char*s 'wordList' to contain an individual word
    from the string 'sentence' at each index.
    */
    int wordCount = 0;
    int i = 0, j, k;
    while(1) {
        while(isspace(sentence[i]) && sentence[i] != '\0') i++;
        j = i;
        k = 0;
        char word[64] = "";
        while(!isspace(sentence[j]) && sentence[j] != '\0') {
            word[k++] = sentence[j];
            j++;
        }
        word[k] = '\0';
        if(i == j) break;
        i = j;
        wordList[wordCount++] = strdup(word);
    }
}

void ewrite(int fd, char *string, int stringlen) {
    /*
    write() with error checking.
    */
    if(write(fd, string, stringlen) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
    }
}

int copyLineRead(int fd, char *message, int bufsize) {
    /*
    Reads from a file descriptor until a newline is read.
    Returns the total number of bytes read.
    */
    char buffer[bufsize];
    int numread = 0, totalNumread = 0;
    strcpy(message, "");

    while(1) {
        numread = read(fd, buffer, bufsize);
        // read error
        if(numread < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            return numread;
        }
        // end of file
        else if(numread == 0) return numread;
        // some bytes read
        else {
            totalNumread += numread;
            strncat(message, buffer, numread);
            // returning on newline
            if(buffer[numread-1] == '\n') return totalNumread;
        }
    }
}

void execWithPipe(char **arglist1, char **arglist2) {
    /*
    Runs exec() twice, with the first commandset being piped into the second
    */
    int fd[2];
    if(pipe(fd) < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        return;
    }
    int reader = fd[0];
    int writer = fd[1];
    int pid = fork();

    if(pid == 0) { //child
        // reassign stdout to write end of pipe
        close(reader);
        close(STDOUT_FILENO);
        dup(writer);
        close(writer);
        // executing...
        execvp(arglist1[0], arglist1);
        // if exec returns, throw error
        fprintf(stderr, "%s\n", strerror(errno));
        return;
    } else { //parent
        // reassign stdin to read end of pipe
        close(writer);
        close(STDIN_FILENO);
        dup(reader);
        close(reader);
        wait(NULL); //wait for child
        // executing...
        execvp(arglist2[0], arglist2);
        // if exec returns, throw error
        fprintf(stderr, "%s\n", strerror(errno));
        return;
    }
}