#include <stdio.h> 
#include <signal.h> 
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <stdlib.h>
#include <sys/shm.h> 
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/sem.h>
#include<string.h>
#include <math.h>
#include <sys/times.h>
#include <limits.h>
#define CLK_TCK sysconf(_SC_CLK_TCK)

//shared memory key 등등 주요 요소 정의
#define SHMKEY1        (key_t)60021    
#define SHMKEY2       (key_t) 60022   
#define IFLAGS (IPC_CREAT |IPC_EXCL) 
#define ERR    ((struct databuf *)-1) 
#define SIZ 80



//점수및 함수 등  정의
int score = 0;
static int shmid1, shmid2;
struct timespec begint, middlet, endt;
clock_t tstart, tmiddle, tend;
struct tms mytms;
struct tms mytms2;

typedef struct databuf {
	int status; 
	int timerstatus;  //client가 준비 된 상태인지 확인+게임 종료 확인
	int value;         //client의 당기는 값 저장
	clock_t starttime; //서버가 받을 준비 된 상태인지 확인
	char d_buf[SIZ]; // 결과를 전달하기위한 char buffer
}databuf;

void sig_handler(int signo);
void* servertimer();
void* pull1Manage();
void* pull2Manage();
void* clienttimer1();
void* clienttimer2();
void* pull1();
void* pull2();
void* checkfinish1(); 
void* checkfinish2();
void getseg(databuf **p1, databuf **p2) { 
	if ((shmid1 = shmget (SHMKEY1, sizeof(databuf),  0600 |IFLAGS)) == -1) 
		exit(1); 
	if ((shmid2 = shmget (SHMKEY2, sizeof(databuf), 0600 |IFLAGS)) == -1) 
		exit(2); 

	if ((*p1 = (struct databuf *) shmat (shmid1, 0, 0)) == ERR) 
		exit(3); 
	if ((*p2 = (struct databuf *) shmat (shmid2, 0, 0)) == ERR) 
		exit(4); 
} 

void remobj (void){
	if (shmctl (shmid1, IPC_RMID, NULL) == -1)
		exit(9);
	if (shmctl (shmid2, IPC_RMID, NULL) == -1)
		exit(10);
}

struct databuf *buf1, *buf2;
pthread_rwlock_t g_rwLock;

int main(){
	signal(SIGINT, sig_handler);

	void* msg;

	pid_t pid1, pid2;
	int ret;
	getseg(&buf1, &buf2);
	buf1->status = 0;
	buf1->timerstatus = 0;
	buf2->status = 0;
	buf2->timerstatus = 0;
	ret = pthread_rwlock_init(&g_rwLock, NULL);
	if(ret !=0){
		printf("wrlock init error");		
	}


	//client 1 fork	
	switch (pid1 = fork()){
		case -1:
			printf("fork client1 error\n");
			exit(12);
		case 0:{	//client 2 fork
			       switch(pid2 = fork()){
				       case -1:
					       printf("fork client2 error");
					       exit(13);
				       case 0:{
						      //shared memory setting, client1 thread start
						      pthread_t time1_thread, pull1_thread, check1_thread;
						      buf1->timerstatus=1;
						      buf1->value=1;
						      buf2->timerstatus=1;
						      buf2->value=-1;
						      pthread_create(&time1_thread,NULL,clienttimer1,NULL);
						      pthread_create(&pull1_thread,NULL,pull1,NULL);
						      pthread_create(&check1_thread,NULL,checkfinish1,NULL);
						      pthread_join(time1_thread,&msg);
						      pthread_join(pull1_thread,&msg);
						      pthread_join(check1_thread,&msg);

						      break;
					      }
				       default:{
						       //client2 thread start
						       pthread_t time2_thread, pull2_thread, check2_thread;
						       pthread_create(&time2_thread,NULL,clienttimer2,NULL);
						       pthread_create(&pull2_thread,NULL,pull2,NULL);
						       pthread_create(&check2_thread,NULL,checkfinish2,NULL);
						       pthread_join(time2_thread,&msg);
						       pthread_join(pull2_thread,&msg);
						       pthread_join(check2_thread,&msg);
						       remobj();
						       break;

					       }
			       }	
			       break;
		       }
		default:{
				//server thread start
				pthread_t timer_thread,client1m_thread,client2m_thread;
				pthread_create(&client1m_thread,NULL,pull1Manage,NULL);
				pthread_create(&client2m_thread,NULL,pull2Manage,NULL);
				pthread_create(&timer_thread,NULL,servertimer,NULL);
				pthread_join(client1m_thread,&msg);
				pthread_join(client2m_thread,&msg);
				pthread_join(timer_thread,&msg);
				break;
			}
	}
	//server, client1, client2 종료 후 할당한 shared memory 제거
	printf("game end\n");
	remobj();
	exit(0);

}

