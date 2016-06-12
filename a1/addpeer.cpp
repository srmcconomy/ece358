#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <sstream>
#include <vector>
#include "mybind.c"
#include "pickip.c"

using namespace std;

struct peer {
  string ip;
  int port;
};

string int_to_string(int num) {
  ostringstream ss;
  ss << num;
  return ss.str();
}

int main(int argc, char* argv[]) {
  vector<peer> peers;
  struct in_addr srvip;
  if(pickServerIPAddr(&srvip) < 0) {
    fprintf(stderr, "pickServerIPAddr() returned error.\n");
    exit(-1);
  }

  int sockfd;
  int buflen = 32;
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

    char buf[32] = "";
    string cmd = "newpeer ";
    cmd.append(inet_ntoa(addr.sin_addr));
    cmd.append(" ");
    cmd.append(int_to_string(ntohs(addr.sin_port)));
    strcpy(buf, cmd.c_str());
    send(socktd, buf, strlen(buf), 0);

    recv(socktd, buf, buflen-1, 0);
    printf("Recieved string %s\n", buf);
  } else {
    // If first peer, then simply add yourself to peers
    peer self = {
      inet_ntoa(addr.sin_addr),
      ntohs(addr.sin_port),
    };
    peers.push_back(self);
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
    char buffer[32] = "";
    recvlen = recv(newsockfd, buffer, buflen, 0);
    printf("Recieved %s from %d\n", buffer, getpid());

    istringstream iss(buffer);
    string command;
    iss >> command;

    // Adding a new peer to the system
    if(command == "newpeer"){
      int port;
      string ip;
      iss >> ip >> port;

      peer nakama = {
        ip,
        port,
      };

      peers.push_back(nakama);
      string response = "peers ";
      response.append(int_to_string(peers.size()));
      strcpy(buffer, response.c_str());
    }
    send(newsockfd, buffer, strlen(buffer), 0);
    close(newsockfd);
  }
//} else {

//}
  return 0;
}