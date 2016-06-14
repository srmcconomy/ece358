#define RETURN_NEEDCONTENT 300
#define RETURN_NORMAL 100
#define RETURN_ERROR -1
#define RETURN_SHUTDOWN 1

int handle_message(const int sockfd, vector<peer>& peers, map<unsigned int, string>& content, unsigned int& last_content_id);

//add some new content and notify everyone that we did
string add_new_content(string newcontent, vector<peer>& peers, map<unsigned int, string>& content, unsigned int& last_content_id) {
  string newid = int_to_string(last_content_id);
  content[last_content_id++] = newcontent;
  peers[0].numContent++;

  //tell everyone about my new toy
  string cmd = "pluscontent ";
  cmd.append(int_to_string(last_content_id));
  cmd.append(" ");
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
  return newid;
}

//remove content from the list and tell everyone
void remove_content(unsigned int id,vector<peer>& peers, map<unsigned int, string>& content, unsigned int& last_content_id, int sockfd) {
  content.erase(id);
  peers[0].numContent--;

  //if removing this piece of content means that someone has 2 more than me, I need more content
  for (int i = 1; i < peers.size(); i++) {
    if (peers[i].numContent - peers[0].numContent >= 2) {
      int sockid = socket(AF_INET, SOCK_STREAM, 0);
      connectToPeer(sockid, peers[i].ip, peers[i].port);
      // we need 1 content
      sendMessage(sockid, "needcontent");

      //handle the other peer's "numcontent" broadcast
      handle_message(sockfd, peers, content, last_content_id);

      unsigned int newid;
      unsigned int numcontent;
      string newcontent;
      istringstream newss(recieveMessage(sockid));
      newss >> newid;
      getline(newss, newcontent);
      newcontent = newcontent.substr(1);
      content[newid] = newcontent;
      peers[0].numContent++;
      sendMessage(sockid, "done");
      close(sockid);
      break;
    }
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
}

int handle_message(const int sockfd, vector<peer>& peers, map<unsigned int, string>& content, unsigned int& last_content_id) {
  struct sockaddr_in in_addr;
  socklen_t len = sizeof(struct sockaddr_in);

  int newsockfd;
  if ((newsockfd = accept(sockfd, (struct sockaddr *)&in_addr, &len)) < 0){
    return RETURN_NORMAL;
  }

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
    sendMessage(newsockfd, list_of_peers(peers, last_content_id));
    // add peer to this peer's list of peers
    peers.push_back(nakama);
  }

  // Adding to peer list
  if (command == "addpeer") {
    peer mate = get_peer(iss);
    peers.push_back(mate);

    sendMessage(newsockfd, "done");
  }

  // Removing peer from network
  if (command == "removepeer") {
    peer death = get_peer(iss);

    // Confirm that remove peer and this peer are the same
    if(death == peers[0]) {
      // offload contents to other peers
      if(peers.size() > 1) {

        std::map<unsigned int,string>::iterator it = content.begin();
        unsigned int contentToGive = peers[0].numContent;
        while(contentToGive > 0) {
          // find the poorest node
          unsigned int poorestNode = 1;
          // start at i = 1 so we don't include ourselves
          for (int i = 1 ; i < peers.size() ; i++) {
            if (peers[poorestNode].numContent > peers[i].numContent) {
              poorestNode = i;
            }
          }
          // assemble one content to give
          string cmd = "givecontent ";
          cmd.append(int_to_string(it -> first));
          cmd.append(" ");
          cmd.append(it -> second);
          cmd.append(" ");
          // move to the next peice of content 
          it++;
          --contentToGive;
          // send it
          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[poorestNode].ip, peers[poorestNode].port);
          sendMessage(sockid, cmd);

          // handle numcontent from other peer
          handle_message(sockfd, peers, content, last_content_id);

          recieveMessage(sockid);
          close(sockid);
        }

        // notify other peers that this one is dead
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
      }
      // Let user know we finished
      sendMessage(newsockfd, "done");

      if(shutdown(newsockfd, SHUT_RDWR) < 0) {
        perror("shutdown"); return RETURN_ERROR;
      }

      return RETURN_SHUTDOWN;

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
  }

  // Another peer wants to give us some content
  // the key does not change and the lastcontentid does not change
  // format 'givecontent [numcontent] [content 1 key] [content 1 value] [content 2 key] [content 2 value] ...'
  if (command == "givecontent") {
    unsigned int key;
    string value;
    // read the piece of content given to this peer
    iss>>key;
    getline(iss, value);
    value = value.substr(1);
    content[key] = value;
    // increment the ammount of content in this peer
    peers[0].numContent++;

    string cmd = "numcontent ";
    cmd.append(peers[0].ip);
    cmd.append(" ");
    cmd.append(int_to_string(peers[0].port));
    cmd.append(" ");
    cmd.append(int_to_string(peers[0].numContent));

    //broadcast to everyone the fact that I have more content meow
    for (int i = 1; i < peers.size(); i++) {
      int sockid = socket(AF_INET, SOCK_STREAM, 0);
      connectToPeer(sockid, peers[i].ip, peers[i].port);
      sendMessage(sockid, cmd);
      recieveMessage(sockid);
      close(sockid);
    }

    // return the number of content and the kvp with key and values delimited by spaces
    sendMessage(newsockfd, "done");
  }

  //Adding content to the network
  //format `addcontent [...content]`
  if (command == "addcontent") {
    string newid;
    string newcontent;
    getline(iss, newcontent);
    newcontent = newcontent.substr(1);
    bool you_got_dis = false;

    //check if any peers have less content
    for (int i = 1; i < peers.size(); i++) {
      //the first peer that has less content takes this new piece
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
    //if no peer took it then put it on this peer's list
    if (!you_got_dis) {
      newid = add_new_content(newcontent, peers, content, last_content_id);
    }

    sendMessage(newsockfd, newid);
  }

  //Add content to this peer's list
  //format `newcontent [...content]`
  if (command == "newcontent") {
    string newcontent;
    getline(iss, newcontent);
    newcontent = newcontent.substr(1);
    string newid = add_new_content(newcontent, peers, content, last_content_id);
    sendMessage(newsockfd, newid);
  }

  //notification that a peer now has a different number of content AND that the last_content_id has changed
  //format `pluscontent [last_content_id] [ip] [port] [numcontent]`
  if (command == "pluscontent") {
    iss >> last_content_id;
    peer p = get_peer(iss);
    for (int i = 1; i < peers.size(); i++) {
      if (peers[i] == p) {
        peers[i].numContent = p.numContent;
      }
    }
    sendMessage(newsockfd, "done");
  }

  //remove content from the network
  //format `removecontent [key]`
  if (command == "removecontent") {
    unsigned int id;
    iss >> id;
    //if this peer does not have the content
    if (!content.count(id)) {
      bool exists = false;
      string cmd = "deletecontent ";
      cmd.append(int_to_string(id));
      //tell all peers to delete this content
      for (int i = 1; i < peers.size(); i++) {
        int sockid = socket(AF_INET, SOCK_STREAM, 0);
        connectToPeer(sockid, peers[i].ip, peers[i].port);
        sendMessage(sockid, cmd);

        //Will recieve "naw b" if the peer does not have the content
        //Will recieve "1 sec" if the peer does
        if ("naw b" == recieveMessage(sockid)) {
          continue;
        }
        //if the peer does have the content, then we expect it to either broadcast "needcontent" or "numcontent"
        //so handle those
        if (handle_message(sockfd, peers, content, last_content_id) == RETURN_NEEDCONTENT) {
          handle_message(sockfd, peers, content, last_content_id);
        }

        //confirmation that the peer has finished
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
      remove_content(id, peers, content, last_content_id, sockfd);
      sendMessage(newsockfd, "done");
    }
  }

  //remove content from this peer's list
  //format `deletecontent [key]`
  if (command == "deletecontent") {
    unsigned int id;
    iss >> id;

    //if this peer has this content
    if (content.count(id)) {
      //tell the asker to chill while we broadcast info about our new stats
      sendMessage(newsockfd, "1 sec");
      remove_content(id, peers, content, last_content_id, sockfd);

      //k now we done
      sendMessage(newsockfd, "yee boi");
    } else {
      sendMessage(newsockfd, "naw b");
    }
  }

  //lookup content on the network
  //format `lookupcontent [key]`
  if (command == "lookupcontent") {
    unsigned int id;
    iss >> id;
    string ret;
    //if this peer has the content then just return it
    if (content.count(id)) {
      ret = content[id];
    } else {
      string cmd = "getcontent ";
      cmd.append(int_to_string(id));
      //ask all peers if they have the content
      for (int i = 1; i < peers.size(); i++) {
        int sockid = socket(AF_INET, SOCK_STREAM, 0);
        connectToPeer(sockid, peers[i].ip, peers[i].port);
        sendMessage(sockid, cmd);
        istringstream newss(recieveMessage(sockid));
        close(sockid);
        string c;
        newss >> c;
        //if the peer does have the content then we done
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

  //check if this peer has a specific piece of content
  //format `getcontent [key]`
  if (command == "getcontent") {
    unsigned int id;
    iss >> id;
    if (content.count(id)) {
      string cmd = "done ";
      cmd.append(content[id]);
      sendMessage(newsockfd, cmd);
    } else {
      sendMessage(newsockfd, "nexist");
    }
  }

  //another peer is asking for some content because it is less fortunate, luckily I have some extra
  //format `needcontent`
  if (command == "needcontent") {
    std::map<unsigned int,string>::iterator it = content.begin();
    // load up content to send
    string contentTosend = int_to_string(it-> first);
    contentTosend.append(" ");
    contentTosend.append(it->second);
    // remove content from this peer
    content.erase(it -> first);

    // decrement the number of content in this peer
    peers[0].numContent -= 1;

    string cmd = "numcontent ";
    cmd.append(peers[0].ip);
    cmd.append(" ");
    cmd.append(int_to_string(peers[0].port));
    cmd.append(" ");
    cmd.append(int_to_string(peers[0].numContent));

    //broadcast to everyone the fact that I have less content meow
    for (int i = 1; i < peers.size(); i++) {
      int sockid = socket(AF_INET, SOCK_STREAM, 0);
      connectToPeer(sockid, peers[i].ip, peers[i].port);
      sendMessage(sockid, cmd);
      recieveMessage(sockid);
      close(sockid);
    }

    // return the number of content and the kvp with key and values delimited by spaces
    sendMessage(newsockfd, contentTosend);
  }

  //a notification that a peer has changed its number of content
  //format `numcontent [ip] [port] [numcontent]`
  if (command == "numcontent") {
    peer p = get_peer(iss);
    for (int i = 1; i < peers.size(); i++) {
      if (peers[i] == p) {
        peers[i].numContent = p.numContent;
        break;
      }
    }
    sendMessage(newsockfd, "done");
  }

  //format `allkeys`
  if (command == "allkeys") {
    string msg;
    for (map<unsigned int, string>::iterator pair = content.begin(); pair != content.end(); pair++) {
      msg.append(int_to_string(pair->first));
      msg.append(",");
    }
    msg.append("0");
    sendMessage(newsockfd, msg);
  }

  close(newsockfd);
  if (command == "needcontent") return RETURN_NEEDCONTENT;
  return RETURN_NORMAL;
}
