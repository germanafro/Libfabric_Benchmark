#include "InputNode.h"
#include "ProcessingNode.h"
#include "Config.h"
#include "host2ip.h"

using namespace std;

struct ctx *ctx;

Config * config;
ProcessingNode * pnode;
InputNode * inode;

int server()
{
	pnode = new ProcessingNode(NULL, FI_SOURCE, config);
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
		ret = fi_cq_sread(inode->cq, &comp, 1, NULL, 1000);
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

	return NULL;
};

void * client_thread(void *arg)
{
	struct ctx *ctx = (struct ctx*)arg;
	int i;
	ssize_t ret;
	size_t msg_size = sizeof(int);//ctx->size; //TODO determine datatype
	for (i = 0; i < ctx->count; i++) {
		ret = fi_read(inode->ep, inode->msg_buff + msg_size*ctx->id , msg_size, fi_mr_desc(inode->mr),
			0, inode->keys.addr + msg_size*ctx->id, inode->keys.rkey, ctx);
		if (ret) {
			perror("fi_read");
			break;
		}

		pthread_mutex_lock(&ctx->lock);
		while (!ctx->ready)
			pthread_cond_wait(&ctx->cond, &ctx->lock);
		ctx->ready = 0;

        int temp;
        memcpy(&temp, inode->msg_buff + msg_size*ctx->id, msg_size);
        printf("thread[%d] iter %d: fi_read: %d\n", ctx->id, i, temp++);
		pthread_mutex_unlock(&ctx->lock);
        printf("thread[%d] iter %d: fi_write: %d\n", ctx->id, i, temp);
        memcpy(inode->msg_buff + msg_size*ctx->id, &temp, msg_size);
        ret = fi_write(inode->ep, inode->msg_buff + msg_size*ctx->id , msg_size, fi_mr_desc(inode->mr),
                      0, inode->keys.addr + msg_size*ctx->id, inode->keys.rkey, ctx);
        if (ret) {
            perror("fi_read");
            break;
        }

	}
	return 0;
}

int client(char *addr, int threads, int size, int count) 
{
	inode = new InputNode(addr, 0, config);

	int ret = inode->init();
	if (ret)
		return ret;

	ret = inode->initClient(cq_thread);
	if (ret)
		return ret;

	int i;
	for (i = 0; i < threads; i++) {
	    ctx[i].id = i;
		ret = pthread_create(&ctx[i].thread, NULL,
			client_thread, &ctx[i]);
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

	if (argc == 1) {
		return server();
	}

	if (argc != 5) {
		fprintf(stderr, "arguments given: %d\n", argc);
		fprintf(stderr, "usage: %s addr threads size count rkey addr\n", argv[0]);
		return -1;
	}

	char *addr = argv[1];
	addr = host2ip::resolve(addr);
	if (addr == NULL){
	    return -1;
	}
	printf("addr: %s\n", addr);
	int threads = atoi(argv[2]);
	printf("threads: %d\n", threads);
	int size = atoi(argv[3]);
	printf("size: %d\n", size);
	int count = atoi(argv[4]);
	printf("count: %d\n", count);

	ctx = (struct ctx *)calloc(threads, sizeof(*ctx));
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
