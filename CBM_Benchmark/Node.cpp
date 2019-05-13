#include "Node.h"
#include "Clock.h"
#include <omp.h>
/*
 *
 * TODO Docstring
 */
Node::Node(int id, uint64_t flags, Config * config)
{
	this->id = id;
	this->flags = flags;
	this->config = config;
}

// yeah this will cause big problems if we didnt init() yet... so always run init() ^-^
Node::~Node()
{

    fi_close(&cq_tx->fid);
    fi_close(&cq_rx->fid);
    fi_close(&eq->fid);
    fi_close(&domain->fid);
    fi_close(&fabric->fid);
}

int Node::init(int mode)
{
    this->mode = mode;
    int num_en = config->num_en;
    int num_pn = config->num_pn;
    int slot = config->slot;
    checkpoint = (struct transfer *) malloc(num_en * num_pn * sizeof(struct transfer));
    summary = (struct transfer *) malloc(num_en * num_pn * sizeof(struct transfer));

    if ((mode < 0) || (mode >= BM_MAX)){
        printf("WARNING: invalid mode: %d\n", mode);
    }


    int ret;
    struct fi_eq_attr eq_attr = config->eq_attr;
    struct fi_cq_attr cq_attr = config->cq_attr;

    ret= fi_getinfo(FIVER, NULL, NULL, 0, config->hints, &fi);
    if (ret) {
        perror("Node.fi_getinfo");
        return ret;
    }
#ifdef DEBUG
	printf("fi_getinfo: %s %s\n", fi->fabric_attr->prov_name, fi->fabric_attr->name);
#endif //DEBUG
    ret = fi_fabric(fi->fabric_attr, &fabric, NULL);
    if (ret) {
        printf("fi_fabric: %s\n", fi_strerror(ret));
        return ret;
    }
 
    ret = fi_eq_open(fabric, &eq_attr, &eq, NULL);
    if (ret) {
        printf("fi_eq_open: %s\n", fi_strerror(ret));
        return ret;
    }

    ret = fi_domain(fabric, fi, &domain, NULL);
    if (ret) {
        printf("fi_domain: %s\n", fi_strerror(ret));
        return ret;
    }

    ret = fi_cq_open(domain, &cq_attr, &cq_tx, NULL);
    if (ret) {
        printf("fi_cq_open: %s\n", fi_strerror(ret));
        return ret;
    }

    ret = fi_cq_open(domain, &cq_attr, &cq_rx, NULL);
    if (ret) {
        printf("fi_cq_open: %s\n", fi_strerror(ret));
        return ret;
    }



    switch (mode) {
        case BM_SERVER:
        {
            int int_port = atoi(config->start_port);
            config->num_ep = num_en;
            for(int i = 0; i<num_en; i++){
                char *port = new char(10);
#pragma warning(suppress : 4996)
                sprintf(port,"%d",int_port+i); // FIXME: find better method
#ifdef DEBUG
                printf("[%d] pushing Endpoint | addr: %s, port: %s\n", i, addr, port);
#endif //DEBUG
                eps.push_back(new Endpoint(this, addr, port, flags, config, i));
            }
            // EP to controller
            int controller_port = atoi(config->controller_port)+config->server_offset;
            char* port = new char(10);
#pragma warning(suppress : 4996)
            sprintf(port,"%d", id+controller_port); // FIXME: find better method
            cep = new Endpoint(this, config->controller_addr, port,  0, config, id);
            cep->init();
            cep->client();
            break;
        }
        case BM_CLIENT:
        {
            config->num_ep = num_pn;
            if (num_pn > config->addr_v.size()){
                printf("Error: serverlist only provides %lu of %d required addresses\n", config->addr_v.size(), config->num_ep );
                return -1;
            }
            for(int i = 0; i<num_pn; i++){
#ifdef DEBUG
                printf("[%d] pushing Endpoint | addr: %s, port: %s\n", i, config->addr_v[i].c_str(), config->port);
#endif //DEBUG
                eps.push_back(new Endpoint(this, config->addr_v[i].c_str(), config->port, flags, config, i));
            }
            // EP to controller
            int controller_port = atoi(config->controller_port);
            char* port = new char(10);
#pragma warning(suppress : 4996)
            sprintf(port,"%d", id+controller_port); // FIXME: find better method
            cep = new Endpoint(this, config->controller_addr, port,  0, config, id);
            cep->init();
            cep->client();
            break;
        }
        case BM_CONTROLLER:
        {
            // first push clients
            int int_port = atoi(config->controller_port);
            for(int i = 0; i<num_en; i++){
                char *port = new char(10);
                #pragma warning(suppress : 4996)
                sprintf(port,"%d",int_port+i);
#ifdef DEBUG
                printf("[%d] pushing Endpoint | addr: %s, port: %s\n", i, addr, port);
#endif //DEBUG
                eps.push_back(new Endpoint(this, addr, port, flags, config, i));
            }
            // second push servers
            for(int i = 0; i<num_pn; i++){
                char *port = new char(10);
                #pragma warning(suppress : 4996)
                sprintf(port,"%d",int_port+i+config->server_offset);
#ifdef DEBUG
                printf("[%d] pushing Endpoint | addr: %s, port: %s\n", i, addr, port);
#endif //DEBUG
                eps.push_back(new Endpoint(this, addr, port, flags, config, i+1000));
            }
            break;
        }
        default:
            printf("unknown mode: %d\n", mode);
            break;
    }

    for (int n=0; n<config->num_ep; n++) {
        int ret;
        ret = eps[n]->init();
        if(ret){
            printf("[%d] ep.init(): %s\n", n, fi_strerror(ret));
        }

        if (!ret) {
            switch (mode) {
                case BM_SERVER:
                    eps[n]->server();
                    break;
                case BM_CLIENT:
                    eps[n]->client();
                    break;
                case BM_CONTROLLER:
                    eps[n]->server();
                    break;
                default:
                    printf("[%d] unknown mode: %d\n", n, mode);
                    break;
            }
        }
    }
    return 0;
}

