#include "Config.h"



const char * Config::console_spacer()
{
	const char * spacer = "--------------------\n";
	return spacer;
}

Config::Config()
{
	buff_size = 32 * 1024 * 1024;
	msg_size = 10;
	ctrl_size = 32 * 1024 * 1024;

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
