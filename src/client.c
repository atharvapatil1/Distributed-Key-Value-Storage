#include "kv_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

kv_client_t* kv_client_create(void) {
    kv_client_t* client = (kv_client_t*)malloc(sizeof(kv_client_t));
    if (!client) {
        perror("Failed to allocate client structure");
        return NULL;
    }

    client->socket = -1;
    client->is_connected = false;
    return client;
}

bool kv_client_connect(kv_client_t* client, const char* host, int port) {
    if (!client || !host) {
        fprintf(stderr, "Invalid client or host\n");
        return false;
    }

    // Create socket
    client->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket < 0) {
        perror("Socket creation failed");
        return false;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", host);
        close(client->socket);
        return false;
    }

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = 5;  // 5 seconds timeout
    tv.tv_usec = 0;
    setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(client->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Connect to server
    printf("Connecting to %s:%d...\n", host, port);
    if (connect(client->socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection failed: %s\n", strerror(errno));
        close(client->socket);
        return false;
    }

    client->is_connected = true;
    printf("Connected successfully\n");
    return true;
}

void kv_client_disconnect(kv_client_t* client) {
    if (!client) return;

    if (client->socket != -1) {
        close(client->socket);
        client->socket = -1;
    }
    client->is_connected = false;
    printf("Disconnected from server\n");
}

void kv_client_destroy(kv_client_t* client) {
    if (!client) return;

    kv_client_disconnect(client);
    free(client);
    printf("Client destroyed\n");
}

kv_error_t kv_client_put(kv_client_t* client, const char* key, const char* value) {
    if (!client || !client->is_connected || !key || !value) {
        printf("Invalid parameters or client not connected\n");
        return KV_ERROR_INVALID_KEY;
    }

    // Prepare message
    kv_message_t msg;
    msg.type = MSG_PUT;
    strncpy(msg.key, key, MAX_KEY_SIZE - 1);
    msg.key[MAX_KEY_SIZE - 1] = '\0';
    strncpy(msg.value, value, MAX_VALUE_SIZE - 1);
    msg.value[MAX_VALUE_SIZE - 1] = '\0';

    printf("Sending PUT %s=%s\n", key, value);

    // Send message
    if (send(client->socket, &msg, sizeof(msg), 0) != sizeof(msg)) {
        perror("Failed to send PUT message");
        return KV_ERROR_NETWORK;
    }

    // Receive response
    kv_error_t result;
    if (recv(client->socket, &result, sizeof(result), 0) != sizeof(result)) {
        perror("Failed to receive response");
        return KV_ERROR_NETWORK;
    }

    printf("PUT operation result: %d\n", result);
    return result;
}

kv_error_t kv_client_get(kv_client_t* client, const char* key, char* value) {
    if (!client || !client->is_connected || !key || !value) {
        printf("Invalid parameters or client not connected\n");
        return KV_ERROR_INVALID_KEY;
    }

    // Prepare message
    kv_message_t msg;
    msg.type = MSG_GET;
    strncpy(msg.key, key, MAX_KEY_SIZE - 1);
    msg.key[MAX_KEY_SIZE - 1] = '\0';

    printf("Sending GET %s\n", key);

    // Send message
    if (send(client->socket, &msg, sizeof(msg), 0) != sizeof(msg)) {
        perror("Failed to send GET message");
        return KV_ERROR_NETWORK;
    }

    // Receive response status
    kv_error_t result;
    if (recv(client->socket, &result, sizeof(result), 0) != sizeof(result)) {
        perror("Failed to receive response status");
        return KV_ERROR_NETWORK;
    }

    // If successful, receive value
    if (result == KV_SUCCESS) {
        if (recv(client->socket, value, MAX_VALUE_SIZE, 0) <= 0) {
            perror("Failed to receive value");
            return KV_ERROR_NETWORK;
        }
        value[MAX_VALUE_SIZE - 1] = '\0';
        printf("GET operation successful: %s=%s\n", key, value);
    } else {
        printf("GET operation failed: %d\n", result);
    }

    return result;
}

kv_error_t kv_client_delete(kv_client_t* client, const char* key) {
    if (!client || !client->is_connected || !key) {
        printf("Invalid parameters or client not connected\n");
        return KV_ERROR_INVALID_KEY;
    }

    // Prepare message
    kv_message_t msg;
    msg.type = MSG_DELETE;
    strncpy(msg.key, key, MAX_KEY_SIZE - 1);
    msg.key[MAX_KEY_SIZE - 1] = '\0';

    printf("Sending DELETE %s\n", key);

    // Send message
    if (send(client->socket, &msg, sizeof(msg), 0) != sizeof(msg)) {
        perror("Failed to send DELETE message");
        return KV_ERROR_NETWORK;
    }

    // Receive response
    kv_error_t result;
    if (recv(client->socket, &result, sizeof(result), 0) != sizeof(result)) {
        perror("Failed to receive response");
        return KV_ERROR_NETWORK;
    }

    printf("DELETE operation result: %d\n", result);
    return result;
}