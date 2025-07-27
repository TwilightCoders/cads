/* Integration test for complete Forj algorithm */

#include "../unity.h"
#include "../../include/algorithm_registry.h"
#include "../../src/core/packet_data.h"

void setUp(void) {
    initialize_algorithm_registry();
}

void tearDown(void) {
    cleanup_algorithm_registry();
}

// Test the complete Forj algorithm sequence
void test_forj_algorithm_sequence(void) {
    setUp();  // Initialize algorithm registry
    
    // Test packet: CH1 = [9c 30 01 00 00 00 00 00] -> 0x31
    uint8_t packet[] = {0x9c, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t expected = 0x31;
    
    // Step 1: Load field[5] = 0x00
    uint64_t result = packet[5];
    TEST_ASSERT_EQUAL_HEX8(0x00, (uint8_t)result);
    
    // Step 2: ADD field[4], then ONES_COMPLEMENT
    uint64_t temp = execute_algorithm(OP_ADD, result, packet[4], 0);
    TEST_ASSERT_EQUAL_HEX8(0x00, (uint8_t)temp);  // 0x00 + 0x00 = 0x00
    
    result = execute_algorithm(OP_ONES_COMPLEMENT, temp, 0, 0);
    TEST_ASSERT_EQUAL_HEX8(0xFF, (uint8_t)result);  // ~0x00 = 0xFF
    
    // Step 3: CONST_ADD with field[3] and constant 0xD0  
    result = execute_algorithm(OP_CONST_ADD, result, packet[3], 0xD0);
    TEST_ASSERT_EQUAL_HEX8(0xCF, (uint8_t)result);  // 0xFF + 0x00 + 0xD0 = 0xCF
    
    // Step 4: XOR with field[2]
    result = execute_algorithm(OP_XOR, result, packet[2], 0);
    TEST_ASSERT_EQUAL_HEX8(0xCE, (uint8_t)result);  // 0xCF ^ 0x01 = 0xCE
    
    // Step 5: Final ONES_COMPLEMENT
    result = execute_algorithm(OP_ONES_COMPLEMENT, result, 0, 0);
    TEST_ASSERT_EQUAL_HEX8(expected, (uint8_t)result);  // ~0xCE = 0x31
    
    tearDown();  // Clean up
}

// Test Forj algorithm against all GMRS packets
void test_forj_algorithm_all_packets(void) {
    setUp();  // Initialize algorithm registry
    
    packet_dataset_t* dataset = create_default_gmrs_dataset();
    TEST_ASSERT_NOT_NULL(dataset);
    
    for (size_t i = 0; i < dataset->count; i++) {
        const test_packet_t* packet = &dataset->packets[i];
        
        // Skip packets that are too short for Forj algorithm
        if (packet->packet_length < 6) continue;
        
        // Apply Forj algorithm
        uint64_t result = packet->packet_data[5];  // Load field[5]
        
        // ADD field[4], then ONES_COMPLEMENT
        uint64_t temp = execute_algorithm(OP_ADD, result, packet->packet_data[4], 0);
        result = execute_algorithm(OP_ONES_COMPLEMENT, temp, 0, 0);
        
        // CONST_ADD with field[3] and constant 0xD0
        result = execute_algorithm(OP_CONST_ADD, result, packet->packet_data[3], 0xD0);
        
        // XOR with field[2]
        result = execute_algorithm(OP_XOR, result, packet->packet_data[2], 0);
        
        // Final ONES_COMPLEMENT
        result = execute_algorithm(OP_ONES_COMPLEMENT, result, 0, 0);
        
        // Verify against expected checksum
        uint8_t calculated = (uint8_t)(result & 0xFF);
        uint8_t expected = (uint8_t)(packet->expected_checksum & 0xFF);
        
        if (calculated != expected) {
            printf("\nPacket %zu (%s) failed:\n", i, packet->description);
            printf("  Expected: 0x%02X, Got: 0x%02X\n", expected, calculated);
        }
        
        TEST_ASSERT_EQUAL_HEX8(expected, calculated);
    }
    
    free_packet_dataset(dataset);
    tearDown();  // Clean up
}

// Test Forj algorithm with edge cases
void test_forj_algorithm_edge_cases(void) {
    setUp();  // Initialize algorithm registry
    
    // Test with all zeros
    uint8_t zeros[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    uint64_t result = zeros[5];  // 0x00
    uint64_t temp = execute_algorithm(OP_ADD, result, zeros[4], 0);  // 0x00
    result = execute_algorithm(OP_ONES_COMPLEMENT, temp, 0, 0);      // 0xFF
    result = execute_algorithm(OP_CONST_ADD, result, zeros[3], 0xD0); // 0xCF
    result = execute_algorithm(OP_XOR, result, zeros[2], 0);         // 0xCF
    result = execute_algorithm(OP_ONES_COMPLEMENT, result, 0, 0);    // 0x30
    
    TEST_ASSERT_EQUAL_HEX8(0x30, (uint8_t)result);
    
    // Test with all 0xFF
    uint8_t ones[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    result = ones[5];  // 0xFF
    temp = execute_algorithm(OP_ADD, result, ones[4], 0);        // 0xFE (overflow)
    result = execute_algorithm(OP_ONES_COMPLEMENT, temp, 0, 0);  // 0x01
    result = execute_algorithm(OP_CONST_ADD, result, ones[3], 0xD0); // 0xD0
    result = execute_algorithm(OP_XOR, result, ones[2], 0);      // 0x2F
    result = execute_algorithm(OP_ONES_COMPLEMENT, result, 0, 0); // 0xD0
    
    TEST_ASSERT_EQUAL_HEX8(0xD0, (uint8_t)result);
    
    tearDown();  // Clean up
}

int main(void) {
    TEST_SETUP();
    
    RUN_TEST(test_forj_algorithm_sequence);
    RUN_TEST(test_forj_algorithm_all_packets);
    RUN_TEST(test_forj_algorithm_edge_cases);
    
    return TEST_SUMMARY();
}