#include "Endpoint.h"
#include "Config.h"
using namespace std;
/* supportet flags are: FI_RECV, FI_TRANSMIT */
Endpoint::Endpoint(Domain * domain, uint64_t flag)
{
	setDomain(domain);
	flags = flag;
}

Endpoint::~Endpoint()
{
	/*fid * fid;
	if (ep) {
		fid = &(ep->fid);
		fi_cancel(fid, 0); // TODO context
		fi_close(fid);
	}
	if (sep) {
		fid = &(sep->fid);
		fi_cancel(fid, 0); // TODO context
		fi_close(fid);
	}
	//CQ
	if (&rx_cq) fi_close(&(&rx_cq)->fid);
	if (&tx_cq) fi_close(&(&tx_cq)->fid);

	//AV

	if (&rx_av) fi_close(&(&rx_av)->fid);

	if (&tx_av) fi_close(&(&tx_av)->fid);
	*/
}

int Endpoint::initEP( void * context)
{
	int ret;
	fid_cq * cq_temp;
	fid_av * av_temp;

	fi_info * info = domain->getInfo();
	fid_domain * accDomain = domain->getAccDomain();

	//configuration TODO: move to configs class
	// Completion Queue
	tx_cq_attr.wait_obj = FI_WAIT_NONE; // non blocking
	tx_cq_attr.format = FI_CQ_FORMAT_CONTEXT; // completion provides context only
	rx_cq_attr.size = info->rx_attr->size; // make sure CQ is the same size as rx_context
	tx_cq_attr.size = info->tx_attr->size; // make sure CQ is the same size as tx_context
	
	// Adress Vector
	av_attr.type = info->domain_attr->av_type;
	av_attr.count = 1; // TODO: what is this?!

	// CQ
	ret = fi_cq_open(accDomain, &rx_cq_attr, &rx_cq, NULL);
	cout << "rx cq: " << fi_strerror(ret) << ", ";
	ret = fi_cq_open(accDomain, &tx_cq_attr, &tx_cq, NULL);
	cout << "tx cq: " << fi_strerror(ret) << "\n";

	// AV TODO: apparently one adress vector is enough
	ret = fi_av_open(accDomain, &av_attr, &av, NULL);
	cout << "av: " << fi_strerror(ret) << "\n";

	// EP
	ret = fi_endpoint(domain->getAccDomain(), domain->getInfo(), &ep, context);
	cout << "ep: " << fi_strerror(ret) << ", ";
	ret = bind();
	cout << "bind: " << fi_strerror(ret) << "\n";
	ret = fi_enable(ep);
	//ret = fi_accept();
	return ret;
}

int Endpoint::initSEP(void * context)
{
	int ret;
	ret = fi_scalable_ep(domain->getAccDomain(), domain->getInfo(), &sep, context);
	return ret;
}

int Endpoint::bindRx() {
	int ret;
	ret = fi_ep_bind(ep, &av->fid, 0);
	if (ret < 0) return ret;
	ret = fi_ep_bind(ep, &rx_cq->fid, FI_RECV);
	return ret;
}

int Endpoint::bindTx() {
	int ret;
	//ret = fi_ep_bind(ep, &tx_av->fid, 0);
	//if (ret < 0) return ret;
	ret = fi_ep_bind(ep, &tx_cq->fid, FI_TRANSMIT);
	return ret;
}

int Endpoint::bind() {
	int ret = 0;
	ret = fi_ep_bind(ep, &av->fid, 0);
	if (ret < 0) return ret;
	switch(flags) {
	case FI_RECV:
		ret = fi_ep_bind(ep, &rx_cq->fid, FI_RECV);
		break;
	case FI_TRANSMIT:
		ret = fi_ep_bind(ep, &tx_cq->fid, FI_TRANSMIT);
		break;
	default:
		cout << "flag not set! context unclear - binding both transmit and recv";
		ret = fi_ep_bind(ep, &rx_cq->fid, FI_RECV);
		if (ret < 0) return ret;
		ret = fi_ep_bind(ep, &tx_cq->fid, FI_TRANSMIT);
		break;

	}
	return ret;
}

int Endpoint::pair() {
	/*size_t addrlen = MAX_CTRL_MSG_SIZE;
	fi_av_insert(rx_av, domain->getInfo()->dest_addr, 1, &remote_addr, 0, NULL);
	//fi_getname(&ep->fid, tx_ep_buf, &addrlen);
	*/
}

ssize_t Endpoint::recv()
{
	ssize_t ret;
	//TODO check if FI_RECV is bound
	ret = fi_recv(ep, rx_ep_buf, domain->getInfo()->ep_attr->max_msg_size, 0, 0, NULL);
	return ret;
}

ssize_t Endpoint::send()
{
	//TODO check if FI_TRANSMIT is bound
	/*ssize_t ret = 0;
	ret = fi_send(ep, tx_ep_buf, addrlen, 0, remote_addr, NULL);
	return ret;*/
}

int Endpoint::waitComp(struct fid_cq * cq)
{
	struct fi_cq_entry entry;
	int ret;
	//TODO: this needs to be threaded!!! or maybe an agent
	while (1) {
		ret = fi_cq_read(cq, &entry, 1);
		if (ret > 0) return 0;
		if (ret != -FI_EAGAIN) {
			struct fi_cq_err_entry err_entry;
			fi_cq_readerr(cq, &err_entry, 0);
			printf("%s %s \n", fi_strerror(err_entry.err), fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
			return ret;
		}
	}
}

//get - set
Domain * Endpoint::getDomain()
{
	return domain;
}

void Endpoint::setDomain(Domain * dom)
{
	domain = dom;
}

fid_ep * Endpoint::getEP()
{
	return ep;
}

void Endpoint::setEP(fid_ep * ep)
{
	this->ep = ep;
}

fid_ep * Endpoint::getSEP()
{
	return sep;
}

void Endpoint::setSEP(fid_ep * sep)
{
	this->sep = sep;
}