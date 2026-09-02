CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wstrict-prototypes -Wundef -fno-common -O0 -g -Icompiler/include -DALWAYS_VERBOSE
LDFLAGS =

SRC_DIR = compiler/src
BUILD_DIR = compiler/build

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

ALL_SRC = $(call rwildcard,$(SRC_DIR),*.c)

SRC = $(filter-out $(SRC_DIR)/Driver/test_main.c $(wildcard $(SRC_DIR)/Tests/*.c) $(wildcard $(SRC_DIR)/Tests/**/*.c), $(ALL_SRC))

TEST_SRC = $(filter-out $(SRC_DIR)/Driver/main.c, $(ALL_SRC))

OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
TEST_OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/test_%.o, $(TEST_SRC))

TARGET = $(BUILD_DIR)/deuterium
TEST_TARGET = $(BUILD_DIR)/run_tests

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(BUILD_DIR)/test_%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(TEST_OBJ) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test clean
