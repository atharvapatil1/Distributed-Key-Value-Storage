// Main header with all declarations

#ifndef KV_STORE_H
#define KV_STORE_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Configuration
#define MAX_KEY_SIZE 32
#define MAX_VALUE_SIZE 256
#define TABLE_SIZE 1024
#define MAX_CLIENTS 10

// Error codes
typedef enum {
    KV_SUCCESS = 0,
    KV_ERROR_NOT_FOUND,
    KV_ERROR_NO_SPACE,
    KV_ERROR_INVALID_KEY,
    KV_ERROR_NETWORK,
} kv_error_t;

// Storage entry structure
typedef struct {
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
    bool is_occupied;
} kv_entry_t;

// Storage structure
typedef struct {
    kv_entry_t entries[TABLE_SIZE];
    pthread_mutex_t locks[TABLE_SIZE];  // Per-bucket locks
    char* backup_file;                  // For persistence
} kv_store_t;

// Message types
typedef enum {
    MSG_PUT,
    MSG_GET,
    MSG_DELETE,
    MSG_REPLICATE
} message_type_t;

// Network message structure
typedef struct {
    message_type_t type;
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
} kv_message_t;

// Function declarations
// Storage operations
kv_store_t* kv_store_create(const char* backup_file);
void kv_store_destroy(kv_store_t* store);
kv_error_t kv_store_put(kv_store_t* store, const char* key, const char* value);
kv_error_t kv_store_get(kv_store_t* store, const char* key, char* value);
kv_error_t kv_store_delete(kv_store_t* store, const char* key);
void kv_store_save(kv_store_t* store);
void kv_store_load(kv_store_t* store);

// Server operations
typedef struct {
    int socket;
    kv_store_t* store;
    bool is_running;
    pthread_t worker_threads[MAX_CLIENTS];
    int client_sockets[MAX_CLIENTS];
    int backup_socket;  // Connection to backup server
} kv_server_t;

kv_server_t* kv_server_create(kv_store_t* store, int port);
void kv_server_destroy(kv_server_t* server);
void kv_server_start(kv_server_t* server);
void kv_server_stop(kv_server_t* server);
bool kv_server_set_backup(kv_server_t* server, const char* host, int port);

// Client operations
typedef struct {
    int socket;
    bool is_connected;
} kv_client_t;

kv_client_t* kv_client_create(void);
void kv_client_destroy(kv_client_t* client);
bool kv_client_connect(kv_client_t* client, const char* host, int port);
void kv_client_disconnect(kv_client_t* client);
kv_error_t kv_client_put(kv_client_t* client, const char* key, const char* value);
kv_error_t kv_client_get(kv_client_t* client, const char* key, char* value);
kv_error_t kv_client_delete(kv_client_t* client, const char* key);

#endif // KV_STORE_H