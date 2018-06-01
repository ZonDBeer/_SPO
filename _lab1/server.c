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

int getrand(int min, int max)
{
    return (double)rand() / (RAND_MAX + 1.0) * (max - min) + min;
}

double wtime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1E-6;
}

int main(int argc, char **argv) {
	FILE *file;
	int sockServ, sockClient, lengthMsg, lengthAddr, i, j, size;
	int countDgram[70000][2];
	char buffer[BUFLEN], blocknum[10], packetnum[10];
	char filename[20];
	char path[100];

	srand(wtime(NULL));
	printf("%s", "start server...");
	/*
	AF_INET для сетевого протокола IPv4
	SOCK_STREAM (надёжная потокоориентированная служба (сервис) 
		или потоковый сокет)
	SOCK_DGRAM (служба датаграмм или датаграммный сокет)
	*/
	// Создаем главный поток управления пересылкой (TCB)
	sockServ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockServ < 0) {
		perror("[fail] - e1\n");
		exit (1);
	}

	/*struct sockaddr_in {
		short int          sin_family;  // Семейство адресов
		unsigned short int sin_port;    // Номер порта
		struct in_addr     sin_addr;    // IP-адрес
		unsigned char      sin_zero[8]; // "Дополнение" до размера структуры sockaddr
	};*/
	struct sockaddr_in servAddr, clientAddr; // структура для адреса
	
	servAddr.sin_family = AF_INET;
	// Предпологаем прием клиентских соединений от любых локальных IP адресов
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	// Указывая "0" ищем свободный порт
	//servAddr.sin_port = 0;
	servAddr.sin_port = 0;

	/*bind() Связывает сокет с конкретным адресом.
	Когда сокет создается при помощи socket(), он ассоциируется 
	с некоторым семейством адресов, но не с конкретным адресом. 
	До того как сокет сможет принять входящие соединения, 
	он должен быть связан с адресом. bind() принимает три аргумента:
	
	sockfd — дескриптор, представляющий сокет при привязке
	serv_addr — указатель на структуру sockaddr, представляющую адрес,
	к которому привязываем.
	addrlen — поле socklen_t, представляющее длину структуры sockaddr.*/
	if (bind(sockServ, &servAddr, sizeof(servAddr))) {
		perror("error 2\n");
		exit(1);
	}

	lengthAddr = sizeof(servAddr);
	if (getsockname(sockServ, &servAddr, &lengthAddr)) {
		perror("error 3\n");
		exit(1);
	}
	printf("done\nserver port: %d\n", ntohs(servAddr.sin_port));
	printf("server ip: %s\n", inet_ntoa(servAddr.sin_addr));
	
	for (i = 0; i < 70000; ++i) {
		countDgram[i][0] = 0;
		countDgram[i][1] = 0;
	}

	printf("waiting...\n");
	for ( ; ; ) {
		lengthAddr = sizeof(sockClient);
		bzero(buffer, BUFLEN);
		bzero(filename, 20);
		bzero(blocknum, 10);
		bzero(path, 100);

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
				//printf("PORT: %d\n", clientAddr.sin_port);
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

		if ((atoi(packetnum) == countDgram[clientAddr.sin_port][1] + 1) && (getrand(0, 6) < 3)) {

			strncat(path, "/home/tor/Dropbox/__Masters/_SPO/_lab1/", strlen("/home/tor/Dropbox/__Masters/_SPO/_lab1/"));
			strncat(path, filename, strlen(filename));
			file = fopen(path, "ab");
			for(++i; i < BUFLEN; ++i) {
				fwrite(&buffer[i], sizeof(char), 1, file);
			}
			fclose(file);
			
			printf("<client ip: %s, ", inet_ntoa(clientAddr.sin_addr));
			printf("port: %d, packet %d/%d>\n", clientAddr.sin_port, atoi(packetnum), atoi(blocknum));

			// Если был присла нужный кусок, то все в порядке
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
	return 0;
}
