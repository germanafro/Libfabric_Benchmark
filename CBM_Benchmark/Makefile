CC = g++
CFLAGS = -Wall -fopenmp 
DEPS = Config.h Endpoint.h host2ip.h Node.h
OBJ = Main.o Endpoint.o Config.o host2ip.o Node.o
LIBS = -lfabric -lpthread

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Benchmark: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
