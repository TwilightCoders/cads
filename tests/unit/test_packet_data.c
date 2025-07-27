/* Unit tests for packet data handling */

#include "../unity.h"
#include "../../src/core/packet_data.h"

void setUp(void) {
    // No setup needed for packet data tests
}

void tearDown(void) {
    // No teardown needed
}

// Test GMRS dataset creation
void test_create_gmrs_dataset(void) {
    packet_dataset_t* dataset = create_default_gmrs_dataset();
    
    TEST_ASSERT_NOT_NULL(dataset);
    TEST_ASSERT(dataset->count > 0);
    TEST_ASSERT_NOT_NULL(dataset->packets);
    
    // Test first packet structure
    TEST_ASSERT_NOT_NULL(dataset->packets[0].description);
    TEST_ASSERT(dataset->packets[0].packet_length > 0);
    TEST_ASSERT(dataset->packets[0].packet_length <= CADS_MAX_PACKET_SIZE);
    TEST_ASSERT_NOT_NULL(dataset->packets[0].packet_data);
    
    free_packet_dataset(dataset);
}

// Test specific GMRS packet data
void test_gmrs_packet_content(void) {
    packet_dataset_t* dataset = create_default_gmrs_dataset();
    TEST_ASSERT_NOT_NULL(dataset);
    
    // Test CH1 packet: [9c 30 01 00 00 00 00 00] -> 0x31
    const test_packet_t* ch1 = &dataset->packets[0];
    TEST_ASSERT_EQUAL_STRING("CH1", ch1->description);
    TEST_ASSERT_EQUAL(8, ch1->packet_length);
    TEST_ASSERT_EQUAL_HEX8(0x9C, ch1->packet_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x30, ch1->packet_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01, ch1->packet_data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x31, (uint8_t)ch1->expected_checksum);
    
    free_packet_dataset(dataset);
}

// Test packet validation
void test_packet_validation(void) {
    packet_dataset_t* dataset = create_default_gmrs_dataset();
    TEST_ASSERT_NOT_NULL(dataset);
    
    // All packets should have consistent checksum size
    for (size_t i = 0; i < dataset->count; i++) {
        const test_packet_t* packet = &dataset->packets[i];
        TEST_ASSERT_EQUAL(1, packet->checksum_size);  // GMRS uses 1-byte checksums
        TEST_ASSERT(packet->packet_length >= 6);      // Minimum for Forj algorithm
    }
    
    free_packet_dataset(dataset);
}

// Test dataset cleanup
void test_dataset_cleanup(void) {
    packet_dataset_t* dataset = create_default_gmrs_dataset();
    TEST_ASSERT_NOT_NULL(dataset);
    
    // Should not crash
    free_packet_dataset(dataset);
    
    // Test cleanup of NULL pointer (should not crash)
    free_packet_dataset(NULL);
}

// Test dataset size
void test_dataset_size(void) {
    packet_dataset_t* dataset = create_default_gmrs_dataset();
    TEST_ASSERT_NOT_NULL(dataset);
    
    // Should have multiple test packets for proper validation
    TEST_ASSERT(dataset->count >= 3);  // At least CH1, CH3, CH4
    TEST_ASSERT(dataset->count <= 100); // Reasonable upper bound
    
    free_packet_dataset(dataset);
}

int main(void) {
    TEST_SETUP();
    
    RUN_TEST(test_create_gmrs_dataset);
    RUN_TEST(test_gmrs_packet_content);
    RUN_TEST(test_packet_validation);
    RUN_TEST(test_dataset_cleanup);
    RUN_TEST(test_dataset_size);
    
    return TEST_SUMMARY();
}