/*
* Client Operation
* connect to controller and await "connect" command
**/
int Node::connectToController()
{
    int ret;
    int CONFIRM = BM_CONNECT;
    int COMMAND = 0;

    ret = cep->connect();
    if(ret){
        return ret;
    }

    ret = cep->ctrl_recv(sizeof(int));

    //struct ctx * ctx = (struct ctx *)malloc(sizeof(struct ctx)); // in case of segmentation fault
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    ret= waitComp(cq_rx, msg_entry);
    //printf("DEBUG recv_comp %d\n", msg_entry->len);
    if(msg_entry->len == sizeof(int)){
        memcpy(&COMMAND, cep->ctrl_buff, sizeof(int));
        if(COMMAND == CONFIRM){
            free(msg_entry);
            return 0;
        }else {
            printf("ERROR: unexpected COMMAND: %d\n", COMMAND);
            free(msg_entry);
            return -1;
        }
    } else{
        printf("WARNING: msg_size %d does not match expected size %d\n", msg_entry->len, sizeof(int));
    }
    free(msg_entry);
    if (ret) return ret;

    return 0;
}

/*
* Client Operation
* synchronously connect to peers
*/
int Node::connectToServers()
{
    if (mode != BM_CLIENT){
        return -1;
    }
    int ret;
    int num_ep = config->num_ep;
    int COMMAND = BM_CONNECTED;
    int CONFIRM = BM_START;

    for (int n=0; n<num_ep; n++) {
        ret = eps[n]->connect();
        if(ret){
            printf("[%d] node.connectToServer(): %s\n", n, fi_strerror(ret));
            return ret;
        }
    }
    // confirm to controller
    ret = cep->ctrl_recv(sizeof(int));
    if (ret){
        return ret;
    }
    ret = cep->sendCommand(&COMMAND);
    if (ret){
        return ret;
    }
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    ret= waitComp(cq_tx, msg_entry);
    //printf("DEBUG tx_comp %d\n", msg_entry->len);
    ret= waitComp(cq_rx, msg_entry);
    //printf("DEBUG rx_comp %d\n", msg_entry->len);
    if(msg_entry->len == sizeof(int)){
        memcpy(&COMMAND, cep->ctrl_buff, sizeof(int));
        if(COMMAND == CONFIRM){
            free(msg_entry);
            return 0;
        }else {
            printf("ERROR: unexpected COMMAND: %d\n", COMMAND);
            free(msg_entry);
            return -1;
        }
    } else{
        printf("WARNING: msg_size %d does not match expected size %d\n", msg_entry->len, sizeof(int));
    }
    free(msg_entry);
    return ret;
}

