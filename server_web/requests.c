#include "hash_map/hash_map.h"
#include "requests.h"
#include <zlib/zlib.h>
#include <jansson/jansson.h>


int send_empty_response(SOCKET socket, int code, const char* message) {
    char send_buffer[256];

    int offset = sprintf(send_buffer,
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: 0\r\n"
        "Content-Type: text/plain\r\n"
        "Server: ppetrica\r\n\r\n", code, message);

    int res = send(socket, send_buffer, offset, 0);

    if (res == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());

        return -1;
    }

    return 0;
}


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


int parse_resource(const char *recvbuf, char *resource, int max_length) {
    if (!recvbuf || !resource || !max_length) return -1;

    const char *start = strchr(recvbuf, ' ');
    if (!start) return -1;

    while (isspace(*++start));

    const char *end = start + 1;

    while (!isspace(*++end));

    int copied = min(max_length, (int)(end - start));

    memcpy(resource, start, copied);

    return copied;
}


#define CHUNK 16384


int process_get_request(SOCKET socket, const char *recvbuf, const char *message_end) {
    size_t i = message_end - recvbuf;

    char path[256] = "../continut";

    int parsed = parse_resource(recvbuf, path + 11, sizeof(path) - 12);
    if (parsed < 0) return -1;

    path[11 + parsed] = '\0';

    printf("Reading path: %s\n", path);

    static char compress_buffer[CHUNK];

    static char send_buffer[CHUNK];

    HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        printf("Failed to find requested resource.\n");

        return send_empty_response(socket, 404, "Not Found");
    }

    char content_type[256];

    struct hash_map headers;
    hash_map_init(&headers, 10);

    parse_request_headers(recvbuf, &headers);

    // What you need is what you get :D Or maybe not
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

    int compress = 0;

    elem = hash_map_get(&headers, "Accept-Encoding");
    if (elem) {
        char *pos = strchr(elem->value, ',');
        char *start = elem->value;
        if (pos) {
            do {
                if (strncmp(start, "gzip", 4) == 0) {
                    compress = 1;
                    break;
                }

                start = pos + 1;
                while (isspace(*start))
                    ++start;

                pos = strchr(start, ',');
            } while (pos);

            if (!pos && strncmp(start, "gzip", 4) == 0)
                compress = 1;
        } else {
            if (strcmp(elem->value, "gzip")) {
                compress = 1;
            }
        }
    }

    if (compress)
        printf("Sending compressed format.\n");

    hash_map_free(&headers);

    DWORD n_bytes = GetFileSize(file, NULL);

    printf("Sending response.\n");
    printf("Found requested resource of size %d\n", n_bytes);

    struct z_stream_s stream;

    int sent;
    if (compress) {
        stream.zalloc = NULL;
        stream.zfree = NULL;
        stream.opaque = NULL;

        if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            fprintf(stderr, "Failed to initialize deflate for gzip stream.\n");

            return 1;
        }

        int offset = sprintf(send_buffer,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %ul\r\n"
            "Content-Type: %s\r\n"
            "Content-Encoding: gzip\r\n"
            "Server: ppetrica\r\n\r\n", n_bytes, content_type);

        printf("Sending response header.\n");
        sent = send(socket, send_buffer, offset, 0);
    } else {
        int offset = sprintf(send_buffer,
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %ul\r\n"
            "Content-Type: %s\r\n"
            "Server: ppetrica\r\n\r\n", n_bytes, content_type);

        printf("Sending response header.\n");
        sent = send(socket, send_buffer, offset, 0);
    }

    DWORD total_read = 0;
    DWORD read;
    printf("Reading and sending response.\n");
    do {
        if (!ReadFile(file, send_buffer, min(n_bytes - total_read, CHUNK), &read, NULL)) {
            printf("Failed reading from file: %d.\n", GetLastError());
            CloseHandle(file);

            return -1;
        }

        total_read += read;

        if (!compress) {
            if (!read) break;

            int sent = send(socket, send_buffer, read, 0);

            if (sent == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                CloseHandle(file);

                return -1;
            }
        } else {
            do {
                stream.next_in = send_buffer;
                stream.avail_in = read;
                stream.next_out = compress_buffer;
                stream.avail_out = CHUNK;

                deflate(&stream, Z_FULL_FLUSH);

                int have = CHUNK - stream.avail_out;
                int sent = send(socket, compress_buffer, have, 0);
                if (sent == SOCKET_ERROR) {
                    printf("send failed: %d\n", WSAGetLastError());
                    deflateEnd(&stream);
                    CloseHandle(file);

                    return -1;
                }
            } while (stream.avail_out == 0);
        }
    } while (total_read != n_bytes);

    if (compress)
        deflateEnd(&stream);

    CloseHandle(file);

    return 0;
}


