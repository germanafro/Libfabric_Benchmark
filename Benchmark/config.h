//
// Created by andreas on 27.10.18.
//

//libfabric includes
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>
//Linux
#include <pthread.h>
//etc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BENCHMARK_CONFIG_H
#define BENCHMARK_CONFIG_H

struct keys {
    uint64_t rkey;
    uint64_t addr;
};

class config {
    public:
        config();
        ~config();
        size_t buff_size;
        struct fi_opt * option;
        struct fi_info * hints;
        //console art
        static char * console_spacer();
    private:

    };

#define FIVER FI_VERSION(1, 6)

//
#define DEFAULT_WORKER_SIZE 10;
#define DEFAULT_ENTRY_NODE_SIZE 10;

// message sizing
#define MTU_LEN (4096); // 256 ~ 4096B
//#define MIN_MSG_LEN = MTU_LEN;
#define MAX_MSG_LEN 2147483648; // 2GB
#define MAX_CTRL_MSG_SIZE 2147483; // FIXME


#endif //BENCHMARK_CONFIG_H
