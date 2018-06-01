#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFLEN 65000

int main (int argc, char **argv) {

	int sock, i, size, addrLength, blockcount;
	struct sockaddr_in addr;
	char name[20], data[BUFLEN], blocknum[10], bcounter[10], answer[2], symbol;
	bzero(name, 20);
	bzero(answer, 2);
	bzero(data, BUFLEN);
	
	printf("%s", "connect to server...");
	if ((sock = socket(AF_INET, SOCK_DGRAM, NULL)) < 0) {
		perror("fail connect to server");
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


	FILE *file = fopen(argv[3], "rb");
	i = 0;
	while(!(feof(file))) {
		if (!(feof(file))) {
			fread(&symbol, 1, 1, file);
			++i;
		} else {
			break;
		}
	}
	fclose(file);
	size = i;



	file = fopen(argv[3], "rb");

	// Составляем имя файла, которые будет отсылаться с каждой датаграммой
	strncat(name, argv[4], strlen(argv[4]));
	strncat(name, "#", 1);
	strncat(data, name, strlen(name));

	// Составляем максимальное колличество блоков
	if (size % (BUFLEN - strlen(name) - 10 - 10) == 0) {
		size = size / (BUFLEN - strlen(name) - 10 - 10);
	} else {
		size = size / (BUFLEN - strlen(name) - 10 - 10) + 1;
	}
	
	bzero(blocknum, 10);
	snprintf(blocknum, 10, "%d", size);
	for (i = strlen(blocknum); i < 10; ++i) {
		if (i == 9) {
			blocknum[i] = '#';
		} else {
			blocknum[i] = '!';
		}
	}
	
	
	strncat(data, blocknum, strlen(blocknum));

	blockcount = 0;
	// Отсылка всех датаграмм
	while(blockcount < size) {
		strcpy(answer, "no");
		//printf("as: %s\n", answer);
		
		// Составяем номер пакета
		++blockcount;
		bzero(bcounter, 10);
		snprintf(bcounter, 10, "%d", blockcount);
		for (i = strlen(bcounter); i < 10; ++i) {
			if (i == 9) {
				bcounter[i] = '#';
			} else {
				bcounter[i] = '!';
			}
		}
		//printf("BCOUNTER: %s\n", bcounter);

		for (i = strlen(name) + 10;(i < BUFLEN); ++i) {
			data[i] = bcounter[i - (strlen(name) + 10)];
		}

		for(i = strlen(name) + 10 + 10; (i < BUFLEN); ++i) {
			if (!(feof(file))) {
				fread(&data[i], 1, 1, file);
			} else {
				--i;
				break;
			}
		}

		while (!(strcmp(answer, "no"))) {
			if (send(sock, data, i, 0) < 0) {
				perror("error 3");
				exit(1);
			}
			//printf("DATAGRAMM: %s\nlen: %d\n", data, strlen(data));
			addrLength = sizeof(sock);
			recvfrom(sock, answer, 2, NULL, &addr, &addrLength);
			if (!(strcmp(answer, "no"))) {
				printf("resend packet %d...\n", blockcount);
			}
		}
	}
	fclose(file);
	printf("%s\n", "send done");
	return 0;
}
