CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
LDFLAGS ?=
LDLIBS  ?=

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = build

SRCS = $(SRC_DIR)/sequence_counter.c $(SRC_DIR)/main.c
OBJS = $(OBJ_DIR)/sequence_counter.o $(OBJ_DIR)/main.o
TARGET = $(BIN_DIR)/pi_counter

.PHONY: all clean dirs run benchmark analyze

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) results figures

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c include/sequence_counter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET) data/pi_digits.txt --csv results/pattern_counts.csv

benchmark: $(TARGET)
	bash scripts/benchmark.sh

analyze:
	python3 scripts/analyze.py

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
