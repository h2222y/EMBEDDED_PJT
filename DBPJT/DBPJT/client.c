#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

const char *IP = "";
const char *PORT = "";

pthread_t send_tid;
pthread_t receive_tid;
int sock;

void *receiveMsg() {
	char buf[100];

	while(1) {
		memset(buf, 0, 100);
		int len = read(sock, buf, 99);
		if (len == 0) {
			printf("INFO :: Server Disconnected\n");
			kill(0, SIGINT);
			break;
		}

		printf("%s (server)\n", buf);
	}
}

void *sendMsg() {
	char str[100];
	sock = 0;

	while(1) {
		printf("SHELL > ");
		fgets(str, 100, stdin);
		
		str[strlen(str) - 1] = '\0';

		if (strlen(str) == 0) {
			continue;
		}
		else if (!strcmp(str, "exit")){
			write(sock, str, strlen(str));
			break;
		}
		else if (!strcmp(str, "connect")) {
			if (sock != 0) {	
				printf("INFO :: Already connected\n");
				continue;
			}

			//Socket Create
			sock = socket(PF_INET, SOCK_STREAM, 0);
			if (sock == -1) {
				printf("ERROR :: 1_Socket Create Error\n");
				exit(1);
			}

			struct sockaddr_in addr = {0};
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(IP);
			addr.sin_port = htons(atoi(PORT));
		
			//Connect
			if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
				printf("ERROR :: 2_Connect Error\n");
				exit(1);
			}
	
			pthread_create(&receive_tid, NULL, receiveMsg, NULL);
		}
		else if (!strncmp(str, "save ", 5) && strlen(str) >= 8) {
			if (sock == 0) {
				printf("INFO :: PLEASE, CONNECT WITH SERVER\n");
				continue;
			}
			
			char buf[100] = "save_";
			strcat(buf, &str[5]);
			
			write(sock, buf, strlen(buf));
		}
		else if (!strncmp(str, "load ", 5) && strlen(str) >= 6) {
			if (sock == 0) {
				printf("INFO :: PLEASE, CONNECT WITH SERVER\n");
				continue;
			}
			char buf[100] = "load_";
			strcat(buf, &str[5]);
				
			write(sock, buf, strlen(buf));
			usleep(100 * 1000);
		}
		else if (!strcmp(str, "clear")) {
			if (sock == 0) {
				printf("INFO :: PLEASE, CONNECT WITH SERVER\n");
				continue;
			}
		}
		else if (!strcmp(str, "close")) {
			if (sock == 0) {
				printf("INFO :: PLEASE, CONNECT WITH SERVER\n");
				continue;
			}
			
			pthread_cancel(receive_tid);
			pthread_join(receive_tid, 0);
			close(sock);
			sock = 0;
		}
		else {
			printf("INFO :: Wrong Command\n");
		}
	}
}

void interrupt(int arg)
{
	printf("\nYou typped Ctrl + C\n");
	printf("Bye\n");

	pthread_cancel(send_tid); //force exit
	pthread_cancel(receive_tid); //force exit

	pthread_join(send_tid, 0); //wait + clean
	pthread_join(receive_tid, 0); //wait + clean

	close(sock);
	exit(1);
}

int main()
{
	signal(SIGINT, interrupt);

	//Thread Run
	pthread_create(&send_tid, NULL, sendMsg, NULL);

	pthread_join(send_tid, 0);
	pthread_join(receive_tid, 0);
	
	//close sock
	close(sock);

	return 0;
}