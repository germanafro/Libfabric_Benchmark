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

int client(char *addr, int threads, int size, int count) 
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
		fprintf(stderr, "usage: %s addr threads size count rkey addr\n", argv[0]);
		return -1;
	}

	char *addr = argv[1];
	addr = host2ip::resolve(addr);
	if (addr == NULL){
	    return -1;
	}
	printf("addr: %s\n", addr);
	int threads = atoi(argv[2]);
	printf("threads: %d\n", threads);
	int num_ep = atoi(argv[3]);
	config->num_ep = num_ep;
	printf("Endpoints: %d\n", num_ep);
	int count = atoi(argv[4]);
	printf("count: %d\n", count);

	return client(addr, threads, num_ep, count);
}
