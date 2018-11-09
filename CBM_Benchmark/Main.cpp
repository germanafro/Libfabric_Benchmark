#include "Config.h"
#include "host2ip.h"
#include "Node.h"

using namespace std;



Config * config;
Node * pnode;
Node * inode;

int server()
{
	pnode = new Node(NULL, FI_SOURCE, config);
	int ret = pnode->init(BM_SERVER);
	if (ret)
		return ret;



	return 0;
}

int client(char *addr) 
{
    //TODO pass params to config
	inode = new Node(addr, 0, config);

	int ret = inode->init(BM_CLIENT);
	if (ret)
		return ret;


	return 0;
}

int
main(int argc, char *argv[])
{
	config = new Config();

	if (argc == 1) {
		return server();
	}

	if (argc != 5) {
		fprintf(stderr, "arguments given: %d\n", argc);
		fprintf(stderr, "usage: %s addr threads num_ep count\n", argv[0]);
		return -1;
	}

	char *addr = argv[1];
	addr = host2ip::resolve(addr);
	if (addr == NULL){
	    return -1;
	}
	else {
		config->addr = addr;
	}
	printf("addr: %s\n", config->addr);
	int threads = atoi(argv[2]);
	if (threads > 0) {
		config->threads = threads;
	}
	printf("threads: %d\n", threads);
	int num_ep = atoi(argv[3]);
	if (num_ep > 0) {
		config->num_ep = num_ep;
	}
	printf("Endpoints: %d\n", config->num_ep);
	int count = atoi(argv[4]);
	if (count> 0) {
		config->count = count;
	}
	printf("count: %d\n", config->count);

	return client(addr);
}
