.PHONY = all clean

CC=gcc
CFLAGS= -Wall

all: client
	
client: client.c
	gcc -pthread client.c -o client  $(CFLAGS)
	
clean:
	rm -rvf client