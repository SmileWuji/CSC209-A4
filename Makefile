CC = gcc
CFLAGS = -Wall -std=c99 -Werror
PORT=53892
CFLAGS+= -DPORT=\$(PORT)

all: mismatch

mismatch: 
	$(CC) -c $(CFLAGS) questions.c 
	$(CC) -c $(CFLAGS) qtree.c 
	$(CC) -c $(CFLAGS) utils.c 
	$(CC) $(CFLAGS) questions.o qtree.o utils.o mismatch_server.c -o mismatch_server 
	

clean:  
	rm mismatch_server
	rm *.o
