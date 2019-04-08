# server-client

Simple server-client written entirely in C. Works with UNIX sockets.
Connect with client app to the server and get translations for German words.

## Prerequisites
gcc

## Running
Server:
```
$ gcc -o myserver myserver.c
$ ./myserver
```

Client:
```
$ gcc -o myclient myclient.c
$ ./myclient <serverIP>

  Word: das Brot
```
