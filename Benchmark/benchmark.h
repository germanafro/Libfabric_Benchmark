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
    struct fid_cq * cq;
    struct fid_mr * mr;
    void * desc;
    uint64_t rkey;

    struct fid_ep * ep;
    struct fid_pep * pep;
    struct fi_eq_cm_entry entry;
    uint32_t event;

    char* addr;
	char* port;
	char* flag;


    //int argc;
    //char * argv[];
};


#endif //BENCHMARK_BENCHMARK_H
