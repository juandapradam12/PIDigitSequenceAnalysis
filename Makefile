# Prefer GCC so -fopenmp links libgomp without extra packages.
ifeq ($(origin CC),default)
  CC = gcc
endif
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
LDFLAGS ?=
LDLIBS  ?=
OMP_CC  ?= $(CC)
OMP_FLAGS ?= -fopenmp

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = build
TEST_DIR = tests

LIB_OBJS = $(OBJ_DIR)/sequence_counter.o
APP_OBJS = $(LIB_OBJS) $(OBJ_DIR)/main.o
TEST_OBJS = $(LIB_OBJS) $(OBJ_DIR)/test_sequence_counter.o

OMP_LIB_OBJS = $(OBJ_DIR)/sequence_counter_omp.o
OMP_APP_OBJS = $(OMP_LIB_OBJS) $(OBJ_DIR)/main_omp.o

TARGET = $(BIN_DIR)/pi_counter
TARGET_OMP = $(BIN_DIR)/pi_counter_omp
TEST_BIN = $(BIN_DIR)/test_sequence_counter

.PHONY: all clean dirs run benchmark analyze test parallel-bench recall generate-digits

all: dirs $(TARGET) $(TARGET_OMP)

dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) results figures

$(TARGET): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET_OMP): $(OMP_APP_OBJS)
	$(OMP_CC) $(LDFLAGS) $(OMP_FLAGS) -o $@ $^ $(LDLIBS)

$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/sequence_counter.o: $(SRC_DIR)/sequence_counter.c include/sequence_counter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/sequence_counter_omp.o: $(SRC_DIR)/sequence_counter.c include/sequence_counter.h
	$(OMP_CC) $(CPPFLAGS) $(CFLAGS) $(OMP_FLAGS) -c -o $@ $<

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c include/sequence_counter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/main_omp.o: $(SRC_DIR)/main.c include/sequence_counter.h
	$(OMP_CC) $(CPPFLAGS) $(CFLAGS) $(OMP_FLAGS) -c -o $@ $<

$(OBJ_DIR)/test_sequence_counter.o: $(TEST_DIR)/test_sequence_counter.c include/sequence_counter.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET) data/pi_digits.txt --csv results/pattern_counts.csv

benchmark: $(TARGET)
	bash scripts/benchmark.sh

parallel-bench: $(TARGET) $(TARGET_OMP)
	bash scripts/parallel_bench.sh

recall: $(TARGET)
	python3 scripts/verify_partition_recall.py

analyze:
	python3 scripts/analyze.py

generate-digits:
	python3 scripts/generate_pi_digits.py --digits $${N:-10000000} --output data/pi_digits_large.txt

test: dirs $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
