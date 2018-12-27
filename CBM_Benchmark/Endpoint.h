#pragma once
#include "Config.h"
class Endpoint
{
public:
	Endpoint(const char * addr, char * port,  uint64_t flags, Config * config, int id);
	~Endpoint();
	//func()
	int init();
	int waitComp(fid_cq * cq);

    int client();
    int connectToServer();
    int setDataBuffer();
    struct transfer writeToServer();
    struct transfer readFromServer();

    int server();
    int listenServer();


    //deprecated
    int client_thread(struct ctx * ctx);
    int cq_thread();

    int run;
    int id;
    const char * addr;
    char * av_addr;
    char * port;
    uint64_t flags;
    Config * config;


    struct ctx * ctx;
    int * msg_buff;
    void * ctrl_buff;
    struct keys keys;

    struct fi_info *fi;
    struct fid_fabric *fabric;
    struct fid_eq *eq;
    struct fid_cq *cq;
    struct fid_domain *domain;
    struct fid_mr *mr;
	struct fid_mr *cmr;

    struct fid_ep * ep;
    struct fid_pep * pep;


private:









};

