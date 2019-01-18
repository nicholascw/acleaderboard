#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libmng_types.h>
#include <signal.h>

char sigint_captured = 0;
void sigint_handler(){
    sigint_captured = 1;
}
int main() {
    signal(SIGINT, sigint_handler);
    printf("NW_ACLeaderBoard v0.1"
           "A GPLv3-licensed Assetto Corsa Dedicated Server plugin.\n");

    int s;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo("localhost", "12000", &hints, &res);
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(bind(sockfd, res->ai_addr, res->ai_addrlen)) {
        perror("bind() failed! check if another instance is already started, or other plugins are occupying port 12000.");
        exit(1);
    }

    struct sockaddr_storage addr;
    int addrlen = sizeof(addr);
    while(!sigint_captured) {
        char buf[1024];
        ssize_t byte_count = recvfrom(sockfd, buf, sizeof(buf), 0, &addr, &addrlen);
        buf[byte_count] = '\0';

        printf("Read %zu chars\n", byte_count);
        printf("====\n");
        printf("%s\n", buf);
    }
    return 0;
}