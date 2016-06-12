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

using namespace std;

int main(int argc, char* argv[]) {
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
  unsigned int last_content_id = 0;

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

    if (command == "addcontent") {
      string newid;
      printf("%s\n", iss.str().c_str());
      string newcontent;
      getline(iss, newcontent);
      newcontent = newcontent.substr(1);
      printf("content to add: %s\n", newcontent.c_str());
      bool you_got_dis = false;
      for (int i = 1; i < peers.size(); i++) {
        if (peers[0].numContent - peers[i].numContent >= 1) {
          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[i].ip, peers[i].port);
          string cmd = "newcontent ";
          cmd.append(newcontent);
          sendMessage(sockid, cmd);
          {
            struct sockaddr_in in_addr2;
            socklen_t len2 = sizeof(struct sockaddr_in);
            printf("Starting to accept\n");

            int newsockfd2;
            if ((newsockfd2 = accept(sockfd, (struct sockaddr *)&in_addr2, &len2)) < 0){
            }
            printf("Connection accepted from %s %d\n",
                inet_ntoa(in_addr2.sin_addr), ntohs(in_addr2.sin_port));

            istringstream iss2(recieveMessage(newsockfd2));
            string command2;
            iss2 >> command2;

            if (command2 == "pluscontent") {
              printf("%s\n", iss2.str().c_str());
              iss2 >> last_content_id;
              peer p2 = get_peer(iss2);
              for (int i = 1; i < peers.size(); i++) {
                if (peers[i] == p2) {
                  peers[i].numContent = p2.numContent;
                }
              }
              sendMessage(newsockfd2, "done");
            }
          }
          newid = recieveMessage(sockid);
          close(sockid);
          you_got_dis = true;
          break;
        }
      }
      if (!you_got_dis) {
        printf("I got dis\n");
        newid = int_to_string(last_content_id);
        content[last_content_id++] = newcontent;
        peers[0].numContent++;
        for (int i = 1; i < peers.size(); i++) {
          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[i].ip, peers[i].port);
          string cmd = "pluscontent ";
          cmd.append(int_to_string(last_content_id));
          cmd.append(" ");
          cmd.append(peers[0].ip);
          cmd.append(" ");
          cmd.append(int_to_string(peers[0].port));
          cmd.append(" ");
          cmd.append(int_to_string(peers[0].numContent));
          sendMessage(sockid, cmd);
          recieveMessage(sockid);
          close(sockid);
        }
      }
      for (map<unsigned int, string>::iterator pair = content.begin(); pair != content.end(); pair++) {
        printf("%d => %s\n", pair->first, pair->second.c_str());
      }
      sendMessage(newsockfd, newid);
    }

    if (command == "newcontent") {
      printf("%s\n", iss.str().c_str());
      string newcontent;
      getline(iss, newcontent);
      newcontent = newcontent.substr(1);
      content[last_content_id++] = newcontent;
      peers[0].numContent++;
      for (int i = 1; i < peers.size(); i++) {
        int sockid = socket(AF_INET, SOCK_STREAM, 0);
        connectToPeer(sockid, peers[i].ip, peers[i].port);
        string cmd = "pluscontent ";
        cmd.append(int_to_string(last_content_id));
        cmd.append(" ");
        cmd.append(peers[0].ip);
        cmd.append(" ");
        cmd.append(int_to_string(peers[0].port));
        cmd.append(" ");
        cmd.append(int_to_string(peers[0].numContent));
        sendMessage(sockid, cmd);
        recieveMessage(sockid);
        close(sockid);
      }
      for (map<unsigned int, string>::iterator pair = content.begin(); pair != content.end(); pair++) {
        printf("%d => %s\n", pair->first, pair->second.c_str());
      }
      sendMessage(newsockfd, "done");
    }

    if (command == "pluscontent") {
      printf("%s\n", iss.str().c_str());
      iss >> last_content_id;
      peer p = get_peer(iss);
      for (int i = 1; i < peers.size(); i++) {
        if (peers[i] == p) {
          peers[i].numContent = p.numContent;
        }
      }
      sendMessage(newsockfd, "done");
    }

    if (command == "removecontent") {
      printf("%s\n", iss.str().c_str());
      unsigned int id;
      iss >> id;
      if (!content.count(id)) {
        bool exists = false;
        string cmd = "deletecontent ";
        cmd.append(int_to_string(id));
        for (int i = 1; i < peers.size(); i++) {
          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[i].ip, peers[i].port);
          sendMessage(sockid, cmd);
          {
            struct sockaddr_in in_addr2;
            socklen_t len2 = sizeof(struct sockaddr_in);
            printf("Starting to accept\n");

            int newsockfd2;
            if ((newsockfd2 = accept(sockfd, (struct sockaddr *)&in_addr2, &len2)) < 0){
            }
            printf("Connection accepted from %s %d\n",
                inet_ntoa(in_addr2.sin_addr), ntohs(in_addr2.sin_port));

            istringstream iss2(recieveMessage(newsockfd2));
            string command2;
            iss2 >> command2;

            if (command2 == "numcontent") {
              printf("%s\n", iss.str().c_str());
              peer p = get_peer(iss);
              for (int i = 1; i < peers.size(); i++) {
                if (peers[i] == p) {
                  peers[i].numContent = p.numContent;
                  break;
                }
              }
              sendMessage(newsockfd, "done");
            }
          }

          if ("yee boi" == recieveMessage(sockid)) {
            exists = true;
          }
          close(sockid);
          if (exists) break;
        }
        if (exists) {
          sendMessage(newsockfd, "done");
        } else {
          sendMessage(newsockfd, "nexist");
        }
      } else {
        content.erase(id);
        int numContent = peers[0].numContent - 1;
        for (int i = 1; i < peers.size(); i++) {
          if (peers[i].numContent - peers[0].numContent == 2) {
            int sockid = socket(AF_INET, SOCK_STREAM, 0);
            connectToPeer(sockid, peers[i].ip, peers[i].port);
            sendMessage(sockid, "needcontent");
            unsigned int newid;
            string newcontent;
            istringstream newss(recieveMessage(sockid));
            newss >> newid >> newcontent;
            content[newid] = newcontent;
            numContent++;
            sendMessage(sockid, "done");
            close(sockid);
            break;
          }
        }
        if (numContent != peers[0].numContent) {
          string cmd = "numcontent";
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
        }
        sendMessage(newsockfd, "done");
      }
    }

    if (command == "deletecontent") {
      printf("%s\n", iss.str().c_str());
      unsigned int id;
      iss >> id;
      if (content.count(id)) {
        content.erase(id);
        peers[0].numContent--;
        for (int i = 1; i < peers.size(); i++) {
          if (peers[i].numContent - peers[0].numContent == 2) {
            int sockid = socket(AF_INET, SOCK_STREAM, 0);
            connectToPeer(sockid, peers[i].ip, peers[i].port);
            sendMessage(sockid, "needcontent");
            unsigned int newid;
            string newcontent;
            istringstream newss(recieveMessage(sockid));
            newss >> newid >> newcontent;
            content[newid] = newcontent;
            peers[0].numContent++;
            sendMessage(sockid, "done");
            close(sockid);
            break;
          }
        }
        string cmd = "numcontent";
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
        sendMessage(newsockfd, "yee boi");
      } else {
        sendMessage(newsockfd, "naw b");
      }
    }

    if (command == "lookupcontent") {
      printf("%s\n", iss.str().c_str());
      unsigned int id;
      iss >> id;
      if (content.count(id)) {
        string cmd = "y ";
        cmd.append(content[id]);
        sendMessage(newsockfd, cmd);
      } else {
        sendMessage(newsockfd, "n");
      }
    }

    if (command == "needcontent") {
      printf("%s\n", iss.str().c_str());
      map<unsigned int, string>::iterator it = content.begin();
      unsigned int id = it->first;
      string c = it->second;
      content.erase(it);
      string msg = int_to_string(id);
      msg.append(" ");
      msg.append(c);
      sendMessage(newsockfd, msg);
    }

    if (command == "numcontent") {
      printf("%s\n", iss.str().c_str());
      peer p = get_peer(iss);
      for (int i = 1; i < peers.size(); i++) {
        if (peers[i] == p) {
          peers[i].numContent = p.numContent;
          break;
        }
      }
      sendMessage(newsockfd, "done");
    }

    close(newsockfd);
  }
//} else {

//}
  return 0;
}
