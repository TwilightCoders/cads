# CADS Test Suite

## Overview

The CADS test suite uses a minimal Unity-inspired testing framework designed specifically for our C codebase. Tests are organized into unit tests (individual components) and integration tests (complete workflows).

## Structure

```
tests/
├── unity.h                    # Minimal test framework header
├── unity.c                    # Test framework implementation
├── Makefile                   # Build and run tests
├── README.md                  # This file
├── unit/                      # Unit tests
│   ├── test_algorithm_operations.c
│   └── test_packet_data.c
├── integration/               # Integration tests
│   ├── test_forj_algorithm.c
│   └── test_search_engine.c
├── data/                      # Test data files
└── legacy/                    # Old test files (archived)
```

## Running Tests

### All Tests
```bash
cd tests/
make test
```

### Unit Tests Only
```bash
make test-unit
```

### Integration Tests Only
```bash
make test-integration
```

### Individual Tests
```bash
make test_algorithm_operations
./test_algorithm_operations
```

## Test Framework

Our minimal Unity framework provides:

### Assertions
- `TEST_ASSERT(condition)` - Basic boolean assertion
- `TEST_ASSERT_EQUAL(expected, actual)` - Integer equality
- `TEST_ASSERT_EQUAL_HEX8(expected, actual)` - 8-bit hex values
- `TEST_ASSERT_EQUAL_STRING(expected, actual)` - String comparison
- `TEST_ASSERT_NOT_NULL(ptr)` - Null pointer checks
- `TEST_ASSERT_NULL(ptr)` - Null pointer checks
- `TEST_FAIL(message)` - Explicit failure

### Test Structure
```c
#include "../unity.h"
#include "../../include/your_header.h"

void setUp(void) {
    // Run before each test
}

void tearDown(void) {
    // Run after each test
}

void test_your_function(void) {
    // Your test code
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(void) {
    TEST_SETUP();
    RUN_TEST(test_your_function);
    return TEST_SUMMARY();
}
```

## Test Categories

### Unit Tests
- **test_algorithm_operations.c** - Individual algorithm functions
- **test_packet_data.c** - Packet dataset creation and validation

### Integration Tests
- **test_forj_algorithm.c** - Complete Forj algorithm sequence
- **test_search_engine.c** - Search engine with custom operations

## Adding New Tests

1. **Create test file** in `unit/` or `integration/`
2. **Add to Makefile** in the appropriate `*_TESTS` variable
3. **Follow naming convention**: `test_*.c`
4. **Include proper headers** and use framework macros
5. **Run tests** to verify they work

## Best Practices

- **One concept per test file** - Keep tests focused
- **Descriptive test names** - `test_operation_with_edge_case()`
- **Setup/teardown** - Initialize clean state for each test
- **Test edge cases** - Null pointers, boundary values, error conditions
- **Integration over units** - Test complete workflows when possible
- **Fast execution** - Keep tests quick for frequent running

## Example Test

```c
void test_add_operation(void) {
    setUp();  // Optional explicit setup
    
    uint64_t result = execute_algorithm(OP_ADD, 0x30, 0x01, 0);
    TEST_ASSERT_EQUAL_HEX8(0x31, (uint8_t)result);
    
    tearDown();  // Optional explicit teardown
}
```

## Debugging Failed Tests

When tests fail, the framework provides:
- **Line numbers** where assertions failed
- **Expected vs actual values** in clear format
- **Colored output** for easy identification
- **Test summaries** showing pass/fail counts

Look for output like:
```
ASSERTION FAILED at line 42:
  Expected: result = 0x31
  Actual:   result = 0x30
```

This indicates the exact location and nature of the failure.