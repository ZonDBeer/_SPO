#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFLEN 65000

int childWork(int sockClient) {
	FILE *file;
	int length;
	char buffer[BUFLEN];
	char filename[20];
	char path[100];

	bzero(filename, 20);
	bzero(buffer, BUFLEN);
	bzero(path, 100);

	recv(sockClient, filename, 20, NULL);

	send(sockClient, "ok", 2, NULL);

	strncat(path, "/home/tor/Dropbox/__Masters/_SPO/_lab3/", strlen("/home/tor/Dropbox/__Masters/_SPO/_lab3/"));
	strncat(path, filename, strlen(filename));
	file = fopen(path, "ab");
	while(length = recv(sockClient, buffer, BUFLEN, NULL)) {
		fwrite(buffer, 1, length, file);
	}
	fclose(file);
	printf("<file %s correctly recv>\n", filename);
}

int papaWork(int sockServ) {
	FILE *file;
	struct sockaddr_in clientAddr;
	int i, j, size, lengthMsg, lengthAddr;
	int countDgram[70000][2];
	char buffer[BUFLEN], blocknum[10], packetnum[10];
	char filename[20];
	char path[100];
	for (i = 0; i < 70000; ++i) {
		countDgram[i][0] = 0;
		countDgram[i][1] = 0;
	}

	for ( ; ; ) {
		//printf("Cycle.\n");
		bzero(buffer, BUFLEN);
		bzero(filename, 20);
		bzero(blocknum, 10);
		bzero(path, 100);

		lengthAddr = sizeof(clientAddr);
		if ((lengthMsg = recvfrom(sockServ, buffer, BUFLEN, NULL,
			&clientAddr, &lengthAddr)) < 0) {
			perror ("bad client sock");
			exit(1);
		}

		// Считываем имя файла
		for(i = 0; i < BUFLEN; ++i) {
			if (buffer[i] != '#') {
				filename[i] = buffer[i];
			} else {
				break;
			}
		}

		// Считываем количество блоков
		for(++i, j = 0; i < BUFLEN; ++i, ++j) {
			if (buffer[i] != '!') {
				blocknum[j] = buffer[i];
			}	
			if (buffer[i] == '#') {
				// индекс - порт, (0) - количество, (1) - счетчик
				if (countDgram[clientAddr.sin_port][0] == 0) {
					size = atoi(blocknum);
					//printf("SIZE: %d\n", size);
					countDgram[clientAddr.sin_port][0] = size;
				}
				break;
			}
		}

		bzero(packetnum, 10);
		for(++i, j = 0; i < BUFLEN; ++i, ++j) {
			if (buffer[i] != '!') {
				packetnum[j] = buffer[i];
			}
			if (buffer[i] == '#') {
				break;
			}
		}

		if ((atoi(packetnum) == countDgram[clientAddr.sin_port][1] + 1)) {
			strncat(path, "/home/tor/Dropbox/__Masters/_SPO/_lab3/", strlen("/home/tor/Dropbox/__Masters/_SPO/_lab3/"));
			strncat(path, filename, strlen(filename));
			file = fopen(path, "ab");
			for(++i; i < BUFLEN; ++i) {
				fwrite(&buffer[i], sizeof(char), 1, file);
			}
			fclose(file);
			
			sendto(sockServ, "ok", 2, 0, &clientAddr, sizeof(clientAddr));
			++countDgram[clientAddr.sin_port][1];

			// Проверка на окончание сохранения файла
			if (countDgram[clientAddr.sin_port][1] == countDgram[clientAddr.sin_port][0]) {
				countDgram[clientAddr.sin_port][0] = 0;
				countDgram[clientAddr.sin_port][1] = 0;
				printf("<file %s correctly recv>\n", filename);
			}
		} else {
			// Что-то пошло не так
			sendto(sockServ, "no", 2, 0, &clientAddr, sizeof(clientAddr));
			printf("<rerequest packet %d>\n", atoi(packetnum));
		}
	}
}

int main(int argc, char **argv) {
	int sockTCP, sockUDP, sockClient;
	int lengthAddr, result, child;

	printf("%s\n", "start server...");
	printf("%s", "initialization tcp socket...");

	sockTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (sockTCP < 0) {
		perror("TCP error 1\n");
		exit (1);
	}
	struct sockaddr_in tcp_Addr, udp_Addr, clientAddr;
	
	tcp_Addr.sin_family = AF_INET;
	tcp_Addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	tcp_Addr.sin_port = 0;

	if (bind(sockTCP, &tcp_Addr, sizeof(tcp_Addr))) {
		perror("TCP error 2\n");
		exit(1);
	}

	lengthAddr = sizeof(tcp_Addr);
	if (getsockname(sockTCP, &tcp_Addr, &lengthAddr)) {
		perror("TCP error 3\n");
		exit(1);
	}
	printf("done\ntcp_port: %d\n", ntohs(tcp_Addr.sin_port));
	printf("tcp_ip: %s\n", inet_ntoa(tcp_Addr.sin_addr));
	listen(sockTCP, 10);

	printf("%s", "initialization udp socket...");

	sockUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockUDP < 0) {
		perror("UDP error 1\n");
		exit (1);
	}
	udp_Addr.sin_family = AF_INET;
	udp_Addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	udp_Addr.sin_port = 0;

	if (bind(sockUDP, &udp_Addr, sizeof(udp_Addr))) {
		perror("[UDP error 2\n");
		exit(1);
	}

	lengthAddr = sizeof(udp_Addr);
	if (getsockname(sockUDP, &udp_Addr, &lengthAddr)) {
		perror("UDP error 3\n");
		exit(1);
	}
	printf("done\nudp_port: %d\n", ntohs(udp_Addr.sin_port));
	printf("udp_ip: %s\n", inet_ntoa(udp_Addr.sin_addr));
	
	
	// Заполняем множество сокетов
	fd_set readset;
	// Инициализируем readset
	FD_ZERO(&readset);
	// Задаем элемнты множества соответствующего файловому дескриптору
	FD_SET(sockTCP, &readset);

    struct timeval timeout;
    timeout.tv_sec = 1000;
    timeout.tv_usec = 0;

    if ((child = fork()) < 0) {
			perror("error : fork UDP\n");
			exit (1);
		} else if (child == 0) {
			papaWork(sockUDP);
			close(sockClient);
			close(sockTCP);
			close(sockUDP);
			exit(0);
	}

	for ( ; ; ) {
	  // FD_ISSET возвращает ненулевое значение, если файловый дескриптор, на который ссылается fd, является элементом структуры fd_set, на которую указывает параметр fdset
		if (result = select(FD_SETSIZE, &readset, (fd_set *)NULL, 
			(fd_set*)NULL, &timeout) < 0) {
			perror("error - select\n");
			exit (1);
		}

		if (FD_ISSET(sockTCP, &readset))
        {
            // Поступил новый запрос на соединение, используем accept
            lengthAddr = sizeof(clientAddr);
            sockClient = accept(sockTCP, &clientAddr, &lengthAddr);

            if(sockClient < 0)
            {
                perror("isset tcp\n");
                exit(1);
            }
        }
	
		if ((child = fork()) < 0) {
			perror("error 5\n");
			exit (1);
		} else if (child == 0) {
			childWork(sockClient);
			close(sockTCP);
			close(sockUDP);
			close(sockClient);
			exit(0);
		}
		close(sockClient);
	}

	return 0;
}
