//
// Created by andreas on 27.10.18.
//

#include "benchmark.h"
#include "host2ip.h"

benchmark::benchmark(int argc, char *argv[]) {
   /* this->argc = argc;
    for (int i = 0; i < argc ; i++){
        this->argv[i] = argv[i];
    }*/
    conf = new config();
	int ret;
    switch(argc){
		case 1:
			addr = host2ip::resolve("localhost");
			port = "6666";
			flag = FI_SOURCE;
			printf("server: %s %s \n", addr, port);
			ret = init();
			printf("init: %s \n", fi_strerror(ret));
			server();
			break;
        case 2:
			addr = host2ip::resolve("localhost");
			port = argv[1];
            flag = FI_SOURCE;
			printf("server: %s %s \n", addr, port);
			ret = init();
			printf("init: %s \n", fi_strerror(ret));
            server();
            break;
        case 3:
            addr = host2ip::resolve(argv[1]);
			port = argv[2];
            flag = NULL;
			printf("client: %s %s \n", addr, port);
			ret = init();
			printf("init: %s \n", fi_strerror(ret));
            client();
            break;
        default:
            puts("test");
            break;
    }
}

benchmark::~benchmark() {
    //fi_shutdown(&ep->fid, 0);
    fi_close(&ep->fid);
    fi_close(&mr->fid);
    fi_close(&cq->fid);
    fi_close(&eq->fid);
    fi_close(&domain->fid);
    fi_close(&fabric->fid);
    fi_freeinfo(fi);
}

int benchmark::init() {
    int ret;

    ret = fi_getinfo(FIVER, addr, port, flag, conf->hints, &fi);
    if(ret != 0){
        fprintf(stderr, "fi_getinfo");
        return ret;
    }

    ret = fi_fabric(fi->fabric_attr, &fabric, NULL);
    if(ret != 0){
        fprintf(stderr, "fi_fabric");
        return ret;
    }

    memset(&eq_attr, 0, sizeof(eq_attr));
    eq_attr.size = 64;
    eq_attr.wait_obj = FI_WAIT_UNSPEC;
    ret = fi_eq_open(fabric, &eq_attr, &eq, NULL);
    if(ret != 0){
        fprintf(stderr, "fi_eq_open");
        return ret;
    }

    ret = fi_domain(fabric, fi, &domain, NULL);
    if(ret != 0){
        fprintf(stderr, "fi_domain");
        return ret;
    }

    memset(&cq_attr, 0, sizeof(cq_attr));
    cq_attr.format = FI_CQ_FORMAT_UNSPEC;
    cq_attr.size = 100;
    ret = fi_cq_open(domain, &cq_attr, &cq, NULL);
    if(ret != 0){
        fprintf(stderr, "fi_cq_open");
        return ret;
    }

    buff = malloc(conf->buff_size);
    ret = fi_mr_reg(domain, buff, conf->buff_size,
                    FI_REMOTE_READ |
                    FI_REMOTE_WRITE |
                    FI_SEND |
                    FI_RECV,
                    0, 0, 0, &mr, NULL);
    if (ret) {
        perror("fi_mr_reg");
        return -1;
    }
    desc = fi_mr_desc(mr);
    rkey = fi_mr_key(mr);

    return ret;
}

int benchmark::server() {
    int ret;
    ssize_t rret;

    ret = fi_passive_ep(fabric,fi, &pep, NULL);
    if (ret) {
        perror("fi_passive_ep");
        return ret;
    }
    ret = fi_pep_bind(pep, &eq->fid, 0);
    if (ret) {
        perror("fi_pep_bind(eq)");
        return ret;
    }
    ret = fi_listen(pep);
    if (ret) {
        perror("fi_listen");
        return ret;
    }
    while (1) {

        printf("listening\n");

        rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
        if (rret != sizeof(entry)) {
            perror("fi_eq_sread");
            return (int)rret;
        }

        if (event != FI_CONNREQ) {
            fprintf(stderr, "invalid event %u\n", event);
            return -1;
        }

        printf("connection request\n");

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

        ret = fi_accept(ep, NULL, 0);
        if (ret) {
            perror("fi_accept");
            return ret;
        }

        rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
        if (rret != sizeof(entry)) {
            perror("fi_eq_sread");
            return (int)rret;
        }

        if (event != FI_CONNECTED) {
            fprintf(stderr, "invalid event %u\n", event);
            return -1;
        }

        memcpy(buff, &rkey, sizeof(rkey));

        rret = fi_send(ep, buff, sizeof(rkey), desc, 0, NULL);
        if (rret) {
            perror("fi_send");
            return (int)rret;
        }

        struct fi_cq_msg_entry comp;
        rret = fi_cq_sread(cq, &comp, 1, NULL, -1);
        if (rret != 1) {
            perror("fi_cq_sread");
            return rret;
        }

        printf("connected\n");

        rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
        if (rret != sizeof(entry)) {
            perror("fi_eq_sread");
            return (int)rret;
        }

        if (event != FI_SHUTDOWN) {
            fprintf(stderr, "invalid event %u\n", event);
            return -1;
        }

        fi_close(&ep->fid);
    }

    return 0;

}

int benchmark::client() {

    int ret;
    void * vaddr;
    printf("addr: %s \n", addr);
    size_t addrlen = 0;

    //FIXME memory access violation?
    /*puts("test1");
    fi_getname(&ep->fid, vaddr, &addrlen);
    puts("test2");
    vaddr = malloc(addrlen);
    puts("test3");
    ret = fi_getname(&ep->fid, vaddr, &addrlen);
    puts("test4");
    printf("vaddr: %s", vaddr); */

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
    rret = fi_recv(ep, buff, sizeof(rkey), desc, 0, NULL);
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
    fprintf(stderr, "fi_eq_sread: %s\n", fi_strerror(rret));
    if (rret != sizeof(entry)) {
        perror("fi_eq_sread");
        return (int)rret;
    }

    if (event != FI_CONNECTED) {
        fprintf(stderr, "invalid event %u\n", event);
        return -1;
    }
    printf("connected\n");

    struct fi_cq_msg_entry comp;
    rret = fi_cq_sread(cq, &comp, 1, NULL, -1);
    if (rret != 1) {
        perror("fi_cq_sread");
        return rret;
    }

    memcpy(&rkey, buff, sizeof(rkey));

    int run = 1;
    while (run) {
        ret = fi_cq_sread(cq, &comp, 1, NULL, 1000);
        if (!run)
            break;
        if (ret == -FI_EAGAIN)
            continue;

        if (ret != 1) {
            perror("fi_cq_sread");
            break;
        }

        if (comp.flags & FI_READ) {
            rret = fi_read(ep, buff, conf->buff_size, desc,
                          0, 0, rkey, NULL);
            if (rret) {
                perror("fi_read");
                break;
            }
            run = 0;
        }
    }

    return 0;

}
