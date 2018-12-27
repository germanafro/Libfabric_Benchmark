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
    this->mode = mode;
    int num_ep = config->num_ep;
    int slot = config->slot;

    if (num_ep < 1){
        printf("invalid number of Endpoints: %d\n", num_ep);
    }
    if ((mode < 0) || (mode > BM_MAX)){
        printf("invalid mode: %d\n", mode);
    }

    //omp_set_dynamic(0);
    omp_set_num_threads(num_ep);
    omp_set_nested(1);
    int int_port = atoi(config->port);
    for(int i = 0; i<num_ep; i++){
        char *port = new char(10);
#pragma warning(suppress : 4996)
        sprintf(port,"%d",int_port+i+slot);
        printf("[%d] pushing Endpoint | addr: %s, port: %s\n", i, addr, port);
        eps.push_back(new Endpoint(addr, port, flags, config, i));
    }
#pragma omp parallel
    {
    #pragma omp for //nowait
        for (int n=0; n<num_ep; n++) {
            int ret;
            int thread = omp_get_thread_num();
            ret = eps[n]->init();
            printf("[%d] ep.init(): %s\n", n, fi_strerror(ret));

            if (!ret) {
                switch (mode) {
                    case BM_SERVER:
                        eps[n]->server();
                        break;
                    case BM_CLIENT:
                        eps[n]->client();
                        break;
                    default:
                        printf("[%d] unknown mode: %d\n", n, mode);
                        break;
                }
            }
        }
    }
    return 0;
}

int Node::connectToServer()
{
    if (mode != BM_CLIENT){
        return -1;
    }
    int num_ep = config->num_ep;
#pragma omp parallel
    {
    #pragma omp for
        for (int n=0; n<num_ep; n++) {
            int ret;
            int thread = omp_get_thread_num();
            ret = eps[n]->connectToServer();
            printf("[%d] ep.connectToServer(): %s\n", n, fi_strerror(ret));
        }
    }
    return 0;
}

int Node::setDataBuffer()
{
    if (mode != BM_CLIENT){ // this was ment to fill client buffer not server buffer
        return -1;
    }
    int num_ep = config->num_ep;
#pragma omp parallel
    {
    #pragma omp for
        for (int n=0; n<num_ep; n++) {
            int ret;
            int thread = omp_get_thread_num();
            ret = eps[n]->setDataBuffer();
            printf("[%d] ep.setDataBuffer(): %s\n", n, fi_strerror(ret));
        }
    }
    return 0;
}

struct transfer Node::writeToServer()
{
    struct transfer transfer_sum;
    transfer_sum.data_byte = 0;
    transfer_sum.time_sec = 0;
    if (mode != BM_CLIENT){
        return transfer_sum;
    }
    int num_ep = config->num_ep;

#pragma omp parallel
    {
    #pragma omp for
        for (int n=0; n<num_ep; n++) {
            struct transfer ret;
            int thread = omp_get_thread_num();
            ret = eps[n]->writeToServer();
            transfer_sum.data_byte += ret.data_byte;
            transfer_sum.time_sec += ret.time_sec;
        }
    }
    return transfer_sum;
}

int Node::listenServer()
{
    if (mode != BM_SERVER){
        return -1;
    }
    int num_ep = config->num_ep;
#pragma omp parallel
    {
    #pragma omp for
        for(int n=0; n<num_ep; n++) {
            int ret;
            int thread = omp_get_thread_num();
            ret = eps[n]->listenServer();
            printf("[%d] ep.listenServer(): %s\n", n, fi_strerror(ret));
        }
    }
    return 0;
}