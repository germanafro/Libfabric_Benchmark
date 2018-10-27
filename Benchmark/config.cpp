//
// Created by andreas on 27.10.18.
//

#include "config.h"

char * config::console_spacer()
{
    char spacer[] = "--------------------\n";
    return spacer;
}

config::config()
{
    buff_size = 32 * 1024 * 1024;

    hints = fi_allocinfo();
    if (!hints) {
        perror("fi_allocinfo");
    }
    hints->addr_format = FI_SOCKADDR;
    hints->ep_attr->type = FI_EP_MSG;
    hints->domain_attr->mr_mode = FI_MR_BASIC;
    hints->caps = FI_MSG | FI_RMA;
    hints->mode = FI_LOCAL_MR;
}


config::~config()
{
    fi_freeinfo(hints);
}