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
#include <cmath>
#include "core.cpp"
#include "peer.cpp"
#include "pickip.c"
#include "process.cpp"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 3 && argc != 1) {
    printf("Usage: %s [ ip port ]\n", argv[0]);
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
  // add self to list of peers
  peers.push_back(self);

  if (argc == 3) {
    // if a ip and port of an existing network was specified then sync with that network
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

  listen(sockfd, 10);

  // request content from other peers in order to balance out content
  int totalContent = 0;

  for (int i = 0 ; i < peers.size() ; i++) {
    totalContent += peers[i].numContent;
  }
  int contentNeeded = floor(totalContent / (double)peers.size());
  // array that contains the number of content that this peer will request from other peers
  int askArray[peers.size()] = {0}; 
  while(contentNeeded > 0) {
    // take one content from the peer with the most content
    int biggestContentPeer = 0;
    for (int i = 1 ; i < peers.size() ; i++) {
        if (peers[biggestContentPeer].numContent <= peers[i].numContent) {
          biggestContentPeer = i;
        }
    }
    // increment ask array by one meaning that we will ask that peer for one more content
    askArray[biggestContentPeer] += 1;
    --contentNeeded;
  }
  // now that we have our ask array we can ask other peers for content
  for (int i = 1; i < peers.size(); i++) {
    if(askArray[i] > 0) {
      int sockid = socket(AF_INET, SOCK_STREAM, 0);
      connectToPeer(sockid, peers[i].ip, peers[i].port);
      string cmd = "needcontent";
      cmd.append(" ");
      cmd.append(int_to_string(askArray[i]));
      sendMessage(sockid, cmd);

      //handle the other peer's "numcontent" broadcast
      handle_message(sockfd, peers, content, last_content_id);

      unsigned int newid;
      unsigned int numcontent;
      string newcontent;
      istringstream newss(recieveMessage(sockid));
      // get the number of content actually returned by the peer
      newss >> numcontent;
      // add content into this peer
      for(int x = 0 ; x < numcontent ; x++) {
        newss >> newid >> newcontent;
        content[newid] = newcontent;
      }
      // increment the ammount of content in this peer
      peers[0].numContent += numcontent;
      sendMessage(sockid, "done");
      close(sockid);
    }
  }
  // finished load balancing on add peer
  cout << inet_ntoa(addr.sin_addr) << " " << ntohs(addr.sin_port) << endl;

  // Infinite listen loop
  for (;;) {
    if (handle_message(sockfd, peers, content, last_content_id)) break;
  }
//} else {

//}
  return 0;
}
