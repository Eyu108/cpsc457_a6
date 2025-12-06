# Makefile for CPSC 457 Assignment 6
# Compiles dining philosophers and producer-consumer programs

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -g
LDFLAGS = -pthread

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Source files
SEMAPHORE_SRC = $(SRC_DIR)/semaphore.c
DINING_DEADLOCK_SRC = $(SRC_DIR)/dining_deadlock.c
DINING_FREE_SRC = $(SRC_DIR)/dining_deadlock_free.c
PRODUCER_CONSUMER_SRC = $(SRC_DIR)/producer_consumer.c

# Object files
SEMAPHORE_OBJ = $(OBJ_DIR)/semaphore.o
DINING_DEADLOCK_OBJ = $(OBJ_DIR)/dining_deadlock.o
DINING_FREE_OBJ = $(OBJ_DIR)/dining_deadlock_free.o
PRODUCER_CONSUMER_OBJ = $(OBJ_DIR)/producer_consumer.o

# Executables
DINING_DEADLOCK_BIN = dining_deadlock
DINING_FREE_BIN = dining_deadlock_free
PRODUCER_CONSUMER_BIN = producer_consumer

# Default target: build all executables
all: $(DINING_DEADLOCK_BIN) $(DINING_FREE_BIN) $(PRODUCER_CONSUMER_BIN)

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Build semaphore object file
$(SEMAPHORE_OBJ): $(SEMAPHORE_SRC) $(INC_DIR)/semaphore.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $(SEMAPHORE_SRC) -o $(SEMAPHORE_OBJ)

# Build dining_deadlock executable
$(DINING_DEADLOCK_BIN): $(DINING_DEADLOCK_OBJ) $(SEMAPHORE_OBJ)
	$(CC) $(LDFLAGS) $(DINING_DEADLOCK_OBJ) $(SEMAPHORE_OBJ) -o $(DINING_DEADLOCK_BIN)

$(DINING_DEADLOCK_OBJ): $(DINING_DEADLOCK_SRC) $(INC_DIR)/semaphore.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $(DINING_DEADLOCK_SRC) -o $(DINING_DEADLOCK_OBJ)

# Build dining_deadlock_free executable
$(DINING_FREE_BIN): $(DINING_FREE_OBJ) $(SEMAPHORE_OBJ)
	$(CC) $(LDFLAGS) $(DINING_FREE_OBJ) $(SEMAPHORE_OBJ) -o $(DINING_FREE_BIN)

$(DINING_FREE_OBJ): $(DINING_FREE_SRC) $(INC_DIR)/semaphore.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $(DINING_FREE_SRC) -o $(DINING_FREE_OBJ)

# Build producer_consumer executable
$(PRODUCER_CONSUMER_BIN): $(PRODUCER_CONSUMER_OBJ) $(SEMAPHORE_OBJ)
	$(CC) $(LDFLAGS) $(PRODUCER_CONSUMER_OBJ) $(SEMAPHORE_OBJ) -o $(PRODUCER_CONSUMER_BIN)

$(PRODUCER_CONSUMER_OBJ): $(PRODUCER_CONSUMER_SRC) $(INC_DIR)/semaphore.h $(INC_DIR)/buffer.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $(PRODUCER_CONSUMER_SRC) -o $(PRODUCER_CONSUMER_OBJ)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(DINING_DEADLOCK_BIN) $(DINING_FREE_BIN) $(PRODUCER_CONSUMER_BIN)
	rm -f output_exp1.csv output_exp2.csv

# Phony targets
.PHONY: all clean