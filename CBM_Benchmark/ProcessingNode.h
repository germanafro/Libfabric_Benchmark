#pragma once
#include "Node.h"
class ProcessingNode :
	public Node
{
public:
	ProcessingNode(const char * addr, uint64_t flags, Config * config);
	~ProcessingNode();

	int initServer();

	struct fid_pep *pep;
};

