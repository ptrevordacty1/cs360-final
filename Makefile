all: client server

client: mftp.o sockets.o clientcommands.o servercommands.o miscfunctions.o mftp.h 
	gcc -o client mftp.o sockets.o clientcommands.o servercommands.o miscfunctions.o
server: mftpserve.o sockets.o clientcommands.o servercommands.o miscfunctions.o mftp.h
	gcc -o server mftpserve.o sockets.o clientcommands.o servercommands.o miscfunctions.o
mftp.o: mftp.c
	gcc -c mftp.c
mftpserve.o: mftpserve.c
	gcc -c mftpserve.c
sockets.o: sockets.c
	gcc -c sockets.c
clientcommands.o: clientcommands.c
	gcc -c clientcommands.c
servercommands.o: servercommands.c
	gcc -c servercommands.c
miscfunctions.o: miscfunctions.c
	gcc -c miscfunctions.c
