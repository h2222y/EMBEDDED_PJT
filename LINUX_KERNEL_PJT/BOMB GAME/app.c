#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

int cnt=0;
int flag=0;
char buf[15];

void btn_first(int signum){ //btn1  click
	printf("1\n");
	cnt++;
	strcat(buf,"1");
}

void btn_second(int signum){ //btn2 click
	printf("2\n");
	cnt++;
	strcat(buf,"2");
}

int main()
{
	int pid; //pid signal
	signal(SIGIO, btn_first); //interrupt
	signal(SIGUSR1,btn_second);

    printf("OPEN FILE!!\n"); //open file 
    int fd = open("/dev/nobrand", O_RDWR);
    if (fd == -1) {
        printf("FILE OPEN ERROR!!!\n");
        close(fd);
        return 0;
    }

    //read
	pid = getpid();
	ioctl(fd,_IO(0,4),pid);
	for(int i=0;i<15;i++){
		ioctl(fd, _IO(0, 5), 0);
		usleep(250 * 1000);
		ioctl(fd, _IO(0, 6), 0);
		usleep(250 * 1000);
	if(cnt == 4) { //password ==1212
		if(!strcmp(buf, "1212")){
			flag = 1;
		}
			break;
		}
	}

	if(flag == 1){ // game complete
		printf("Complete!!!\n");
		ioctl(fd, _IO(0, 7), 0);
		sleep(2);
	}
	else{ // game over
		printf("Game Over!!!\n");
		ioctl(fd, _IO(0, 8), 0);
		sleep(2);
	}	
    printf("CLOSE FILE!!\n");
    close(fd); //close app

    return 0;
}