/*
* Client Operation
* tell each endpoint to fill their data buffers
*/
int Node::setDataBuffers()
{
    if (mode != BM_CLIENT){ // this is ment to fill client buffer not server buffer
        return -1;
    }
    int num_ep = config->num_ep;
    int ret;

    for (int n=0; n<num_ep; n++) {
        ret = eps[n]->setDataBuffer();
        if(ret){
            printf("[%d] ep.setDataBuffer(): %s\n", n, fi_strerror(ret));
            return ret;
        }
    }

    return ret;
}

/*
* Client Operation
* data transfer loop
* loop: (poll the cq_rx/cq_tx  and decide -> send new data / compute a transfer / stop)
**/
int Node::benchmark()
{
    int ret;
    bool stop = false;
    int STOP = BM_STOP;
    int CHECKPOINT = BM_CHECKPOINT;
    int COMMAND = 0;
     std::cout << "mark1" << std::endl;
    struct ctx * ctx =(struct ctx*)malloc(sizeof(struct ctx));
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    std::cout << "mark1.1" << std::endl;
    //init arrays of custom transfer structure: one per connected processing node
    struct transfer * temp = (struct transfer *)malloc(sizeof(struct transfer)*config->num_pn);
    std::cout << "mark1.2" << std::endl;
    struct transfer * transfer = (struct transfer *)malloc(sizeof(struct transfer)*config->num_pn);
    std::cout << "mark1.3" << std::endl;
    struct transfer * delta = (struct transfer *)malloc(sizeof(struct transfer)*config->num_pn);
    std::cout << "mark2" << std::endl;

    for(int i=0; i<config->num_pn; i++){
        temp[i].from = cep->id;
        temp[i].to = i;
        temp[i].completions = 0;
        temp[i].time_sec = 0.0;
        temp[i].delta_sec = 0.0;

        transfer[i].from = cep->id;
        transfer[i].to = i;
        transfer[i].completions = 0;
        transfer[i].time_sec = 0.0;
        temp[i].delta_sec = 0.0;

        delta[i].from = cep->id;
        delta[i].to = i;
        delta[i].completions = 0;
        delta[i].time_sec = 0.0;
        temp[i].delta_sec = 0.0;
    }
     std::cout << "mark3" << std::endl;

    ret = cep->ctrl_recv(0);
    if (ret){
        printf("benchmark.rdma_recv() error posting another recv\n");
    }
     std::cout << "mark4" << std::endl;
    Clock * timer = new Clock();
    timer->start();
     std::cout << "mark5" << std::endl;
    int queuesize = 10;
    for(int i = 0; i<queuesize; i++){
        for (int j= 0; j< config->num_pn; j++){
            eps[j]->rdma_write();
            if (ret){
                printf("[%d]benchmark.rdma_write() error posting another write\n", j);
                //free(msg_entry);
                //free(ctx);
                //return ret;
            }
        }
    }
     std::cout << "mark6" << std::endl;
    int keepbusy = 0;
    while (!stop){
        ret = checkComp(cq_rx, msg_entry);
        if(ret>0){
            //printf("DEBUG rx_comp %d\n", msg_entry->len);
            if(msg_entry->len == sizeof(int)){
                memcpy(&COMMAND, cep->ctrl_buff, sizeof(int));
            }else{
                printf("ERROR: size of msg expected %d but received %d", sizeof(int), msg_entry->len);
                //free(temp);
                //free(transfer);
                //free(delta);
                //free(msg_entry);
                //free(ctx);
                return -1;
            }
            timer->stop();
            double time_ = timer->getDelta();
            for(int i=0; i<config->num_pn; i++){
                transfer[i].time_sec = time_;
            }
            switch(COMMAND){
                case BM_STOP:{
                    int sum = 0;
                    for(int i=0; i<config->num_pn; i++){
                        delta[i].time_sec = transfer[i].time_sec;
                        delta[i].delta_sec = transfer[i].time_sec;
                        delta[i].completions = transfer[i].completions;
                        sum += transfer[i].completions;
                    }
                    stop = true;
                    printf("queued %d, completed %d\n", queuesize, sum);
                    break;
                }
                case BM_CHECKPOINT:{
                    timer->stop();
                    for(int i=0; i<config->num_pn; i++){
                        delta[i].delta_sec = transfer[i].time_sec - temp[i].time_sec;
                        delta[i].time_sec = transfer[i].time_sec;
                        delta[i].completions = transfer[i].completions - temp[i].completions;
                        temp[i].time_sec = transfer[i].time_sec;
                        temp[i].completions = transfer[i].completions;
                    }
                    ret = cep->ctrl_recv(sizeof(int));
                    if (ret){
                        printf("benchmark.rdma_recv() error posting another recv\n");
                    }
                    break;
                }
                default:{
                    printf("WARNING: cannot recognize command: %d\n", COMMAND);
                    break;
                }
            }
            memcpy(cep->ctrl_buff, &delta[0], sizeof(struct transfer)*config->num_pn);
            for(int i=0; i<config->num_pn; i++){
                std::cout << delta[i].from << " " << delta[i].to << " " << delta[i].completions << " " << delta[i].time_sec << std::endl;
            }
            ret = cep->ctrl_send(sizeof(struct transfer)*config->num_pn);
            if (ret){
                //free(temp);
                //free(transfer);
                //free(delta);
                //free(msg_entry);
                //free(ctx);
                return ret;
            }
        }
        ret = 0;
        ret = checkComp(cq_tx, msg_entry);

        if(ret>0){
            
            ctx = (struct ctx *) msg_entry->op_context;
            //if(msg_entry->len == ctx->size){
                transfer[ctx->id].completions++;
                queuesize++;
                ret = eps[ctx->id]->rdma_write();
                if (ret){
                    printf("[%d]benchmark.rdma_write() error posting another write\n", id);
                    //free(msg_entry);
                    //free(ctx);
                    //return ret;
                }
           // }
            
        }
        keepbusy++;

    }
     std::cout << "mark8" << std::endl;
    free(temp);
    free(transfer);
    free(delta);
    free(msg_entry);
    free(ctx);
    return 0;
}

