#pragma once
#include "Domain.h"
#include <vector>
//fabric includes
#include <rdma/fi_rma.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>

class Endpoint
{
public:
	Endpoint(Domain * domain, uint64_t flags);
	~Endpoint();
	//func()
	int initEP(void * context);
	int initSEP(void * context);
	int bindRx();
	int bindTx();
	int bind();

	int pair();

	ssize_t recv();
	ssize_t send();

	int waitComp(fid_cq * cq);

	//get-set
	Domain * getDomain();
	void setDomain(Domain * domain);
	struct fid_ep * getEP();
	void setEP(fid_ep * ep);
	struct fid_ep * getSEP();
	void setSEP(fid_ep * sep);

private:
	//attributes

	Domain * domain;
	uint64_t flags;

	//libfabric endpoint struct
	struct fid_ep * ep;
	struct fid_ep * sep;

	//completion queues
	struct fi_cq_attr rx_cq_attr;
	struct fi_cq_attr tx_cq_attr;
	fid_cq * rx_cq;
	fid_cq * tx_cq;

	//address vectors
	struct fi_av_attr av_attr; // logic would dictate that this should be universal TODO: move to configs
	fid_av * av;

	//TODO: use correct type for msg buffer
	void * rx_ep_buf;
	void * tx_ep_buf;

	//probably a vector class here
	void * rx_sep_buf;
	void * tx_sep_buf;


	//func()

};

