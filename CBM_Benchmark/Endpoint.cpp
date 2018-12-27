#include "Endpoint.h"
#include "Clock.h"
#include <omp.h>
using namespace std;
/*
 *
 * TODO Docstring
 */
Endpoint::Endpoint(const char * addr, char * port,  uint64_t flags, Config * config, int id)
{
    this->addr = addr;
    this->port = port;
    this->flags = flags;
    this->config = config;
    this->id = id;

    ctx = (struct ctx *)malloc(sizeof(*ctx));
    if (!ctx) {
        perror("malloc");
    }
    omp_init_lock(&ctx->lock);
    ctx->ready = 0;
    ctx->total_data_size = config->total_data_size;
    ctx->size = config->max_packet_size;
    ctx->id = id;
}

Endpoint::~Endpoint()
{
    fi_shutdown(ep, 0);
    fi_close(&ep->fid);
    fi_close(&mr->fid);
    fi_close(&cq->fid);
    fi_close(&eq->fid);
    fi_close(&domain->fid);
    fi_close(&fabric->fid);
    fi_freeinfo(fi);
    //free(msg_buff); // TODO is this necessary or will fi_close(mr) free this resource?
    free(ctrl_buff);
    omp_destroy_lock(&ctx->lock);
}

int Endpoint::init()
{
    int ret;
    struct fi_eq_attr eq_attr = config->eq_attr;
    struct fi_cq_attr cq_attr = config->cq_attr;
    printf("[%d] initializing Endpoint | addr: %s, port: %s\n", id, addr, port);
    ret= fi_getinfo(FIVER, addr, port, 0, config->hints, &fi);
    if (ret) {
        printf("[%d] fi_getinfo: %s\n", id, fi_strerror(ret));
        return ret;
    }
	printf("[%d] fi_getinfo: %s %s\n", id, fi->fabric_attr->prov_name, fi->fabric_attr->name);

    ret = fi_fabric(fi->fabric_attr, &fabric, NULL);
    if (ret) {
        printf("[%d] fi_fabric: %s\n", id, fi_strerror(ret));
        return ret;
    }


    ret = fi_eq_open(fabric, &eq_attr, &eq, NULL);
    if (ret) {
        printf("[%d] fi_eq_open: %s\n", id, fi_strerror(ret));
        return ret;
    }

    ret = fi_domain(fabric, fi, &domain, NULL);
    if (ret) {
        printf("[%d] fi_domain: %s\n", id, fi_strerror(ret));
        return ret;
    }

    ret = fi_cq_open(domain, &cq_attr, &cq, NULL);
    if (ret) {
        printf("[%d] fi_cq_open: %s\n", id, fi_strerror(ret));
        return ret;
    }

    msg_buff = (int *) malloc(config->buff_size);
    if (!msg_buff) {
        printf("[%d] error malloc msg\n", id);
        return ret;
    }
    ctrl_buff = malloc(config->ctrl_size);
    if (!ctrl_buff) {
        printf("[%d] error malloc ctrl\n", id);
        return ret;
    }

    ret = fi_mr_reg(domain, msg_buff, config->buff_size,
                    FI_REMOTE_READ | FI_REMOTE_WRITE,
                    0, 0, 0, &mr, NULL);
    if (ret) {
        printf("[%d] fi_mr_reg mr: %s\n", id, fi_strerror(ret));
        return ret;
    }
	ret = fi_mr_reg(domain, ctrl_buff, config->ctrl_size,
					FI_SEND | FI_RECV | FI_WRITE | FI_READ,
                    0, 0, 0, &cmr, NULL);
    if (ret) {
        printf("[%d] fi_mr_reg cmr: %s\n", id, fi_strerror(ret));
        return ret;
    }
	return 0;
}




//client functionality
int Endpoint::client()
{
    int ret;

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

    ret = fi_ep_bind(ep, &cq->fid, FI_TRANSMIT | FI_RECV);
    if (ret) {
        perror("fi_ep_bind(cq)");
        return ret;
    }

    ret = fi_enable(ep);
    if (ret) {
        perror("fi_enable");
        return ret;
    }

    return 0;
}