/*
    * post listen for clients
    * connect to the controller
    * wait for connreq
    * accept connreq and send out keys
    * recv "stop" signal
    * stay alive
    * wait for "stop"
    */
int Node::listenServer()
{
    if (mode != BM_SERVER){
        return -1;
    }
    int ret;
    int num_en = config->num_en;

    for(int n=0; n<num_en; n++) {
        ret = eps[n]->listen();
        if (ret){
            printf("[%d] ep.listen(): %s\n", n, fi_strerror(ret));
            return ret;
        }
    }

    /*
    * tell the controller you are listening
    * 
    */
    ret = cep->connect();
    if(ret){
        return ret;
    }

    //wait till all clients are connected
    ret = waitAllConn(num_en);
    if (ret) return ret;
    //printf("DEBUG ALL CONNECTED\n");

    //recv stop signal
    ret = cep->ctrl_recv(sizeof(int));
    if (ret) return ret;

    //stay alive till stop
    struct ctx * ctx = (struct ctx *)malloc(sizeof(struct ctx));
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    struct fi_eq_cm_entry * entry = (struct fi_eq_cm_entry *)malloc(sizeof(struct fi_eq_cm_entry));
    uint32_t event;
    bool run = true;
    int COMMAND = 0;
    int keepbusy = 0;
    while(run){
        ret = checkComp(cq_rx, msg_entry);
        if (ret > 0){
            //printf("DEBUG rx_comp %d\n", msg_entry->len);
            if(msg_entry->len == sizeof(int)){
                memcpy(&COMMAND, cep->ctrl_buff, sizeof(int));
            }else{
                printf("ERROR: size of msg expected %d but received %d", sizeof(int), msg_entry->len);
                free(msg_entry);
                free(entry);
                free(ctx);
                return -1;
            }
            switch(COMMAND){
                case BM_STOP:{
                    run = false;
                    printf("stopping\n");
                    int N = config->msg_size/sizeof(int);
                    for(int i=0; i<config->num_en; i++){
                        printf("%d\n",eps[i]->msg_buff[0]);
                        printf("%d\n",eps[i]->msg_buff[N/2]);
                        printf("%d\n",eps[i]->msg_buff[N-1]);
                    }
                    break;
                }
                default:{
                    printf("warning: unrecognizeable command: %s\n", COMMAND);
                    break;
                }
            }
        }

        ret = checkCmEvent(&event, entry);
        if(ret>0){
            if(event == FI_CONNREQ)
            {
                //accept connreq
                struct fid_pep * pep = (struct fid_pep *) entry->fid;
                for(int n=0; n<num_en; n++) {
                    if (eps[n]->pep == pep){
                        printf("REMOVEME Node.cpp: accepting\n");
                        ret = eps[n]->accept(entry);
                        break;
                    }
                }
            }
            if(event == FI_CONNECTED) {
                struct fid_ep * ep = (struct fid_ep *) entry->fid;
                for(int n=0; n<num_en; n++) {
                    if (eps[n]->ep == ep){
                        printf("REMOVEME Node.cpp: sendKeys\n");
                        ret = eps[n]->sendKeys();
                        break;
                    }
                }
            }
            if(event == FI_SHUTDOWN) {
                struct fid_ep * ep = (struct fid_ep *) entry->fid;
                for(int n=0; n<num_en; n++) {
                    if (eps[n]->ep == ep){
                        printf("REMOVEME Node.cpp: disconnected\n");
                        //ret = eps[n]->listen(); 
                        break;
                    }
                }
            }
            if(ret<0){
                free(msg_entry);
                return ret;
            }
        }
        keepbusy++;
    }
    free(msg_entry);
    free(entry);
    free(ctx);
    return 0; // no issues
}

