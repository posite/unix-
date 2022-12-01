CC = gcc
TARGET = ipc

$(TARGET) : shmtest.o main.o
	$(CC) -o $(TARGET) shmtest.o main.o -lpthread


shmtest.o : shmtest.c
	$(CC) -c -o shmtest.o shmtest.c -lpthread

main.o : main.c
	$(CC) -c -o main.o main.c -lpthread

clean : 
	rm shmtest.o main.o ipc
