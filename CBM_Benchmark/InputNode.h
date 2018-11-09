#pragma once
#include "Node.h"
#include <vector>

class InputNode :
	public Node
{
public:
	InputNode(const char *addr, uint64_t flags, Config * config);
	~InputNode();

	int initClient(void * cq_thread(void* arg));


	int run;
private:
	//Endpoints
	/*std::vector<Endpoint> rx_ep;
	std::vector<Endpoint> tx_ep;*/
};