//서버의 타이머 게임 시작 가능한지 확인 및 일정 score이상되면 종료하고 종료를 알림
void* servertimer(){
	double start,middle,end;
	struct timespec begins,middles,ends;
	//게임 시작 가능한지 확인ㅇ
	for(;;){
		if(buf1->timerstatus==1 && buf2->timerstatus==1){
			start = times(&mytms);
			middle = times(&mytms);
			clock_gettime(CLOCK_MONOTONIC, &begins);
			clock_gettime(CLOCK_MONOTONIC, &middles);
			buf1->starttime=start;
			buf2->starttime=start;
			printf("서버 시작시간 %f \n",(middles.tv_sec - begins.tv_sec) + (middles.tv_nsec - begins.tv_nsec) / 1000000000.0 );
			break;
		}
	}

	//점수를 readlock으로 확인하고unlock  특정 값 이상, 이하가 되면 종료후 종료를 client에게 알림
	for(;;){
		pthread_rwlock_rdlock(&g_rwLock);
		end =times(&mytms);
		clock_gettime(CLOCK_MONOTONIC, &ends);
		if(score>=10){
			printf("score = %d\n",score);
			strcpy(buf1->d_buf, "승리!!");
			strcpy(buf2->d_buf,"패배...." );
			buf1->timerstatus=2;
			buf2->timerstatus=2;
			printf("game 종료 걸린 시간: %f\n",(ends.tv_sec - begins.tv_sec) + (ends.tv_nsec - begins.tv_nsec) / 1000000000.0);
			pthread_rwlock_unlock(&g_rwLock);
			pthread_exit(NULL);
		}
		if(score<=-10){
			printf("score = %d\n",score);
			strcpy(buf1->d_buf, "패배....");
			strcpy(buf2->d_buf,"승리!!" );
			buf1->timerstatus=2;
			buf2->timerstatus=2;
			printf("game 종료 걸린 시간: %f\n",(ends.tv_sec - begins.tv_sec) + (ends.tv_nsec - begins.tv_nsec) / 1000000000.0);
			pthread_rwlock_unlock(&g_rwLock);
			pthread_exit(NULL);
		}
		pthread_rwlock_unlock(&g_rwLock);
	}
}

//client1에서 당겼는지 확인하고 writelock을 걸고 score값을 변경하고 unlock 특정값 이상,이하가 되면 종료함
void* pull1Manage(){
	for(;;){
		if(buf1->status == 1){
			pthread_rwlock_wrlock(&g_rwLock);
			if(score>=10 ||score<=-10){
				pthread_rwlock_unlock(&g_rwLock);
				pthread_exit(NULL);
			}

			score = score+ buf1->value;
                        printf("client 1 pull, score = %d\n", score);
			buf1->status = 0;
			printf("server changed buf1->status : %d\n",buf1->status);
			pthread_rwlock_unlock(&g_rwLock);
		}
		if(buf1->timerstatus==2){
                        pthread_exit(NULL);
                }
	}
}

//client2에서 당겼는지 확인하고writelock을 걸고 score값을 변경하고 unlock 특정값 이상,이하가 되면 종료함
void* pull2Manage(){
	for(;;){
		if(buf2->status == 1){
			pthread_rwlock_wrlock(&g_rwLock);
			if(score<=-10 ||score>10){
				pthread_rwlock_unlock(&g_rwLock);
				pthread_exit(NULL);
			}
			score = score+ buf2->value;
                        printf("client 2 pull, score = %d\n", score);
			buf2->status = 0;
			printf("server changed buf2->status : %d\n",buf2->status);
			pthread_rwlock_unlock(&g_rwLock);
                }
		if(buf2->timerstatus==2){
			pthread_exit(NULL);
		}
	}
}

