#include "kv_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

// Structure for thread arguments
typedef struct {
    int client_socket;
    kv_store_t* store;
} client_thread_args;

// Handle client connection
static void* handle_client_connection(void* arg) {
    client_thread_args* args = (client_thread_args*)arg;
    int client_socket = args->client_socket;
    kv_store_t* store = args->store;
    free(args);

    printf("New client handler started\n");

    while (1) {
        // Receive message from client
        kv_message_t message;
        ssize_t recv_size = recv(client_socket, &message, sizeof(message), 0);
        
        if (recv_size <= 0) {
            printf("Client disconnected\n");
            break;
        }

        printf("Received command: %d, Key: %s\n", message.type, message.key);

        kv_error_t result;
        char value[MAX_VALUE_SIZE];

        // Process message
        switch (message.type) {
            case MSG_PUT:
                result = kv_store_put(store, message.key, message.value);
                send(client_socket, &result, sizeof(result), 0);
                printf("PUT %s=%s: %d\n", message.key, message.value, result);
                break;

            case MSG_GET:
                result = kv_store_get(store, message.key, value);
                send(client_socket, &result, sizeof(result), 0);
                if (result == KV_SUCCESS) {
                    send(client_socket, value, MAX_VALUE_SIZE, 0);
                }
                printf("GET %s: %d\n", message.key, result);
                break;

            case MSG_DELETE:
                result = kv_store_delete(store, message.key);
                send(client_socket, &result, sizeof(result), 0);
                printf("DELETE %s: %d\n", message.key, result);
                break;

            default:
                printf("Unknown command received: %d\n", message.type);
                result = KV_ERROR_INVALID_KEY;
                send(client_socket, &result, sizeof(result), 0);
                break;
        }
    }

    close(client_socket);
    printf("Client handler finished\n");
    return NULL;
}

kv_server_t* kv_server_create(kv_store_t* store, int port) {
    printf("Creating server on port %d\n", port);
    
    kv_server_t* server = (kv_server_t*)malloc(sizeof(kv_server_t));
    if (!server) {
        perror("Failed to allocate server structure");
        return NULL;
    }

    // Initialize server structure
    server->store = store;
    server->socket = -1;
    server->is_running = false;
    server->backup_socket = -1;
    memset(server->client_sockets, -1, sizeof(server->client_sockets));

    // Create socket
    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket < 0) {
        perror("Socket creation failed");
        free(server);
        return NULL;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server->socket);
        free(server);
        return NULL;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket
    if (bind(server->socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server->socket);
        free(server);
        return NULL;
    }

    // Listen
    if (listen(server->socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server->socket);
        free(server);
        return NULL;
    }

    printf("Server created successfully\n");
    return server;
}

void kv_server_start(kv_server_t* server) {
    if (!server || server->is_running) {
        return;
    }

    printf("Starting server...\n");
    server->is_running = true;

    // Accept client connections
    while (server->is_running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // Accept connection
        int client_socket = accept(server->socket, 
                                 (struct sockaddr*)&client_addr,
                                 &client_addr_len);

        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Print client information
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("New connection from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // Create thread arguments
        client_thread_args* args = malloc(sizeof(client_thread_args));
        if (!args) {
            perror("Failed to allocate thread arguments");
            close(client_socket);
            continue;
        }
        args->client_socket = client_socket;
        args->store = server->store;

        // Create thread for client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client_connection, args) != 0) {
            perror("Failed to create thread");
            free(args);
            close(client_socket);
            continue;
        }
        pthread_detach(thread);
    }
}

void kv_server_stop(kv_server_t* server) {
    if (!server) return;

    printf("Stopping server...\n");
    server->is_running = false;

    // Close all client connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->client_sockets[i] != -1) {
            close(server->client_sockets[i]);
            server->client_sockets[i] = -1;
        }
    }

    // Close server socket
    if (server->socket != -1) {
        close(server->socket);
        server->socket = -1;
    }

    printf("Server stopped\n");
}

void kv_server_destroy(kv_server_t* server) {
    if (!server) return;

    kv_server_stop(server);
    free(server);
    printf("Server destroyed\n");
}

bool kv_server_set_backup(kv_server_t* server, const char* host, int port) {
    if (!server || !host) return false;

    printf("Setting up backup server %s:%d\n", host, port);

    struct sockaddr_in backup_addr;
    backup_addr.sin_family = AF_INET;
    backup_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &backup_addr.sin_addr) <= 0) {
        perror("Invalid backup address");
        return false;
    }

    int backup_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (backup_socket < 0) {
        perror("Backup socket creation failed");
        return false;
    }

    if (connect(backup_socket, (struct sockaddr*)&backup_addr, sizeof(backup_addr)) < 0) {
        perror("Backup connection failed");
        close(backup_socket);
        return false;
    }

    if (server->backup_socket != -1) {
        close(server->backup_socket);
    }

    server->backup_socket = backup_socket;
    printf("Backup server connected successfully\n");
    return true;
}