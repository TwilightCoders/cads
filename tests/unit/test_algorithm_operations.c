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

// Advanced operations wiring test
void test_advanced_operations_wired(void) {
    setUp();
    // Just ensure they execute and produce deterministic transformations for known inputs
    uint8_t swapn = (uint8_t)execute_algorithm(OP_SWAP_NIBBLES, 0xAB, 0, 0);
    TEST_ASSERT_EQUAL_HEX8(0xBA, swapn);
    uint8_t revb = (uint8_t)execute_algorithm(OP_REVERSE_BITS, 0x96, 0, 0); // 0x96 -> 0x69
    TEST_ASSERT_EQUAL_HEX8(0x69, revb);
    // CRC variants: ensure non-zero and differ between types
    uint8_t crc_ccitt = (uint8_t)execute_algorithm(OP_CRC8_CCITT, 0xAA, 0x55, 0);
    uint8_t crc_dallas = (uint8_t)execute_algorithm(OP_CRC8_DALLAS, 0xAA, 0x55, 0);
    TEST_ASSERT(crc_ccitt != 0x00);
    TEST_ASSERT(crc_ccitt != crc_dallas);
    uint8_t rotl = (uint8_t)execute_algorithm(OP_ROTLEFT, 0x81, 0x01, 0); // 1000 0001 rotl1 (8-bit) -> 0000 0011 (0x03)
    TEST_ASSERT_EQUAL_HEX8(0x03, rotl);
    uint8_t rotr = (uint8_t)execute_algorithm(OP_ROTRIGHT, 0x81, 0x01, 0); // rotr1 -> 0xC0
    TEST_ASSERT_EQUAL_HEX8(0xC0, rotr);
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
    RUN_TEST(test_advanced_operations_wired);

    // Advanced operation smoke tests (ensure function pointers wired)
    setUp();
    uint64_t rotl = execute_algorithm(OP_ROTLEFT, 0x12, 0x01, 0);
    (void)rotl; // Accept any non-crash result
    uint64_t rotr = execute_algorithm(OP_ROTRIGHT, 0x12, 0x01, 0);
    (void)rotr;
    uint64_t crc = execute_algorithm(OP_CRC8_CCITT, 0xAA, 0x55, 0);
    (void)crc;
    uint64_t fletch = execute_algorithm(OP_FLETCHER8, 0x11, 0x22, 0);
    (void)fletch;
    uint64_t swapn = execute_algorithm(OP_SWAP_NIBBLES, 0xAB, 0, 0);
    TEST_ASSERT_EQUAL_HEX8(0xBA, (uint8_t)swapn);
    uint64_t revb = execute_algorithm(OP_REVERSE_BITS, 0x96, 0, 0); // 0x96 = 1001 0110 -> 0110 1001 = 0x69
    TEST_ASSERT_EQUAL_HEX8(0x69, (uint8_t)revb);
    tearDown();
    
    return TEST_SUMMARY();
}
