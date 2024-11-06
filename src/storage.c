#include "kv_store.h"

// Hash function for keys
static unsigned int hash(const char* key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key;
        key++;
    }
    return hash % TABLE_SIZE;
}

// Create a new key-value store
kv_store_t* kv_store_create(const char* backup_file) {
    kv_store_t* store = malloc(sizeof(kv_store_t));
    if (!store) return NULL;

    // Initialize all entries as empty
    memset(store->entries, 0, sizeof(store->entries));
    
    // Initialize locks
    for (int i = 0; i < TABLE_SIZE; i++) {
        pthread_mutex_init(&store->locks[i], NULL);
    }

    store->backup_file = strdup(backup_file);
    
    // Load any existing data
    kv_store_load(store);
    
    return store;
}

// Destroy the store and free resources
void kv_store_destroy(kv_store_t* store) {
    if (!store) return;

    // Save data before destroying
    kv_store_save(store);

    // Destroy locks
    for (int i = 0; i < TABLE_SIZE; i++) {
        pthread_mutex_destroy(&store->locks[i]);
    }

    free(store->backup_file);
    free(store);
}

// Store a key-value pair
kv_error_t kv_store_put(kv_store_t* store, const char* key, const char* value) {
    if (!key || strlen(key) >= MAX_KEY_SIZE) {
        return KV_ERROR_INVALID_KEY;
    }

    unsigned int index = hash(key);
    
    pthread_mutex_lock(&store->locks[index]);

    // Store the entry
    strncpy(store->entries[index].key, key, MAX_KEY_SIZE - 1);
    strncpy(store->entries[index].value, value, MAX_VALUE_SIZE - 1);
    store->entries[index].is_occupied = true;

    pthread_mutex_unlock(&store->locks[index]);
    
    return KV_SUCCESS;
}

// Retrieve a value by key
kv_error_t kv_store_get(kv_store_t* store, const char* key, char* value) {
    if (!key || !value) return KV_ERROR_INVALID_KEY;

    unsigned int index = hash(key);
    
    pthread_mutex_lock(&store->locks[index]);

    if (!store->entries[index].is_occupied ||
        strcmp(store->entries[index].key, key) != 0) {
        pthread_mutex_unlock(&store->locks[index]);
        return KV_ERROR_NOT_FOUND;
    }

    strncpy(value, store->entries[index].value, MAX_VALUE_SIZE - 1);
    
    pthread_mutex_unlock(&store->locks[index]);
    
    return KV_SUCCESS;
}

// Delete a key-value pair
kv_error_t kv_store_delete(kv_store_t* store, const char* key) {
    if (!key) return KV_ERROR_INVALID_KEY;

    unsigned int index = hash(key);
    
    pthread_mutex_lock(&store->locks[index]);

    if (!store->entries[index].is_occupied ||
        strcmp(store->entries[index].key, key) != 0) {
        pthread_mutex_unlock(&store->locks[index]);
        return KV_ERROR_NOT_FOUND;
    }

    store->entries[index].is_occupied = false;
    
    pthread_mutex_unlock(&store->locks[index]);
    
    return KV_SUCCESS;
}

// Save store to disk
void kv_store_save(kv_store_t* store) {
    if (!store || !store->backup_file) return;

    FILE* fp = fopen(store->backup_file, "w");
    if (!fp) return;

    for (int i = 0; i < TABLE_SIZE; i++) {
        if (store->entries[i].is_occupied) {
            fprintf(fp, "%s,%s\n", 
                store->entries[i].key, 
                store->entries[i].value);
        }
    }

    fclose(fp);
}

// Load store from disk
void kv_store_load(kv_store_t* store) {
    if (!store || !store->backup_file) return;

    FILE* fp = fopen(store->backup_file, "r");
    if (!fp) return;

    char line[MAX_KEY_SIZE + MAX_VALUE_SIZE + 2];
    while (fgets(line, sizeof(line), fp)) {
        char key[MAX_KEY_SIZE];
        char value[MAX_VALUE_SIZE];
        
        char* comma = strchr(line, ',');
        if (comma) {
            *comma = '\0';
            strncpy(key, line, MAX_KEY_SIZE - 1);
            strncpy(value, comma + 1, MAX_VALUE_SIZE - 1);
            value[strcspn(value, "\n")] = 0;  // Remove newline
            
            kv_store_put(store, key, value);
        }
    }

    fclose(fp);
}