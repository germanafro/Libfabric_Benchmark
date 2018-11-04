CC = g++
CFLAGS = -Wall
DEPS = Config.h Domain.h Endpoint.h host2ip.h InputNode.h Node.h ProcessingNode.h
OBJ = Main.o Domain.o Endpoint.o Config.o host2ip.o InputNode.o Node.o ProcessingNode.o
LIBS = -lfabric -lpthread

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Benchmark: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
