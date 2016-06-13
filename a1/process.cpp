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
      sendMessage(sockid, "needcontent 1");

      //handle the other peer's "numcontent" broadcast
      handle_message(sockfd, peers, content, last_content_id);

      unsigned int newid;
      unsigned int numcontent;
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
  printf("Starting to accept\n");

  int newsockfd;
  if ((newsockfd = accept(sockfd, (struct sockaddr *)&in_addr, &len)) < 0){
    return RETURN_NORMAL;
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
    // add peer to this peer's list of peers
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
      // offload contents to other peers
      if(peers.size() > 1) {
        int totalContent = 0;

        for (int i = 0 ; i < peers.size() ; i++) {
          totalContent += peers[i].numContent;
        }
        // remainder number of content after content is evenly ditributed amongst peers
        int remainder = totalContent % (peers.size()-1); 
        // ammount of content per other peer to be redistributed
        int contentPerPeer = (totalContent - remainder)/(peers.size()-1);

        std::map<unsigned int,string>::iterator it = content.begin(); 
        // send out content
        for(int i = 1 ; i < peers.size() ; i++) {
          string cmd = "givecontent ";
          int numcontentToSend = contentPerPeer;
          if(remainder > 0) {
            ++numcontentToSend;
            --remainder;
          }
          cmd.append(int_to_string(numcontentToSend));
          cmd.append(" ");
          for(int x = 0 ; x < numcontentToSend ; x++) {
            cmd.append(int_to_string(it -> first));
            cmd.append(" ");
            cmd.append(it -> second);
            cmd.append(" ");
            it++;
          }

          int sockid = socket(AF_INET, SOCK_STREAM, 0);
          connectToPeer(sockid, peers[i].ip, peers[i].port);
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
    printf("%s\n", list_of_peers(peers).c_str());
  }

  // Another peer wants to give us some content
  // the key does not change and the lastcontentid does not change
  // format 'givecontent [numcontent] [content 1 key] [content 1 value] [content 2 key] [content 2 value] ...'
  if (command == "givecontent") {
    printf("%s\n", iss.str().c_str());    
    unsigned int numcontent;
    iss >> numcontent;
    unsigned int key;
    string value;
    // read all the pieces of content given to this peer
    for (int i = 0 ; i < numcontent ; i++) {
      iss>>key;
      iss>>value; 
      content[key] = value;
      printf("");
    }
    // increment the ammount of content in this peer
    peers[0].numContent += numcontent;

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
    printf("%s\n", iss.str().c_str());
    string newcontent;
    getline(iss, newcontent);
    newcontent = newcontent.substr(1);
    printf("content to add: %s\n", newcontent.c_str());
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
      printf("I got dis\n");
      newid = add_new_content(newcontent, peers, content, last_content_id);
    }
    for (map<unsigned int, string>::iterator pair = content.begin(); pair != content.end(); pair++) {
      printf("%d => %s\n", pair->first, pair->second.c_str());
    }
    sendMessage(newsockfd, newid);
  }

  //Add content to this peer's list
  //format `newcontent [...content]`
  if (command == "newcontent") {
    printf("%s\n", iss.str().c_str());
    string newcontent;
    getline(iss, newcontent);
    newcontent = newcontent.substr(1);
    string newid = add_new_content(newcontent, peers, content, last_content_id);
    for (map<unsigned int, string>::iterator pair = content.begin(); pair != content.end(); pair++) {
      printf("%d => %s\n", pair->first, pair->second.c_str());
    }
    sendMessage(newsockfd, newid);
  }

  //notification that a peer now has a different number of content AND that the last_content_id has changed
  //format `pluscontent [last_content_id] [ip] [port] [numcontent]`
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

  //remove content from the network
  //format `removecontent [key]`
  if (command == "removecontent") {
    printf("%s\n", iss.str().c_str());
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
    printf("%s\n", iss.str().c_str());
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
    printf("%s\n", iss.str().c_str());
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
    printf("%s\n", iss.str().c_str());
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
  //format `needcontent [numcontent]`
  if (command == "needcontent") {
    printf("%s\n", iss.str().c_str());
    // check how much content is requested
    unsigned int numContentRequested;
    iss >> numContentRequested;

    cout<<"supplying content qty = "<<numContentRequested<<endl;
    cout<<"numcontent in peer = "<<peers[0].numContent<<endl;
    string contentTosend = "";
    std::map<unsigned int,string>::iterator it = content.begin();
    // while we still need to append content to send or we've reached the end of our content in this node
    for(int i = 0 ; i < numContentRequested; i++) {
      unsigned int toEraseInt = it->first;
      // load up content to send
      contentTosend.append(int_to_string(it-> first));
      contentTosend.append(" ");
      contentTosend.append(it->second);
      contentTosend.append(" ");
      // remove content from this peer
      content.erase(toEraseInt);
      it++;
    }
    
    // decrement the number of content in this peer
    peers[0].numContent -= numContentRequested;

    cout<<"supply content string: "<<contentTosend<<endl;
    cout<<"numcontent in peer = "<<peers[0].numContent<<endl;

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

    cout<<"sending back content"<<endl;
    // return the number of content and the kvp with key and values delimited by spaces
    sendMessage(newsockfd, contentTosend);
  }

  //a notification that a peer has changed its number of content
  //format `numcontent [ip] [port] [numcontent]`
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

  //format `allkeys`
  if (command == "allkeys") {
    string msg;
    for (map<unsigned int, string>::iterator pair = content.begin(); pair != content.end(); pair++) {
      msg.append(int_to_string(pair->first));
      msg.append(",");
    }
    msg.append("0");
    printf("%s\n", msg.c_str());
    sendMessage(newsockfd, msg);
  }

  cout<<"closing newsockfd"<<endl;
  close(newsockfd);
  if (command == "needcontent") return RETURN_NEEDCONTENT;
  return RETURN_NORMAL;
}
