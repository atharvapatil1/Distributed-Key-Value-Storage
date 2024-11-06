# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g -I./src
LDFLAGS = -pthread

# Project structure
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
SERVER_SOURCES = $(SRC_DIR)/server_main.c \
                $(SRC_DIR)/server.c \
                $(SRC_DIR)/storage.c

CLIENT_SOURCES = $(SRC_DIR)/client_main.c \
                $(SRC_DIR)/client.c

# Object files
SERVER_OBJECTS = $(SERVER_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Executables
SERVER = $(BIN_DIR)/server
CLIENT = $(BIN_DIR)/client

# Default target
all: setup $(SERVER) $(CLIENT)

# Setup build directories
.PHONY: setup
setup:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

# Compile server
$(SERVER): $(SERVER_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile client
$(CLIENT): $(CLIENT_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Run the server
.PHONY: run-server
run-server: $(SERVER)
	./$(SERVER)

# Run the client
.PHONY: run-client
run-client: $(CLIENT)
	./$(CLIENT)

# Run tests
.PHONY: test
test: $(SERVER) $(CLIENT)
	@echo "Starting server..."
	@./$(SERVER) & \
	SERVER_PID=$$!; \
	sleep 1; \
	echo "Running tests..."; \
	./$(CLIENT) test; \
	TEST_STATUS=$$?; \
	echo "Stopping server..."; \
	kill $$SERVER_PID; \
	exit $$TEST_STATUS