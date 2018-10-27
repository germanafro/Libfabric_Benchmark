//
// Created by andreas on 27.10.18.
//

#include "host2ip.h"
#include <stdio.h>

#ifdef _WIN32
#  include "winsock.h"
#else
#  include <netdb.h>
#  include <arpa/inet.h>
#endif

void host2ip::initialise()
{
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup (MAKEWORD(1, 1), &data) != 0)
    {
        fputs ("Could not initialise Winsock.\n", stderr);
        exit (1);
    }
#endif
}

void host2ip::uninitialise ()
{
#ifdef _WIN32
    WSACleanup ();
#endif
}

char * host2ip::resolve(char *hostname)
{
    struct hostent *he;

    host2ip::initialise();

    he = gethostbyname (hostname); // alternatively theres also getaddrinfo()
    if (he == NULL)
    {
        switch (h_errno)
        {
            case HOST_NOT_FOUND:
                fputs ("The host was not found.\n", stderr);
                break;
            case NO_ADDRESS:
                fputs ("The name is valid but it has no address.\n", stderr);
                break;
            case NO_RECOVERY:
                fputs ("A non-recoverable name server error occurred.\n", stderr);
                break;
            case TRY_AGAIN:
                fputs ("The name server is temporarily unavailable.", stderr);
                break;
            default:
                fputs ("unknown error", stderr);
                break;
        }
        return NULL;
    }
    host2ip::uninitialise();

    return inet_ntoa (*((struct in_addr *) he->h_addr_list[0]));
}