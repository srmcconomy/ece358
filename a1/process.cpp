#define NEEDCONTENT 420


int handle_message(const int sockfd, vector<peer>& peers, map<unsigned int, string>& content, unsigned int& last_content_id) {
  struct sockaddr_in in_addr;
  socklen_t len = sizeof(struct sockaddr_in);
  printf("Starting to accept\n");

  int newsockfd;
  if ((newsockfd = accept(sockfd, (struct sockaddr *)&in_addr, &len)) < 0){
    return 0;
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

      return 1;

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
        handle_message(sockfd, peers, content, last_content_id);
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
    string newid = int_to_string(last_content_id);
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
    sendMessage(newsockfd, newid);
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
        if (handle_message(sockfd, peers, content, last_content_id) == NEEDCONTENT) {
          handle_message(sockfd, peers, content, last_content_id);
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
      peers[0].numContent--;
      for (int i = 1; i < peers.size(); i++) {
        if (peers[i].numContent - peers[0].numContent == 2) {
          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[i].ip, peers[i].port);
          sendMessage(sockid, "needcontent");
          handle_message(sockfd, peers, content, last_content_id);
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
      printf("%d\n", peers[0].numContent);
      for (int i = 1; i < peers.size(); i++) {
        printf("%d\n", peers[i].numContent);
        if (peers[i].numContent - peers[0].numContent >= 2) {
          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[i].ip, peers[i].port);
          sendMessage(sockid, "needcontent");
          handle_message(sockfd, peers, content, last_content_id);
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
      sendMessage(newsockfd, "yee boi");
    } else {
      sendMessage(newsockfd, "naw b");
    }
  }

  if (command == "lookupcontent") {
    printf("%s\n", iss.str().c_str());
    unsigned int id;
    iss >> id;
    string ret;
    if (content.count(id)) {
      ret = content[id];
    } else {
      string cmd = "getcontent ";
      cmd.append(int_to_string(id));
      for (int i = 1; i < peers.size(); i++) {
        int sockid = socket(AF_INET, SOCK_STREAM, 0);
        connectToPeer(sockid, peers[i].ip, peers[i].port);
        sendMessage(sockid, cmd);
        istringstream newss(recieveMessage(sockid));
        close(sockid);
        string c;
        newss >> c;
        if (c == "done") {
          getline(newss, ret);
          ret = ret.substr(1);
          break;
        }
      }
    }
    if (!ret.empty()) {
      string cmd = "done ";
      cmd.append(ret);
      sendMessage(newsockfd, cmd);
    } else {
      sendMessage(newsockfd, "nexist");
    }

  }

  if (command == "getcontent") {
    printf("%s\n", iss.str().c_str());
    unsigned int id;
    iss >> id;
    string ret;
    if (content.count(id)) {
      string cmd = "done ";
      cmd.append(content[id]);
      sendMessage(newsockfd, cmd);
    } else {
      sendMessage(newsockfd, "nexist");
    }
  }

  if (command == "needcontent") {
    printf("%s\n", iss.str().c_str());
    map<unsigned int, string>::iterator it = content.begin();
    unsigned int id = it->first;
    string c = it->second;
    content.erase(it);
    peers[0].numContent--;
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
  if (command == "needcontent") return NEEDCONTENT;
  return 0;
}
