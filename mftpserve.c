#include "mftp.h"

int main(int argc, char **argv) {
    serverdata server;
    initServer(&server);
    startServer(&server);
    
    return 0;
}