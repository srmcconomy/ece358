#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <strings.h>
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
  char buf[256];
  for(;;) {
    strncpy(buf, msg.c_str(), sizeof(buf));
    buf[255] = 0;

    printf("%s\n", buf);

    send(sockfd, buf, sizeof(buf), 0);
    if (msg.length() >= 255) {
      msg = msg.substr(255);
    } else break;
  }
}

string recieveMessage(int sockfd) {
    char buf[256];
    string s = "";
    do {
      memset(buf, 0, sizeof(buf));
      recv(sockfd, buf, 256, 0);

        printf("%s\n", buf);
      s.append(buf);
    } while(strlen(buf) == 255);
    return s;
}
