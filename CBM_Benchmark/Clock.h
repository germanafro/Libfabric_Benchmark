#ifdef __linux__
	#include <sys/time.h>
#endif

#include "Config.h"


class Clock
{
public:
	Clock();
	~Clock();
	//func()
	int start();
	int stop();
	double getDelta();


private:
#ifdef __linux__
	struct timespec _start;
	struct timespec _stop;
	double accum;
#endif


};

#define BILLION  1000000000L