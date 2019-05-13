#include "Config.h"
#include "host2ip.h"
#include "Node.h"
#include "Clock.h"
#include <chrono>
#include <ctime>

#include <numa.h>
#include <iostream>
#include <fstream>
using namespace std;


Clock * timer = new Clock(); //deprecated

Config * config;
Node * cnode;
Node * pnode;
Node * inode;

int server(int id)
{
	pnode = new Node(id, FI_SOURCE, config);
	int ret = pnode->init(BM_SERVER);
	if (ret)
		return ret;
    
    /*
    * post listen for clients
    * connect to the controller
    * wait for connreq
    * accept connreq and send out keys
    * recv "stop" signal
    * stay alive
    * wait for "stop"
    */
    ret = pnode->listenServer();
	if (ret)
		return ret;


	return 0;
}

int controller(int idn)
{
    // output data
    size_t msg_size = config->msg_size;
    int n = config->num_en;
    int m = config->num_pn;

    // create date timestamp
    /* note: for some reason Clock.h implementation gave signal 11 50% of the time. Stackoverflow yelled at me for using c headers. 
     * Otherwise no constructive feedback - so I began using this instead
     */
    auto now = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(now);
    char * full_date = strdup(std::ctime(&end_time));
    char date[10];
    strncpy(date, strdup(full_date), 10);

    config->num_ep = config->num_pn + config->num_en; // total number of ep of course needs to equal client and server num
	cnode = new Node(idn, FI_SOURCE, config);
	int ret = cnode->init(BM_CONTROLLER);
	if (ret)
		return ret;

    /*
    *check event queue
    *accept connreq
    *wait till counter >= num_ep
    *if connected: counter++
    */
    ret = cnode->listenController();
	if (ret)
		return ret;

    /*
    * tell clients to connect to servers
    */
    ret = cnode->controllerConnect();
    if(ret)
        return ret;

    /*
    * send start signal to all hosts
    */
    ret = cnode->controllerStart();
	if (ret)
		return ret;

    
    
    int from = -1;
    int to = -1;
    double delta_sec = 0.0;
    double time_sec = 0.0;
    long completions = 0;
    long byte = 0;
    double gigabit = 0.0;
    double throughput = 0.0;

    int check = config->checkpoint_intervall;

    /*
    * start timer
    * wait till time t is reached
    */
    auto start = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds;
    
    while (elapsed_seconds.count() < config->max_duration)
    {
        auto end = std::chrono::system_clock::now();
        elapsed_seconds = end-start;
        if((int)elapsed_seconds.count() >= check){
            printf("%d , %d\n", check, config->max_duration);
            ret = cnode->controllerCheckpoint();
            if (ret) return ret;

            while(1){
                ofstream myfile (date, ios::out | ios::app);
                if (myfile.is_open())
                {
                    //calculate bandwidth
                    for(int i=0; i<n;i++){
                        for(int j=0; j<m;j++){
                            from = cnode->checkpoint[i*m+j].from;
                            to = cnode->checkpoint[i*m+j].to;
                            time_sec = cnode->checkpoint[i*m+j].time_sec;
                            delta_sec = cnode->checkpoint[i*m+j].delta_sec;
                            completions = cnode->checkpoint[i*m+j].completions;
                            byte = completions*config->msg_size;
                            gigabit = (double)byte*8/(1000000000);
                            throughput = gigabit / delta_sec;
                            myfile << from << " " << to << " " << n << " " << m << " " << delta_sec << " " << time_sec << " " << completions << " " << msg_size << " " << byte << " " << gigabit << " " << throughput << "\n";
                            std::cout << from << " " << to << " " << n << " " << m << " " << delta_sec << " " << time_sec << " " << completions << " " << msg_size << " " << byte << " " << gigabit << " " << throughput << "\n";
                        }
                    }
                    myfile.close();
                    break;
                }
            }

            check += config->checkpoint_intervall;
        }
    }

    /*
    * post receive for timers and data from clients 
    * send stop signal to all hosts
    */
    ret = cnode->controllerStop();
	if (ret)
		return ret;


    //create output
    while(1){
        ofstream myfile (date, ios::out | ios::app);
        if (myfile.is_open())
        {
            //calculate bandwidth
            for(int i=0; i<n;i++){
                for(int j=0; j<m;j++){
                    from = cnode->summary[i*m+j].from;
                    to = cnode->summary[i*m+j].to;
                    time_sec = cnode->summary[i*m+j].time_sec;
                    delta_sec = cnode->summary[i*m+j].delta_sec;
                    completions = cnode->summary[i*m+j].completions;
                    byte = completions*config->msg_size;
                    gigabit = (double)byte*8/(1000000000);
                    throughput = gigabit / delta_sec;
                    myfile << from << " " << to << " " << n << " " << m << " " << delta_sec << " " << time_sec << " " << completions << " " << msg_size << " " << byte << " " << gigabit << " " << throughput << "\n";
                    std::cout << from << " " << to << " " << n << " " << m << " " << delta_sec << " " << time_sec << " " << completions << " " << msg_size << " " << byte << " " << gigabit << " " << throughput << "\n";
                }
            }
            myfile.close();
            break;
        }
    }
	return 0;
}

