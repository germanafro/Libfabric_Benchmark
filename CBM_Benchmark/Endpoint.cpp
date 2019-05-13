#include "Endpoint.h"
#include "Clock.h"
#include <omp.h>
#include <iostream>
using namespace std;
/*
 *
 * TODO Docstring
 * BIG TODO: I have recently been informed that i should not use malloc in c++ and instead use new in combination with shared pointers 
 * which would result in never running into memory leaks or singal 6/11.
 */
Endpoint::Endpoint(Node * node, const char * addr, char * port,  uint64_t flags, Config * config, int id)
{
    this->node = node;
    this->addr = addr;
    this->port = port;
    this->flags = flags;
    this->config = config;
    this->id = id;
    this->domain = node->domain;
    this->fabric = node->fabric;
    tx_ctx = (struct ctx *)malloc(sizeof(struct ctx));
    if (!tx_ctx) {
        perror("malloc");
    }
    c_ctx = (struct ctx *)malloc(sizeof(struct ctx));
    if (!c_ctx) {
        perror("malloc");
    }
    rx_ctx = (struct ctx *)malloc(sizeof(struct ctx));
    if (!rx_ctx) {
        perror("malloc");
    }
    tx_ctx->size = 0;
    tx_ctx->id = id;
    tx_ctx->mode = node->mode;
    c_ctx->size = 0;
    c_ctx->id = id;
    c_ctx->mode = node->mode;
    rx_ctx->size = 0;
    rx_ctx->id = id;
    rx_ctx->mode = node->mode;

    int ret= fi_getinfo(FIVER, addr, port, 0, config->hints, &fi);
    if (ret) {
        perror("EP.fi_getinfo");
    }
    msg_buff = (int *) malloc(config->msg_size);
    if (!msg_buff) {
        perror("malloc");
    }
    ctrl_buff = malloc(config->ctrl_size);
    if (!ctrl_buff) {
        perror("malloc");
    }
    ret = fi_mr_reg(this->domain, msg_buff, config->msg_size,
                    FI_REMOTE_READ | FI_REMOTE_WRITE | FI_WRITE | FI_READ,
                    0, 0, 0, &mr, NULL);
    if (ret) {
        printf("[%d]", id);
        perror("fi_mr_reg mr");
    }
	ret = fi_mr_reg(this->domain, ctrl_buff, config->ctrl_size,
					FI_SEND | FI_RECV | FI_WRITE | FI_READ,
                    0, 0, 0, &cmr, NULL);
    if (ret) {
        printf("[%d]", id);
        perror("fi_mr_reg cmr");
    }
    local_keys.id = id;
    local_keys.addr = (uint64_t) msg_buff;
    local_keys.rkey = fi_mr_key(mr);
}

Endpoint::~Endpoint()
{
    fi_shutdown(ep, 0);
    fi_close(&ep->fid);
    fi_close(&mr->fid);
    fi_close(&cmr->fid);
    fi_freeinfo(fi);
    //free(msg_buff); // TODO is this necessary or will fi_close(mr) free this resource?
    //free(ctrl_buff);
}

int Endpoint::init()
{
	return 0;
}

/*
* prepare remote key and address for msg buffer
* initialize passive endpoint and bind the pep to the event queue of the node
*/
int Endpoint::server()
{
    int ret;
    struct fid_eq *eq = node->eq;

    int N = config->msg_size / sizeof(int);
    for(int i=0; i<N; i++){
        msg_buff[i] = 0;
    }

    ret = fi_passive_ep(fabric, fi, &pep, NULL);
    if (ret) {
        printf("[%d]", id);
        perror("fi_passive_ep");
        return ret;
    }

    ret = fi_pep_bind(pep, &eq->fid, 0);
    if (ret) {
        printf("[%d]", id);
        perror("fi_pep_bind(eq)");
        return ret;
    }

    return 0;   
}

/*
* initialize endpoint with client behavior
* bind ep to the nodes eq and cq
* enable ep
*/
int Endpoint::client()
{
    int ret;
    struct fid_eq *eq = node->eq;
    struct fid_cq *cq_tx = node->cq_tx; // FI_TRANSMIT
    struct fid_cq *cq_rx = node->cq_rx; // FI_RECV

    ret = fi_endpoint(domain, fi, &ep, NULL);
    if (ret) {
        perror("fi_endpoint");
        return ret;
    }

    ret = fi_ep_bind(ep, &eq->fid, 0);
    if (ret) {
        perror("fi_ep_bind(eq)");
        return ret;
    }

    ret = fi_ep_bind(ep, &cq_tx->fid, FI_TRANSMIT);
    if (ret) {
        perror("fi_ep_bind(cq_tx)");
        return ret;
    }

    ret = fi_ep_bind(ep, &cq_rx->fid, FI_RECV);
    if (ret) {
        perror("fi_ep_bind(cq_rx)");
        return ret;
    }

    ret = fi_enable(ep);
    if (ret) {
        perror("fi_enable");
        return ret;
    }

    return 0;
}

