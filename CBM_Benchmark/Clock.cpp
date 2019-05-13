#include "Clock.h"



#ifdef __linux__

Clock::Clock()
{
    _start.tv_sec = 0;
    _start.tv_nsec = 0;
    _stop.tv_sec = 0;
    _stop.tv_nsec = 0;
}

int Clock::start()
{
	if (clock_gettime(CLOCK_REALTIME, &_start) == -1) {
		perror("clock gettime");
		return -1;
	}
    return 0;
}

int Clock::stop()
{
	if (clock_gettime(CLOCK_REALTIME, &_stop) == -1) {
		perror("clock gettime");
		return -1;
	}
    return 0;
}

double Clock::getDelta()
{
    accum = (double)(_stop.tv_sec - _start.tv_sec)
		+ (double)(_stop.tv_nsec - _start.tv_nsec)
		/ BILLION;

    if(accum > 0) return accum;
    
    return -1;
}

#else // unsupported OS
Clock::Clock()
{

}

int Clock::start()
{
    return 0;
}

int Clock::stop()
{
    return 0;
}

double Clock::getDelta()
{
    return 0;
}

#endif // OS SELECT