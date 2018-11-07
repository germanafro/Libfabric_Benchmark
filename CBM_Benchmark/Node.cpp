#include "Node.h"
#include <omp.h>

/*
 *
 * TODO Docstring
 */
Node::Node(const char *addr, uint64_t flags, Config * config)
{
	this->addr = addr;
	this->flags = flags;
	this->config = config;
}

// yeah this will cause big problems if we didnt init() yet... so always run init() ^-^
Node::~Node()
{

}

int Node::init(int mode)
{
    int num_ep = config->num_ep;

    if (num_ep < 1){
        printf("invalid number of Endpoints: %d\n", num_ep);
    }
    if (mode < 0 or mode > BM_MAX){
        printf("invalid mode: %d\n", mode);
    }

    int rets[num_ep];
    //omp_set_dynamic(0);
    omp_set_num_threads(num_ep);
    omp_set_nested(1);
    int int_port = atoi(config->default_port);
    char *port = new char(10);
    for(int i = 0; i<num_ep; i++){
        sprintf(port,"%d",int_port+i);
        printf("[%d] pushing Endpoint | addr: %s, port: %s\n", i, addr, port);
        eps.push_back(new Endpoint(addr, port, flags, config)); // TODO verbs provider addressing?
    }
#pragma omp parallel
    {
    #pragma omp for nowait
        for (int n=0; n<num_ep; n++) {
            int ret;
            int thread = omp_get_thread_num();
            printf("[%d] initializing Endpoint | addr: %s, port: %s\n", thread, addr, port);
            ret = eps[thread]->init(thread);
            printf("[%d] ep.init(): %s\n", thread, fi_strerror(ret));



            if (!ret) {
                switch (mode) {
                    case BM_SERVER:
                        eps[thread]->server(thread);
                        break;
                    case BM_CLIENT:
                        eps[thread]->client(thread);
                        break;
                    default:
                        printf("[%d] unknown mode: %d\n", thread, mode);
                        break;
                }
            }
        }
    }
    return 0;
}

