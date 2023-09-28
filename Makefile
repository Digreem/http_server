CFLAGS=-g -std=c99 -Wall -O0 -DNDEBUG -DUSE_MEM_POOL=1
OPTFLAGS=

OBJS=server.o hashtable.o llist.o http_parser.o

crazyserver : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(OPTFLAGS)
	
clean :
	rm -f *.o crazyserver