int Endpoint::connectToServer()
{

    size_t ret;
    ssize_t rret;
    rret = fi_recv(ep, ctrl_buff, sizeof(keys), fi_mr_desc(cmr), 0, NULL);
    if (rret) {
        perror("fi_recv");
        return (int)rret;
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
        return ret;
    }


    ret = waitComp(cq);
    if (ret){
        return ret;
    }

    memcpy(&keys, ctrl_buff, sizeof(keys));
    printf("[%d] connected\n", id);
    run = 1;
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

struct transfer Endpoint::writeToServer()
{
    struct transfer transfer;
    transfer.data_byte = 0;
    transfer.time_sec = 0;
    size_t msg_size = config->msg_size;
    ssize_t ret;

    //time
    Clock * clock = new Clock();
    if(clock->start()){
        exit(EXIT_FAILURE);
    }

    ret = fi_write(ep, msg_buff, msg_size, fi_mr_desc(mr),
                    0, keys.addr, keys.rkey, ctx);
    if (ret) {
        printf("[%d] fi_write: %s\n", id, fi_strerror(ret));
        if(clock->stop()){ // even if we couldnt send data we have to measure time
            exit(EXIT_FAILURE);
        }
        transfer.time_sec = clock->getDelta();
        return transfer;
    }

    waitComp(cq);

    //time
	clock->stop();
	transfer.time_sec = clock->getDelta();
    transfer.data_byte = msg_size;
    return transfer;
}

struct transfer Endpoint::readFromServer()
{
    struct transfer transfer;
    transfer.data_byte = 0;
    transfer.time_sec = 0;
    size_t msg_size = config->msg_size;
    ssize_t ret;

    //time
    Clock * clock = new Clock();
    if(clock->start()){
        exit(EXIT_FAILURE);
    }

    ret = fi_read(ep, msg_buff, msg_size, fi_mr_desc(mr),
                    0, keys.addr, keys.rkey, ctx);
    if (ret) {
        printf("[%d] fi_read: %s\n", id, fi_strerror(ret));
        if(clock->stop()){ // even if we couldnt send data we have to measure time
            exit(EXIT_FAILURE);
        }
        transfer.time_sec = clock->getDelta();
        return transfer;
    }

    waitComp(cq);

    //time
	clock->stop();
	transfer.time_sec = clock->getDelta();
    transfer.data_byte = msg_size;
    return transfer;
}


//server functionality
int Endpoint::server()
{
    int ret;
    keys.addr = (uint64_t) msg_buff;
    keys.rkey = fi_mr_key(mr);

#ifdef SOCKETS // get adress (does not work with verbs provider!)
	struct fi_av_attr av_attr;
	struct fid_av * av;
	memset(&av_attr, 0, sizeof(av_attr));
	av_attr.type = FI_AV_UNSPEC; //fi->domain_attr->av_type;
	ret = fi_av_open(domain, &av_attr, &av, NULL);
	if (ret) {
		printf("[%d]fi_av_open: %s\n", id, fi_strerror(ret));
		return ret;
	}

	ret = fi_endpoint(domain, fi, &ep, NULL);
	if (ret) {
		printf("[%d]fi_endpoint: %s\n", id, fi_strerror(ret));
		return ret;
	}

	ret = fi_ep_bind(ep, &av->fid, 0);
	if (ret) {
		printf("[%d]fi_pep_bind(av): %s\n", id, fi_strerror(ret));
		return ret;
	}

	ret = fi_enable(ep);
	if (ret) {
		printf("[%d]fi_enable: %s\n", id, fi_strerror(ret));
		return ret;
	}

	av_addr = (char*)malloc(30);
	size_t len = 30;
	fi_av_straddr(av, fi->src_addr, addr, &len);
	fi_shutdown(ep, 0);
	fi_close(&ep->fid);
#endif // SOCKETS

    ret = fi_passive_ep(fabric, fi, &pep, NULL);
    if (ret) {
        printf("[%d]fi_passive_ep: %s\n", id, fi_strerror(ret));
        return ret;
    }

    ret = fi_pep_bind(pep, &eq->fid, 0);
    if (ret) {
        printf("[%d]fi_pep_bind(eq): %s\n", id, fi_strerror(ret));
        return ret;
    }

    return 0;   
}

int Endpoint::listenServer()
{
    int ret;
    ret = fi_listen(pep);
    if (ret) {
        printf("[%d]fi_listen: %s\n", id, fi_strerror(ret));
        return ret;
    }

    struct fi_eq_cm_entry entry;
    uint32_t event;
    ssize_t rret;
	
    while (1) {

#ifdef SOCKETS
		printf("[%d]listening: %s\n", id, av_addr);
#else
		printf("[%d]listening\n", id);
#endif // DEBUG

        rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
        if (rret > 0) {
            if (event != FI_CONNREQ) {
                printf("invalid event %u\n", event);
                return (int) rret;
            }
        } else if (rret != -FI_EAGAIN) {
            struct fi_eq_err_entry err_entry;
            fi_eq_readerr(eq, &err_entry, 0);
            printf("%s %s \n", fi_strerror(err_entry.err),
                   fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
            return (int) rret;
        }

        printf("[%d]connection request: %s\n", id, entry.info->fabric_attr->name);

        ret = fi_endpoint(domain, entry.info, &ep, NULL);
        if (ret) {
            perror("fi_endpoint");
            return ret;
        }

        ret = fi_ep_bind(ep, &eq->fid, 0);
        if (ret) {
            perror("fi_ep_bind(eq)");
            return ret;
        }

        ret = fi_ep_bind(ep, &cq->fid, FI_TRANSMIT | FI_RECV);
        if (ret) {
            perror("fi_ep_bind(cq)");
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

        rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
        if (rret > 0) {
            if (event != FI_CONNECTED) {
                fprintf(stderr, "invalid event %u\n", event);
                return (int) rret;
            }
        } else if (rret != -FI_EAGAIN) {
            struct fi_eq_err_entry err_entry;
            fi_eq_readerr(eq, &err_entry, 0);
            printf("%s %s \n", fi_strerror(err_entry.err),
                    fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
            return (int) rret;
        }

        //prepare buffers
        memcpy(ctrl_buff, &keys, sizeof(keys));
        int N = config->buff_size / sizeof(int);
        for(int i=0; i<N; i++){
            msg_buff[i] = 0;
        }

        rret = fi_send(ep, ctrl_buff, sizeof(keys), fi_mr_desc(cmr), 0, NULL);
        if (rret) {
            printf("fi_send: %s\n", fi_strerror((int) rret));
            return (int) rret;
        }

        ret = waitComp(cq);
        if (ret){
            return ret;
        }

        printf("[%d]connected\n", id);

        rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
        if (rret != sizeof(entry)) {
            perror("fi_eq_sread");
            return (int) rret;
        }

        if (event != FI_SHUTDOWN) {
            fprintf(stderr, "invalid event %u\n", event);
            return (int) rret;
        }
        printf("[%d] batch received:\n", id);
        printf("%d\n.\n.\n.\n%d\n", msg_buff[0], msg_buff[(config->msg_size)/4-1]);
    }

    fi_close(&ep->fid);
    return 0;
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















































// deprecated

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
	size_t buff_size = config->buff_size;
	printf("msg_size: %d, buff_size: %d, need size: %d\n", msg_size, buff_size, msg_size*config->threads);
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
    double data = 0;
    printf("[%d]job start, target: %f bytes\n", id, ctx->total_data_size);

    //time
    Clock * clock = new Clock();
    if(clock->start()){
        exit(EXIT_FAILURE);
    }
    
    while (data < ctx->total_data_size) {

        /*ret = fi_read(ep, msg_buff + msg_size * ctx->id, msg_size, fi_mr_desc(mr),
                        0, keys.addr + msg_size * ctx->id, keys.rkey, ctx);
        if (ret) {
            printf("[%d] fi_read: %s\n", thread, fi_strerror(ret));
        }

        while (!ctx->ready) {
            //wait
        }
        omp_set_lock(&ctx->lock);
        ctx->ready = 0;
        omp_unset_lock(&ctx->lock);

        int temp;
        memcpy(&temp, msg_buff + msg_size * ctx->id, msg_size);
        //printf("thread[%d] iter %d: fi_read: %d\n", ctx->id, j, temp);
        temp++;
        //printf("thread[%d] iter %d: fi_write: %d\n", ctx->id, j, temp);
        memcpy(msg_buff + msg_size * ctx->id, &temp, msg_size);*/

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
        printf("[%d] fi_write: wrote %f bytes in total \n", thread, data);
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