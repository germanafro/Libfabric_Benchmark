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
	switch (argc) {
	case 1: // runs server with default config
    {
        return server();
    }
	case 4: // runs server
    {
        int num_ep = atoi(argv[2]);
        if (num_ep > 0) {
            config->num_ep = num_ep;
        }
        printf("Endpoints: %d\n", config->num_ep);
        return server();
    }
	case 5: // runs client
    {
        int threads = atoi(argv[1]);
        if (threads > 0) {
            config->threads = threads;
        }
        printf("threads: %d\n", threads);

        int num_ep = atoi(argv[2]);
        if (num_ep > 0) {
            config->num_ep = num_ep;
        }
        printf("Endpoints: %d\n", config->num_ep);

        int total_data_size = atoi(argv[3]);
        if (total_data_size > 0) {
            config->total_data_size = total_data_size * 1024 * 1024;
        }
        printf("total_data_size: %d\n", config->total_data_size);

        char *addr = argv[argc-1];
        addr = host2ip::resolve(addr);
        if (addr == NULL) {
            return -1;
        } else {
            config->addr = addr;
        }
        printf("addr: %s\n", config->addr);

        return client(addr);

    }
        case 6: // runs client
        {
            int threads = atoi(argv[1]);
            if (threads > 0) {
                config->threads = threads;
            }
            printf("threads: %d\n", threads);

            int num_ep = atoi(argv[2]);
            if (num_ep > 0) {
                config->num_ep = num_ep;
            }
            printf("Endpoints: %d\n", config->num_ep);

            int total_data_size = atoi(argv[3]);
            if (total_data_size > 0) {
                config->total_data_size = total_data_size * 1024 * 1024;
            }
            printf("total_data_size: %d\n", config->total_data_size);

            int msg_size = atoi(argv[4]);
            if (msg_size > 0) {
                config->msg_size = msg_size * 1024 * 1024;
            }
            printf("msg_size: %d\n", config->msg_size);

            char *addr = argv[argc-1];
            addr = host2ip::resolve(addr);
            if (addr == NULL) {
                return -1;
            } else {
                config->addr = addr;
            }
            printf("addr: %s\n", config->addr);

            return client(addr);

        }
	default:
    {
        fprintf(stderr, "wrong number of arguments given: %d\n", argc);
        fprintf(stderr, "usage:\n[server auto conf] %s\n[server manual conf] %s threads num_ep count\n[client] %s threads num_ep total_data_Size(MB) msg_size(MB) serveraddr\n", argv[0]);
        return -1;
    }
	}



	return 0;
}
