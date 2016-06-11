#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include "mybind.c"

using namespace std;
int main(int argc, char* argv[]) {

  int sockfd;
  char buffer[256];
  struct sockaddr_in addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    return -1;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = 0;
  mybind(sockfd, &addr);
  cout << inet_ntoa(addr.sin_addr) << " " << addr.sin_port << endl;

  if (!fork()) {
    listen(sockfd, 5);
    for(;;) {
      struct sockaddr_in in_addr;
      socklen_t len = sizeof(struct sockaddr_in);
      printf("Starting to accept\n");
      int newsockfd = accept(sockfd, (struct sockaddr *)&in_addr, &len);
      printf("Accepted \n");
      if (newsockfd < 0) continue;
      ssize_t recvlen;
      recvlen = recv(newsockfd, buffer, 16, 0);
      printf("Recieved %s from %d\n", buffer, getpid());

      //strcpy(buffer, "I'm in California thinking about how we used to be...");
      //send(newsockfd, buffer, strlen(buffer), 0);
      close(newsockfd);
      //bzero(buffer, 256);
      //n = read(newsockfd, buffer, 255);
      //write(newsockfd, "yo", 2);
      //if (n < 0) continue;
      //cout << buffer << endl;
    }
  } else {
    if(argc == 3) {
      int socktd = socket(AF_INET, SOCK_STREAM, 0);
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = 0;
      mybind(socktd, &addr);

      struct sockaddr_in server;
      bzero(&server, sizeof(struct sockaddr_in));
      server.sin_family = AF_INET;
      if(!inet_aton(argv[1], &(server.sin_addr))) {
	perror("invalid ip"); return -1;
      }
      server.sin_port = htons(atoi(argv[2]));

      printf("client associated with %s %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
      printf("trying to connect to %s %d...\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
      if(connect(socktd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
	perror("connect"); return -1;
      }
      printf("What is going oonnnn\n");

      int buflen = 16;
      char buf[buflen];
      printf("What\n");
      strcpy(buf, "danque");
      printf("Sending string....");
      send(sockfd, buf, strlen(buf), 0);
      printf("Sent helloooo....");

      //recv(sockfd, buf, buflen-1, 0);
      printf("%s", buf);
    }

    cout << inet_ntoa(addr.sin_addr) << " " << addr.sin_port << endl;
  }
  return 0;
}
