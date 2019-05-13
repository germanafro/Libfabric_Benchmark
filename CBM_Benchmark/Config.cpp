#include "Config.h"
// reading a text file
#include <iostream>
#include <fstream>
using namespace std;


const char * Config::console_spacer()
{
	const char * spacer = "--------------------\n";
	return spacer;
}

Config::Config()
{
    //Default Node configs
	addr = NULL;
	controller_addr = NULL;
	start_port = strdup("10000");
    port = strdup(start_port); 
	controller_port = strdup("30000");
	server_offset = 10000;
    num_pn = 10;
    num_en = 10;
    num_ep = 0;
    slot = 0;
    max_duration = 60; // 10 seconds
	checkpoint_intervall = 10;
    max_packet_size = 1024*1024*1024; // 1GB

	msg_size = 1024*1024; // 1MB
	ctrl_size = 1024; // 

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
	hints->fabric_attr->prov_name = strdup("verbs");
	hints->ep_attr->type = FI_EP_MSG;
	hints->domain_attr->mr_mode = FI_MR_LOCAL | FI_MR_ALLOCATED | FI_MR_PROV_KEY | FI_MR_VIRT_ADDR;
	//hints->domain_attr->mr_mode = FI_MR_BASIC;
	hints->caps = FI_MSG | FI_RMA;
	hints->mode = FI_RX_CQ_DATA | FI_CONTEXT; // | FI_LOCAL_MR;

	readConfig();
}


Config::~Config()
{
	fi_freeinfo(hints);
}

/* 
* read and parse conf files
* recognizes "//" as commentary inside conf but !DO NOT USE INLINE COMMENTS!
*/
int Config::readConfig () 
{
	printf("reading config file\n");
	string line;
	ifstream confFile ("./conf/conf");
	if (confFile.is_open())
	{
		vector<string> split = {"", ""}; 
		while ( getline (confFile,line) )
		{
			size_t skip = line.find("//");
			if (skip != std::string::npos) continue;
			
			size_t p = line.find("=");
			if (p != std::string::npos){
				split[0] = line.substr(0,p);
				split[1] = line.substr(p+1);
				if(split[0].compare("msg_size")==0){
					printf("msg_size: %d Byte\n", stoi(split[1]));
					msg_size = stoi(split[1]);
				}
				if(split[0].compare("controller_addr")==0){
					controller_addr = new char[split[1].length()+1];
					strcpy(controller_addr, split[1].c_str());
					printf("controller_addr: %s \n", controller_addr);
				}
				if(split[0].compare("start_port")==0){
					start_port = new char[split[1].length()+1];
					strcpy(start_port, split[1].c_str());
					printf("start_port: %s \n", start_port);
				}
				if(split[0].compare("controller_port")==0){
					controller_port = new char[split[1].length()+1];
					strcpy(controller_port, split[1].c_str());
					printf("controller_port: %s \n", controller_port);
				}
				if(split[0].compare("server_offset")==0){
					server_offset = stoi(split[1]);
					printf("server_offset: %d \n", server_offset);
				}
				if(split[0].compare("max_duration")==0){
					printf("max_duration: %d \n", stoi(split[1]));
					max_duration = stoi(split[1]);
				}
				if(split[0].compare("checkpoint_intervall")==0){
					printf("checkpoint_intervall: %d \n", stoi(split[1]));
					checkpoint_intervall = stoi(split[1]);
				}
				if(split[0].compare("num_en")==0){
					printf("num_en: %d \n", stoi(split[1]));
					num_en = stoi(split[1]);
				}
				if(split[0].compare("num_pn")==0){
					printf("num_pn: %d \n", stoi(split[1]));
					num_pn = stoi(split[1]);
				}
			}
		}
		confFile.close();
	}

	else cout << "Unable to open conf file" << "\n";


	printf("reading serverlist file\n");
	ifstream serverFile ("./conf/serverlist");
	if (serverFile.is_open())
	{
		vector<string> split = {"", ""}; 
		while ( getline (serverFile,line) )
		{
			size_t skip = line.find("//");
			if (skip != std::string::npos) continue;
			
			addr_v.push_back(line);
		}
		printf("found %d addresses\n", addr_v.size());
#ifdef DEBUG
		for(int i=0; i<addr_v.size(); i++){
			cout << addr_v[i] << "\n";
		}
#endif //DEBUG
		serverFile.close();
	}

	else cout << "Unable to open serverlist file" << "\n";

	return 0;
}