/*
* await and accept conn_req from hosts
* wait till all hosts are connected
* terminates early with -1 if a host loses its connection
*/
int Node::listenController()
{
    if (mode != BM_CONTROLLER){
        return -1;
    }
    int num_ep = config->num_pn + config->num_en;
    int ret;

    for(int n=0; n<num_ep; n++) {
        ret = eps[n]->listen();
        if (ret){
            printf("[%d] ep.listen(): %s\n", n, fi_strerror(ret));
            return ret;
        }
    }

    ret = waitAllConn(num_ep);
    if(ret){
        return ret;
    }
    //printf("DEBUG: ALL CONNECTED\n");

    return 0; // everyone is connected
}

/*
* send "start" command to all connected hosts
* wait till all messages are successfully sent
*/
int Node::controllerStart()
{
    if (mode != BM_CONTROLLER){
        return -1;
    }
    int COMMAND = BM_START;
    int num_en = config->num_en;
    struct ctx ctx;
    int ret;

    for(int n=0; n<num_en; n++) {
        ret = eps[n]->sendCommand(&COMMAND);
        if (ret){
            printf("[%d] ep.sendCommand('%d'): %s\n", n, COMMAND, fi_strerror(ret));
            return ret;
        }
    }

    int counter = 0;
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    while(counter < num_en) 
    {
        ret= waitComp(cq_tx, msg_entry);
        //printf("DEBUG tx_comp %d\n", msg_entry->len);
        if(ret){
            free(msg_entry);
            return ret;
        }
        counter++;
    }
    free(msg_entry);    
    return 0; // all start commands sent.
}

