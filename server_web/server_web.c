#include "requests.h"
#ifdef _WIN32
#include <WS2tcpip.h>
#else
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>


#ifdef _WIN32
	#define thread_ret_t DWORD
    #define API WINAPI
#else
	#define thread_ret_t void *
	#define closesocket close
	#define INVALID_SOCKET -1
    #define API
#endif


thread_ret_t API process_connection(void *data) {
    socket_t socket = (socket_t)data;

    char recvbuf[1024];
    
    printf("Reading client message.\n");
    int res = recv(socket, recvbuf, sizeof(recvbuf), 0);
    if (res > 0) {
        if (res == sizeof(recvbuf)) {
            printf("Client message too big.\n");
            
            return (thread_ret_t)-1;
        }

        recvbuf[res] = '\0';

        printf("Parsing client message.\n");
        
        process_request(socket, recvbuf);
    } else if (res == 0) {
        printf("Connection closing...\n");
    } else {
        printf("recv failed: %d\n", LAST_SOCKET_ERROR());
    }

    closesocket(socket);

    return 0;
}


int main() {
	int res;

#ifdef _WIN32
    WSADATA wsa_data;
    res = WSAStartup(0x2020, &wsa_data);
    if (res != 0) {
        printf("WSAStartup failed %d\n", res);
        return 1;
    }
#endif

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int ret = 0;

    res = getaddrinfo("0.0.0.0", "5678", &hints, &result);
    if (res != 0) {
        printf("getaddrinfo failed: %d\n", res);

        ret = 1;
        goto wsa_cleanup;
    }

    socket_t ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", LAST_SOCKET_ERROR());
        freeaddrinfo(result);
        
        ret = 1;
        goto wsa_cleanup;
    }

#ifndef _WIN32
	int on = 1;
	setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#endif

    res = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (res != 0) {
        printf("bind failed with error: %d\n", LAST_SOCKET_ERROR());
        freeaddrinfo(result);
        
        ret = 1;
        goto close_socket;
    }

    freeaddrinfo(result);

    printf("Listening on 0.0.0.0:5678.\n");
    if (listen(ListenSocket, SOMAXCONN ) != 0) {
        printf("Listen failed with error: %ld\n", LAST_SOCKET_ERROR() );
    
        ret = 1;
        goto close_socket;
    }

    socket_t client_socket;
    while (1) {
        printf("Waiting for incoming connection.\n");
        client_socket = accept(ListenSocket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            printf("accept failed: %d\n", LAST_SOCKET_ERROR());
            continue;
        }
        
        printf("Accepted connection.\n");
#ifdef _WIN32
        HANDLE thread = CreateThread(NULL, 0, process_connection, (void *)client_socket, 0, NULL);
#else
		pthread_t thread;
		pthread_create(&thread, NULL, process_connection, (void *)client_socket);
#endif

        if (!thread)
            fprintf(stderr, "Failde to create new thread.\n");
        else
#ifdef _WIN32
            CloseHandle(thread);
#else
			pthread_detach(thread);
#endif
    }

close_socket:
    closesocket(ListenSocket);

wsa_cleanup:
#ifdef _WIN32
    WSACleanup();
#endif

    return ret;
}
