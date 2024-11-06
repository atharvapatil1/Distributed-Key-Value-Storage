#include "kv_store.h"
#include <signal.h>

static volatile bool running = true;

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down server...\n");
        running = false;
    }
}

int main(int argc, char* argv[]) {
    // Default port
    int port = 8080;
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Setup signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    printf("Starting key-value store server on port %d...\n", port);

    // Create storage
    kv_store_t* store = kv_store_create("store.dat");
    if (!store) {
        fprintf(stderr, "Failed to create storage\n");
        return 1;
    }

    // Create server
    kv_server_t* server = kv_server_create(store, port);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        kv_store_destroy(store);
        return 1;
    }

    // Optional: Setup backup server connection
    if (argc > 3) {
        const char* backup_host = argv[2];
        int backup_port = atoi(argv[3]);
        if (!kv_server_set_backup(server, backup_host, backup_port)) {
            printf("Warning: Failed to connect to backup server\n");
        } else {
            printf("Connected to backup server at %s:%d\n", backup_host, backup_port);
        }
    }

    // Start server (this will block until server is stopped)
    kv_server_start(server);

    // Cleanup
    kv_server_destroy(server);
    kv_store_destroy(store);

    printf("Server shutdown complete\n");
    return 0;
}