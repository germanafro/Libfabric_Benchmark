#include "Endpoint.h"
#include <omp.h>
using namespace std;
/*
 *
 * TODO Docstring
 */
Endpoint::Endpoint(const char * addr, char * port,  uint64_t flags, Config * config)
{
    this->addr = addr;
    this->port = port;
    this->flags = flags;
    this->config = config;

    ctx = (struct ctx *)calloc(config->threads, sizeof(*ctx));
    if (!ctx) {
        perror("calloc");
    }

    int i;
    for (i = 0; i < config->threads; i++) {
        omp_init_lock(&ctx[i].lock);
        ctx[i].ready = 0;
        ctx[i].total_data_size = config->total_data_size;
        ctx[i].size = config->max_packet_size;
    }
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
}

int Endpoint::init(int thread)
{
    int ret;
    struct fi_eq_attr eq_attr = config->eq_attr;
    struct fi_cq_attr cq_attr = config->cq_attr;
    printf("[%d] initializing Endpoint | addr: %s, port: %s\n", thread, addr, port);
    ret= fi_getinfo(FIVER, addr, port, flags, config->hints, &fi);
    if (ret) {
        printf("[%d] fi_getinfo: %s\n", thread, fi_strerror(ret));
        return ret;
    }

    ret = fi_fabric(fi->fabric_attr, &fabric, NULL);
    if (ret) {
        printf("[%d] fi_fabric: %s\n", thread, fi_strerror(ret));
        return ret;
    }
    printf("[%d] fi_getinfo: %s %s\n", thread, fi->fabric_attr->prov_name, fi->fabric_attr->name);


    ret = fi_eq_open(fabric, &eq_attr, &eq, NULL);
    if (ret) {
        printf("[%d] fi_eq_open: %s\n", thread, fi_strerror(ret));
        return ret;
    }

    ret = fi_domain(fabric, fi, &domain, NULL);
    if (ret) {
        printf("[%d] fi_domain: %s\n", thread, fi_strerror(ret));
        return ret;
    }

    ret = fi_cq_open(domain, &cq_attr, &cq, NULL);
    if (ret) {
        printf("[%d] fi_cq_open: %s\n", thread, fi_strerror(ret));
        return ret;
    }

    msg_buff = (int *) malloc(config->buff_size);
    if (!msg_buff) {
        printf("[%d] error malloc msg\n", thread);
        return ret;
    }
    ctrl_buff = malloc(config->ctrl_size *10);
    if (!ctrl_buff) {
        printf("[%d] error malloc ctrl\n", thread);
        return ret;
    }

    ret = fi_mr_reg(domain, msg_buff, config->buff_size,
                    FI_REMOTE_READ | FI_REMOTE_WRITE | FI_SEND | FI_RECV,
                    0, 0, 0, &mr, NULL);
    if (ret) {
        printf("[%d] fi_mr_reg: %s\n", thread, fi_strerror(ret));
        return ret;
    }
	return 0;
}

