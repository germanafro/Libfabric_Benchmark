#pragma once
#include "Node.h"
#include <vector>

class InputNode :
	public Node
{
public:
	InputNode(const char *addr, uint64_t flags, Config * config, void *buff, struct keys keys);
	~InputNode();

	void * cq_thread(void *arg);
	int initClient();


	struct keys keys;
	int run;
	pthread_t thread;
private:
	//Endpoints
	/*std::vector<Endpoint> rx_ep;
	std::vector<Endpoint> tx_ep;*/
};

