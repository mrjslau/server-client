#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>  // close()
#include <netdb.h>   // addrinfo struct, getaddrinfo()

#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    struct addrinfo hints, *addr, *p;
    int err;

    fd_set master, read_fds;
    int fdlistener, fdmax, new_clientfd;

    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char remoteIP[INET6_ADDRSTRLEN];

    struct timeval tv;

    tv.tv_sec = 25;
    tv.tv_usec = 0;

    //Read
    char buf[256];    // buffer for client data
    int recv_bytes;

    memset(&hints, 0, sizeof hints);  // clean struct
    hints.ai_family = AF_UNSPEC;      // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_flags = AI_PASSIVE;      // use my IP

    // Setupinam addr structus
    if ((err = getaddrinfo(NULL, PORT, &hints, &addr)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    // Loopinam per visus ir bandom bind'intis
    for(p = addr; p != NULL; p = p->ai_next) {
        // Gaunam socketo fd
        if ((fdlistener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // Bindinames prie socketo
        if (bind(fdlistener, p->ai_addr, p->ai_addrlen) == -1) {
            close(fdlistener);
            perror("server: bind");
            continue;
        }

        break;
    }
    // Free memory
    freeaddrinfo(addr);

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    else {
        printf("server: Bind successful; fdlistener = %d\n", fdlistener);
    }

    // Laukiam connectionu + Kiek connectionu priimsim i queue
    if (listen(fdlistener, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    else {
        printf("server: Waiting for connections ...\n");
    }

    // add the listener to the master set
    FD_SET(fdlistener, &master);
    // keep track of the biggest file descriptor
    fdmax = fdlistener; // so far, it's this one

    // main loop
    for(;;) {
        int action_flag = 0;
        printf("fdmax = %d\n", fdmax);
        read_fds = master; // copy set
        // monitors fds ------read----write-except-timeout--------
        if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                action_flag = 1;
                // NEW CONNECTION
                if (i == fdlistener) {
                    // handle new connections
                    addrlen = sizeof(remoteaddr);
                    if ((new_clientfd = accept(fdlistener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1 ) {
                        perror("accept");
                    } else {
                        send(new_clientfd, "Hello", 32, 0);
                        FD_SET(new_clientfd, &master); // add to master set
                        if (new_clientfd > fdmax) {    // keep track of the max
                            fdmax = new_clientfd;
                        }
                        printf("server: New connection from %s on socket %d\n",
                            inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), new_clientfd);
                            // ^ return IP in string form from client struct
                    }
                // READ CURRENT CONNECTIONS
                } else {
                    // handle data from a client
                    if ((recv_bytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (recv_bytes == 0) {
                            // connection closed
                            printf("server: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        for(int j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != fdlistener && j != i) {
                                    if (send(j, buf, recv_bytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
        
        if (action_flag == 0){
            printf("server: Timed out.\n");
            exit(1);
        }
        
    } // END for(;;)

    return 0;
}