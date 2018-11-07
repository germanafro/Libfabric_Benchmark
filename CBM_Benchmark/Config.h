#pragma once
//libfabric includes
#include <rdma/fabric.h>
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

class Config
{
public:
	Config();
	~Config();

	const char* default_port;
    int num_pn;
    int num_en;
    int threads;
    int count;
    int max_packet_size;

	size_t buff_size;
    size_t ctrl_size;
    size_t msg_size;
    struct fi_eq_attr eq_attr;
    struct fi_cq_attr cq_attr;
	struct fi_opt * option;
	struct fi_info * hints;
	//console art
	static const char * console_spacer();
private:

};

struct keys {
    uint64_t rkey;
    uint64_t addr;
};
struct ctx {
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int ready;
    int count;
    int size;
    int id;
};

#define BM_OFFSET	0
enum {
    BM_SERVER   = BM_OFFSET, /* ServerMode Tag for Node*/
    BM_CLIENT   = 1, /* ClientMode Tag for Node*/
    BM_MAX
};

#define FIVER FI_VERSION(1, 6)

//
#define DEFAULT_PROCESSING_NODE_SIZE 10
#define DEFAULT_ENTRY_NODE_SIZE 10

// message sizing
#define MTU_LEN (4096) // 256 ~ 4096B
//#define MIN_MSG_LEN = MTU_LEN;
#define MAX_MSG_LEN 2147483648 // 2GB
#define MAX_CTRL_MSG_SIZE 2147483// FIXME






