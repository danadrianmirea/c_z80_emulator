# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lSDL2 -lSDL2_ttf -lSDL2_image

# Include and library paths for both C: and D:
INCLUDE_DIRS = -IC:/msys64/ucrt64/include -ID:/msys64/ucrt64/include
LIB_DIRS = -LC:/msys64/ucrt64/lib -LD:/msys64/ucrt64/lib

# Source and object directories
SRC_DIR = src
OBJ_DIR = obj

# File lists
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Main target
zx_emulator: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIB_DIRS) $(LDFLAGS)

# Object file rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Create object directory
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Clean up
clean:
	rm -rf zx_emulator $(OBJ_DIR)

.PHONY: clean