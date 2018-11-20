CC=gcc
CFLAGS= -lpthread  -g  
LDFLAGS=  -g -lpthread
EXEC=sched_ws

all: $(EXEC)

sched_ws: sched_ws.o quicksort_ws.o
	$(CC) -o $@ $^ $(LDFLAGS)

sched_ws.o: sched_ws.c
	$(CC) -o $@ -c $< $(CFLAGS)

quicksort_ws.o: quicksort_ws.c sched_ws.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
