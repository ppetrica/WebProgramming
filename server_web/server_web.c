#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>


int read_file(const char *input_path, char **out_buffer) {
    HANDLE file = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file) return -1;

    DWORD n_chars = GetFileSize(file, NULL);

	DWORD read;
	size_t total_read = 0;

	char *buffer;
    
    int ret;

	buffer = (char *)malloc((n_chars + 1) * sizeof(char));
	if (!buffer) {
		fprintf(stderr, "[x] Failed to allocate memory for input buffer.\n");
		ret = -1;

		goto discard_file;
	}

    read = 1;
	while (read) {
        ReadFile(file, buffer + total_read, n_chars - total_read, &read, NULL);

    	total_read += read;
    }

    printf("total_read = %d n_chars = %d\n", total_read, n_chars);
	if (total_read != n_chars) {
		fprintf(stderr, "\n[x] Failed to read from input file.");
		ret = -1;

		goto discard_buffer;
	}

	buffer[n_chars] = '\0';

	*out_buffer = buffer;

	CloseHandle(file);

	return 0;

discard_buffer:
	free(buffer);

discard_file:
	CloseHandle(file);

	return ret;
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

    iResult = getaddrinfo(NULL, "5678", &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    if (listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        printf( "Listen failed with error: %ld\n", WSAGetLastError() );
    
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    #define DEFAULT_BUFLEN 1024

    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;

    char sendbuf[DEFAULT_BUFLEN];

    // Receive until the peer shuts down the connection
    do {
        iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            for (int i = 0; i < iResult - 1; ++i) {
                if (recvbuf[i] == '\n' && recvbuf[i + 1] == '\r') {
                    if (!strncmp(recvbuf, "GET", 3)) {
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

                        printf("recvbuf: %s\n", recvbuf);
                        printf("k = %d j = %d\n", k, j);

                        char path[256] = "../continut";

                        memcpy(path + 11, recvbuf + j, k - j);
                        path[11 + k - j] = '\0';

                        char *buffer;
                        printf("Path: %s\n", path);
                        if (read_file(path, &buffer) < 0) {
                            return 1;
                        }

                        printf("buffer:\n%s\n", buffer);

                        iSendResult = send(ClientSocket, buffer, strlen(buffer), 0);
                        if (iSendResult == SOCKET_ERROR) {
                            printf("send failed: %d\n", WSAGetLastError());
                            closesocket(ClientSocket);
                            WSACleanup();
                            return 1;
                        }

                        free(buffer);
                    }
                }
            }
        } else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
    } while (iResult > 0);

    return 0;
}