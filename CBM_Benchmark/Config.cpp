#include "Config.h"



const char * Config::console_spacer()
{
	const char * spacer = "--------------------\n";
	return spacer;
}

Config::Config()
{
    //Default Node configs
	addr = "127.0.0.1";
    port = "6666"; // max len 10
    num_pn = 10;
    num_en = 10;
    num_ep = 10;
    threads = 10;
    total_data_size = 1024*1024*1024;
    max_packet_size = 1024;

	buff_size = 32 * 1024 * 1024;
	msg_size = sizeof(int); //TODO determine datatype
	ctrl_size = 32 * 1024 * 1024;

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
	hints->ep_attr->type = FI_EP_MSG;
	hints->domain_attr->mr_mode = FI_MR_BASIC;
	hints->caps = FI_MSG | FI_RMA;
	hints->mode = FI_CONTEXT | FI_LOCAL_MR | FI_RX_CQ_DATA;
}


Config::~Config()
{
	fi_freeinfo(hints);
}
