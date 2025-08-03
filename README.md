# CADS - Checksum Algorithm Discovery System

**Reverse engineering radio communication protocols through exhaustive checksum analysis.**

[![CI](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Performance](https://img.shields.io/badge/performance-32M%20tests%2Fsec-blue.svg)]()
[![Language](https://img.shields.io/badge/language-C99-blue.svg)]()
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey.svg)

## Overview

CADS is a high-performance C application designed to reverse engineer unknown checksum algorithms in radio communication protocols. Originally developed for analyzing the Forj Link ESP32 radio firmware, CADS uses recursive exhaustive search to discover complex multi-operation checksum sequences.

Perfect for security researchers, reverse engineers, and radio enthusiasts working with unknown or proprietary communication protocols.

## Features

### 🔍 **Exhaustive Algorithm Discovery**
- **29 checksum algorithms** including CRC variants, Fletcher checksums, and custom operations
- **Recursive operation sequences** with unlimited complexity (practical limit: 6+ operations)
- **Multi-byte checksum support** (1-8 bytes) using 64-bit arithmetic
- **Variable packet sizes** (1-1024 bytes) with dynamic field selection

### ⚡ **High Performance**
- **32+ million tests per second** single-threaded performance
- **Time-based progress updates** eliminate performance overhead
- **Smart search space optimization** with configurable complexity levels
- **Custom operation sets** for targeted algorithm discovery

### 📊 **Advanced Progress Tracking**
- **Real-time ETA calculations** with exponential smoothing
- **Detailed progress display** showing rate, elapsed time, solutions found
- **Color-coded terminal output** with comprehensive metrics
- **Smooth progress bars** with minimal update overhead

### 🧪 **Comprehensive Testing**
- **Unity test framework** with 40+ unit and integration tests
- **JSON-based packet datasets** for reproducible testing
- **Performance profiling** and bottleneck analysis
- **Configurable search parameters** with helper functions

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/TwilightCoders/CADS.git
cd CADS

# Build the application
make clean && make all

# Run the test suite
make test
```

### Basic Usage

```bash
# Run with default GMRS dataset
./build/cads

# Analyze custom packet data
./build/cads --input data/my_packets.jsonl --complexity intermediate

# Target specific algorithms
./build/cads --operations "add,xor,ones_complement" --max-fields 4
```

### Packet Data Format

CADS accepts JSON Lines format for packet datasets:

```json
{"packet": "9c30010000000000", "checksum": "31", "description": "GMRS CH1"}
{"packet": "9c30020000000000", "checksum": "32", "description": "GMRS CH2"}
{"packet": "9c30030000000000", "checksum": "33", "description": "GMRS CH3"}
```

## Architecture

### Core Components

- **Recursive Search Engine** (`src/core/checksum_engine_recursive.c`) - AST-based algorithm discovery
- **Algorithm Registry** (`src/core/algorithm_registry.c`) - 29 checksum operations with metadata
- **Progress Tracking** (`src/core/progress_tracker.c`) - Real-time performance monitoring
- **Packet Loader** (`src/core/packet_data.c`) - JSON dataset management
- **Configuration System** (`src/utils/config.c`) - Clean parameter management

### Supported Algorithms

**Basic Operations** (6 algorithms):
- Identity, Add, XOR, Ones Complement, Const Add, Multiply

**Intermediate Operations** (11 algorithms):  
- Subtract, Shift Left/Right, Rotate Left/Right, AND, OR, NAND, etc.

**Advanced Operations** (12 algorithms):
- CRC variants (CRC8-CCITT, CRC8-Dallas, CRC8-SAE)
- Fletcher checksums, lookup tables, bit manipulation

## Configuration

### Search Complexity Levels

```c
// Basic search (6 operations, fast)
config_t config = create_basic_search_config(3, 16);

// Default search (17 operations, balanced)  
config_t config = create_default_search_config();

// Thorough search (29 operations, comprehensive)
config_t config = create_thorough_search_config();

// Custom operation set (targeted)
operation_t ops[] = {OP_ADD, OP_XOR, OP_ONES_COMPLEMENT};
config_t config = create_custom_operation_config(ops, 3);
```

### Performance Tuning

```c
// Fast discovery (early exit on first solution)
enable_early_exit(&config, 1);

// Find all solutions (comprehensive analysis)
disable_early_exit(&config);

// Adjust progress update frequency
set_progress_interval(&config, 250);  // 250ms updates
```

## Performance

### Benchmark Results

| Test Configuration | Performance | Search Space |
|-------------------|-------------|--------------|
| Basic (3 fields, 16 constants) | 32.7M tests/sec | ~1.4M tests |
| Intermediate (4 fields, 64 constants) | 28.3M tests/sec | ~12M tests |
| Advanced (5 fields, 128 constants) | 25.1M tests/sec | ~85M tests |

### Optimization Features

- **Smart field generation** using bit masks for efficiency
- **Early packet validation** to skip impossible combinations  
- **Function pointer caching** for algorithm dispatch
- **Time-based progress updates** (no count-based overhead)
- **Exponential smoothing** for stable ETA predictions

## Testing

### Test Suite

```bash
# Run all tests
make test

# Run specific test categories
make test-unit           # Algorithm and data structure tests
make test-integration    # End-to-end discovery tests

# Performance profiling
./build/tests/test_performance_profile
```

### Test Coverage

- **40+ test cases** covering all major components
- **Algorithm validation** for all 29 operations
- **Packet data loading** with error handling
- **Search engine integration** with real datasets
- **Configuration management** with helper functions

## Real-World Applications

### Radio Protocol Analysis

CADS was originally developed to reverse engineer the Forj Link ESP32 radio firmware:

- **Discovered multi-byte checksums** in GMRS radio packets
- **Identified complex operation sequences** (ADD + XOR + ONES_COMPLEMENT)
- **Validated protocols** with 16+ packet samples
- **Performance optimized** for embedded analysis workflows

### Use Cases

- **Reverse engineering** proprietary radio protocols
- **Security analysis** of wireless communication systems  
- **Protocol validation** during firmware development
- **Academic research** into checksum algorithm design

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Write tests for your changes
4. Ensure all tests pass (`make test`)
5. Commit your changes (`git commit -am 'Add amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

### Development Guidelines

- **Follow C99 standard** for maximum compatibility
- **Add comprehensive tests** for new features
- **Update documentation** for API changes
- **Profile performance** for algorithm additions
- **Use consistent code style** matching existing patterns

## Technical Details

### Algorithm Implementation

Each algorithm is implemented as a pure function:

```c
uint64_t algorithm_add(uint64_t a, uint64_t b, uint64_t constant) {
    return (a + b + constant) & 0xFFFFFFFFFFFFFFFFULL;
}
```

### Search Strategy

1. **Generate field combinations** using bit mask enumeration
2. **Create operation permutations** recursively
3. **Test all constant values** (0-255 or configurable range)
4. **Validate against packet dataset** with early termination
5. **Report solutions** with complete algorithm description

### Memory Management

- **Static allocation** for performance-critical paths
- **Bounded data structures** to prevent memory leaks
- **Efficient packet storage** with minimal overhead
- **No dynamic allocation** in hot code paths

## Roadmap

### Phase 3: Threading Infrastructure ⏳
- **Multi-threaded search** for 4x+ performance improvement
- **Work queue distribution** across CPU cores
- **Thread-safe progress tracking** with atomic operations

### Future Enhancements
- **GPU acceleration** using CUDA/OpenCL
- **Machine learning** pattern recognition
- **Network protocol support** (TCP/UDP packet analysis)
- **Real-time packet capture** integration

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Unity Test Framework** for comprehensive testing infrastructure
- **TwilightCoders** for project architecture and optimization guidance
- **Open source community** for algorithm implementations and feedback

---

*CADS achieves 32+ million checksum tests per second, making it one of the fastest protocol analysis tools available for reverse engineering radio communication systems.*
