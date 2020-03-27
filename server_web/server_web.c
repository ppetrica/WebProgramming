#include "hash_map.h"
#include <WinSock2.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>


int parse_request_headers(const char *buffer, struct hash_map *map) {
    static char key[256];
    static char value[512];

    const char *line_end = buffer;
    line_end = strstr(line_end, "\r\n");
    if (!line_end)
        return 0;

    for (;;) {
        const char *header_start = line_end + 2;
    
        const char *colon = strstr(header_start, ":");
        if (!colon)
            return 0;

        while (colon > line_end && line_end != 0) {
            header_start = line_end + 2;

            line_end = strstr(header_start, "\r\n");
        }

        if (!line_end)
            return 0;

        size_t l = colon - header_start;
        memcpy(key, header_start, l);
        key[colon - header_start] = '\0';

        while (isspace(*(++colon)));
        
        l = line_end - colon;
        memcpy(value, colon, l);
        value[line_end - colon] = '\0';

        hash_map_add(map, key, value);
    }
}


int process_request(SOCKET socket, const char *recvbuf, size_t size) {
    const char *message_end = strstr(recvbuf, "\r\n\r\n");
    if (!message_end) {
        printf("Invalid client message.\n");

        return -1;
    }

    size_t i = message_end - recvbuf;
    if (strncmp(recvbuf, "GET", 3) != 0)
        return -1;

    printf("Found GET request.\n");
    int j;
    for (j = 3; j < i; ++j) {
        if (!isspace(recvbuf[j]))
            break;
    }

    int k;
    for (k = j; k < i; ++k) {
        if (isspace(recvbuf[k]))
            break;
    }

    char path[256] = "../continut";

    memcpy(path + 11, recvbuf + j, (size_t)k - j);
    path[11 + k - j] = '\0';
    

    printf("Reading path: %s\n", path);
    
    char send_buffer[1024];

    HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        printf("Failed to find requested resource.\n");

        int offset = sprintf(send_buffer,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Content-Type: text/plain\r\n"
            "Server: ppetrica\r\n\r\n");

        int res = send(socket, send_buffer, offset, 0);
        
        if (res == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());

            return -1;
        }

        return 0;
    }

    char content_type[256];
    
    struct hash_map headers;
    hash_map_init(&headers, 10);

    parse_request_headers(recvbuf, &headers);

    struct hash_map_bucket_node *elem = hash_map_get(&headers, "Accept");
    if (!elem) {
        printf("Could not find requested content type\n");

        memcpy(content_type, "text/plain", 11);
    } else {
        char *pos = strchr(elem->value, ',');
        if (pos) {
            size_t l = pos - elem->value;
            memcpy(content_type, elem->value, l);
            content_type[l] = '\0';
        } else {
            strcpy(content_type, elem->value);
        }

        printf("Found content type: %s\n", content_type);
    }

    hash_map_free(&headers);

    DWORD n_bytes = GetFileSize(file, NULL);

    printf("Sending response.\n");
    printf("Found requested resource of size %d\n", n_bytes);

    int offset = sprintf(send_buffer,
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %ul\r\n"
        "Content-Type: %s\r\n"
        "Server: ppetrica\r\n\r\n", n_bytes, content_type);

    int sent = send(socket, send_buffer, offset, 0);
    
    DWORD total_read = 0;
    DWORD read;
    do {
        if (!ReadFile(file, send_buffer, min(n_bytes - total_read, sizeof(send_buffer)), &read, NULL)) {
            printf("Failed reading from file: %d.\n", GetLastError());
            CloseHandle(file);
            
            return -1;
        }

        total_read += read;

        if (!read) break;
        int sent = send(socket, send_buffer, read, 0);

        if (sent == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
            CloseHandle(file);

            return -1;
        }
    } while (read);
    
    CloseHandle(file);

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

    #define DEFAULT_BUFLEN 1024

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    SOCKET ClientSocket;
    while (1) {
        printf("Waiting for incoming connection.\n");
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());

            goto close_client_socket;
        }
        
        printf("Accepted connection.\n");
        
        printf("Reading client message.\n");
        res = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (res > 0) {
            if (res == DEFAULT_BUFLEN) {
                printf("Client message too big.\n");
                
                goto close_client_socket;
            }

            printf("Parsing client message.\n");
            
            process_request(ClientSocket, recvbuf, res);
        } else if (res == 0) {
            printf("Connection closing...\n");
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
        }

    close_client_socket:
        closesocket(ClientSocket);
    }

close_socket:
    closesocket(ListenSocket);

wsa_cleanup:
    WSACleanup();

    return ret;
}