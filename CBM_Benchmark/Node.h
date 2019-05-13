#pragma once
#include "Config.h"
#include "Endpoint.h"
#include <vector>
//etc
#include <iostream>
class Node
{
public:
	Node(int id, uint64_t flags, Config * config);
	~Node();

    int init(int mode);
    //client defines
    int connectToController();
    int connectToServers();
	int setDataBuffers();
    int benchmark();
    //server defines
    int listenServer();
    //controller defines
    int listenController();
    int controllerStart();
    int controllerConnect();
    int controllerStop();
    int controllerCheckpoint();

    int checkComp(struct fid_cq * cq, struct fi_cq_msg_entry * entry);
    int checkCmEvent(uint32_t * event, struct fi_eq_cm_entry * entry);
    int waitComp(struct fid_cq * cq, struct fi_cq_msg_entry * entry);
    int waitCmEvent(uint32_t * event, struct fi_eq_cm_entry * entry);
    int waitAllConn(int goal);

	int run;
    int id;
    int mode;
    char * addr;
    char * port;
    uint64_t flags;
    Config * config;
    std::vector<struct Endpoint*> eps;

    struct fi_info *fi;
    struct fid_fabric *fabric;
    struct fid_domain *domain;
    Endpoint *cep;    // endpoint resposnsible for connecting to the controller
    struct fid_eq *eq;
    struct fid_cq *cq_tx; // FI_TRANSMIT
    struct fid_cq *cq_rx; // FI_RECV

    struct transfer * summary;
    struct transfer * checkpoint;
    std::vector<struct transfer> deltas;
};

