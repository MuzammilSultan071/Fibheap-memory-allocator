CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c11

all: bench

bench: allocator.o fib_heap.o benchmark.o
	$(CC) $(CFLAGS) -o bench allocator.o fib_heap.o benchmark.o

allocator.o: allocator.c allocator.h fib_heap.h
	$(CC) $(CFLAGS) -c allocator.c

fib_heap.o: fib_heap.c fib_heap.h
	$(CC) $(CFLAGS) -c fib_heap.c

benchmark.o: benchmark.c allocator.h
	$(CC) $(CFLAGS) -c benchmark.c

clean:
	rm -f *.o bench