int Endpoint::cq_thread()
{
    struct fi_cq_msg_entry comp;
    ssize_t ret;
    struct fi_cq_err_entry err;
    const char *err_str;
    struct fi_eq_entry eq_entry;
    uint32_t event;

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

int Endpoint::client_thread(struct ctx * ctxx )
{
    int k = 0;
    size_t msg_size = config->msg_size;
    int arraysize = msg_size / sizeof(int);
    int message [arraysize];
    // generate testdata
    for (int i =0 ; i< arraysize; i++){
        message[i] = i;
    }
    printf("message size: %d , buff size %d\n", (message[arraysize-1] + 1)* sizeof(int), config->buff_size);

#pragma omp parallel num_threads(config->threads)
    {
     #pragma omp for
        for (int i = 0; i < config->threads; i++)
            {
                int thread = omp_get_thread_num();
                for (int j=0; j<ctx->total_data_size; j += msg_size) {
                    ctxx[i].id = i;
                    struct ctx *ctx = &ctxx[thread];

                    ssize_t ret;

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
                    printf("debug 1\n");
                    memcpy(msg_buff + msg_size * ctx->id, &message, msg_size);
                    printf("debug 2\n");
                    ret = fi_write(ep, msg_buff + msg_size * ctx->id, msg_size, fi_mr_desc(mr),
                                   0, keys.addr + msg_size * ctx->id, keys.rkey, ctx);
                    if (ret) {
                        printf("[%d] fi_write: %s\n", thread, fi_strerror(ret));
                    }

                    while (!ctx->ready) {
                        //wait
                    }
                    printf("[%d] fi_write: wrote %d bytes", thread, j + msg_size);
                    omp_set_lock(&ctx->lock);
                    ctx->ready = 0;
                    omp_unset_lock(&ctx->lock);

                    //omp_destroy_lock(&ctx->lock);
                }
            }
    }
    printf("job done\n");
    return 0;
}

int Endpoint::client(int thread)
{
    int ret;
    int k = 0;

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

    ssize_t rret;
    rret = fi_recv(ep, ctrl_buff, sizeof(keys), fi_mr_desc(mr), 0, NULL);
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
        printf("[%d] %s %s \n", thread, fi_strerror(err_entry.err), fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
        return ret;
    }



    struct fi_cq_msg_entry comp;
    ret = fi_cq_sread(cq, &comp, 1, NULL, -1);
    if (ret != 1) {
        perror("fi_cq_sread");
        return ret;
    }

    memcpy(&keys, ctrl_buff, sizeof(keys));
    printf("[%d] connected\n", thread);
    run = 1;
#pragma omp parallel num_threads(2)
    {
#pragma omp sections
        {
#pragma omp section
            {
                cq_thread();
            }
#pragma omp section
            {
                client_thread(ctx);
                run = 0;
            }

        }
    }
    return 0;
}

int Endpoint::server(int thread)
{
    int ret;
    int k = 0;

    keys.addr = (uint64_t) msg_buff;
    keys.rkey = fi_mr_key(mr);

    ret = fi_passive_ep(fabric, fi, &pep, NULL);
    if (ret) {
        printf("[%d]fi_passive_ep: %s\n", thread, fi_strerror(ret));
        return ret;
    }

    ret = fi_pep_bind(pep, &eq->fid, 0);
    if (ret) {
        printf("[%d]fi_pep_bind(eq): %s\n", thread, fi_strerror(ret));
        return ret;
    }

    ret = fi_listen(pep);
    if (ret) {
        printf("[%d]fi_listen: %s\n", thread, fi_strerror(ret));
        return ret;
    }
    size_t buff_Size = config->buff_size;
    struct fi_eq_cm_entry entry;
    uint32_t event;
    ssize_t rret;

    while (1) {

        printf("[%d]listening\n", thread);

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

        printf("[%d]connection request: %s\n", thread, entry.info->fabric_attr->name);

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

        /*ret = fi_enable(ep);
        if (ret) {
            perror("fi_enable");
            return ret;
        }*/

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

        memcpy(ctrl_buff, &keys, sizeof(keys));
        /*int temp[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        memcpy(msg_buff, &temp, sizeof(int) * 10);*/
        int N = config->buff_size / sizeof(int);
        for(int i=0; i<N; i++){
            msg_buff[i] = 0;
        }

        rret = fi_send(ep, ctrl_buff, sizeof(keys), fi_mr_desc(mr), 0, NULL);
        if (rret) {
            printf("fi_send: %s\n", fi_strerror((int) rret));
            return (int) rret;
        }

        struct fi_cq_msg_entry comp;
        ret = fi_cq_sread(cq, &comp, 1, NULL, -1);
        if (ret < 1) {
            perror("fi_cq_sread");
            return ret;
        }

        printf("[%d]connected\n", thread);

        rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
        if (rret != sizeof(entry)) {
            perror("fi_eq_sread");
            return (int) rret;
        }

        if (event != FI_SHUTDOWN) {
            fprintf(stderr, "invalid event %u\n", event);
            return (int) rret;
        }
        //memcpy(&temp, msg_buff, sizeof(int) * 10);
        printf("[%d] batch received:\n", thread);
        printf("%d\n.\n.\n.\n%d\n", msg_buff[0], msg_buff[N-1]);


        fi_close(&ep->fid);
    }
    return ret;
}

int Endpoint::waitComp(struct fid_cq * cq)
{
	struct fi_cq_entry entry;
	int ret;
	//TODO: this needs to be threaded!!! or maybe an agent
	while (1) {
		ret = fi_cq_read(cq, &entry, 1);
		if (ret > 0) return 0;
		if (ret != -FI_EAGAIN) {
			struct fi_cq_err_entry err_entry;
			fi_cq_readerr(cq, &err_entry, 0);
			printf("%s %s \n", fi_strerror(err_entry.err), fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
			return ret;
		}
	}
}