//client1 기준 타이머
void* clienttimer1(){
	for(;;){
		if(buf1->timerstatus==1 && buf2->timerstatus==1){
			clock_gettime(CLOCK_MONOTONIC, &begint);
			clock_gettime(CLOCK_MONOTONIC, &middlet);
			printf("client1 시작시간 %f \n",(middlet.tv_sec - begint.tv_sec) + (middlet.tv_nsec - begint.tv_nsec) / 1000000000.0  );
			pthread_exit(NULL);
		}
	}
}	

//client2 기준 타이머
void* clienttimer2(){
	for(;;){
		if(buf1->timerstatus==1 && buf2->timerstatus==1){
			clock_gettime(CLOCK_MONOTONIC, &begint);
                        clock_gettime(CLOCK_MONOTONIC, &middlet);
			printf("client2 시작시간 %f \n",(middlet.tv_sec - begint.tv_sec) + (middlet.tv_nsec - begint.tv_nsec) / 1000000000.0  );
			pthread_exit(NULL);
		}
	}
}

//client1에서 서버에서 받았는지 확인하고 직전 보냈던 시간에서 일정 시간이 지나면 보냄 client은 1초
void* pull1(){
	for(;;){
		if(buf1->starttime  && buf2->starttime){
			struct timespec del;
			clock_gettime(CLOCK_MONOTONIC, &del);
			double deltime = (del.tv_sec - middlet.tv_sec) + (del.tv_nsec - middlet.tv_nsec) / 1000000000.0;
			if(buf1->timerstatus==2){
                                pthread_exit(NULL);
                        }
			if(deltime >1.0){
				if(buf1->status == 0){
					buf1->status=1;
				}
				clock_gettime(CLOCK_MONOTONIC, &middlet);
			}
		}
	}
}

//client2에서 서버에서 받았는지 확인하고 직전 보냈던 시간에서 일정 시간이 지나면 보냄client2 는 0.5초
void* pull2(){
	for(;;){
		if(buf1->starttime && buf2->starttime){
			struct timespec del;
                        clock_gettime(CLOCK_MONOTONIC, &del);
                        double deltime = (del.tv_sec - middlet.tv_sec) + (del.tv_nsec - middlet.tv_nsec) / 1000000000.0;
			if(buf2->timerstatus==2){
				pthread_exit(NULL);
			}
			if(deltime >0.5){
				if(buf2->status == 0){
					buf2->status=1;
				}
				clock_gettime(CLOCK_MONOTONIC, &middlet);
			}
		}

	}
}

//client1에서 게임이 끝났는지 확인하는 함수 결과도 보여줌
void* checkfinish1(){
	for(;;){
		if(buf1->timerstatus==2 && buf2->timerstatus==2){
			clock_gettime(CLOCK_MONOTONIC, &endt);
			printf("client1 timer 걸린시간 : %f\n",(endt.tv_sec - begint.tv_sec) + (endt.tv_nsec - begint.tv_nsec) / 1000000000.0);
			printf("client1 : %s\n",buf1->d_buf);	
			pthread_exit(NULL);
		}
	}	
}

//client2에서 게임이 끝났는지 확인하는 함수 결과도 보여줌
void* checkfinish2(){
	for(;;){
		if(buf1->timerstatus==2 && buf2->timerstatus==2){
			clock_gettime(CLOCK_MONOTONIC, &endt);
			printf("client2 timer 걸린시간 : %f\n",(endt.tv_sec - begint.tv_sec) + (endt.tv_nsec - begint.tv_nsec) / 1000000000.0);
			printf("client2 : %s\n",buf2->d_buf);
			pthread_exit(NULL);
		}
	}
}

//sigint일때 shared memory 다 지워주는 함수
void sig_handler(int signo)
{
	printf(" graceful exit \n");
	if (shmctl (shmid1, IPC_RMID, NULL) == -1)
		exit(13);
	if (shmctl (shmid2, IPC_RMID, NULL) == -1)
		exit(14);
	exit(15);
}

