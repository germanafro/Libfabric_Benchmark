#pragma once
#include "Config.h"
#include "Endpoint.h"
#include <vector>
//etc
#include <iostream>
class Node
{
public:
	Node(const char *addr, uint64_t flags, Config * config);
	~Node();

	int init(int num_ep, int mode);

	int run;
    const char * addr;
    char * port;
    uint64_t flags;
    Config * config;
    std::vector<Endpoint*> eps;
};

