#ifndef REQUESTS_H_
#define REQUESTS_H_


#include <WinSock2.h>


int process_request(SOCKET socket, const char *recvbuf);


#endif
