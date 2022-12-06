#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define MAXSIZE 6
char *msg1 = "my";
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int point = 0;
bool checkPoint = true;
bool nonBlock = true;
void* checkingChild0Msg(int pip[2]);
void* checkingChild1Msg(int pip[2]);
void* sendMsgCtoP(int pip[2]);
int child(int pip[2],int resultp[2]);
void* recvMsgPtoC(int resP[2]);
void* sendMsgCtoP1(int pip[2]);
int child1(int pip[2],int resultp[2]);
void* recvMsgPtoC1(int resP[2]);
int parent(int pip[2][2],int resultp[2][2]);
pid_t child0Pid;
pid_t child1Pid;
struct timespec begin, middle, end;


void pipetest(){
	int i;
	int pip[2][2];
	int resultp[2][2];

	for(i =0; i<2; i++){
		if(pipe(pip[i]) == -1){
			printf("pipe error\n");
			exit(0);
		}
		if(pipe(resultp[i]) == -1){
			printf("result pipe error \n");
			exit(0);
		}

		if(fcntl(pip[i][0],F_SETFL,O_NONBLOCK) == -1)
			printf("pip error\n");

		if(fcntl(resultp[i][0],F_SETFL,O_NONBLOCK) == -1)
			printf("resultp error\n");
		printf("pipe create%d\n",i);

	}
	switch(child0Pid = fork()){
		case -1:
			printf("fork error!\n");
			exit(0);
		case 0:{
			       switch(child1Pid = fork()){
				       case -1:
					       printf("fork error2!\n");
					       exit(0);
				       case 0:{
						      child(pip[0],resultp[0]);
						      break;
					      }
				       default:{
						       child1(pip[1],resultp[1]);
						       break;
					       }
			       }

			       break;
		       }
		default:{
				parent(pip,resultp);
			}
	}

}

void* checkingChild0Msg(int pip[2]){
	int count =0;
	char buf[MAXSIZE];
	for(;;){
		pthread_mutex_lock(&lock);
		if(checkPoint && (point < 3 && point > -3)){
			if(read(pip[0],buf,MAXSIZE)>0){
				point++;
				printf("Child0 point = %d\n",point);
			}
			pthread_mutex_unlock(&lock);
			sleep(0.0000001);
		}
		else{
			pthread_mutex_unlock(&lock);
			pthread_exit(NULL);
		}
	}
}

void* checkingChild1Msg(int pip[2]){
	char buf[MAXSIZE];
	for(;;){
		pthread_mutex_lock(&lock);

		if(checkPoint && (point < 3 && point > -3)){
			if(read(pip[0],buf,MAXSIZE)>0){
				point--;
				printf("Child1 point = %d\n",point);
			}
			/*else{
			  printf("error read 1\n");
			  pthread_exit(NULL);
			  }
			  */
			pthread_mutex_unlock(&lock);
			sleep(0.0000001);
		}
		else{
			pthread_mutex_unlock(&lock);
			pthread_exit(NULL);
		}
	}
}
void* managePoint(int resultp[2][2]){
	char *buf1 = "Winner";
	char *buf2 = "Loser1";

	for(;;){
		pthread_mutex_lock(&lock);
		if(point >= 3 || point <= -3){
			printf("finsih\n");
			pthread_mutex_unlock(&lock);
			checkPoint = false;
			if(point >=3){
				write(resultp[0][1],buf1,MAXSIZE);
				write(resultp[1][1],buf2,MAXSIZE);
				printf("끝남\n");
			}
			else{
				write(resultp[0][1],buf2,MAXSIZE);
				write(resultp[1][1],buf1,MAXSIZE);
				printf("끈남2\n");
			}
			pthread_exit(NULL);
		}
		pthread_mutex_unlock(&lock);
	}
}
int parent(int pip[2][2],int resultp[2][2]){
	int i;
	pthread_t c1thread;
	pthread_t c2thread;
	pthread_t managethread;
	int thread1;
	int thread2;
	int thread3;
	void* t1;
	void* t2;
	void* t3;
	int ret1;
	int ret2;
	int ret3;
	int status1;
	int status2;
	int *p1;
	int *p2;
	p1 = pip[0];
	p2 = pip[1];
	printf("parent process start\n");


	thread1 = pthread_create(&c1thread,NULL,checkingChild0Msg,p1);
	thread2 = pthread_create(&c2thread,NULL,checkingChild1Msg,p2);
	thread3 = pthread_create(&managethread,NULL,managePoint,resultp);

	pthread_join(&c1thread,&t1);
	pthread_join(&c2thread,&t1);
	pthread_join(&managethread,&t1);
	printf("finish parent\n");
}


