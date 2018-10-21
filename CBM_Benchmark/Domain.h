#pragma once
//fabric includes
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_errno.h>

#include <iostream>
/*class descriptor representing the local network domain providing access to underlying fabric architecture*/
class Domain
{
public:
	Domain();
	//Domain(fid_fabric * fabric, fi_info * info, void * context);
	~Domain();


	//func()
	int initInfo();
	int initFabric();
	int initAccDomain();
	
	
	//get-set
	struct fid_fabric * getFabric();
	void setFabric(fid_fabric * fabric);
	struct fid_domain * getAccDomain();
	void setAccDomain(fid_domain * accDomain);
	struct fi_info * getInfo();
	void setInfo(fi_info * info);
	struct fi_info * getHints();
	void setHints(fi_info * hints);
	void* getContext();
	void setContext(void * context);

private:
	//attr
	struct fid_fabric* fabric;
	struct fid_domain * accDomain;
	struct fi_info* info;
	struct fi_info* hints;
	void *context;

	//func()
};

