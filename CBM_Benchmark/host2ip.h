//
// Created by andreas on 27.10.18.
// Source: https://stackoverflow.com/questions/2151854/c-resolve-a-host-ip-address-from-a-url

#ifndef LIBFABRIC_BENCHMARK_HOST2IP_H
#define LIBFABRIC_BENCHMARK_HOST2IP_H


class host2ip {
public:
    static char * resolve(char* hostname);
    static void initialise();
    static void uninitialise ();

};


#endif //LIBFABRIC_BENCHMARK_HOST2IP_H
