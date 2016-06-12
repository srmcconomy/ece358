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
#include "core.cpp"
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

    // String-format: "newpeer 172.46.14.3 10013"
    string cmd = "newpeer ";
    cmd.append(inet_ntoa(addr.sin_addr));
    cmd.append(" ");
    cmd.append(int_to_string(ntohs(addr.sin_port)));

    connectToPeer(socktd, argv[1], atoi(argv[2]));
    sendMessage(socktd, cmd);
    istringstream iss(recieveMessage(socktd));
    string peerCount;
    iss >> peerCount;
    printf("Peer Count: %s\n", peerCount.c_str());

    shutdown(socktd, SHUT_RDWR);

  } else {
    // Initial peer
    peer self = {
      inet_ntoa(addr.sin_addr),
      ntohs(addr.sin_port),
    };
    peers.push_back(self);
  }

  cout << inet_ntoa(addr.sin_addr) << " " << ntohs(addr.sin_port) << endl;

  listen(sockfd, 5);

  // Infinite listen loop
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

    istringstream iss(recieveMessage(newsockfd));
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
      sendMessage(newsockfd, response);
    }
    close(newsockfd);
  }
//} else {

//}
  return 0;
}