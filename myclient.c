#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <netdb.h>  

#define PORT "3490" // the port client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes, err;
    struct addrinfo hints, *servinfo, *p;

    char s[INET6_ADDRSTRLEN];
    char buf[MAXDATASIZE];

    if (argc != 2) {
        fprintf(stderr,"usage: enter server IP as argument\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // If the AI_PASSIVE flag is not set in hints.ai_flags, then the returned socket addresses
    // will be suitable for use with connect(2), sendto(2), or sendmsg(2).

    if ((err = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    // Free memory
    freeaddrinfo(servinfo); 

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    else {
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
        printf("client: connected to %s , sockfd = %d\n", s, sockfd);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("client: recv");
        exit(1);
    }
    buf[numbytes-1] = '\0';
    printf("client: received '%s'\n",buf);

    // MESSAGING -------------------
    for(;;) {
        printf("Word: ");
        fgets(buf, sizeof(buf), stdin);
        
        // QUIT
        if(buf[0] == 'q' && buf[1] == '(' && buf[2] == ')') {
            break;
        }
        // MONITOR
        if(buf[0] == 'm' && buf[1] == '(' && buf[2] == ')') {
            for(;;) {
                if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }
                printf("client-monitor: received '%s'\n",buf);   
            }
            break;
        }

        //buf[numbytes] = '\0';

        send(sockfd, buf, sizeof(buf), 0);

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }        
        printf("client: received '%s'\n",buf);
    }
    // END MESSAGING -------------------

    printf("client: disconnected.\n");
    close(sockfd);
    return 0;
}