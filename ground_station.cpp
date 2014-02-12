#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>

#define MAXBUFLEN 65500

#include <iostream>
#include <fstream>
using namespace std;

#include <time.h>

int main(int argc, char **argv)
{
  int sockfd;
    
  if ((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0 ){
    perror("Error Creating Sending Socket");
    exit(EXIT_FAILURE);
  }

  char *hostname = "192.168.1.80";
  int server_portno = 9999;

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_portno);
  inet_pton(AF_INET, hostname, &(server_addr.sin_addr));
  
  int sentbytes=0,numbytes=0;
  char tmpbuf[50];
  sprintf(tmpbuf,"Start Image");
  if ( numbytes = sendto(sockfd, tmpbuf, strlen(tmpbuf),0, (struct sockaddr *)&(server_addr), sizeof (server_addr)) == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }
  printf("Ground Station: Sent initialization\n");

  struct sockaddr remote_addr;
  socklen_t remote_addr_len;
  remote_addr_len = sizeof remote_addr;

  if ( (numbytes = recvfrom(sockfd, tmpbuf, 50, 0, 
			     (struct sockaddr *) &remote_addr, &remote_addr_len) ) == -1 ) {
    printf("Error: recvfrom\n");
    return -1;
  }

  printf("Ground Station: got response\n");

  exit(EXIT_SUCCESS);

  return 0;
}
