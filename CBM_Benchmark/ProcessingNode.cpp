#include "ProcessingNode.h"
#include "host2ip.h"
#include <omp.h>

ProcessingNode::ProcessingNode(const char *addr, uint64_t flags, Config * config) : Node(addr, flags, config)
{

}


ProcessingNode::~ProcessingNode()
{
}

int ProcessingNode::initServer()
{
	int ret;
	omp_set_dynamic(0);
	omp_set_num_threads(10);
	int int_port = atoi(config->default_port);
#pragma omp parallel for
	for (int n=0; n<10; n++) {
		int thread = omp_get_thread_num();
        struct fi_info *fi;
		int init = 1;

		keys.addr = (uint64_t) msg_buff;
		keys.rkey = fi_mr_key(mr);

        //char *port; = itoa();
        char *port = new char(10);
        sprintf(port,"%d",int_port+thread);
        printf("[%d] addr: %s, port: %s\n", thread, addr, port);
        ret = fi_getinfo(FIVER, addr, port, flags, config->hints, &fi);
        if(ret){
            printf("[%d]fi_getinfo: %s\n", thread, fi_strerror(ret));
            init = 0;
        }
		ret = fi_passive_ep(fabric, fi, &pep, NULL);
		if (ret) {
			printf("[%d]fi_passive_ep: %s\n", thread, fi_strerror(ret));
			init = 0;
		}

		ret = fi_pep_bind(pep, &eq->fid, 0);
		if (ret) {
			printf("[%d]fi_pep_bind(eq): %s\n", thread, fi_strerror(ret));
			init = 0;
		}

		ret = fi_listen(pep);
		if (ret) {
			printf("[%d]fi_listen: %s\n", thread, fi_strerror(ret));
			init = 0;
		}
		size_t buff_Size = config->buff_size;
		struct fi_eq_cm_entry entry;
		uint32_t event;
		ssize_t rret;

		while (init) {

			printf("[%d]listening\n", n);

			rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
			if (rret > 0) {
				if (event != FI_CONNREQ) {
					fprintf(stderr, "invalid event %u\n", event);
					ret = (int) rret;
					break;
				}
			} else if (rret != -FI_EAGAIN) {
				struct fi_eq_err_entry err_entry;
				fi_eq_readerr(eq, &err_entry, 0);
				printf("%s %s \n", fi_strerror(err_entry.err),
					   fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
				ret = (int)rret;
				break;
			}

			printf("[%d]connection request: %s\n", n, entry.info->fabric_attr->name);

			ret = fi_endpoint(domain, entry.info, &ep, NULL);
			if (ret) {
				perror("fi_endpoint");
				break;
			}

			ret = fi_ep_bind(ep, &eq->fid, 0);
			if (ret) {
				perror("fi_ep_bind(eq)");
				break;
			}

			ret = fi_ep_bind(ep, &cq->fid, FI_TRANSMIT | FI_RECV);
			if (ret) {
				perror("fi_ep_bind(cq)");
				break;
			}

			/*ret = fi_enable(ep);
            if (ret) {
                perror("fi_enable");
                return ret;
            }*/

			ret = fi_accept(ep, NULL, 0);
			if (ret) {
				perror("fi_accept");
				break;
			}

			rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
			if (rret > 0) {
				if (event != FI_CONNECTED) {
					fprintf(stderr, "invalid event %u\n", event);
					ret = (int) rret;
					break;
				}
			} else if (rret != -FI_EAGAIN) {
				struct fi_eq_err_entry err_entry;
				fi_eq_readerr(eq, &err_entry, 0);
				printf("%s %s \n", fi_strerror(err_entry.err),
					   fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
				ret = (int)rret;
				break;
			}

			memcpy(ctrl_buff, &keys, sizeof(keys));

			int temp[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
			memcpy(msg_buff, &temp, sizeof(int) * 10);


			rret = fi_send(ep, ctrl_buff, sizeof(keys), fi_mr_desc(mr), 0, NULL);
			if (rret) {
				printf("fi_send: %s\n", fi_strerror((int) rret));
				ret = (int) rret;
				break;
			}

			struct fi_cq_msg_entry comp;
			ret = fi_cq_sread(cq, &comp, 1, NULL, -1);
			if (ret != 1) {
				perror("fi_cq_sread");
				break;
			}

			printf("[%d]connected\n", n);

			rret = fi_eq_sread(eq, &event, &entry, sizeof(entry), -1, 0);
			if (rret != sizeof(entry)) {
				perror("fi_eq_sread");
				ret = (int) rret;
				break;
			}

			if (event != FI_SHUTDOWN) {
				fprintf(stderr, "invalid event %u\n", event);
				ret = (int)rret;
				break;
			}
			memcpy(&temp, msg_buff, sizeof(int) * 10);
			printf("[%d] batch received:\n", n);
			for (int i = 0; i < 10; i++) {
				printf("%d\n", temp[i]);
			}

			fi_close(&ep->fid);
		}
	}
	return ret;
}
