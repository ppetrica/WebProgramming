#ifndef REQUESTS_H_
#define REQUESTS_H_


#ifdef _WIN32
	#include <WinSock2.h>
	#define socket_t SOCKET
	#define LAST_SOCKET_ERROR() WSAGetLastError()
#else
	#include <stdint.h>
	#define socket_t int64_t
	#include <sys/socket.h>
	#include <errno.h>
	#define LAST_SOCKET_ERROR() errno
#endif

int process_request(socket_t socket, const char *recvbuf);


#endif