/*
* 1 fill data buffers 
* 2 connect to controller
* 3 recv and await "connect" command
* 4 connect to servers
* 5 send "connected" confirm
* 6 recv and await "start" command
* 7 recv "stop"
* 8 loop: (poll the cq_rx/cq_tx  and decide -> send data / compute a transfer / stop)
* 9 finalize data and send to controller
**/
int client(int id) 
{
    //TODO pass params to config
	inode = new Node(id, 0, config);
	int ret = inode->init(BM_CLIENT);
	if (ret) return ret;
    
    // 1
    ret = inode->setDataBuffers();
    if (ret) return ret;

    // 2 & 3
    ret = inode->connectToController();
    if (ret) return ret;

    // 4 & 5 & 6
    ret = inode->connectToServers();
    if (ret) return ret;

    // 7 & 8
    ret = inode->benchmark();
    if (ret) return ret;
    
    // 9
    //TODO
    return 0;
    
}

/*
* run with ./Benchmark <Nodetype> <id>
*/
int main(int argc, char *argv[])
{
    int ret;
    //note: this app needs to run on the same socket, the HCA is connected to. Otherwise cpu clock speed may drop and performance with it.
    //TODO: add startup param or config entry or both
    ret = numa_run_on_node(0);
    if(ret){
        perror("numa_run_on_node(0)");
    }
	config = new Config();
    if (config->ctrl_size < sizeof(struct transfer)*config->num_pn){
        printf("Warning: control buffer size %d is smaller than required size %d", config->ctrl_size, sizeof(struct transfer)*config->num_pn);
    }
	struct fi_info * fi;
	ret = fi_getinfo(FIVER, NULL, NULL, 0, config->hints, &fi); //initial network check
	if (ret) {
        perror("fi_getinfo");
		printf("initial fi_getinfo: %s\n", fi_strerror(ret));
		ret = fi_getinfo(FIVER, NULL, NULL, 0, NULL, &fi);
		if (ret) {
            perror("fi_getinfo");
			printf("initial fi_getinfo: %s\n", fi_strerror(ret));
			return ret;
		}
	}
	while (fi->next) {
		printf("provider: %s %s\n", fi->fabric_attr->prov_name, fi->fabric_attr->name);
		fi = fi->next;
	}
	switch (argc) {
	case 3: 
    {
        printf("[DEBUG] %s %s \n", argv[1], argv[2]);
        char* mode = argv[1];
        if(strcmp("en", mode)==0){
            printf("Entry Node\n");
            int id = atoi(argv[2]);
            int int_port = atoi(config->start_port);
            #pragma warning(suppress : 4996)
            sprintf(config->port,"%d", id+int_port); // FIXME: find better method
            return client(id);
        }
        if(strcmp("pn", mode)==0){
            printf("Processing Node\n");
            int id = atoi(argv[2]);
            return server(id);
        }
        if(strcmp("cn", mode)==0){
            printf("Central Node\n");
            int id = atoi(argv[2]);
            return controller(id);
        }
    }
	default:
    {
        fprintf(stderr, "wrong number of arguments given: %d\n", argc);
        fprintf(stderr, "usage:\n[Processing Node] %s pn <id>(int)\n[Entry Node] %s en <id>(int)\n[Central Node] %s cn 0\n", argv[0], argv[0], argv[0]);
        return -1;
    }
	}



	return 0;
}
