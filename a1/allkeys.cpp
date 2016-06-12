#include <stdlib.h>
#include <string.h>
#include <vector>
#include <sstream>
#include "core.cpp"

using namespace std;

int main(int argc, char* argv[]) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connectToPeer(sockfd, argv[1], atoi(argv[2])) < 0)
		printf("Error: no such peer\n");
  sendMessage(sockfd, "allkeys");
  prinft("%s\n", recieveMessage(sockfd).c_str());

	return 0;
}
