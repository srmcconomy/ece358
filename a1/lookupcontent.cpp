#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include "core.cpp"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Usage: %s ip port key", argv[0]);
    return -1;
  }

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connectToPeer(sockfd, argv[1], atoi(argv[2])) < 0)
		printf("Error: no such peer\n");

	string command = "lookupcontent ";

	printf("addcontent: %s\n", argv[3]);
	command.append(argv[3]);
	sendMessage(sockfd, command);
  istringstream iss(recieveMessage(sockfd));
  iss >> command;
  if (command == "nexist") {
    printf("Error: no such content\n");
  } else {
    string s;
    getline(iss, s);
    printf("%s\n", s.substr(1).c_str());
  }

	return 0;
}
