#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <netdb.h>

#define PORT "3490"
#define BACKLOG 5

struct modBuf processBuffer(char *text);
char * search_inFile(char *fname, char *str);

struct modBuf {
    char str[256];
    int len;
};

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
    char buf[256]; 
    int recv_bytes;
    struct modBuf modded;

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
        printf("server: bind successful; fdlistener = %d\n", fdlistener);
    }

    // Laukiam connectionu + Kiek connectionu priimsim i queue
    if (listen(fdlistener, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    else {
        printf("server: waiting for connections ...\n");
    }

    FD_SET(fdlistener, &master);
    fdmax = fdlistener;

    for(;;) {
        int action_flag = 0;
        read_fds = master;
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
                    addrlen = sizeof(remoteaddr);
                    if ((new_clientfd = accept(fdlistener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1 ) {
                        perror("accept");
                    } else {
                        send(new_clientfd, "server: Welcome to SocketDictionary", 36, 0);
                        FD_SET(new_clientfd, &master); // add to master set
                        if (new_clientfd > fdmax) {    // keep track of the max
                            fdmax = new_clientfd;
                        }
                        printf("server: new connection from %s on socket %d\n",
                            inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), new_clientfd);
                    }
                // READ CURRENT CONNECTIONS
                } else {
                    getpeername(i, (struct sockaddr *)&remoteaddr, &addrlen); 
                    const char * tempIP = inet_ntop(AF_INET6, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN);
                    printf("client: %s : ", tempIP);

                    // handle data from a client
                    if ((recv_bytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (recv_bytes == 0) {
                            // connection closed
                            printf("hung up on socket %d\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); 
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // Get translation
                        modded = processBuffer(buf);
                        printf("asked to translate - %s \n", buf);
                        char *match = search_inFile("dictionary.txt", modded.str);
                        modded = processBuffer(match);

                        if (send(i, modded.str, modded.len, 0) == -1) {
                            perror("send");
                        }
                        else {
                            printf("server: sent %s to %s\n", modded.str, tempIP);
                        }
                        /*
                        for(int j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != fdlistener ) {//&& j != i
                                    if (send(j, modded.str, modded.len, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        } */
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
        
        if (action_flag == 0){
            printf("server: Timed out.\n");
            close(fdlistener);
            exit(1);
        }
        
    } // END for(;;)

    return 0;
}

struct modBuf processBuffer(char *text) {
   int length, c, d;
   char *start;
   struct modBuf resultStruct;
   
   c = d = 0;
   
   length = strlen(text);
 
   start = (char*)malloc(length+1);
   
   if (start == NULL)
      exit(EXIT_FAILURE);
   
   while (*(text+c) != '\0') {
      if (*(text+c) == ' ') {
         int temp = c + 1;
         if (*(text+temp) != '\0') {
            while (*(text+temp) == ' ' && *(text+temp) != '\0') {
               if (*(text+temp) == ' ') {
                  c++;
               }  
               temp++;
            }
         }
      }
      *(start+d) = *(text+c);
      c++;
      d++;
   }
   *(start+d)= '\0';

   for(int i = 0; i <= d; i++) {
       resultStruct.str[i] = *(start+i);
   } 
   resultStruct.len = d;
   resultStruct.str[resultStruct.len-1] = '\0';

   return resultStruct;
}

char * search_inFile(char *fname, char *str) {
	FILE *fp;
	int line_num = 1;
	int find_result = 0;
	char *result;
    result = (char*)malloc(512);

	if((fp = fopen(fname, "r")) == NULL) {
        printf("server: fileopen error\n");
		return("error occured");
	}

	while(fgets(result, 512, fp) != NULL) {
		if((strstr(result, str)) != NULL) {
			//printf("A match found on line: %d\n", line_num);
			//printf("\n%s\n", result);
			find_result++;
            break;
		}
		line_num++;
    }

    if(fp) {
		fclose(fp);
	}

	if(find_result == 0) {
		printf("server: filesearch: couldn't find a match.\n");
        return("no match found in dictionary ");
	}
   	return(result);
}