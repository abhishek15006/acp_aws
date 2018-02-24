#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <binn.h>

#define MYPORT "4950"   // the port users will be connecting to
#define MAXBUFLEN 1200

unsigned char *package(unsigned int seq);
unsigned int unpack(unsigned char buffer[]);

unsigned char *package(unsigned int seq){
    unsigned char *data = (unsigned char *)malloc(1024*sizeof(unsigned char));

    unsigned int n1 = seq>>8;
    unsigned int n2 = seq & 0xff;

    data[0] = (unsigned char)n1;
    data[1] = (unsigned char)n2;

    return data;
}

unsigned int unpack(unsigned char buffer[]){

    unsigned int high = (unsigned int)buffer[0];
    unsigned int low = (unsigned int)buffer[1];
    unsigned int sq = (high<<8) + low;

    //printf("%x %x %x %x %u\n",buffer[0],buffer[1],buffer[2],buffer[3],sq);
    //printf("high=%u low=%u sq=%u\n",high,low,sq);
    return sq;
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    unsigned int seq = 0;
    struct sockaddr_in their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");
    
    while(1){
        memset(buf,0,sizeof(buf));

        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        seq = unpack(buf);
        //printf("Bytes=%d Seq=%u\n", numbytes, seq);

        //Send Ack
        
        unsigned char *packet = package(seq);

        if ((numbytes = sendto(sockfd, packet, 20, 0,(struct sockaddr *)&their_addr, addr_len)) == -1) {
            perror("talker: sendto");
            exit(1);
        }

        free(packet);
    }
    
    close(sockfd);
    return 0;
}
