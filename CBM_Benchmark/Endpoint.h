#pragma once
#include "Config.h"
#include "Node.h"
class Endpoint
{
public:
	Endpoint(struct Node * node, const char * addr, char * port,  uint64_t flags, Config * config, int id);
	~Endpoint();
	//func()
	int init();
	int waitComp(fid_cq * cq);

    int client();
    int connect();
    int setDataBuffer();
    ssize_t rdma_write();
    ssize_t rdma_read();

    int server();
    int ctrl_recv(size_t size);
    int ctrl_send(size_t size);
    int sendKeys();
    int sendCommand(int * COMMAND);
    int accept(struct fi_eq_cm_entry * entry);
    int listen();

    struct Node * node;
    int run;
    int id;
    const char * addr;
    char * av_addr;
    char * port;
    uint64_t flags;
    Config * config;

    struct ctx * tx_ctx;
    struct ctx * c_ctx;
    struct ctx * rx_ctx;
    int * msg_buff;
    void * ctrl_buff;
    struct keys local_keys;
    struct keys remote_keys;

    struct fi_info *fi;
    struct fid_fabric *fabric;
    struct fid_domain *domain;
    struct fid_mr *mr;
	struct fid_mr *cmr;

    struct fid_ep * ep;
    struct fid_pep * pep;

     //deprecated
    int client_thread(struct ctx * ctx);
    int cq_thread();


private:









};

