#include <stdlib.h>
#include <string.h>
#include <vector>
#include "core.cpp"

using namespace std;

int main(int argc, char* argv[]) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connectToPeer(sockfd, argv[1], atoi(argv[2])) < 0)
		printf("Error: no such peer\n");

	string command = "addcontent ";

	printf("addcontent: %s\n", argv[3]);
	command.append(argv[3]);
	sendMessage(sockfd, command);

	printf("%s\n", recieveMessage(sockfd).c_str());

	return 0;
}
