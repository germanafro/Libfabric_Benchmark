#include "Config.h"



const char * Config::console_spacer()
{
	const char * spacer = "--------------------\n";
	return spacer;
}

Config::Config()
{
    //Default Node configs
	addr = NULL;
    port = strdup("6666"); // max len 10
    num_pn = 10;
    num_en = 10;
    num_ep = 10;
    slot = 0;
	threads = 10;
    total_data_size = 1024*1024*1024; // default 1 GB - - the total amount of traffic generated will be this * num_ep * threads
    max_packet_size = 1024*1024; // TODO how big can Messages be?

	msg_size = 1024*1024; // 1MB the size of each remote write operation per thread
    buff_size = threads*msg_size; // each thread takes up one msg_size space
	ctrl_size = 1 * 1024 * 1024; // TODO this can probably be reduced to a few bytes

	//Event queue config
    eq_attr.size = 0;
    eq_attr.flags = 0;
    eq_attr.wait_obj = FI_WAIT_UNSPEC;
    eq_attr.signaling_vector = 0;
    eq_attr.wait_set = NULL;

    //Completion queue config
	cq_attr.size = 0;
	cq_attr.flags = 0;
	cq_attr.format = FI_CQ_FORMAT_MSG;
	cq_attr.wait_obj = FI_WAIT_UNSPEC;
	cq_attr.signaling_vector = 0;
	cq_attr.wait_cond = FI_CQ_COND_NONE;
	cq_attr.wait_set = NULL;


	hints = fi_allocinfo();
	if (!hints) {
		perror("fi_allocinfo");
	}
	hints->addr_format = FI_SOCKADDR_IN;
	hints->fabric_attr->prov_name = strdup("verbs");
	hints->ep_attr->type = FI_EP_MSG;
	hints->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_ALLOCATED | FI_MR_PROV_KEY | FI_MR_VIRT_ADDR;
	//hints->domain_attr->mr_mode = FI_MR_BASIC;
	hints->caps = FI_MSG | FI_RMA;
	hints->mode = FI_RX_CQ_DATA | FI_CONTEXT; // | FI_LOCAL_MR;
}


Config::~Config()
{
	fi_freeinfo(hints);
}
