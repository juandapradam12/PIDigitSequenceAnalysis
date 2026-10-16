CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
LDFLAGS ?=
LDLIBS  ?=

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = build
TEST_DIR = tests

LIB_OBJS = $(OBJ_DIR)/sequence_counter.o
APP_OBJS = $(LIB_OBJS) $(OBJ_DIR)/main.o
TEST_OBJS = $(LIB_OBJS) $(OBJ_DIR)/test_sequence_counter.o

TARGET = $(BIN_DIR)/pi_counter
TEST_BIN = $(BIN_DIR)/test_sequence_counter

.PHONY: all clean dirs run benchmark analyze test

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) results figures

$(TARGET): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/sequence_counter.o: $(SRC_DIR)/sequence_counter.c include/sequence_counter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c include/sequence_counter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/test_sequence_counter.o: $(TEST_DIR)/test_sequence_counter.c include/sequence_counter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET) data/pi_digits.txt --csv results/pattern_counts.csv

benchmark: $(TARGET)
	bash scripts/benchmark.sh

analyze:
	python3 scripts/analyze.py

test: dirs $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
