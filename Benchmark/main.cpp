#include <iostream>
#include "benchmark.h"
int main(int argc, char *argv[]) {
    std::cout << "Hello, World!" << std::endl;
    benchmark * bench = new benchmark(argc, argv);
    return 0;
}