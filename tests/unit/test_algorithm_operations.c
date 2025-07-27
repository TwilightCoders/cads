/* Unit tests for individual algorithm operations */

#include "../unity.h"
#include "../../include/algorithm_registry.h"

// Test setup - run before each test
void setUp(void) {
    // Initialize algorithm registry for each test
    initialize_algorithm_registry();
}

// Test teardown - run after each test  
void tearDown(void) {
    cleanup_algorithm_registry();
}

// Test basic ADD operation
void test_add_operation(void) {
    setUp();
    uint64_t result = execute_algorithm(OP_ADD, 0x30, 0x01, 0);
    TEST_ASSERT_EQUAL_HEX8(0x31, (uint8_t)result);
    tearDown();
}

// Test basic XOR operation
void test_xor_operation(void) {
    setUp();
    uint64_t result = execute_algorithm(OP_XOR, 0x30, 0x01, 0);
    TEST_ASSERT_EQUAL_HEX8(0x31, (uint8_t)result);
    tearDown();
}

// Test IDENTITY operation
void test_identity_operation(void) {
    setUp();
    uint64_t result = execute_algorithm(OP_IDENTITY, 0x30, 0x01, 0);
    TEST_ASSERT_EQUAL_HEX8(0x30, (uint8_t)result);
    tearDown();
}

// Test ONES_COMPLEMENT operation
void test_ones_complement_operation(void) {
    setUp();
    uint64_t result = execute_algorithm(OP_ONES_COMPLEMENT, 0x30, 0x01, 0);
    TEST_ASSERT_EQUAL_HEX8(0xCE, (uint8_t)result);
    tearDown();
}

// Test CONST_ADD operation
void test_const_add_operation(void) {
    setUp();
    uint64_t result = execute_algorithm(OP_CONST_ADD, 0xFF, 0x00, 0xD0);
    TEST_ASSERT_EQUAL_HEX8(0xCF, (uint8_t)result);
    tearDown();
}

// Test algorithm registry initialization
void test_algorithm_registry_initialization(void) {
    setUp();
    const algorithm_registry_entry_t* entry = get_algorithm_by_operation(OP_ADD);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NOT_NULL(entry->func);
    tearDown();
}

// Test invalid operation
void test_invalid_operation(void) {
    setUp();
    uint64_t result = execute_algorithm((operation_t)999, 0x30, 0x01, 0);
    TEST_ASSERT_EQUAL_HEX8(0x00, (uint8_t)result);  // Should return 0 for invalid ops
    tearDown();
}

// Test operation metadata
void test_operation_metadata(void) {
    setUp();
    const algorithm_registry_entry_t* entry = get_algorithm_by_operation(OP_CONST_ADD);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_STRING("C+", entry->name);
    TEST_ASSERT(entry->requires_constant);
    
    entry = get_algorithm_by_operation(OP_ADD);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_STRING("ADD", entry->name);
    TEST_ASSERT(!entry->requires_constant);
    tearDown();
}

// Main test runner
int main(void) {
    TEST_SETUP();
    
    RUN_TEST(test_add_operation);
    RUN_TEST(test_xor_operation);
    RUN_TEST(test_identity_operation);
    RUN_TEST(test_ones_complement_operation);
    RUN_TEST(test_const_add_operation);
    RUN_TEST(test_algorithm_registry_initialization);
    RUN_TEST(test_invalid_operation);
    RUN_TEST(test_operation_metadata);
    
    return TEST_SUMMARY();
}