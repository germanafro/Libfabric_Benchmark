#include "ProcessingNode.h"



ProcessingNode::ProcessingNode(const char *addr, uint64_t flags, Config * config, void *buff, struct keys keys) : Node(addr, flags, config, buff)
{
	this->keys = keys;
}


ProcessingNode::~ProcessingNode()
{
}

int ProcessingNode::initServer()
{
	int ret;

	keys.rkey = fi_mr_key(mr);

	ret = fi_passive_ep(fabric, fi, &pep, NULL);
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

	struct fi_eq_cm_entry entry;
	uint32_t event;
	ssize_t rret;

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

		memcpy(buff, &keys, sizeof(keys));

		rret = fi_send(ep, buff, sizeof(keys), fi_mr_desc(mr), 0, NULL);
		if (rret) {
			perror("fi_send");
			return (int)rret;
		}

		struct fi_cq_msg_entry comp;
		ret = fi_cq_sread(cq, &comp, 1, NULL, -1);
		if (ret != 1) {
			perror("fi_cq_sread");
			return ret;
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