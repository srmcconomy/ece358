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
#include <map>
#include "core.cpp"
#include "peer.cpp"
#include "pickip.c"
#include "process.cpp"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 3 && argc != 1) {
    printf("Usage: %s [ ip port ]", argv[0]);
    return -1;
  }
  struct in_addr srvip;
  if (pickServerIPAddr(&srvip) < 0) {
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
  if (getsockname(sockfd, (struct sockaddr *)&addr, &alen) < 0) {
    perror("getsockname"); return -1;
  }
  //if (fork()) {
  vector<peer> peers;
  map<unsigned int, string> content;
  unsigned int last_content_id = 1;

  peer self = {
    inet_ntoa(addr.sin_addr),
    ntohs(addr.sin_port),
    0
  };
  peers.push_back(self);

  if (argc == 3) {

    int socktd = socket(AF_INET, SOCK_STREAM, 0);

    // String-format: "newpeer 172.46.14.3 10013"
    string cmd = "newpeer ";
    cmd.append(inet_ntoa(addr.sin_addr));
    cmd.append(" ");
    cmd.append(int_to_string(ntohs(addr.sin_port)));

    connectToPeer(socktd, argv[1], atoi(argv[2]));
    sendMessage(socktd, cmd);
    istringstream iss(recieveMessage(socktd));
    int peerCount;
    iss >> peerCount;

    for (int i = 0; i < peerCount; i++) {
      peer dao = get_peer(iss);
      peers.push_back(dao);
    }

    printf("%s\n", list_of_peers(peers).c_str());

    shutdown(socktd, SHUT_RDWR);

  }

  cout << inet_ntoa(addr.sin_addr) << " " << ntohs(addr.sin_port) << endl;

  listen(sockfd, 5);

  // Infinite listen loop
  for (;;) {
    if (handle_message(sockfd, peers, content, last_content_id)) break;
  }
//} else {

//}
  return 0;
}
