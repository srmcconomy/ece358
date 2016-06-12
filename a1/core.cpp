#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include "mybind.c"

using namespace std;

int connectToPeer(int sockfd, string ip, int port) {
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = 0;
    mybind(sockfd, &client);

    struct sockaddr_in server;
    bzero(&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    if(!inet_aton(ip.c_str(), &(server.sin_addr))) {
        perror("invalid ip"); return -1;
    }
    server.sin_port = htons(port);

    if(connect(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
        perror("connect"); return -1;
    }

    return 0;
}

void sendMessage(int sockfd, string msg) {
  char buf[256] = "";
  strncpy(buf, msg.c_str(), sizeof(buf));
  buf[sizeof(buf) - 1] = 0;
  send(sockfd, buf, strlen(buf), 0);
}

string recieveMessage(int sockfd) {
    char buf[256] = "";
    recv(sockfd, buf, 256-1, 0);
    return buf;
}
