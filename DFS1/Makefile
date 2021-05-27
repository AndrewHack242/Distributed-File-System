.PHONY = all clean

CC=gcc
CFLAGS= -Wall

all: server
	
server: server.c
	gcc -pthread server.c -o server  $(CFLAGS)
	
clean:
	rm -rvf server