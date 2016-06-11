#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include "mybind.c"
#include "pickip.c"

using namespace std;
int main(int argc, char* argv[]) {

  struct in_addr srvip;
  if(pickServerIPAddr(&srvip) < 0) {
    fprintf(stderr, "pickServerIPAddr() returned error.\n");
    exit(-1);
  }

  int sockfd;
  int buflen = 16;
  struct sockaddr_in addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    return -1;
  addr.sin_family = AF_INET;
  memcpy(&(addr.sin_addr), &srvip, sizeof(struct in_addr));
  addr.sin_port = 0;
  mybind(sockfd, &addr);
  socklen_t alen = sizeof(struct sockaddr_in);
  if(getsockname(sockfd, (struct sockaddr *)&addr, &alen) < 0) {
    perror("getsockname"); return -1;
  }
  //if (fork()) {

  if(argc == 3) {

    int socktd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = 0;
    mybind(socktd, &client);

    struct sockaddr_in server;
    bzero(&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    if(!inet_aton(argv[1], &(server.sin_addr))) {
      perror("invalid ip"); return -1;
    }
    server.sin_port = htons(atoi(argv[2]));

    printf("client associated with %s %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    printf("trying to connect to %s %d...\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    if(connect(socktd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
      perror("connect"); return -1;
    }

    char buf[buflen];
    strcpy(buf, "danque");
    send(socktd, buf, strlen(buf), 0);
    printf("Sent danque\n");

    recv(socktd, buf, buflen-1, 0);
    printf("Recieved string %s\n", buf);
  }

  cout << inet_ntoa(addr.sin_addr) << " " << ntohs(addr.sin_port) << endl;

  listen(sockfd, 5);

  for(;;) {
    struct sockaddr_in in_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    printf("Starting to accept\n");

    int newsockfd;
    if((newsockfd = accept(sockfd, (struct sockaddr *)&in_addr, &len)) < 0){
      continue;
    }
    printf("Connection accepted from %s %d\n",
        inet_ntoa(in_addr.sin_addr), ntohs(in_addr.sin_port));
    ssize_t recvlen;
    char buffer[buflen];
    recvlen = recv(newsockfd, buffer, 16, 0);
    printf("Recieved %s from %d\n", buffer, getpid());

    strcpy(buffer, "shequels");
    send(newsockfd, buffer, strlen(buffer), 0);
    close(newsockfd);
    //bzero(buffer, 256);
    //n = read(newsockfd, buffer, 255);
    //write(newsockfd, "yo", 2);
    //if (n < 0) continue;
    //cout << buffer << endl;
  }
//} else {

//}
  return 0;
}