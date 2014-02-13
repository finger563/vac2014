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

#define MAXBUFLEN 10240

char *hostname = "192.168.1.80";
int server_portno = 9999;

int sockfd;
struct sockaddr_in server_addr;
struct sockaddr remote_addr;
socklen_t remote_addr_len;

#include <iostream>
#include <fstream>
using namespace std;

#include <time.h>

char* recvImage() {
  char tmpbuf[50];
  int imgSize = 0;
  int recvBytes = 0;
  bool recvEnd = false;
  int totalBytesReceived = 0;
  while ( (recvBytes = recvfrom(sockfd, tmpbuf, 50, 0, 
				(struct sockaddr *) &remote_addr, &remote_addr_len) ) == -1 );
  char *p = strtok(tmpbuf,",");
  p = strtok(NULL,","); // want second argument
  imgSize = atoi(p);
  if ( imgSize <= 0 )
    return NULL;
  char *data = new char[imgSize];
  while (recvBytes < imgSize && !recvEnd) {
    recvBytes = recvfrom(sockfd, data+totalBytesReceived, MAXBUFLEN, 0, 
			 (struct sockaddr *) &remote_addr, &remote_addr_len);
    totalBytesReceived += recvBytes;
    if (!strcmp(data+totalBytesReceived,"END"))
      recvEnd = true;
  }
  return data;
}

int main(int argc, char **argv)
{    
  if ((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0 ){
    perror("Error Creating Sending Socket");
    exit(EXIT_FAILURE);
  }

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

  remote_addr_len = sizeof remote_addr;

  if ( (numbytes = recvfrom(sockfd, tmpbuf, 50, 0, 
			     (struct sockaddr *) &remote_addr, &remote_addr_len) ) == -1 ) {
    printf("Error: recvfrom\n");
    return -1;
  }

  printf("Ground Station: got response\n");

  char * data;
  while (true) {
    data = recvImage();
    printf("Ground Station: received image!\n");
  }

  exit(EXIT_SUCCESS);

  return 0;
}
