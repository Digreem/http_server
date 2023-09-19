CFLAGS=-std=c99 -Wall -O3 -DNDEBUG -DUSE_MEM_POOL=1
OPTFLAGS=

OBJS=server.o

crazyserver : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(OPTFLAGS)
	
clean :
	rm -f *.o crazyserver