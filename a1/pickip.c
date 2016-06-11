#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <ifaddrs.h>

/* Mahesh V. Tripunitara, ECE358, S'16 */
/* Cycle through IP addresses. Let user pick one. */
int pickServerIPAddr(struct in_addr *srv_ip) {
    if(srv_ip == NULL) return -1;
    bzero(srv_ip, sizeof(struct in_addr));

    struct ifaddrs *ifa;
    if(getifaddrs(&ifa) < 0) {
	perror("getifaddrs"); exit(-1);
    }

    char c;
    for(struct ifaddrs *i = ifa; i != NULL; i = i->ifa_next) {
	if(i->ifa_addr == NULL) continue;
	if(i->ifa_addr->sa_family == AF_INET) {
	    memcpy(srv_ip, &(((struct sockaddr_in *)(i->ifa_addr))->sin_addr), sizeof(struct in_addr));
            if(!strcmp(inet_ntoa(*srv_ip), "127.0.0.1")) continue;
	    freeifaddrs(ifa);
	    return 0;
	}
    }

    return -1;
}
