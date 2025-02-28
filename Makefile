/* Makefile */
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

zx_emulator: $(OBJS)
	$(CC) $(CFLAGS) -o zx_emulator $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf zx_emulator $(OBJ_DIR)
