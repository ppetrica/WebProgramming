#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>


// TODO: Large files should be read and sent in chunks
int read_file(const char *input_path, char **out_buffer) {
    HANDLE file = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) return -1;

    DWORD n_bytes = GetFileSize(file, NULL);

	DWORD read;
	size_t total_read = 0;

	char *buffer;
    
    int ret;

	buffer = (char *)malloc((n_bytes + 1) * sizeof(char));
	if (!buffer) {
		ret = -1;

		goto discard_file;
	}

    read = 1;
	while (read) {
        ReadFile(file, buffer + total_read, n_bytes - total_read, &read, NULL);

    	total_read += read;
    }

	if (total_read != n_bytes) {
		ret = -1;

		goto discard_buffer;
	}

	buffer[n_bytes] = '\0';

	*out_buffer = buffer;

	CloseHandle(file);

	return n_bytes;

discard_buffer:
	free(buffer);

discard_file:
	CloseHandle(file);

	return ret;
}


int process_request(SOCKET ClientSocket, const char *recvbuf, size_t size) {
    for (int i = 0; i < size - 3; ++i) {
        if (recvbuf[i] == '\r' && recvbuf[i + 1] == '\n' &&
            recvbuf[i + 2] == '\r' && recvbuf[i + 3] == '\n') {
            if (!strncmp(recvbuf, "GET", 3)) {
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

                memcpy(path + 11, recvbuf + j, k - j);
                path[11 + k - j] = '\0';

                char *ind = strstr(recvbuf, "Accept:");
                char *t_offset = ind + 7;
                while (isspace(*t_offset)) {
                    ++t_offset;
                }

                char *finish = t_offset;
                while (!isspace(*finish) && *finish != ',') {
                    finish++;
                }

                char content_type[256] = {0};
                strncpy(content_type, t_offset, finish - t_offset);
                printf("Requested content type: %s\n", content_type);

                char *buffer = NULL;
                printf("Reading path: %s\n", path);

                int n_bytes = read_file(path, &buffer);
                char *send_buffer = NULL;
                size_t offset;
                if (n_bytes < 0) {
                    printf("Failed to find requested resource.\n");
                    n_bytes = 0;

                    send_buffer = malloc(200);
                    offset = sprintf(send_buffer,
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Length: 0\r\n"
                        "Content-Type: text/plain\r\n"
                        "Server: ppetrica\r\n\r\n");

                    printf("Sending response.\n");
                } else {
                    printf("Found requested resource of size %d\n", n_bytes);
                    send_buffer = malloc(n_bytes + 200);

                    offset = sprintf(send_buffer,
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Length: %d\r\n"
                        "Content-Type: %s\r\n"
                        "Server: ppetrica\r\n\r\n", n_bytes, content_type);
                    memcpy(send_buffer + offset, buffer, n_bytes);
                }

                int iSendResult = send(ClientSocket, send_buffer, n_bytes + offset, 0);

                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed: %d\n", WSAGetLastError());
                    free(buffer);
                    free(send_buffer);

                    return -1;
                }

                free(buffer);
                free(send_buffer);
            }
        }
    }
}


int main() {
    WSADATA wsa_data;
    HRESULT iResult = WSAStartup(0x2020, &wsa_data);
    if (iResult != 0) {
        printf("WSAStartup failed %d\n", iResult);
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int ret = 0;

    iResult = getaddrinfo(NULL, "5678", &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);

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

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
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
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;

    SOCKET ClientSocket;
    // Receive until the peer shuts down the connection
    while (1) {
        printf("Waiting for incoming connection.\n");
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());

            ret = 1;
            goto close_socket;
        }
        
        printf("Accepted connection.\n");
        
        printf("Reading client message.\n");
        iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            printf("Parsing client message.\n");
            if (process_request(ClientSocket, recvbuf, iResult) < 0) {
                goto close_socket;
            }
            
        } else if (iResult == 0) {
            printf("Connection closing...\n");
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);

            ret = 1;
            goto close_socket;
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