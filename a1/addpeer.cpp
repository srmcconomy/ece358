#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
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
  // setup messaging between the child and parent
  int pipefd[2];
  pipe(pipefd); 
  // fork child process
  int pid = fork();
  if (pid == 0) {
    // child process
    vector<peer> peers;
    map<unsigned int, string> content;

    peer self = {
      inet_ntoa(addr.sin_addr),
      ntohs(addr.sin_port),
      0
    };
    // add self to list of peers
    peers.push_back(self);
    // Keep track of the id given to the last piece of content
    unsigned int last_content_id = 1;

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

      iss >> last_content_id;

      shutdown(socktd, SHUT_RDWR);

    }

    listen(sockfd, 10);

    // request content from other peers in order to balance out content
    int totalContent = 0;

    for (int i = 0 ; i < peers.size() ; i++) {
      totalContent += peers[i].numContent;
    }
    int contentNeeded = floor(totalContent / (double)peers.size());
    while(contentNeeded > 0) {
      // take one content from the peer with the most content
      int biggestContentPeer = 1;
      for (int i = 1 ; i < peers.size() ; i++) {
          if (peers[biggestContentPeer].numContent <= peers[i].numContent) {
            biggestContentPeer = i;
          }
      }

      int sockid = socket(AF_INET, SOCK_STREAM, 0);
      connectToPeer(sockid, peers[biggestContentPeer].ip, peers[biggestContentPeer].port);
      sendMessage(sockid, "needcontent");

      //handle the other peer's "numcontent" broadcast
      handle_message(sockfd, peers, content, last_content_id);

      unsigned int newid;
      string newcontent;
      istringstream newss(recieveMessage(sockid));
      // add content into this peer
      newss >> newid;
      getline(newss, newcontent);
      newcontent = newcontent.substr(1);
      content[newid] = newcontent;
      // increment the ammount of content in this peer
      peers[0].numContent++;
      sendMessage(sockid, "done");
      close(sockid);
      --contentNeeded;
    }
    
    //tell everyone I have a different amount of content now
    string cmd = "numcontent ";
    cmd.append(peers[0].ip);
    cmd.append(" ");
    cmd.append(int_to_string(peers[0].port));
    cmd.append(" ");
    cmd.append(int_to_string(peers[0].numContent));
    for (int i = 1; i < peers.size(); i++) {
      int sockid = socket(AF_INET, SOCK_STREAM, 0);
      connectToPeer(sockid, peers[i].ip, peers[i].port);
      sendMessage(sockid, cmd);
      recieveMessage(sockid);
      close(sockid);
    }
    // finished load balancing on add peer
    cout << inet_ntoa(addr.sin_addr) << " " << ntohs(addr.sin_port) << endl;

    // we are done syncing with the network. 
    // notify the parent to terminate
    close(pipefd[0]); // close the read-end of the pipe, child does not use it
    write(pipefd[1], "x", 1); // send some char to the parent
    close(pipefd[1]); // close the write-end of the pipe, thus sending EOF to the reader

    // Infinite listen loop
    for (;;) {
      int retCode = handle_message(sockfd, peers, content, last_content_id);
      if (retCode == RETURN_ERROR || retCode == RETURN_SHUTDOWN) break;
    }
  } else if(pid > 0){
    // parent process
    close(pipefd[1]); // close the write-end of the pipe, parent does not use it
    char buf;
    while (read(pipefd[0], &buf, 1) > 0) {
      if(buf == 'x') break;
    }
    close(pipefd[0]); // close the read-end of the pipe
  } else {
    // fork failed
    cout<<"Fork failed!"<<endl;
  }
  return 0;
}
