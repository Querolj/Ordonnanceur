CC=gcc
CFLAGS= -lpthread  -g  
LDFLAGS=  -g -lpthread
EXEC=sched2

all: $(EXEC)

sched2: sched2.o quicksort2.o
	$(CC) -o $@ $^ $(LDFLAGS)

sched2.o: sched2.c
	$(CC) -o $@ -c $< $(CFLAGS)

quicksort2.o: quicksort2.c sched2.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
