#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

using namespace std;
int main(int argc; char* argv[]) {
  //if (!fork()) {
    int sockfd, portno;
    char buffer[256];
    struct sockaddr_in addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
      return -1;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    mybind(sockfd, &addr, sizeof(addr));
    listen(sockfd, 5);
    cout << "listening on " << sockfd << endl;
    for(;;) {
      int n;
      struct sockaddr_in in_addr;
      int len = sizeof(in_addr);
      int newsockfd = accept(sockfd, &in_addr, &len);
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
