#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFLEN 65000

int childWork(int sockClient) {
	int length;
	FILE *file;
	char buffer[BUFLEN];
	char filename[20];
	char path[100];

	bzero(filename, 20);
	bzero(buffer, BUFLEN);
	bzero(path, 100);
	
	recv(sockClient, filename, 20, NULL);
	send(sockClient, "ok", 2, NULL);
	printf("%s\n", filename);
	strncat(path, "/home/tor/Dropbox/__Masters/_SPO/_lab2/", strlen("/home/tor/Dropbox/__Masters/_SPO/_lab2/"));
	strncat(path, filename, strlen(filename));
	file = fopen(path, "ab");
	while(length = recv(sockClient, buffer, BUFLEN, NULL)) {
		//printf("%s\n", buffer);
		fwrite(buffer, 1, length, file);
		//fprintf(file, "%s", buffer);
	}
	fclose(file);
	printf("fork finished\n");
}

int main(int argc, char **argv) {
	int sockServ, sockClient, lengthAddr, child;

	printf("%s", "start server...");

	sockServ = socket(AF_INET, SOCK_STREAM, 0);
	if (sockServ < 0) {
		perror("[fail] - e1\n");
		exit (1);
	}
	struct sockaddr_in servAddr, clientAddr;
	
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAddr.sin_port = 0;

	if (bind(sockServ, &servAddr, sizeof(servAddr))) {
		perror("[fail] - e2\n");
		exit(1);
	}
	lengthAddr = sizeof(servAddr);
	if (getsockname(sockServ, &servAddr, &lengthAddr)) {
		perror("[fail] - e3\n");
		exit(1);
	}
	printf("[done]\nserver port: %d\n", ntohs(servAddr.sin_port));
	printf("server ip: %s\n", inet_ntoa(servAddr.sin_addr));
	printf("waiting for clients...\n");
	listen(sockServ, 10);
	
	for ( ; ; ) {
		if ((sockClient = accept(sockServ, 0, 0)) < 0) {
			perror("[fail] - e4\n");
			exit (1);
		}		
		if ((child = fork()) < 0) {
			perror("[fail] - e5\n");
			exit (1);
		} else if (child == 0) {
			close(sockServ);
			childWork(sockClient);
			close(sockClient);
			exit(0);
		}
		close(sockClient);
	}

	return 0;
}
