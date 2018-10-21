#include "Node.h"



Node::Node(const char *addr, uint64_t flags, Config * config, void *buff)
{
	this->addr = addr;
	this->flags = flags;
	this->config = config;
	this->buff = buff;
}

// yeah this will cause big problems if we didnt init() yet... so always run init() ^-^
Node::~Node()
{
	fi_shutdown(ep, 0);
	fi_close(&ep->fid);
	fi_close(&mr->fid);
	fi_close(&cq->fid);
	fi_close(&eq->fid);
	fi_close(&domain->fid);
	fi_close(&fabric->fid);
	fi_freeinfo(fi);
}

int Node::init()
{
	int ret;

	ret = fi_getinfo(FIVER, addr, "1234", flags, config->hints, &fi);
	if (ret) {
		perror("fi_getinfo");
		return ret;
	}

	ret = fi_fabric(fi->fabric_attr, &fabric, NULL);
	if (ret) {
		perror("fi_fabric");
		return ret;
	}

	struct fi_eq_attr eq_attr;
	eq_attr.size = 0;
	eq_attr.flags = 0;
	eq_attr.wait_obj = FI_WAIT_UNSPEC;
	eq_attr.signaling_vector = 0;
	eq_attr.wait_set = NULL;

	ret = fi_eq_open(fabric, &eq_attr, &eq, NULL);
	if (ret) {
		perror("fi_eq_open");
		return ret;
	}

	ret = fi_domain(fabric, fi, &domain, NULL);
	if (ret) {
		perror("fi_domain");
		return ret;
	}

	struct fi_cq_attr cq_attr;
	cq_attr.size = 0;
	cq_attr.flags = 0;
	cq_attr.format = FI_CQ_FORMAT_MSG;
	cq_attr.wait_obj = FI_WAIT_UNSPEC;
	cq_attr.signaling_vector = 0;
	cq_attr.wait_cond = FI_CQ_COND_NONE;
	cq_attr.wait_set = NULL;


	ret = fi_cq_open(domain, &cq_attr, &cq, NULL);
	if (ret) {
		perror("fi_cq_open");
		return ret;
	}

	ret = fi_mr_reg(domain, buff, config->buff_size,
		FI_REMOTE_READ | FI_REMOTE_WRITE | FI_SEND | FI_RECV,
		0, 0, 0, &mr, NULL);
	if (ret) {
		perror("fi_mr_reg");
		return -1;
	}

	return 0;
}