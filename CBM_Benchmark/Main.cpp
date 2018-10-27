#include "InputNode.h"
#include "ProcessingNode.h"
#include "Config.h"
#include "host2ip.h"

using namespace std;

void *buff;
struct ctx *ctx;
struct keys keys;

Config * config;
ProcessingNode * pnode;
InputNode * inode;

int server()
{
	pnode = new ProcessingNode(nullptr, FI_SOURCE, config, buff, keys);
	int ret = pnode->init();
	if (ret)
		return ret;

	ret = pnode->initServer();

	return ret;
}

void * cq_thread(void * arg)
{
	struct fi_cq_msg_entry comp;
	ssize_t ret;
	struct fi_cq_err_entry err;
	const char *err_str;
	struct fi_eq_entry eq_entry;
	uint32_t event;

	while (inode->run) {
		ret = fi_cq_sread(inode->cq, &comp, 1, nullptr, 1000);
		if (!inode->run)
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

	return nullptr;
};

void * client_thread(void *arg)
{
	struct ctx *ctx = (struct ctx*)arg;
	int i;
	ssize_t ret;
	for (i = 0; i < ctx->count; i++) {

		ret = fi_read(inode->ep, inode->buff, ctx->size, fi_mr_desc(inode->mr),
			0, inode->keys.addr, inode->keys.rkey, ctx);
		if (ret) {
			perror("fi_read");
			break;
		}

		pthread_mutex_lock(&ctx->lock);
		while (!ctx->ready)
			pthread_cond_wait(&ctx->cond, &ctx->lock);
		ctx->ready = 0;
		pthread_mutex_unlock(&ctx->lock);
	}
	return 0;
}

int client(char *addr, int threads, int size, int count) 
{
	inode = new InputNode(addr, 0, config, buff, keys);

	int ret = inode->init();
	if (ret)
		return ret;

	ret = inode->initClient(cq_thread);
	if (ret)
		return ret;

	int i;
	for (i = 0; i < threads; i++) {
		ret = pthread_create(&ctx[i].thread, nullptr,
			client_thread, &ctx[i]);
		if (ret) {
			perror("pthread_create");
			return ret;
		}
	}

	for (i = 0; i < threads; i++) {
		pthread_join(ctx[i].thread, nullptr);
	}

	inode->run = 0;
	pthread_join(inode->thread, nullptr);

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
		fprintf(stderr, "arguments given: %d\n", argc);
		fprintf(stderr, "usage: %s addr threads size count rkey addr\n", argv[0]);
		return -1;
	}

	char *addr = argv[1];
	addr = host2ip::resolve(addr);
	if (addr == nullptr){
	    return -1;
	}
	fprintf(stderr, "addr: %s\n", addr);
	int threads = atoi(argv[2]);
	fprintf(stderr, "threads: %d\n", threads);
	int size = atoi(argv[3]);
	fprintf(stderr, "size: %d\n", size);
	int count = atoi(argv[4]);
	fprintf(stderr, "count: %d\n", count);

	ctx = reinterpret_cast<struct ctx *>(calloc(threads, sizeof(*ctx)));
	if (!ctx) {
		perror("calloc");
		return -1;
	}

	int i;
	for (i = 0; i < threads; i++) {
		pthread_mutex_init(&ctx[i].lock, nullptr);
		pthread_cond_init(&ctx[i].cond, nullptr);
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
