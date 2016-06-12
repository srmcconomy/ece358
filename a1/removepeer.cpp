#include <stdlib.h>
#include <string.h>
#include <vector>
#include "core.cpp"

using namespace std;

int main(int argc, char* argv[]) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connectToPeer(sockfd, argv[1], atoi(argv[2])) < 0)
		printf("Error: no such peer");

	string command = "removepeer ";
	command.append(argv[1]);
	command.append(" ");
	command.append(argv[2]);
	sendMessage(sockfd, command);

	if(recieveMessage(sockfd) != "done")
		printf("Error: no such peer");

	return 0;
}
