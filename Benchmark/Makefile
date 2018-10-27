CC = g++
CFLAGS = -Wall
DEPS = config.h benchmark.h host2ip.h
OBJ = main.o benchmark.o config.o host2ip.o
LIBS = -lfabric -lpthread

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Benchmark: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
