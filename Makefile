CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I$(INCLUDE_DIRS)
#CFLAGS += -mwindows
LDFLAGS = -lSDL2 -lSDL2_ttf -lSDL2_image

# MSYS2 UCRT64 paths
INCLUDE_DIRS = C:/msys64/ucrt64/include
LIB_DIRS = C:/msys64/ucrt64/lib

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

zx_emulator: $(OBJS)
	$(CC) -o $@ $(OBJS) -L$(LIB_DIRS) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf zx_emulator $(OBJ_DIR)

.PHONY: clean