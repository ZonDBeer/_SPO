#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFLEN 65000

int main(int argc, char **argv) {
	FILE *file;
	int sock, i;
	char data[BUFLEN];
	char name[20];
	struct sockaddr_in addr;
	printf("%s", "connect to server...");
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <0) {
			perror("error 1");
			exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));
	if (connect(sock, &addr, sizeof(addr)) < 0) {
		perror("error 2");
		exit(1);
	}
	printf("%s\n", "done");
	printf("%s\n", "send file...");
	
	file = fopen(argv[3], "rb");

	bzero(data, BUFLEN);
	bzero(name, 20);
	strncat(name, argv[4], strlen(argv[4]));
	name[strlen(argv[4])] = '\0';
	printf("%s\n", name);
	
	send(sock, name, strlen(name)+1, 0);
	recv(sock, data, 2, 0);
	while(!(feof(file))) {
		for(i = 0; i < BUFLEN; ++i) {
            if (!(feof(file))) {
                fread(&data[i], 1, 1, file);
            } else {
                --i;
                break;
            }
        }
		//printf("%s\n", data);
		if (send(sock, data, i, 0) < 0) {
			perror("error 3");
			exit(1);
		}
	}
	close(sock);
	close(file);
	printf("%s\n", "done");
	return 0;
}
