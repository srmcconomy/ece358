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
#include "peer.cpp"
#include "pickip.c"

using namespace std;

int main(int argc, char* argv[]) {
  vector<peer> peers;
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

  peer self = {
    inet_ntoa(addr.sin_addr),
    ntohs(addr.sin_port),
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
    struct sockaddr_in in_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    printf("Starting to accept\n");

    int newsockfd;
    if ((newsockfd = accept(sockfd, (struct sockaddr *)&in_addr, &len)) < 0){
      continue;
    }
    printf("Connection accepted from %s %d\n",
        inet_ntoa(in_addr.sin_addr), ntohs(in_addr.sin_port));

    istringstream iss(recieveMessage(newsockfd));
    string command;
    iss >> command;

    // Adding peer to the network
    if (command == "newpeer") {
      peer nakama = get_peer(iss);

      string cmd = "addpeer ";
      cmd.append(nakama.ip);
      cmd.append(" ");
      cmd.append(int_to_string(nakama.port));
      // start at 1 so we don't send message to ourself
      for (int i = 1; i < peers.size(); i++) {
        int sockid = socket(AF_INET, SOCK_STREAM, 0);
        connectToPeer(sockid, peers[i].ip, peers[i].port);
        sendMessage(sockid, cmd);
        recieveMessage(sockid);
        close(sockid);
      }
      
      // Give list of networks to new peer
      sendMessage(newsockfd, list_of_peers(peers));
      peers.push_back(nakama);

      printf("%s\n", list_of_peers(peers).c_str());
    }

    // Adding to peer list
    if (command == "addpeer") {
      peer mate = get_peer(iss);
      peers.push_back(mate);

      sendMessage(newsockfd, "done");
      printf("%s\n", list_of_peers(peers).c_str());
    }

    // Removing peer from network
    if (command == "removepeer") {
      peer death = get_peer(iss);

      // Confirm that remove peer and this peer are the same
      if(death == peers[0]) {        
        string cmd = "deletepeer ";
        cmd.append(death.ip);
        cmd.append(" ");
        cmd.append(int_to_string(death.port));
        // start at 1 so we don't send message to ourself
        for (int i = 1; i < peers.size(); i++) {
          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[i].ip, peers[i].port);
          sendMessage(sockid, cmd);
          recieveMessage(sockid);
          close(sockid);
        }

        // Let user know we finished
        sendMessage(newsockfd, "done");

        if(shutdown(newsockfd, SHUT_RDWR) < 0) {
          perror("shutdown"); return -1;
        }

        break;

      } else {
        sendMessage(newsockfd, "nexist");
      }
    }

    // Deleting from peer list
    if (command == "deletepeer") {
      peer cleanup = get_peer(iss);
      for (int i = 0; i < peers.size(); i++){
        if (peers[i] == cleanup) {
          peers.erase(peers.begin() + i);
          break;
        }
      }

      sendMessage(newsockfd, "done");
      printf("%s\n", list_of_peers(peers).c_str());
    }

    close(newsockfd);
  }
//} else {

//}
  return 0;
}
