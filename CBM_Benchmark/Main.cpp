#include "Domain.h"
#include "InputNode.h"
#include "ProcessingNode.h"
#include "Config.h"

using namespace std;

void *buff;
struct ctx *ctx;
struct keys keys;

Config * config;
ProcessingNode * pnode;
InputNode * inode;

int server(void)
{
	pnode = new ProcessingNode(NULL, FI_SOURCE, config, buff, keys);
	int ret = pnode->init();
	if (ret)
		return ret;

	ret = pnode->initServer();

	return ret;
}

int client(char *addr, int threads, int size, int count) 
{
	inode = new InputNode(addr, 0, config, buff, keys);

	int ret = inode->init();
	if (ret)
		return ret;

	ret = inode->initClient();
	if (ret)
		return ret;

	int i;
	for (i = 0; i < threads; i++) {
		ret = pthread_create(&ctx[i].thread, NULL,
			inode->client_thread, &ctx[i]);
		if (ret) {
			perror("pthread_create");
			return ret;
		}
	}

	for (i = 0; i < threads; i++) {
		pthread_join(ctx[i].thread, NULL);
	}

	inode->run = 0;
	pthread_join(inode->thread, NULL);

	//maybe? maybe not? currently this is done in destructor of node.
	/*fi_shutdown(inode->ep, 0);
	fi_close(&inode->ep->fid);
	fi_close(&inode->mr->fid);
	fi_close(&inode->cq->fid);
	fi_close(&inode->eq->fid);
	fi_close(&inode->domain->fid);
	fi_close(&inode->fabric->fid);
	fi_freeinfo(inode->fi);*/
}

int
main(int argc, char *argv[])
{
	config = new Config();

	buff = malloc(config->buff_size);
	if (!buff) {
		perror("malloc");
		return -1;
	}

	if (argc == 1) {
		keys.addr = (uint64_t)buff;
		return server();
	}

	if (argc != 5) {
		fprintf(stderr, "usage: %s addr threads size count rkey addr\n", argv[0]);
		return -1;
	}

	char *addr = argv[1];
	int threads = atoi(argv[2]);
	int size = atoi(argv[3]);
	int count = atoi(argv[4]);

	ctx = reinterpret_cast<struct ctx *>(calloc(threads, sizeof(*ctx)));
	if (!ctx) {
		perror("calloc");
		return -1;
	}

	int i;
	for (i = 0; i < threads; i++) {
		pthread_mutex_init(&ctx[i].lock, NULL);
		pthread_cond_init(&ctx[i].cond, NULL);
		ctx[i].count = count;
		ctx[i].size = size;
	}

	return client(addr, threads, size, count);
}

/*void main() {
const char * address = "127.0.0.1";
const char * port = "6666";
Domain* domain = new Domain();
EntryNode* EN = new EntryNode(domain, 10 , 1);
//Worker* worker = new Worker(domain);
}*/