/*
* post recv for "ready"
* send "connect" command to all clients
* 
*/
int Node::controllerConnect()
{
    if (mode != BM_CONTROLLER){
        return -1;
    }
    int COMMAND =  BM_CONNECT;
    int num_en = config->num_en;
    struct ctx * ctx = (struct ctx *)malloc(sizeof(struct ctx));
    if (!ctx) {
        perror("malloc");
    }
    int ret;

    // post recv for each entry node
    for(int n=0; n<num_en; n++) {
        ret = eps[n]->ctrl_recv(sizeof(int));
        if (ret){
            printf("[%d] ep.recv(): %s\n", n, fi_strerror(ret));
            return ret;
        }
    }
    printf("DEBUG: sending 'connect'...\n");
    for(int n=0; n<num_en; n++) {
        ret = eps[n]->sendCommand(&COMMAND);
        if (ret){
            printf("[%d] ep.sendCommand('%d'): %s\n", n, COMMAND, fi_strerror(ret));
            return ret;
        }
    }

    int counter = 0;
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    while(counter < num_en) // wait till all clients have received 'connect'
    {
        ret= waitComp(cq_tx, msg_entry);
        //printf("DEBUG tx_comp %d\n", msg_entry->len);
        if(ret){
            free(msg_entry);
            free(ctx);
            return ret;
        }
        counter++;
    }
    printf("DEBUG: awaiting confirmation...\n");
    counter = 0;
    while(counter < num_en) // wait till we received all 'connected' confirmations
    {
        ret= waitComp(cq_rx, msg_entry);
        ctx =(struct ctx *) msg_entry->op_context;
        //printf("[%d]DEBUG recv_comp %d\n", ctx->id, msg_entry->len);
        if(msg_entry->len == sizeof(int)){
            memcpy(&COMMAND, eps[ctx->id]->ctrl_buff, sizeof(int));
        }else{
            printf("ERROR: size of msg expected %d but received %d", sizeof(int), msg_entry->len);
            free(msg_entry);
            free(ctx);
            return -1;
        }
        switch(COMMAND){
            case BM_CONNECTED:{
                counter++;
                break;
            }
            default:{
                printf("warning: unrecognizeable command: %s\n", COMMAND);
                break;
            }
        }
        
        if(ret){
            free(msg_entry);
            free(ctx);
            return ret;
        }
    }
    free(msg_entry);
    free(ctx);
    printf("DEBUG: all ready\n");
    return 0; // all connect commands sent and confirmations received.
}

/*
* post recv for "transfer" struct
* send "checkpoint" command to all clients
* await and store the expected transfer struct
*/
int Node::controllerCheckpoint()
{
    printf("--------------\nsendCheckpoint\n--------------\n");
    if (mode != BM_CONTROLLER){
        return -1;
    }
    int num_en = config->num_en;
    int COMMAND = BM_CHECKPOINT;
    struct ctx * ctx = (struct ctx *)malloc(sizeof(struct ctx));
    int ret;

    // post recv for each entry node
    for(int n=0; n<num_en; n++) {
        ret = eps[n]->ctrl_recv(sizeof(struct transfer)*config->num_pn);
        if (ret){
            printf("[%d] ep.recv(): %s\n", n, fi_strerror(ret));
            free(ctx);
            return ret;
        }
    }

    // send checkpoint signals to all entry nodes
    for(int n=0; n<num_en; n++) {
        ret = eps[n]->sendCommand(&COMMAND);
        if (ret){
            printf("[%d] ep.sendCommand('%d'): %s\n", n, COMMAND, fi_strerror(ret));
            free(ctx);
            return ret;
        }
    }

    int counter = 0;
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    while(counter < num_en) // wait till hosts have received checkpoint
    {
        ret = waitComp(cq_tx, msg_entry);
        //printf("DEBUG tx_comp %d\n", msg_entry->len);
        if(ret){
            free(msg_entry);
            free(ctx);
            return ret;
        }
        counter++;
    }

    counter = 0;
    while(counter < num_en) // wait till we received transfer data from each entry node
    {
        ret = waitComp(cq_rx, msg_entry);
        ctx = (struct ctx*) msg_entry->op_context;
        //printf("[%d]DEBUG recv_comp %d, %d\n", ctx->id, msg_entry->len, ctx->size);
        if(msg_entry->len == sizeof(struct transfer)*config->num_pn){
            memcpy(&checkpoint[ctx->id*config->num_pn], eps[ctx->id]->ctrl_buff, sizeof(struct transfer)*config->num_pn);
        }else{
            printf("Warning: size of msg %d does not match size of recv %d", msg_entry->len, sizeof(struct transfer));
        }
        if(ret){
            free(msg_entry);
            free(ctx);
            return ret;
        }
        counter++;
    }
    free(msg_entry);
    free(ctx);
    return 0; // all checkpoint commands sent and confirmations received.
}