int parse_params(const char *body_start, struct hash_map *map) {
    char name[256];
    char value[512];

    const char *param_start = body_start;
    const char *param_val_start;

    int params_parsed = 0;

start:
    param_val_start = strchr(param_start, '=');

    if (!param_val_start) return params_parsed;

    size_t len = param_val_start - param_start;
    if (len > sizeof(name) - 1) {
        printf("Invalid parameter name.\n");

        return -1;
    }

    memcpy(name, param_start, len);
    name[len] = '\0';

    ++param_val_start;

    const char *param_val_end = strchr(param_val_start, '&');
    if (!param_val_end) {
        strcpy(value, param_val_start);
        hash_map_add(map, name, value);

        return params_parsed + 1;
    }

    len = param_val_end - param_val_start;
    memcpy(value, param_val_start, len);
    value[len] = '\0';

    hash_map_add(map, name, value);

    ++params_parsed;

    param_start = param_val_end + 1;

    goto start;
}



int create_user_json(const struct hash_map *map, json_t **out_json) {
    json_t *user_json = json_object();
    if (!user_json)
        return -1;

    int ret = 0;
    struct hash_map_bucket_node *username = hash_map_get(map, "username");
    if (!username) {
        ret = -1;
        goto discard_user_json;
    }

    json_t *json_username = json_string(username->value);
    if (!json_username) {
        ret = -1;
        goto discard_user_json;
    }

    if (json_object_set_new(user_json, "utilizator", json_username) < 0) {
        json_decref(json_username);

        ret = -1;
        goto discard_user_json;
    }

    struct hash_map_bucket_node *password = hash_map_get(map, "password");
    if (!password) {
        ret = -1;

        goto discard_user_json;
    }

    json_t *json_password = json_string(password->value);
    if (!json_password) {
        ret = -1;
        goto discard_user_json;
    }

    if (json_object_set_new(user_json, "parola", json_password) < 0) {
        json_decref(json_password);

        ret = -1;
        goto discard_user_json;
    }

    *out_json = user_json;

    return 0;

discard_user_json:
    json_decref(user_json);

    return ret;
}


#define USERS_FILE "../continut/resurse/utilizatori.json"


int process_post_request(SOCKET socket, const char *recvbuf, const char *message_end) {
    char resource[256];

    int parsed = parse_resource(recvbuf, resource, sizeof(resource) - 1);
    if (parsed < 0) return -1;

    resource[parsed] = '\0';

    if (strncmp(resource, "/api/utilizatori", parsed) != 0) {
        printf("Failed to find requested resource.\n");

        return send_empty_response(socket, 404, "Not Found");
    }

    const char *body_start = message_end + 4;

    struct hash_map params;
    hash_map_init(&params, 8);

    int ret = 0;

    if (parse_params(body_start, &params) < 1) {
        printf("Invalid request parameters.\n");

        send_empty_response(socket, 400, "Bad request");

        ret = -1;
        goto discard_hash_map;
    }

    json_error_t err;
    json_t *users_json = json_load_file(USERS_FILE, 0, &err);
    if (!users_json) {
        printf("Failed to parse json users file: %s.\n", err.text);
        send_empty_response(socket, 500, "Internal Server Error");

        ret = -1;
        goto discard_hash_map;
    }

    if (!json_is_array(users_json)) {
        fprintf(stderr, "Invalid users json found.\n");
        send_empty_response(socket, 500, "Internal Server Error");

        ret = -1;
        goto discard_json;
    }

    json_t *user_json;
    if (create_user_json(&params, &user_json) < 0) {
        printf("Failed to create new user.\n");
        send_empty_response(socket, 400, "Bad Request");

        ret = -1;
        goto discard_hash_map;
    }

    const char *username = json_string_value(json_object_get(user_json, "utilizator"));

    json_t *user;
    size_t index;
    json_array_foreach(users_json, index, user) {
        const char *name = json_string_value(json_object_get(user, "utilizator"));
        if (!name) {
            fprintf(stderr, "Warning: Invalid json entry found in users file.\n");
            continue;
        }

        if (strcmp(username, name) == 0) {
            fprintf(stderr, "User Already in database.\n");
            json_decref(user_json);

            send_empty_response(socket, 400, "Bad request");

            ret = -1;
            goto discard_json;
        }
    }

    if (json_array_append_new(users_json, user_json) < 0) {
        json_decref(user_json);

        fprintf(stderr, "Failed to append new user to existing.\n");
        send_empty_response(socket, 500, "Internal Server Error");

        ret = -1;
        goto discard_json;
    }

    if (json_dump_file(users_json, USERS_FILE, JSON_INDENT(4)) < 0) {
        fprintf(stderr, "Failed to append new user to existing.\n");
        send_empty_response(socket, 500, "Internal Server Error");

        ret = -1;
        goto discard_json;
    }

    printf("Successfully added user to json file.\n");

    ret = send_empty_response(socket, 200, "OK");

discard_json:
    json_decref(users_json);

discard_hash_map:
    hash_map_free(&params);

    return ret;
}


int process_request(SOCKET socket, const char *recvbuf) {
    const char *message_end = strstr(recvbuf, "\r\n\r\n");
    if (!message_end) {
        printf("Invalid client message.\n");

        return -1;
    }

    if (strncmp(recvbuf, "GET", 3) == 0) {
        printf("Found GET request.\n");
        return process_get_request(socket, recvbuf, message_end);
    }

    if (strncmp(recvbuf, "POST", 4) == 0) {
        printf("Found POST request.\n");
        return process_post_request(socket, recvbuf, message_end);
    }

    printf("Unsupported request type.\n");

    return -1;
}
