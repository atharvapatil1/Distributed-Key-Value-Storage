# Distributed Key-Value Store

A lightweight distributed key-value store implementation in C that demonstrates core systems programming concepts including networking, concurrency, and file I/O.

## Overview

This project implements a simple distributed key-value store with the following features:
- In-memory key-value storage with persistence
- Multi-threaded server handling multiple client connections
- Client-server communication over TCP/IP
- Basic CRUD operations (Create, Read, Update, Delete)
- Thread-safe operations
- Simple command-line interface

## Project Structure

```
distributed-kv-store/
├── src/
│   ├── kv_store.h      # Main header file
│   ├── storage.c       # Storage implementation
│   ├── server.c        # Server implementation
│   ├── client.c        # Client implementation
│   ├── server_main.c   # Server entry point
│   └── client_main.c   # Client application
├── Makefile            # Build configuration
└── README.md           # This file
```

## Building and Running

### Prerequisites
- GCC compiler
- POSIX-compliant operating system (Linux/Unix)
- pthread library

### Building
```bash
# Clean and build the project
make clean
make
```

### Running
1. Start the server:
```bash
./build/bin/server
```

2. In another terminal, use the client:
```bash
# Store a value
./build/bin/client put name "John Doe"

# Retrieve a value
./build/bin/client get name

# Delete a value
./build/bin/client delete name

# Run tests
./build/bin/client test
```

## Component Details

### 1. Header File (kv_store.h)
This file defines the core data structures and interfaces for the key-value store.

Key Components:
- Maximum sizes for keys and values
- Error codes
- Message types for client-server communication
- Storage, server, and client structures
- Function declarations

```c
// Key constants
#define MAX_KEY_SIZE 32
#define MAX_VALUE_SIZE 256
#define TABLE_SIZE 1024

// Error codes
typedef enum {
    KV_SUCCESS = 0,
    KV_ERROR_NOT_FOUND,
    KV_ERROR_NO_SPACE,
    // ...
} kv_error_t;
```

### 2. Storage Implementation (storage.c)
Implements the core key-value store functionality with thread-safe operations.

Key Features:
- Hash table-based storage
- Thread-safe operations using mutex locks
- Basic CRUD operations
- File-based persistence

Key Functions:
```c
// Create storage instance
kv_store_t* kv_store_create(const char* backup_file);

// Store operations
kv_error_t kv_store_put(kv_store_t* store, const char* key, const char* value);
kv_error_t kv_store_get(kv_store_t* store, const char* key, char* value);
kv_error_t kv_store_delete(kv_store_t* store, const char* key);
```

Implementation Details:
- Uses a hash table for O(1) average case operations
- Each bucket has its own mutex for fine-grained locking
- Periodic persistence to disk
- Crash recovery from backup file

### 3. Server Implementation (server.c)
Handles client connections and request processing.

Key Features:
- Multi-threaded request handling
- TCP socket communication
- Connection management
- Request routing to storage operations

Key Components:
```c
// Server context
typedef struct {
    int socket;
    kv_store_t* store;
    bool is_running;
    pthread_t worker_threads[MAX_CLIENTS];
} kv_server_t;
```

Implementation Details:
- Creates a thread pool for handling client connections
- Uses epoll for efficient I/O multiplexing
- Implements message protocol for client communication
- Handles graceful shutdown

### 4. Client Implementation (client.c)
Provides a clean interface for interacting with the key-value store server.

Key Features:
- Connection management
- Command-line interface
- Error handling
- Automatic reconnection

Key Functions:
```c
// Client operations
kv_client_t* kv_client_create(void);
bool kv_client_connect(kv_client_t* client, const char* host, int port);
kv_error_t kv_client_put(kv_client_t* client, const char* key, const char* value);
kv_error_t kv_client_get(kv_client_t* client, const char* key, char* value);
```

### 5. Main Programs (server_main.c, client_main.c)
Entry points for the server and client applications.

Server Main:
- Handles command-line arguments
- Sets up signal handlers
- Initializes storage and server
- Manages server lifecycle

Client Main:
- Parses commands and arguments
- Manages client lifecycle
- Provides user-friendly interface
- Implements basic testing functionality

## Protocol

The client and server communicate using a simple message protocol:

```c
typedef struct {
    message_type_t type;  // PUT, GET, DELETE
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
} kv_message_t;
```

Message Flow:
1. Client sends request message
2. Server processes request
3. Server sends response (status code + optional value)
4. Client processes response

## Error Handling

The system includes comprehensive error handling:
- Network errors
- Storage errors
- Invalid inputs
- Resource exhaustion
- Concurrent access issues

## Configuration

Environment Variables:
- `KV_HOST`: Server hostname (default: 127.0.0.1)
- `KV_PORT`: Server port (default: 8080)
- `KV_VERBOSE`: Enable verbose output (0 or 1)

## Future Improvements

Potential enhancements:
1. Data replication
2. Consistent hashing
3. Transaction support
4. Better persistence strategy
5. Authentication/Authorization
6. Compression
7. Monitoring and metrics

## Authors
Atharva Patil