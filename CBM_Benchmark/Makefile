CC = g++
CFLAGS = -Wall -v
DEPS = Config.h Node.h InputNode.h ProcessingNode.h
OBJ = Main.o Config.o InputNode.o Node.o ProcessingNode.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Benchmark: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^