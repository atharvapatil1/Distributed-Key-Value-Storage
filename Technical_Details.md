# Technical Implementation Details

## 1. Data Storage Architecture

### Hash Table Implementation
```c
typedef struct {
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
    bool is_occupied;
} kv_entry_t;

typedef struct {
    kv_entry_t entries[TABLE_SIZE];
    pthread_mutex_t locks[TABLE_SIZE];  // Per-bucket locks
    char* backup_file;                  // For persistence
} kv_store_t;
```

### Storage Flow
1. **Hashing Process**:
```c
static unsigned int hash(const char* key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key;
        key++;
    }
    return hash % TABLE_SIZE;
}
```

2. **Data Storage Process**:
```plaintext
PUT Operation Flow:
[Key] → [Hash Function] → [Index] → [Store in Table]

Example:
Key: "user1" → Hash: 12345 → Index: 345 → Store at entries[345]
```

## 2. Memory Layout

### Entry Storage
```plaintext
Memory Layout:
[Entry 0] → key: "user1", value: "John", is_occupied: true
[Entry 1] → empty
[Entry 2] → key: "email", value: "john@example.com", is_occupied: true
...
[Entry N] → empty
```

### Thread Safety
```plaintext
Each bucket has its own mutex:
[Entry 0] ← Mutex 0
[Entry 1] ← Mutex 1
[Entry 2] ← Mutex 2
...
[Entry N] ← Mutex N
```

## 3. Network Protocol

### Message Format
```c
// Network message structure
typedef struct {
    message_type_t type;    // Operation type (PUT/GET/DELETE)
    char key[MAX_KEY_SIZE]; // Key
    char value[MAX_VALUE_SIZE]; // Value (for PUT)
} kv_message_t;
```

### Protocol Flow
```plaintext
PUT Operation:
Client → Server: {type: PUT, key: "user1", value: "John"}
Server → Client: {status: SUCCESS}

GET Operation:
Client → Server: {type: GET, key: "user1"}
Server → Client: {status: SUCCESS, value: "John"}

DELETE Operation:
Client → Server: {type: DELETE, key: "user1"}
Server → Client: {status: SUCCESS}
```

## 4. Output Examples with Technical Details

### 1. PUT Operation
```bash
$ ./build/bin/client put user1 "John Doe"
```

Technical Flow:
```plaintext
1. Client creates message:
   {type: PUT, key: "user1", value: "John Doe"}

2. Server receives and processes:
   a. Hash("user1") = 12345
   b. index = 12345 % TABLE_SIZE
   c. Lock bucket[index]
   d. Store data
   e. Unlock bucket
   f. Send success response

3. Output shows:
   ✓ user1 = John Doe
```

### 2. GET Operation
```bash
$ ./build/bin/client get user1
```

Technical Flow:
```plaintext
1. Client creates message:
   {type: GET, key: "user1"}

2. Server processes:
   a. Hash("user1") = 12345
   b. index = 12345 % TABLE_SIZE
   c. Lock bucket[index]
   d. Read data
   e. Unlock bucket
   f. Send value in response

3. Output: John Doe
```

## 5. Data Distribution

### Current Implementation (Single Node)
```plaintext
[Client] ↔ [Server + Storage]
```

### Distribution Options (Future)

1. **Partitioning**:
```plaintext
Key Range Partitioning:
[A-M] → Server 1
[N-Z] → Server 2

Hash Partitioning:
hash(key) % num_servers → server_id
```

2. **Replication**:
```plaintext
Primary-Secondary:
[Primary Server] → [Secondary 1]
                → [Secondary 2]

Write: Primary → Sync to Secondaries
Read: Any Server
```

## 6. Storage Persistence

### File Format
```plaintext
version:1
instance:1
---
key1,value1
key2,value2
...
```

### Recovery Process
```plaintext
1. Load backup file
2. For each line:
   - Parse key,value
   - Calculate hash
   - Store in memory
```

## 7. Performance Characteristics

### Time Complexity
```plaintext
Average Case:
- PUT: O(1)
- GET: O(1)
- DELETE: O(1)

Worst Case (with collisions):
- All operations: O(n)
```

### Space Complexity
```plaintext
Memory Usage:
- Fixed size table: O(TABLE_SIZE)
- Per entry: sizeof(key) + sizeof(value) + 1 bit
```

## 8. Thread Management

### Server Threading Model
```plaintext
Main Thread: Accept connections
Worker Threads: Handle client requests

[Main Thread] → Accept Connection
    ↓
[Worker Thread Pool] → Process Requests
    ↓
[Storage Access] → Thread-safe operations
```

### Lock Granularity
```plaintext
Fine-grained locking:
- Each bucket has its own mutex
- Multiple operations can proceed in parallel
- Reduces contention
```

## 9. Error Handling and Recovery

### Network Errors
```plaintext
1. Connection Lost:
   - Client: Automatic reconnection
   - Server: Clean up resources

2. Timeout:
   - Client: Retry operation
   - Server: Cancel operation
```

### Storage Errors
```plaintext
1. Full Table:
   - Return KV_ERROR_NO_SPACE
   - Log error

2. Corrupt Data:
   - Load from backup
   - Log error
```