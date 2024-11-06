#include "kv_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>  // Add this for va_start, va_end

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8080

// Colors for output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

void print_usage(const char* program) {
    printf("Usage:\n");
    printf("  %s put <key> <value>    Store a key-value pair\n", program);
    printf("  %s get <key>            Retrieve a value by key\n", program);
    printf("  %s delete <key>         Delete a key-value pair\n", program);
    printf("  %s test                 Run tests\n", program);
    printf("\nExamples:\n");
    printf("  %s put mykey \"my value\"\n", program);
    printf("  %s get mykey\n", program);
}

void print_success(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf(COLOR_GREEN "✓ ");
    vprintf(format, args);
    printf(COLOR_RESET "\n");
    va_end(args);
}

void print_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, COLOR_RED "✗ ");
    vfprintf(stderr, format, args);
    fprintf(stderr, COLOR_RESET "\n");
    va_end(args);
}

// Run basic tests
void run_tests(kv_client_t* client) {
    printf("Running tests...\n");

    // Test PUT
    printf("1. Store value: ");
    if (kv_client_put(client, "test_key", "test_value") == KV_SUCCESS) {
        print_success("OK");
    } else {
        print_error("Failed");
        return;
    }

    // Test GET
    printf("2. Retrieve value: ");
    char value[MAX_VALUE_SIZE];
    if (kv_client_get(client, "test_key", value) == KV_SUCCESS) {
        print_success("OK (%s)", value);
    } else {
        print_error("Failed");
        return;
    }

    // Test DELETE
    printf("3. Delete value: ");
    if (kv_client_delete(client, "test_key") == KV_SUCCESS) {
        print_success("OK");
    } else {
        print_error("Failed");
        return;
    }

    print_success("All tests passed!");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Get server details from environment or use defaults
    const char* host = getenv("KV_HOST") ? getenv("KV_HOST") : DEFAULT_HOST;
    int port = getenv("KV_PORT") ? atoi(getenv("KV_PORT")) : DEFAULT_PORT;

    // Create and connect client
    kv_client_t* client = kv_client_create();
    if (!client) {
        print_error("Failed to create client");
        return 1;
    }

    if (!kv_client_connect(client, host, port)) {
        print_error("Failed to connect to server at %s:%d", host, port);
        kv_client_destroy(client);
        return 1;
    }

    int result = 0;

    // Handle commands
    if (strcmp(argv[1], "put") == 0) {
        if (argc != 4) {
            print_usage(argv[0]);
            result = 1;
        } else if (kv_client_put(client, argv[2], argv[3]) == KV_SUCCESS) {
            print_success("%s = %s", argv[2], argv[3]);
        } else {
            print_error("Failed to store value");
            result = 1;
        }
    }
    else if (strcmp(argv[1], "get") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            result = 1;
        } else {
            char value[MAX_VALUE_SIZE];
            if (kv_client_get(client, argv[2], value) == KV_SUCCESS) {
                printf("%s\n", value);
            } else {
                print_error("Key not found: %s", argv[2]);
                result = 1;
            }
        }
    }
    else if (strcmp(argv[1], "delete") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            result = 1;
        } else if (kv_client_delete(client, argv[2]) == KV_SUCCESS) {
            print_success("Deleted: %s", argv[2]);
        } else {
            print_error("Failed to delete key: %s", argv[2]);
            result = 1;
        }
    }
    else if (strcmp(argv[1], "test") == 0) {
        run_tests(client);
    }
    else {
        print_usage(argv[0]);
        result = 1;
    }

    kv_client_destroy(client);
    return result;
}