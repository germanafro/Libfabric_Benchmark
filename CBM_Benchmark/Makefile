CC = gcc
CFLAGS = -Wall
DEPS = Config.h Domain.h Endpoint.h Node.h InputNode.h ProcessingNode.h resource.h
OBJ = Main.o Config.o Domain.o Endpoint.o InputNode.o Node.o ProcessingNode.o

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Benchmark: $(OBJ)
	gcc $(CFLAGS) -o $@ $^


Benchmark: Main.c
		gcc -o Benchmark Main.c