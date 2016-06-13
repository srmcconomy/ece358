#include <stdlib.h>
#include <string.h>
#include <vector>
#include "core.cpp"

using namespace std;

int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("Usage: %s ip port key\n", argv[0]);
		return -1;
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connectToPeer(sockfd, argv[1], atoi(argv[2])) < 0) {
		fprintf(stderr, "Error: no such peer\n");
		return -1;
	}

	string command = "removecontent ";
	command.append(argv[3]);
	sendMessage(sockfd, command);

	if(recieveMessage(sockfd) != "done") {
		fprintf(stderr, "Error: no such content\n");
		return -1;
	}

	return 0;
}
