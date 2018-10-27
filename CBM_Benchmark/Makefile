CC = g++
CFLAGS = -Wall
DEPS = Config.h Node.h InputNode.h ProcessingNode.h host2ip.h
OBJ = Main.o Config.o InputNode.o Node.o ProcessingNode.o host2ip.o
LIBS = -lfabric -lpthread

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Benchmark: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)