void* sendMsgCtoP(int pip[2]){
	char buf[MAXSIZE];
	struct timespec del;
	for(;;){
		pthread_mutex_lock(&lock);
		if(checkPoint && (nonBlock)){
			clock_gettime(CLOCK_MONOTONIC, &del);
			double deltime = (del.tv_sec - middle.tv_sec) + (del.tv_nsec - middle.tv_nsec) / 1000000000.0;
			nonBlock = true;

			if(deltime > 0.6){
				printf("00000\n");
				write(pip[1],msg1,MAXSIZE);
				clock_gettime(CLOCK_MONOTONIC, &middle);
			}

			pthread_mutex_unlock(&lock);
		}
		else{
			pthread_mutex_unlock(&lock);
			pthread_exit(NULL);
		}
	}
}

void* recvMsgPtoC(int resP[2]){
	char buf[MAXSIZE];
	for(;;){
		printf("adsfkjlsd\n");
		pthread_mutex_lock(&lock);
		if(checkPoint && nonBlock == false){
			pthread_mutex_unlock(&lock);

			printf("before read\n");
			if(read(resP[0],buf,MAXSIZE) >0){
				pthread_mutex_lock(&lock);
				printf("Child0 %s \n",buf);
				checkPoint = false;
				pthread_mutex_unlock(&lock);
				pthread_exit(NULL);
			}
			sleep(1);
		}
		pthread_mutex_unlock(&lock);
	}

}

int child(int pip[2],int resultp[2]){
	int count;
	pthread_t p_send;
	pthread_t p_timer;
	pthread_t p_recv;
	int thread1;
	int timerCheck;
	int thread2;
	void* t2;
	void* t1;
	printf("child process start\n");
	clock_gettime(CLOCK_MONOTONIC,&middle);

	if(fcntl(resultp[0],F_SETFL,O_NONBLOCK) == -1)
		printf("error fcntl\n");

	thread1 =  pthread_create(&p_send,NULL,sendMsgCtoP,pip);
	//thread2 = pthread_create(&p_recv,NULL,recvMsgPtoC,resultp);

	pthread_join(&p_send,&t1);
	pthread_join(&p_recv,&t1);
	printf("child end\n");
}


void* sendMsgCtoP1(int pip[2]){
	char buf[MAXSIZE];
	struct timespec del;
	for(;;){
		pthread_mutex_lock(&lock);
		if(checkPoint){

			clock_gettime(CLOCK_MONOTONIC, &del);
			double deltime = (del.tv_sec - middle.tv_sec) + (del.tv_nsec - middle.tv_nsec) / 1000000000.0;
			if(deltime > 0.5){
				printf("1111\n");
				write(pip[1],msg1,MAXSIZE);
				clock_gettime(CLOCK_MONOTONIC, &middle);
			}


			pthread_mutex_unlock(&lock);
		}
		else{
			pthread_mutex_unlock(&lock);
			pthread_exit(NULL);
		}
	}
}

void* recvMsgPtoC1(int resP[2]){
	char buf[MAXSIZE];
	for(;;){
		printf("checkPoint = %d",checkPoint);
		pthread_mutex_lock(&lock);
		if(checkPoint){
			printf("before read11\n");
			pthread_mutex_unlock(&lock);

			if(read(resP[0],buf,MAXSIZE) >0){
				printf("Child1 %s \n",buf);
				pthread_mutex_lock(&lock);
				checkPoint = false;
				pthread_mutex_unlock(&lock);
				pthread_exit(NULL);
			}

		}
		pthread_mutex_unlock(&lock);
	}
}

int child1(int pip[2],int resultp[2]){
	int count;
	pthread_t p_send;
	pthread_t p_timer;
	pthread_t p_recv;
	int thread1;
	int timerCheck;
	int thread2;
	void* t2;
	void* t1;

	printf("child process start\n");
	clock_gettime(CLOCK_MONOTONIC,&middle);

	thread2 = pthread_create(&p_recv,NULL,recvMsgPtoC1,resultp);
	thread1 =  pthread_create(&p_send,NULL,sendMsgCtoP1,pip);

	pthread_join(&p_send,&t1);
	pthread_join(&p_recv,&t1);
	printf("child end\n");
}



