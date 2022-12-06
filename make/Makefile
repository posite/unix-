CC = gcc

all: ipc1 ipc2  

ipc1 : shmtest.o main.o
	$(CC) -o ipc1 shmtest.o main.o -lpthread

ipc2 : pipetest.o main1.o
	$(CC) -o ipc2 pipetest.o main1.o -lpthread


shmtest.o : shmtest.c
	$(CC) -c -o shmtest.o shmtest.c -lpthread

main.o : main.c
	$(CC) -c -o main.o main.c -lpthread

pipetest.o : pipe.c
	$(CC) -c -o pipetest.o pipe.c -lpthread

main1.o : main1.c
	$(CC) -c -o main1.o main1.c -lpthread


clean : 
	rm shmtest.o main.o ipc1 ipc2  pipetest.o main1.o
