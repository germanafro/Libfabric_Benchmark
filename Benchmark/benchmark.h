//
// Created by andreas on 27.10.18.
//
#include "config.h"
#ifndef BENCHMARK_BENCHMARK_H
#define BENCHMARK_BENCHMARK_H


class benchmark {
public:
    benchmark(int argc, char *argv[]);

    virtual ~benchmark();

    int init();
    int comp(struct fid_cq * cq);
    int server();
    int client();


private:
    config * conf;
    void * buff;

    struct fi_info * fi;
    struct fid_fabric * fabric;
    struct fi_eq_attr eq_attr;
    struct fid_eq * eq;
    struct fid_domain * domain;
    struct fi_cq_attr cq_attr;
    struct fid_cq * rx_cq;
    struct fid_cq * tx_cq;
    struct fi_av_attr av_attr;
    struct fid_av * av;
    struct fid_mr * mr;
    void * desc;
    uint64_t rkey;


    struct fid_ep * ep;
    struct fid_pep * pep;
    uint32_t event;

    int argc;

    char* addr;
	char* port;
	unsigned long long flag;
	fi_addr_t remote_addr;

    struct fi_eq_cm_entry entry;
    char * argv[];


};


#endif //BENCHMARK_BENCHMARK_H
