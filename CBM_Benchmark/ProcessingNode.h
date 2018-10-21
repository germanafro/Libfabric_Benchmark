#pragma once
#include "Node.h"
struct keys {
	uint64_t rkey;
	uint64_t addr;
};
class ProcessingNode :
	public Node
{
public:
	ProcessingNode(const char * addr, uint64_t flags, Config * config, void * buff, struct keys keys);
	~ProcessingNode();

	int initServer();

	struct keys keys;
	struct fid_pep *pep;
};

