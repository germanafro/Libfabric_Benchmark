#pragma once
//libfabric includes
#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>
//openMP
#include <omp.h>
//etc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

class Config
{
public:
	Config();
	~Config();

    int readConfig();

	const char * addr;
    char * controller_addr;
	char * start_port;
    char * port;
    char * controller_port;
    int server_offset;
    std::vector<std::string> addr_v;

    int num_pn;
    int num_en;
    int num_ep;
    int slot;
    int threads;
    int max_packet_size;
    int max_duration;
    int checkpoint_intervall;

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

//custom data structures

struct keys {
    int id;         // used for the controller to assign id to node
    uint64_t rkey;  // remote key that grants access to the remote memory region
    uint64_t addr;  // the remote address descriptor to the remote memory region
};
/*
* a transfer time stamp. each client will send an array of these transfers
* to recollect the dataflow of the entire system when the Benchmark is finished.
*/
struct transfer {       
    int from;           // local id
    int to;             // target id
    double delta_sec;   // current elapsed time delta
    double time_sec;    // total elapsed time
    long completions;   // number of clompetions counted up to this point per endpoint (index = remote id)
    long data_byte;     // deprecated
};

/**/
struct ctx {
    int id;
    int mode;
    int size;
};

#define BM_OFFSET	0
enum {
    //NODE MODES
    BM_SERVER       = BM_OFFSET,    // ServerMode Tag for Node
    BM_CLIENT       = 1,            // ClientMode Tag for Node
    BM_CONTROLLER   = 2,            // ControllerMode Tag for Node

    //COMMANDS
    BM_STOP         = 100,          //
    BM_START        = 101,          //
    BM_CONNECT      = 102,          //
    BM_CONNECTED    = 103,          //
    BM_CHECKPOINT   = 104,          //

    //MAX
    BM_MAX
};

#define FIVER FI_VERSION(1, 6)
//#define DEBUG
//
#define DEFAULT_PROCESSING_NODE_SIZE 10
#define DEFAULT_ENTRY_NODE_SIZE 10

// message sizing
#define MTU_LEN (4096) // 256 ~ 4096B
//#define MIN_MSG_LEN = MTU_LEN;
#define MAX_MSG_LEN 2147483648 // 2GB
#define MAX_CTRL_MSG_SIZE 2147483// FIXME


//numbers
#define MILLION  1000000L
#define BILLION  1000000000L
#define GIGABYTE 1024*1024*1024