#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include "mybind.c"

using namespace std;
int main(int argc, char* argv[]) {
  //if (!fork()) {
    int sockfd, portno;
    char buffer[256];
    struct sockaddr_in addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
      return -1;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    mybind(sockfd, &addr);
    listen(sockfd, 5);
    cout << "listening on " << addr.sin_port << endl;
    for(;;) {
      int n;
      struct sockaddr_in in_addr;
      socklen_t len = sizeof(in_addr);
      int newsockfd = accept(sockfd, (struct sockaddr*)&in_addr, &len);
      if (newsockfd < 0) continue;
      bzero(buffer, 256);
      n = read(newsockfd, buffer, 255);
      write(newsockfd, "yo", 2);
      if (n < 0) continue;
      cout << buffer << endl;
      close(newsockfd);
    }
  // } else {
  // }
  return 0;
}