int Endpoint::setDataBuffer()
{
    size_t msg_size = config->msg_size;
    int arraysize = msg_size / sizeof(int);
    int * message = (int *)malloc(msg_size);
    // generate testdata
    for (int i =0 ; i< arraysize; i++){
        message[i] = i;
    }

    memcpy(msg_buff, &message[0], msg_size);
    return 0;
}

ssize_t Endpoint::rdma_write()
{
    ssize_t ret;
    tx_ctx->size = config->msg_size;
    ret = fi_write(ep, msg_buff, config->msg_size, fi_mr_desc(mr),
                    0, remote_keys.addr, remote_keys.rkey, tx_ctx);
    if (ret) {
        perror("fi_write");
        return ret;
    }

    return ret;
}

ssize_t Endpoint::rdma_read()
{
    ssize_t ret;
    tx_ctx->size = config->msg_size;
    ret = fi_read(ep, msg_buff, config->msg_size, fi_mr_desc(mr),
                    0, remote_keys.addr, remote_keys.rkey, tx_ctx);
    if (ret) {
        perror("fi_read");
        return ret;
    }

    return ret;
}

/*
* await a message on the ctrl_buff with given size
* size <= 0 results in the size of the ctrl_buff
*/
int Endpoint::ctrl_recv(size_t size)
{
    //printf("[%d]DEBUG: posting ctrl_receive %d\n", id, size);
    int ret;
    if(size <= 0){
        size = config->ctrl_size;
    }
    rx_ctx->id = id;
    rx_ctx->size = size;
    ret = fi_recv(ep, ctrl_buff, size, fi_mr_desc(cmr), 0, rx_ctx);
    if (ret) {
        perror("fi_recv");
    }
    return ret;
}

/*
* send a control message from the ctrl_buff with given size
* size <= 0 results in the size of the ctrl_buff
*/
int Endpoint::ctrl_send(size_t size)
{
    //printf("[%d]DEBUG: posting ctrl_send: %d\n", id, size);
    int ret;
    if(size <= 0){
        size = config->ctrl_size;
    }
    c_ctx->size = size;
    ret = fi_send(ep, ctrl_buff, size, fi_mr_desc(cmr), 0, c_ctx);
    if (ret) {
        perror("fi_send");
    }
    return ret;
}

/*
* copy local keypair into ctrl_buff and send to remote peer
*/
int Endpoint::sendKeys()
{
    int ret;
    size_t size = sizeof(struct keys);
    memcpy(ctrl_buff, &local_keys, size);
    ret = ctrl_send(size);

    return ret;
}

/*
* copy command string into ctrl_buff and send to remote peer
*/
int Endpoint::sendCommand(int * COMMAND)
{
    int ret;
    size_t size = sizeof(int);

    memcpy(ctrl_buff, COMMAND, size);
    c_ctx->size = size;
    ret = ctrl_send(size);

    return ret;
}

/*
* listen for incoming conn_req on the pep
* make sure to have run server(struct fid_eq *eq) at least once before calling this function.
*/
int Endpoint::listen()
{
    int ret;
    ret = fi_listen(pep);
    if (ret) {
        perror("fi_listen");
        return ret;
    }
	printf("[%d]listening\n", id);

    return ret;
}

