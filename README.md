# cs360-final
Final project for CS360 class at WSUV

This program is meant to be run on Linux.
This is an example of a client-server File Transfer Protocol (FTP) interface implemented in C.

The client connects to the server on port 54789 and is then able to run the following commands:
- exit:             Exit the server process corresponding to this client.
- cd 'pathname':    Changes the client's directory to 'pathname'.
- rcd 'pathname':   Changes the server's directory to 'pathname'.
- ls:               Lists the contents of the client's current directory.
- rls:              Lists the contents of the server's current directory.
- get 'pathname':   Get the file at <pathname> from the server and store it locally.
- show 'pathname':  Print the contents of the file at <pathname> from the server.
- put 'pathname':   Copy the contents of the local file at <pathname> to the server.
