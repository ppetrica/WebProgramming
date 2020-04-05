#include "requests.h"
#include <WS2tcpip.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>


DWORD WINAPI process_connection(LPVOID data) {
    SOCKET socket = (SOCKET)data;

    char recvbuf[1024];
    
    printf("Reading client message.\n");
    int res = recv(socket, recvbuf, sizeof(recvbuf), 0);
    if (res > 0) {
        if (res == sizeof(recvbuf)) {
            printf("Client message too big.\n");
            
            return -1;
        }

        recvbuf[res] = '\0';

        printf("Parsing client message.\n");
        
        process_request(socket, recvbuf);
    } else if (res == 0) {
        printf("Connection closing...\n");
    } else {
        printf("recv failed: %d\n", WSAGetLastError());
    }

    closesocket(socket);

    return 0;
}


int main() {
    WSADATA wsa_data;
    int res = WSAStartup(0x2020, &wsa_data);
    if (res != 0) {
        printf("WSAStartup failed %d\n", res);
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int ret = 0;

    res = getaddrinfo(NULL, "5678", &hints, &result);
    if (res != 0) {
        printf("getaddrinfo failed: %d\n", res);

        ret = 1;
        goto wsa_cleanup;
    }

    SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        
        ret = 1;
        goto wsa_cleanup;
    }

    res = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (res == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        
        ret = 1;
        goto close_socket;
    }

    freeaddrinfo(result);

    printf("Listening on localhost:5678.\n");
    if (listen(ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        printf("Listen failed with error: %ld\n", WSAGetLastError() );
    
        ret = 1;
        goto close_socket;
    }

    SOCKET client_socket;
    while (1) {
        printf("Waiting for incoming connection.\n");
        client_socket = accept(ListenSocket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            continue;
        }
        
        printf("Accepted connection.\n");
        HANDLE thread = CreateThread(NULL, 0, process_connection, (LPVOID)client_socket, 0, NULL);

        if (!thread)
            fprintf(stderr, "Failde to create new thread.\n");
        else
            CloseHandle(thread);
    }

close_socket:
    closesocket(ListenSocket);

wsa_cleanup:
    WSACleanup();

    return ret;
}
