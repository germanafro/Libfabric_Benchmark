#pragma once
#include "Config.h"
//etc
#include <iostream>
class Node
{
public:
	Node(const char *addr, uint64_t flags, Config * config, void *buff);
	~Node();
	int init();

	const char * addr; 
	uint64_t flags; 
	Config * config;
	void * buff;

	struct fi_info *fi;
	struct fid_fabric *fabric;
	struct fid_eq *eq;
	struct fid_cq *cq;
	struct fid_domain *domain;
	struct fid_mr *mr;

	struct fid_ep *ep;
};

