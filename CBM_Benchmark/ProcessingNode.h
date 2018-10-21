#pragma once
#include "Node.h"
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

