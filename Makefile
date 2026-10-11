# Makefile for the ls project (OS Programming Assignment 02)

CC      = gcc
CFLAGS  = -Wall -Wextra -g

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TARGET  = $(BIN_DIR)/ls
SRCS    = $(wildcard $(SRC_DIR)/*.c)
OBJS    = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Default rule: build the executable
all: $(TARGET)

# Link all object files into bin/ls
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@

# Compile each .c file in src/ into a .o file in obj/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create output folders if they do not exist
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Remove build products
clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)

.PHONY: all clean
