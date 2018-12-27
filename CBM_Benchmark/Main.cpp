#include "Config.h"
#include "host2ip.h"
#include "Node.h"
#include "Clock.h"

using namespace std;



Config * config;
Node * pnode;
Node * inode;

int server()
{
	pnode = new Node(NULL, FI_SOURCE, config);
	int ret = pnode->init(BM_SERVER);
	if (ret)
		return ret;
    ret = pnode->listenServer();
	if (ret)
		return ret;


	return 0;
}

int client(char *addr) 
{
    /* number of iterations until all data is sent
    NOTE: total_data_size is the data target  per Endpoint so actual data sent 
    will be total_data_size * num_ep */

    int N = (int)(config->total_data_size / config->msg_size);
    struct transfer * transfer = (struct transfer *)calloc(N,sizeof(*transfer));
    double data_sum = 0;
    double time_sum = 0;

    //TODO pass params to config
	inode = new Node(addr, 0, config);
	int ret = inode->init(BM_CLIENT);
	if (ret) return ret;
    
    ret = inode->connectToServer();
    if (ret) return ret;
    ret = inode->setDataBuffer();
    if (ret) return ret;

    /* write will run multiple fi_write on different fi_ep for each processing node the ep is connected to.
    We don't want to lock up finished eps because of one ep who hasnt finished it's transfer
    so we will allow multiple write batches.

    TODO: however we could test the difference in bandwidth if we wait for every ep to finish 
    before starting the next batch. 
    The difference measured will give us an estimate of fairness in bandwidth distribution*/
    Clock * clock = new Clock();
    clock->start();
//#pragma omp parallel num_threads(2)
{
    //#pragma omp for
    for(int i = 0; i < N; i++){
        transfer[i] = inode->writeToServer();
        printf("transer[%d] data_byte: %fl, time_sec: %fl \n", i, transfer[i].data_byte, transfer[i].time_sec);
    }
}
    clock->stop();

    for(int i = 0; i < N; i++){
        data_sum += transfer[i].data_byte;
        time_sum += transfer[i].time_sec;
    }
    printf("summary: data_byte %fl, time_sec %fl, time_sum %fl", data_sum, clock->getDelta(), time_sum);
	return 0;
}


int main(int argc, char *argv[])
{
	config = new Config();
	struct fi_info * fi;
	int ret = fi_getinfo(FIVER, NULL, NULL, 0, config->hints, &fi); //initial network check
	if (ret) {
		printf("initial fi_getinfo: %s\n", fi_strerror(ret));
		ret = fi_getinfo(FIVER, NULL, NULL, 0, NULL, &fi);
		if (ret) {
			printf("initial fi_getinfo: %s\n", fi_strerror(ret));
			return ret;
		}
	}
	while (fi->next) {
		printf("provider: %s %s\n", fi->fabric_attr->prov_name, fi->fabric_attr->name);
		fi = fi->next;
	}
	switch (argc) {
	case 1: // runs server with default config
    {
        return server();
    }
	case 3: // runs server
    {
        int num_ep = atoi(argv[1]);
        if (num_ep > 0) {
            config->num_ep = num_ep;
        }
        printf("Endpoints: %d\n", config->num_ep);

        int msg_size = atoi(argv[2]);
        if (msg_size > 0) {
            config->msg_size = msg_size * 1024 * 1024;
            config->buff_size = config->threads*config->msg_size;
        }
        printf("msg_size: %d\n", config->msg_size);


        return server();
    }
	case 5: // runs client
    {
        int threads = atoi(argv[1]);
        if (threads > 0) {
            config->threads = threads;
        }
        printf("threads: %d\n", threads);

        int num_ep = atoi(argv[2]);
        if (num_ep > 0) {
            config->num_ep = num_ep;
        }
        printf("Endpoints: %d\n", config->num_ep);

        double total_data_size = atof(argv[3]);
        if (total_data_size > 0) {
            config->total_data_size = total_data_size * 1024 * 1024;
        }
        printf("total_data_size: %f\n", config->total_data_size);

        char *addr = argv[argc-1];
        addr = host2ip::resolve(addr);
        if (addr == NULL) {
            return -1;
        } else {
            config->addr = addr;
        }
        printf("addr: %s\n", config->addr);

        return client(addr);

    }
        case 6: // runs client
        {
            
            int num_ep = atoi(argv[1]);
            if (num_ep > 0) {
                config->num_ep = num_ep;
            }
            printf("Endpoints: %d\n", config->num_ep);

            double total_data_size = atoi(argv[2]);
            if (total_data_size > 0) {
                config->total_data_size = total_data_size * 1024 * 1024;
            }
            printf("total_data_size: %f\n", config->total_data_size);

            int msg_size = atoi(argv[3]);
            if (msg_size > 0) {
                config->msg_size = msg_size * 1024 * 1024;
                config->buff_size = config->msg_size; // each thread takes up one msg_size space
            }
            printf("msg_size: %d\n", config->msg_size);

            char *addr = argv[argc-2];
            addr = host2ip::resolve(addr);
            if (addr == NULL) {
                return -1;
            } else {
                config->addr = addr;
            }
            printf("addr: %s\n", config->addr);

            char *port = argv[argc-1];
            config->port = port;
            printf("slot: %s\n", port);

            return client(addr);

        }
	default:
    {
        fprintf(stderr, "wrong number of arguments given: %d\n", argc);
        fprintf(stderr, "usage:\n[server auto conf] %s\n[server manual conf] %s num_ep msg_size\n[client] %s num_ep total_data_Size(MB) msg_size(MB) serverip port\n", argv[0]);
        return -1;
    }
	}



	return 0;
}
