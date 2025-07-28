# CADS - Checksum Algorithm Discovery System
# Makefile for modular build system

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS = -lm -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TEST_DIR = tests

# Source directories
CORE_DIR = $(SRC_DIR)/core
ALGO_DIR = $(SRC_DIR)/algorithms
UTILS_DIR = $(SRC_DIR)/utils
CLI_DIR = $(SRC_DIR)/cli

# Include paths
INCLUDES = -I$(INC_DIR) -I$(SRC_DIR)

# Core source files
CORE_SOURCES = $(wildcard $(CORE_DIR)/*.c)
ALGO_SOURCES = $(wildcard $(ALGO_DIR)/*.c)
UTILS_SOURCES = $(wildcard $(UTILS_DIR)/*.c)
CLI_SOURCES = $(wildcard $(CLI_DIR)/*.c)

# All source files
ALL_SOURCES = $(CORE_SOURCES) $(ALGO_SOURCES) $(UTILS_SOURCES) $(CLI_SOURCES)

# Object files
CORE_OBJECTS = $(CORE_SOURCES:$(CORE_DIR)/%.c=$(BUILD_DIR)/core/%.o)
ALGO_OBJECTS = $(ALGO_SOURCES:$(ALGO_DIR)/%.c=$(BUILD_DIR)/algorithms/%.o)
UTILS_OBJECTS = $(UTILS_SOURCES:$(UTILS_DIR)/%.c=$(BUILD_DIR)/utils/%.o)
CLI_OBJECTS = $(CLI_SOURCES:$(CLI_DIR)/%.c=$(BUILD_DIR)/cli/%.o)

ALL_OBJECTS = $(CORE_OBJECTS) $(ALGO_OBJECTS) $(UTILS_OBJECTS) $(CLI_OBJECTS)

# Target executable
TARGET = $(BUILD_DIR)/cads

# Test executables
TEST_SOURCES = $(wildcard $(TEST_DIR)/unit/*.c) $(wildcard $(TEST_DIR)/integration/*.c)
TEST_OBJECTS = $(TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
TEST_TARGETS = $(TEST_SOURCES:%.c=$(BUILD_DIR)/%)

# Legacy target for backwards compatibility
LEGACY_TARGET = $(BUILD_DIR)/ultimate_checksum_cracker

# Default target
.PHONY: all
all: $(TARGET)

# Main target
$(TARGET): $(ALL_OBJECTS) | $(BUILD_DIR)
	@echo "Linking $(TARGET)..."
	$(CC) $(ALL_OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Legacy target (original monolithic version)
$(LEGACY_TARGET): ultimate_checksum_cracker.c | $(BUILD_DIR)
	@echo "Building legacy version..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Object file rules
$(BUILD_DIR)/core/%.o: $(CORE_DIR)/%.c | $(BUILD_DIR)/core
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/algorithms/%.o: $(ALGO_DIR)/%.c | $(BUILD_DIR)/algorithms
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/utils/%.o: $(UTILS_DIR)/%.c | $(BUILD_DIR)/utils
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/cli/%.o: $(CLI_DIR)/%.c | $(BUILD_DIR)/cli
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test object files
$(BUILD_DIR)/tests/%.o: tests/%.c | $(BUILD_DIR)/tests
	@echo "Compiling test $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test executables
$(BUILD_DIR)/tests/%: $(BUILD_DIR)/tests/%.o $(CORE_OBJECTS) $(ALGO_OBJECTS) $(UTILS_OBJECTS) | $(BUILD_DIR)/tests
	@echo "Linking test $@..."
	$(CC) $< $(CORE_OBJECTS) $(ALGO_OBJECTS) $(UTILS_OBJECTS) -o $@ $(LDFLAGS)

# Directory creation
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/core:
	@mkdir -p $(BUILD_DIR)/core

$(BUILD_DIR)/algorithms:
	@mkdir -p $(BUILD_DIR)/algorithms

$(BUILD_DIR)/utils:
	@mkdir -p $(BUILD_DIR)/utils

$(BUILD_DIR)/cli:
	@mkdir -p $(BUILD_DIR)/cli

$(BUILD_DIR)/tests:
	@mkdir -p $(BUILD_DIR)/tests

$(BUILD_DIR)/tests/unit:
	@mkdir -p $(BUILD_DIR)/tests/unit

$(BUILD_DIR)/tests/integration:
	@mkdir -p $(BUILD_DIR)/tests/integration

# Phony targets
.PHONY: clean test test-unit test-integration test-clean install uninstall legacy debug release help

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

# Run tests
test:
	@echo "Running CADS test suite..."
	@$(MAKE) -C $(TEST_DIR) test

# Run unit tests only
test-unit:
	@echo "Running unit tests..."
	@$(MAKE) -C $(TEST_DIR) test-unit

# Run integration tests only  
test-integration:
	@echo "Running integration tests..."
	@$(MAKE) -C $(TEST_DIR) test-integration

# Clean tests
test-clean:
	@echo "Cleaning test artifacts..."
	@$(MAKE) -C $(TEST_DIR) clean

# Install to system
install: $(TARGET)
	@echo "Installing CADS to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/cads
	sudo chmod +x /usr/local/bin/cads
	@echo "Installation complete!"

# Uninstall from system
uninstall:
	@echo "Removing CADS from /usr/local/bin..."
	sudo rm -f /usr/local/bin/cads
	@echo "Uninstallation complete!"

# Legacy build
legacy: $(LEGACY_TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -g3 -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET)

# Show help
help:
	@echo "CADS - Checksum Algorithm Discovery System"
	@echo "Available targets:"
	@echo "  all            - Build main executable (default)"
	@echo "  legacy         - Build original monolithic version"
	@echo "  test           - Build and run all tests"
	@echo "  test-unit      - Run unit tests only"
	@echo "  test-integration - Run integration tests only"
	@echo "  test-clean     - Clean test artifacts"
	@echo "  debug          - Build with debug symbols"
	@echo "  release        - Build optimized release version"
	@echo "  clean          - Remove build artifacts"
	@echo "  install        - Install to system (/usr/local/bin)"
	@echo "  uninstall      - Remove from system"
	@echo "  help           - Show this help message"

# Dependencies (header files)
$(ALL_OBJECTS): $(wildcard $(INC_DIR)/*.h)
$(CORE_OBJECTS): $(wildcard $(CORE_DIR)/*.h)
$(ALGO_OBJECTS): $(wildcard $(ALGO_DIR)/*.h)
$(UTILS_OBJECTS): $(wildcard $(UTILS_DIR)/*.h)
$(CLI_OBJECTS): $(wildcard $(CLI_DIR)/*.h)