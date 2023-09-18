all: server client

server: server.cpp
	gcc -g -o server server.c

client: client.cpp
	gcc -g -o client client.c
	
clean :
	rm -f *.o client server