/*
* post receive for timers and data from clients 
* send stop signal to all hosts
* wait till all messages are successfully sent and received
*/
int Node::controllerStop()
{
    if (mode != BM_CONTROLLER){
        return -1;
    }
    int COMMAND= BM_STOP;
    int num_ep = config->num_en + config->num_pn;
    int num_en = config->num_en; // we need the entry nodes 
    struct ctx * ctx = (struct ctx *)malloc(sizeof(struct ctx));
    int ret;

    // post recv for each entry node
    for(int n=0; n<num_en; n++) {
        ret = eps[n]->ctrl_recv(sizeof(struct transfer)*config->num_pn);
        if (ret){
            printf("[%d] ep.recv(): %s\n", n, fi_strerror(ret));
            free(ctx);
            return ret;
        }
    }

    // send stop signals to all hosts
    for(int n=0; n<num_ep; n++) {
        ret = eps[n]->sendCommand(&COMMAND);
        if (ret){
            printf("[%d] ep.sendCommand('%d'): %s\n", n, COMMAND, fi_strerror(ret));
            free(ctx);
            return ret;
        }
    }

    int counter = 0;
    struct fi_cq_msg_entry * msg_entry = (struct fi_cq_msg_entry *)malloc(sizeof(struct fi_cq_msg_entry));
    while(counter < num_ep) // wait till hosts have received stop
    {
        ret = waitComp(cq_tx, msg_entry);
        //printf("DEBUG tx_comp %d\n", msg_entry->len);
        if(ret){
            free(msg_entry);
            free(ctx);
            return ret;
        }
        counter++;
    }

    counter = 0;
    while(counter < num_en) // wait till we received transfer data from each entry node
    {
        ret = waitComp(cq_rx, msg_entry);
        ctx = (struct ctx*) msg_entry->op_context;
        //printf("[%d]DEBUG recv_comp %d, %d\n", ctx->id, msg_entry->len, ctx->size);
        if(msg_entry->len == sizeof(struct transfer)*config->num_pn){
            memcpy(&summary[ctx->id*config->num_pn], eps[ctx->id]->ctrl_buff, sizeof(struct transfer)*config->num_pn);
        }else{
            printf("Warning: size of msg %d does not match size of recv %d", msg_entry->len, sizeof(struct transfer)*config->num_pn);
        }
        if(ret){
            free(msg_entry);
            free(ctx);
            return ret;
        }
        counter++;
    }

    counter = 0;
    while(counter < num_ep) //wait till all hosts have disconnected properly
    {
        uint32_t event;
        struct fi_eq_cm_entry entry;

        ret = waitCmEvent(&event, &entry);
        if(ret){
            free(msg_entry);
            free(ctx);
            return ret;
        }
        if(event == FI_SHUTDOWN) {
            counter++;
        }
    }
    free(msg_entry);
    free(ctx);
    return 0; // all stop commands are sent the cnode has received all Bandwidth data and all hosts have disconnected.
}

