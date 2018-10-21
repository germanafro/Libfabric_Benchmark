#include "InputNode.h"
#include "Config.h"

using namespace std;
/* m receive, n transfer node */
InputNode::InputNode(const char *addr, uint64_t flags, Config * config, void *buff, struct keys keys) : Node(addr, flags, config, buff)
{
	this->keys = keys;


	/*// init the EPs
	cout << "InputNode().rx_ep.initEP():\n";
	for (int i = 0; i < m; i++) {
		rx_ep.push_back(*new Endpoint(domain, FI_RECV));
		ret = rx_ep[i].initEP(0);
		cout << fi_strerror(ret) << "\n";
	}

	cout << Config::console_spacer() << "InputNode().tx_ep.initEP():\n";
	for (int i = 0; i < n; i++) {
		tx_ep.push_back(*new Endpoint(domain, FI_TRANSMIT));
		ret = tx_ep[i].initEP(0);
		cout << fi_strerror(ret) << "\n";
	}
	cout << Config::console_spacer();
	*/
}

InputNode::~InputNode()
{
	//cleanup 
	
	// note: Endpoints should be cleaned up within the respective class
}

void * InputNode::cq_thread(void * arg)
{
	struct fi_cq_msg_entry comp;
	ssize_t ret;
	struct fi_cq_err_entry err;
	const char *err_str;
	struct fi_eq_entry eq_entry;
	uint32_t event;

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
			struct ctx *ctx = (struct ctx*)comp.op_context;
			pthread_mutex_lock(&ctx->lock);
			ctx->ready = 1;
			pthread_cond_signal(&ctx->cond);
			pthread_mutex_unlock(&ctx->lock);
		}
	}

	return NULL;
}

int InputNode::initClient()
{
	int ret;

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
	rret = fi_recv(ep, buff, sizeof(keys), fi_mr_desc(mr), 0, NULL);
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
	if (rret != sizeof(entry)) {
		perror("fi_eq_sread");
		return (int)rret;
	}

	if (event != FI_CONNECTED) {
		fprintf(stderr, "invalid event %u\n", event);
		return -1;
	}

	struct fi_cq_msg_entry comp;
	ret = fi_cq_sread(cq, &comp, 1, NULL, -1);
	if (ret != 1) {
		perror("fi_cq_sread");
		return ret;
	}

	memcpy(&keys, buff, sizeof(keys));

	run = 1;
	ret = pthread_create(&thread, NULL, cq_thread, NULL);
	if (ret) {
		perror("pthread_create");
		return ret;
	}

	printf("connected\n");

	return 0;
}
