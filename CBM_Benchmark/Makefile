CC = g++
CFLAGS = -Wall -fopenmp 
DEPS = Config.h Endpoint.h host2ip.h Node.h Clock.h
OBJ = Main.o Endpoint.o Config.o host2ip.o Node.o Clock.o
LIBS = -lfabric -lpthread -lnuma

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Benchmark: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm $(OBJ)

remake:
	make clean
	make