/*
* listen for a completion and return the custom context of the respective completion
*/
int Node::checkComp(struct fid_cq * cq, struct fi_cq_msg_entry * entry)
{
	int ret;
    ret = fi_cq_read(cq, entry, 1);
    if (ret > 0) {
        return 1;        
    }
    if (ret != -FI_EAGAIN) {
        struct fi_cq_err_entry err_entry;
        fi_cq_readerr(cq, &err_entry, 0);
        printf("fi_cq_sread: %s %s \n", fi_strerror(err_entry.err), fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
        return ret;
    }
    return 0;
}

/*
* listen for a connection manager event and store the event and entry inside their respective pointer
*/
int Node::checkCmEvent(uint32_t * event, struct fi_eq_cm_entry * entry)
{
    ssize_t rret;
    rret = fi_eq_read(eq, event, entry, sizeof(struct fi_eq_cm_entry), 0);
    if (rret > 0) {
        if (*event == FI_CONNREQ){
            printf("connection request: %s\n", entry->info->fabric_attr->name);
        }
        else if (*event == FI_CONNECTED){
            printf("connected\n");
        }
        else if (*event == FI_SHUTDOWN){
            printf("disconnected\n");
        }
        else{
            printf("invalid event %u\n", *event);
            return -1;
        }

        return 1;
    }
    if (rret != -FI_EAGAIN) {
        struct fi_eq_err_entry err_entry;
        fi_eq_readerr(eq, &err_entry, 0);
        printf("%s %s \n", fi_strerror(err_entry.err),
                fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
        return (int) rret;
    }
    return 0;
}

/*
* wait for a completion and return the custom context of the respective completion
*/
int Node::waitComp(struct fid_cq * cq, struct fi_cq_msg_entry * entry)
{
    struct fi_cq_err_entry * err_entry = (struct fi_cq_err_entry *) malloc(sizeof(struct fi_cq_err_entry));
	int ret;
	while (1) {
		ret = fi_cq_read(cq, entry, 1);
		if (ret > 0) {
            ret = 0;
            break;
        }
		if (ret != -FI_EAGAIN) {
			fi_cq_readerr(cq, err_entry, 0);
			printf("fi_cq_sread: %s %s \n", fi_strerror(err_entry->err), fi_cq_strerror(cq, err_entry->prov_errno, err_entry->err_data, NULL, 0));
			break;
		}
	}
    free(err_entry);
    return ret;
}

/*
* listen for a connection manager event and store the event and entry inside their respective pointer
*/
int Node::waitCmEvent(uint32_t * event, struct fi_eq_cm_entry * entry)
{
    ssize_t rret;
	while (1) {
        rret = fi_eq_read(eq, event, entry, sizeof(struct fi_eq_cm_entry), 0);
        if (rret > 0) {
            if (*event == FI_CONNREQ){
                printf("connection request: %s\n", entry->info->fabric_attr->name);
            }
            else if (*event == FI_CONNECTED){
                printf("connected\n");
            }
            else if (*event == FI_SHUTDOWN){
                printf("disconnected\n");
            }
            else{
                printf("invalid event %u\n", *event);
                return -1;
            }

            return 0;

        }
        if (rret != -FI_EAGAIN) {
            struct fi_eq_err_entry err_entry;
            fi_eq_readerr(eq, &err_entry, 0);
            printf("%s %s \n", fi_strerror(err_entry.err),
                   fi_eq_strerror(eq, err_entry.prov_errno, err_entry.err_data, NULL, 0));
            return (int) rret;
        }
	}
}

/*
* listen for a connection manager event and store the event and entry inside their respective pointer
*/
int Node::waitAllConn(int goal){
    int ret;
    int num_ep = config->num_ep;
    int counter = 0;
    while(counter < goal){
        uint32_t event;
        struct fi_eq_cm_entry entry;

        ret = waitCmEvent(&event, &entry);
        if(ret){
            return ret;
        }

        if(event == FI_CONNREQ)
        {
            //accept connreq
            struct fid_pep * pep = (struct fid_pep *) entry.fid;
            for(int n=0; n<num_ep; n++) {
                if (eps[n]->pep == pep){
                    printf("REMOVEME Node.cpp: accepting\n");
                    ret = eps[n]->accept(&entry);
                    break;
                }
            }
        }
        if(event == FI_CONNECTED) {
            struct fid_ep * ep = (struct fid_ep *) entry.fid;
            for(int n=0; n<num_ep; n++) {
                if (eps[n]->ep == ep){
                    printf("REMOVEME Node.cpp: sendKeys\n");
                    ret = eps[n]->sendKeys();
                    break;
                }
            }
            counter++;
        }
        if(event == FI_SHUTDOWN) {
            struct fid_ep * ep = (struct fid_ep *) entry.fid;
            for(int n=0; n<num_ep; n++) {
                if (eps[n]->ep == ep){
                    printf("REMOVEME Node.cpp: disconnected\n");
                    //ret = eps[n]->listen(); 

                    break;
                }
            }
            counter--;
        }
        if(ret){
            return ret;
        }
    }
    return 0;
}