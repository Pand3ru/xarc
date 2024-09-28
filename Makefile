CC = gcc
CFLAGS = -Wall -Wextra -I include -O0
DEBUG_FLAGS = -g 
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
TARGET = $(BIN_DIR)/xarc
DEBUG_TARGET = $(BIN_DIR)/xarc_debug  

all: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS) 
debug: $(DEBUG_TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^

$(DEBUG_TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET) $(DEBUG_TARGET) $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean debug
