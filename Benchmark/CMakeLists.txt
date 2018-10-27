cmake_minimum_required(VERSION 3.12)
project(Benchmark)

set(CMAKE_CXX_STANDARD 14)

add_executable(Benchmark main.cpp benchmark.cpp benchmark.h config.cpp config.h host2ip.cpp host2ip.h)