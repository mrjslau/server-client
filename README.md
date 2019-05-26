# server-client

Simple server-client written entirely in C. Works with UNIX sockets.
Connect with client app to the server and get translations for German words.

## Prerequisites
UNIX-like OS

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

  // Example
  Word: das Brot
  
  // To disconnect from server
  Word: q()
```
