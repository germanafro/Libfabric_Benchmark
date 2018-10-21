#include "Domain.h"
#include "Config.h"


using namespace std;
/*
*@Summary constructor for standart server aplication
*/
Domain::Domain()
{
	setContext(NULL); // TODO: set right context if necessary

	//init fi_getinfo() 
	cout << "Domain().initInfo(): ";
	int ret = initInfo();
	cout << fi_strerror(ret) << "\n";
	cout << "addr_format: " << info->addr_format << ", dest_addr: " << info->dest_addr << "(" << info->dest_addrlen << ")" <<
		", src_addr: " << info->src_addr << "(" << info->src_addrlen << ")" << "\n";
	cout << Config::console_spacer();

	// init fi_fabric()
	cout << "Domain().initFabric(): ";
	ret = initFabric();
	cout << fi_strerror(ret) << "\n";
	if (ret == 0) {
		cout << "prov_name " << info->fabric_attr->prov_name << "\n";
	}
	cout << Config::console_spacer();

	// init fi_domain()
	cout << "Domain().initAccDomain(): ";
	ret = initAccDomain();
	cout << fi_strerror(ret) << "\n";
	if (ret == 0) {
		//std::cout << "prov_name " << info->fabric_attr->prov_name << "\n";
	}
	cout << Config::console_spacer();

}

Domain::~Domain()
{	//cleanup
	if (info) fi_freeinfo(info);
	if (hints) fi_freeinfo(hints);
	if (fabric) fi_close(&(fabric->fid));
	if (accDomain) fi_close(&(accDomain->fid));
}

//methods
int Domain::initInfo() {
	/*refresh if already exists*/
	if (info) {
		fi_freeinfo(info);
	}

	hints = fi_allocinfo();
	if (!hints) return EXIT_FAILURE;

	/* hints will point to a cleared fi_info structure
	* Initialize hints here to request specific network capabilities
	* TODO: define in Config
	*/
	hints->ep_attr->type = FI_EP_MSG; // TODO: which is best suitable for rma?
	//hints->caps = FI_RMA;
	hints->caps = FI_MSG;
	const char * destination = "127.0.0.1";
	const char * port = "6666";


	int ret = fi_getinfo(FI_VERSION(1, 6), destination, port, 0, hints, &info);
	fi_freeinfo(hints);

	/* return success/failure */
	return ret;
}

int Domain::initFabric() {
	int ret;
	/*check for fi_getinfo(). accquire info if it doesnt already exist*/
	if (info == NULL) {
		ret = initInfo();
		if (ret < 0) return ret; // @error: could not accquire info
	}
	ret =  fi_fabric(info->fabric_attr, &fabric, context);
	return ret;
}

int Domain::initAccDomain() {
	int ret;
	/*check for fi_fabric(). accquire if it doesnt already exist*/
	if (fabric == NULL) {
		ret = initFabric();
		if (ret < 0) return ret; // @error: could not accquire fabric
	}
	ret = fi_domain(fabric, info, &accDomain, context);
	return ret;
}


//getters and setters


fid_fabric* Domain::getFabric()
{
	return this->fabric;
}

void Domain::setFabric(fid_fabric * fabric)
{
	this->fabric = fabric;
}

fid_domain * Domain::getAccDomain()
{
	return accDomain;
}

void Domain::setAccDomain(fid_domain * accDomain)
{
	this->accDomain = accDomain;
}

fi_info* Domain::getInfo()
{
	return this->info;
}

void Domain::setInfo(fi_info * info)
{
	this->info = info;
}

fi_info* Domain::getHints()
{
	return this->hints;
}

void Domain::setHints(fi_info * hints)
{
	this->hints = hints;
}

void* Domain::getContext()
{
	return this->context;
}

void Domain::setContext(void * context)
{
	this->context = context;
}
