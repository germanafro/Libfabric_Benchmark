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
			addr = NULL;
			port = "6666";
			flag = FI_SOURCE;
			printf("server: %s %s \n", addr, port);
			ret = init();
			printf("init: %s \n", fi_strerror(ret));
			server();
			break;
        case 2:
			addr = NULL;
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
    fi_close(&tx_cq->fid);
    fi_close(&rx_cq->fid);
    fi_close(&eq->fid);
    fi_close(&domain->fid);
    fi_close(&fabric->fid);
    fi_freeinfo(fi);
}

int benchmark::init() {
    int ret;

    ret = fi_getinfo(FIVER, addr, port, flag, conf->hints, &fi);
    if(ret != 0){
        perror("fi_getinfo");
        return ret;
    }
    printf("fi_getinfo: %s %s %s %s \n", fi->src_addr, fi->dest_addr, fi->fabric_attr->name, fi->fabric_attr->prov_name);
    ret = fi_fabric(fi->fabric_attr, &fabric, NULL);
    if(ret != 0){
        perror("fi_fabric");
        return ret;
    }

    memset(&eq_attr, 0, sizeof(eq_attr));
    eq_attr.size = 64;
    eq_attr.wait_obj = FI_WAIT_UNSPEC;
    ret = fi_eq_open(fabric, &eq_attr, &eq, NULL);
    if(ret != 0){
        perror("fi_eq_open");
        return ret;
    }

    ret = fi_domain(fabric, fi, &domain, NULL);
    if(ret != 0){
        perror("fi_domain");
        return ret;
    }

    memset(&cq_attr, 0, sizeof(cq_attr));
    cq_attr.wait_obj = FI_WAIT_NONE;
    cq_attr.format = FI_CQ_FORMAT_CONTEXT;
    cq_attr.size = fi->tx_attr->size;
    ret = fi_cq_open(domain, &cq_attr, &tx_cq, NULL);
    if(ret != 0){
        perror("fi_cq_open");
        return ret;
    }
    cq_attr.size = fi->rx_attr->size;
    ret = fi_cq_open(domain, &cq_attr, &rx_cq, NULL);
    if(ret != 0){
        perror("fi_cq_open");
        return ret;
    }

    memset(&av_attr, 0, sizeof(av_attr));
    av_attr.type = fi->domain_attr->av_type;
    av_attr.count = 1; // TODO what does this do exactly?
    ret = fi_av_open(domain, &av_attr, &av, NULL);
    if (ret) {
        perror("fi_av_open");
        return -1;
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

    ret = fi_endpoint(domain, fi, &ep, NULL);
    if(ret){
        perror("fi_endpoint");
        return ret;
    }

    ret = fi_ep_bind(ep, &av->fid, 0);
    if(ret){
        perror("fi_ep_bind(av)");
        return ret;
    }

    ret = fi_ep_bind(ep, &tx_cq->fid, FI_TRANSMIT);
    if(ret){
        perror("fi_ep_bind(tx_cq)");
        return ret;
    }

    ret = fi_ep_bind(ep, &rx_cq->fid, FI_RECV);
    if(ret){
        perror("fi_ep_bind(rx_cq)");
        return ret;
    }

    ret = fi_ep_bind(ep, &eq->fid, 0);
    if (ret) {
        perror("fi_ep_bind(eq)");
        return ret;
    }

    ret = fi_enable(ep);
    if (ret) {
        perror("fi_enable");
        return ret;
    }

    return ret;
}

int benchmark::comp(struct fid_cq *cq){
    struct fi_cq_entry entry;
    ssize_t ret;

    while(1){
        ret = fi_cq_read(cq, &entry, 1);
        if(ret > 0)
            return 0;
        if(ret != -FI_EAGAIN){
            struct fi_cq_err_entry err_entry;
            fi_cq_readerr(cq, &err_entry, 0);
            printf("%s %s \n", fi_strerror(err_entry.err), fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
            return (int)ret;
        }
    }

}

int benchmark::server() {
    int ret;
    ssize_t rret;
    size_t addrlen = 16; //FIXME this is addrlen size for sockets provider!!


    //wait for connection
    printf("listening: \n");
    fi_recv(ep, buff, sizeof(buff), 0, 0, NULL); // waiting to receive client address
    comp(rx_cq);
    fi_av_insert(av, buff, 1, &remote_addr, 0, NULL); // store client address

    fi_recv(ep, buff, sizeof(buff), 0, 0, NULL); //queue up next listen
    fi_send(ep, buff, 1, NULL, remote_addr, NULL); //and tell client you're ready
    comp(tx_cq);

    //at this point we should eb connected
    printf("connected: \n");

    while(1){
        comp(rx_cq);
        fi_recv(ep, buff, conf->buff_size, 0, 0, NULL);
        if(strcmp((char*)buff, "quit")){
            return 0;
        }
        fi_send(ep, buff, conf->buff_size, NULL, remote_addr, NULL);
        comp(tx_cq);
    }

}

int benchmark::client() {
    int k = 0; //TODO debug counter - remove me
    int ret;
    ssize_t rret;
    size_t buff_addrlen = 100; //FIXME
    size_t local_addrlen = 100; //FIXME
    size_t remote_addrlen = 100; //FIXME
    char * laddr = new char(local_addrlen);
    char * raddr = new char(remote_addrlen);

    //laddr = (char*)malloc(sizeof(char*) * 30);
    //raddr = (char*)malloc(sizeof(char*) * 30);

    ret = fi_av_insert(av, &fi->dest_addr, 1, &remote_addr, 0, NULL); // insert server address
    if (ret) {
        printf("fi_av_insert: %s\n", strerror(ret));
        return -1;
    }

    ret = fi_getname(&ep->fid, buff, &buff_addrlen); // get client address
    if (ret) {
        printf("fi_getname: %s\n", strerror(ret));
       // return -1;
    }

    fi_av_straddr(av, buff, laddr, &local_addrlen);
    fi_av_straddr(av, &remote_addr, raddr, &remote_addrlen);
    printf("client: local: %s (%d), remote: %s (%d) \n", laddr, local_addrlen, raddr, remote_addrlen);

    rret = fi_send(ep, buff, buff_addrlen, NULL, remote_addr, NULL); // send client address to server address
    if (rret) {
        perror("fi_send");
        return (int)ret;
    }
    comp(rx_cq);
    rret = fi_recv(ep, buff, sizeof(buff), 0, 0, NULL); // FIXME at this point we should probably only allow recv from remote_addr
    if (rret) {
        perror("fi_recv");
        return (int)ret;
    }
    comp(tx_cq);

    //at this point we should be connected
    printf("connected: \n");

    //now lets do stuff
    for (int i=0; i< 10; i++){
        fi_send(ep, buff, conf->buff_size, NULL, remote_addr, NULL);
        comp(tx_cq);
        comp(rx_cq);
        fi_recv(ep, buff, conf->buff_size, 0, 0, NULL);
        printf("%d \n", i);
    }

    // quit
    char * quit = "quit";
    memcpy(buff, quit, sizeof(quit));
    fi_send(ep, buff, sizeof(quit), NULL, remote_addr, NULL);
    comp(tx_cq);
    comp(rx_cq);
    fi_recv(ep, buff, conf->buff_size, 0, 0, NULL);
    memcpy(quit, buff, sizeof(quit));
    printf("%s \n", quit);
}

/**int benchmark::server() {
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
    //puts("test1");
    //fi_getname(&ep->fid, vaddr, &addrlen);
    //puts("test2");
    //vaddr = malloc(addrlen);
    //puts("test3");
    //ret = fi_getname(&ep->fid, vaddr, &addrlen);
    //puts("test4");
    //printf("vaddr: %s", vaddr);

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

}*/
