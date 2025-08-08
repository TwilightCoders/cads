# CADS Test Suite Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I../include -I..
LDFLAGS = 

# Directories
SRC_DIR = ..
TEST_DIR = .
UNIT_DIR = $(TEST_DIR)/unit
INTEGRATION_DIR = $(TEST_DIR)/integration
BUILD_DIR = ../build/tests

# Source files
CORE_SOURCES = $(SRC_DIR)/src/core/packet_data.c \
			   $(SRC_DIR)/src/core/algorithm_registry.c \
			   $(SRC_DIR)/src/core/checksum_engine_threaded.c \
			   $(SRC_DIR)/src/core/checksum_engine_shared.c \
			   $(SRC_DIR)/src/core/operation_tester.c \
			   $(SRC_DIR)/src/core/progress_tracker.c \
		   $(SRC_DIR)/src/core/sequence_evaluator.c \
			   $(SRC_DIR)/src/algorithms/basic_ops.c \
			   $(SRC_DIR)/src/algorithms/intermediate_ops.c \
			   $(SRC_DIR)/src/algorithms/advanced_ops.c \
			   $(SRC_DIR)/src/utils/field_combiner.c \
			   $(SRC_DIR)/src/utils/config.c

UNITY_SOURCES = $(TEST_DIR)/unity.c

# Test executables (with build directory)
UNIT_TESTS = $(BUILD_DIR)/test_algorithm_operations $(BUILD_DIR)/test_packet_data $(BUILD_DIR)/test_field_combiner
INTEGRATION_TESTS = $(BUILD_DIR)/test_forj_algorithm $(BUILD_DIR)/test_search_engine $(BUILD_DIR)/test_packet_discovery $(BUILD_DIR)/test_performance_profile $(BUILD_DIR)/test_rate_calculation $(BUILD_DIR)/benchmark_core $(BUILD_DIR)/test_thread_equivalence

ALL_TESTS = $(UNIT_TESTS) $(INTEGRATION_TESTS)

# Default target
all: $(BUILD_DIR) $(ALL_TESTS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Unit tests
$(BUILD_DIR)/test_algorithm_operations: $(UNIT_DIR)/test_algorithm_operations.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/test_packet_data: $(UNIT_DIR)/test_packet_data.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/test_field_combiner: $(UNIT_DIR)/test_field_combiner.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

# Integration tests  
$(BUILD_DIR)/test_forj_algorithm: $(INTEGRATION_DIR)/test_forj_algorithm.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/test_search_engine: $(INTEGRATION_DIR)/test_search_engine.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/test_packet_discovery: $(INTEGRATION_DIR)/test_packet_discovery.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/test_performance_profile: $(INTEGRATION_DIR)/profile_test.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/test_rate_calculation: $(INTEGRATION_DIR)/test_rate.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/benchmark_core: $(INTEGRATION_DIR)/benchmark_core.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

$(BUILD_DIR)/test_thread_equivalence: $(INTEGRATION_DIR)/test_thread_equivalence.c $(UNITY_SOURCES) $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(UNITY_SOURCES) $(CORE_SOURCES) $(LDFLAGS)

# Run all tests
test: $(ALL_TESTS)
	@echo "🧪 Running CADS Test Suite"
	@echo "=========================="
	@for test in $(ALL_TESTS); do \
		echo "\n🔍 Running $$test..."; \
		./$$test || exit 1; \
	done
	@echo "\n🎉 All tests passed!"

# Run unit tests only
test-unit: $(UNIT_TESTS)
	@echo "🧪 Running Unit Tests"
	@echo "===================="
	@for test in $(UNIT_TESTS); do \
		echo "\n🔍 Running $$test..."; \
		./$$test || exit 1; \
	done

# Run integration tests only
test-integration: $(INTEGRATION_TESTS)
	@echo "🧪 Running Integration Tests"
	@echo "============================"
	@for test in $(INTEGRATION_TESTS); do \
		echo "\n🔍 Running $$test..."; \
		./$$test || exit 1; \
	done

# Clean up
clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o

# Help
help:
	@echo "CADS Test Suite Makefile"
	@echo "========================"
	@echo "Targets:"
	@echo "  all              - Build all tests"
	@echo "  test             - Run all tests"
	@echo "  test-unit        - Run unit tests only"
	@echo "  test-integration - Run integration tests only"
	@echo "  clean            - Remove test executables"
	@echo "  help             - Show this help"

.PHONY: all test test-unit test-integration clean help
