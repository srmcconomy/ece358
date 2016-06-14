#include <sstream>
#include <string>
#include <vector>
using namespace std;

struct peer {
  string ip;
  int port;
  int numContent;
};

bool operator==(const peer& lhs, const peer& rhs)
{
    if(lhs.ip == rhs.ip && lhs.port == rhs.port) {
    	return true;
    }
    return false;
}

string int_to_string(int num) {
  ostringstream ss;
  ss << num;
  return ss.str();
}

peer get_peer(istringstream& iss) {
  string ip;
  int port;
  int numContent = 0;
  iss >> ip >> port >> numContent;
  peer truth = {ip, port, numContent};
  return truth;
}

string list_of_peers(vector<peer> peers, int last_content_id) {
  string all_peers = int_to_string(peers.size());
  all_peers.append(" ");
  for(int i = 0; i < peers.size(); i++){
    all_peers.append(peers[i].ip);
    all_peers.append(" ");
    all_peers.append(int_to_string(peers[i].port));
    all_peers.append(" ");
    all_peers.append(int_to_string(peers[i].numContent));
    all_peers.append(" ");
  }
  all_peers.append(int_to_string(last_content_id));
  return all_peers;
}