/*
* post recv for remote key pair
* request connection 
* wait for answer, wait for key pair (synchronously)
* store key pair in keys struct
*/
int Endpoint::connect()
{

    int ret;
    ssize_t rret;
    struct fid_eq *eq = node->eq;

    ret = ctrl_recv(sizeof(struct keys));
    if(ret){
        return ret;
    }

    ret = fi_connect(ep, fi->dest_addr, NULL, 0);
    if (ret) {
        perror("fi_connect");
        return ret;
    }
    
    struct fi_eq_cm_entry entry;
    uint32_t event;

    rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
    if (rret > 0){
        if (event != FI_CONNECTED) {
            fprintf(stderr, "invalid event %u\n", event);
            return -1;
        }
    }
    else if (rret != -FI_EAGAIN) {
        struct fi_eq_err_entry err_entry;
        fi_eq_readerr(eq, &err_entry, 0);
        printf("[%d] %s %s \n", id, fi_strerror(err_entry.err), fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
        return -1;
    }

    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    
    ret = node->waitComp(node->cq_rx, msg_entry);
    //printf("DEBUG recv_comp %d\n", msg_entry->len);
    if (ret){
        free(msg_entry);
        return ret;
    }
    struct ctx * ctx = (struct ctx *)msg_entry->op_context;//(struct ctx *)malloc(sizeof(struct ctx)); //FIXME
    //memcpy(ctx, entry.op_context, sizeof(struct ctx));
    if(ctx->size == msg_entry->len){
        memcpy(&remote_keys, ctrl_buff, msg_entry->len);
        printf("[%d] connected\n", id);
        run = 1;
        free(ctx);
        free(msg_entry);
        return 0;
    } else{
        printf("Error: whoops expected to receive something of size keys: %d but received size: %d\n", ctx->size, msg_entry->len);
        printf("ctx: id %d size %d mode %d \n", ctx->id, ctx->size, ctx->mode);
        free(ctx);
        free(msg_entry);
        return -1;
    }
}

/*
* create a new endpoint bind them to the nodes cq based on the information of the cm entry 
* accept the conn_req
* obviously this needs to be run after a validated conn_req event
*/
int Endpoint::accept(struct fi_eq_cm_entry * entry)
{
    int ret;
    struct fid_eq *eq = node->eq;
    struct fid_cq *cq_tx = node->cq_tx; // FI_TRANSMIT
    struct fid_cq *cq_rx = node->cq_rx; // FI_RECV
    
    ret = fi_endpoint(domain, entry->info, &ep, NULL);
    if (ret) {
        perror("fi_endpoint");
        return ret;
    }

    ret = fi_ep_bind(ep, &eq->fid, 0);
    if (ret) {
        perror("fi_ep_bind(eq)");
        return ret;
    }

    ret = fi_ep_bind(ep, &cq_tx->fid, FI_TRANSMIT);
    if (ret) {
        perror("fi_ep_bind(cq_tx)");
        return ret;
    }

    ret = fi_ep_bind(ep, &cq_rx->fid, FI_RECV);
    if (ret) {
        perror("fi_ep_bind(cq_rx)");
        return ret;
    }

    ret = fi_enable(ep);
    if (ret) {
        perror("fi_enable");
        return ret;
    }
    ret = fi_accept(ep, NULL, 0);
    if (ret) {
        perror("fi_accept");
        return ret;
    }

    return ret;
}










































int Endpoint::waitComp(struct fid_cq * cq)
{
	struct fi_cq_msg_entry entry;
	int ret;
	while (1) {
		ret = fi_cq_read(cq, &entry, 1);
		if (ret > 0) return 0;
		if (ret != -FI_EAGAIN) {
			struct fi_cq_err_entry err_entry;
			fi_cq_readerr(cq, &err_entry, 0);
			printf("fi_cq_sread: %s %s \n", fi_strerror(err_entry.err), fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
			return ret;
		}
	}
}















































/* deprecated

int Endpoint::cq_thread()
{
    struct fi_cq_msg_entry comp;
    ssize_t ret;

    while (run) {
        ret = fi_cq_sread(cq, &comp, 1, NULL, 1000);
        if (!run){
            break;
        }
        if (ret == -FI_EAGAIN)
            continue;

        if (ret != 1) {
            perror("fi_cq_sread");
            break;
        }

        if (comp.flags & (FI_READ|FI_WRITE)) {
            struct ctx *ctx = (struct ctx*)comp.op_context;
            omp_set_lock(&ctx->lock);
            ctx->ready = 1;
            omp_unset_lock(&ctx->lock);
        }
    }
    return 0;
}

int Endpoint::client_thread(struct ctx * ctx )
{
    size_t msg_size = config->msg_size;
#ifdef DEBUG
	size_t msg_size = config->msg_size;
	printf("msg_size: %d, msg_size: %d, need size: %d\n", msg_size, msg_size, msg_size*config->threads);
#endif // DEBUG
    int arraysize = msg_size / sizeof(int);
    int * message = (int *)malloc(msg_size);
    // generate testdata
    for (int i =0 ; i< arraysize; i++){
        message[i] = i;
    }

    int thread = 0;
    ssize_t ret;
    uint64_t offset = arraysize * thread;
    memcpy(msg_buff + offset, &message[0], msg_size);

    int ecount = 0;
    long data = 0;
    printf("[%d]job start, target: %l bytes\n", id, ctx->total_data_size);

    //time
    Clock * clock = new Clock();
    if(clock->start()){
        exit(EXIT_FAILURE);
    }
    
    while (data < ctx->total_data_size) {

        ret = fi_write(ep, msg_buff + offset, msg_size, fi_mr_desc(mr),
                        0, keys.addr + offset, keys.rkey, ctx);
        if (ret) {
            printf("[%d] fi_write: %s\n", thread, fi_strerror(ret));
            ecount++;
            continue;
        }

        while (!ctx->ready) {
            //wait
        }
        data += msg_size;


#ifdef DEBUG
        printf("[%d] fi_write: wrote %l bytes in total \n", thread, data);
#endif // DEBUG
        omp_set_lock(&ctx->lock);
        ctx->ready = 0;
        omp_unset_lock(&ctx->lock);

        //omp_destroy_lock(&ctx->lock);
    }
    printf("[%d]job done, total data sent: %f bytes, error count: %d\n", thread, data, ecount);
    //time
	clock->stop();
	printf("time: %lf\n", clock->getDelta());
    return 0;
}
*/