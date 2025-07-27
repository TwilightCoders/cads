/* Generic integration test for packet-based algorithm discovery */

#include "../unity.h"
#include "../../include/checksum_engine.h"
#include "../../src/core/packet_data.h"
#include "../../src/core/progress_tracker.h"

void setUp(void) {
    // No setup needed - search engine initializes its own registry
}

void tearDown(void) {
    // No teardown needed - search engine cleans up its own registry
}

// Test loading JSON packet data
void test_json_packet_loading(void) {
    // Create dataset and load MXT275 UART checksum data
    packet_dataset_t* dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool success = load_packets_from_json(dataset, "data/mxt275_uart_checksum.jsonl");
    TEST_ASSERT(success);
    
    // Should have loaded the expected number of packets
    TEST_ASSERT(dataset->count >= 10);
    TEST_ASSERT(dataset->count <= 20); // Reasonable bounds
    
    // Verify first packet structure
    if (dataset->count > 0) {
        const test_packet_t* packet = &dataset->packets[0];
        TEST_ASSERT(packet->packet_length > 0);
        TEST_ASSERT(packet->checksum_size > 0);
        TEST_ASSERT(strlen(packet->description) > 0);
        
        // MXT275 packets should be 7-8 bytes
        TEST_ASSERT(packet->packet_length >= 7);
        TEST_ASSERT(packet->packet_length <= 8);
    }
    
    free_packet_dataset(dataset);
}

// Test algorithm discovery with MXT275 UART data
void test_mxt275_algorithm_discovery(void) {
    // Create dataset and load test data
    packet_dataset_t* dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool success = load_packets_from_json(dataset, "data/mxt275_uart_checksum.jsonl");
    TEST_ASSERT(success);
    
    // Create focused search configuration (using working parameters)
    search_config_t config = {
        .complexity = COMPLEXITY_INTERMEDIATE,  // Need advanced operations for MXT275
        .max_fields = 5,        // Increased to match working test
        .max_constants = 256,   // Increased to match working test
        .checksum_size = 1,
        .verbose = false,       
        .early_exit = true,     // Stop at first solution
        .max_solutions = 1,     
        .progress_interval_ms = 250
    };
    
    // Create search results
    search_results_t* results = create_search_results(10);
    TEST_ASSERT_NOT_NULL(results);
    
    // Create progress tracker
    progress_tracker_t tracker = {0};
    
    // Execute search - should discover the MXT275 algorithm
    bool search_success = execute_checksum_search(dataset, &config, results, &tracker);
    TEST_ASSERT(search_success);
    
    // Verify we found at least one solution
    TEST_ASSERT(results->solution_count >= 1);
    
    if (results->solution_count > 0) {
        const checksum_solution_t* solution = &results->solutions[0];
        TEST_ASSERT(solution->validated);
        TEST_ASSERT_EQUAL(1, solution->checksum_size);
        
        // Solution should use multiple fields and operations for MXT275
        TEST_ASSERT(solution->field_count >= 3);
        TEST_ASSERT(solution->operation_count >= 2);
        
        printf("✅ Discovered MXT275 algorithm:\n");
        printf("   Fields: ");
        for (int i = 0; i < solution->field_count; i++) {
            printf("%d ", solution->field_indices[i]);
        }
        printf("\n   Operations: %d total\n", solution->operation_count);
        printf("   Constant: 0x%02llX\n", (unsigned long long)solution->constant);
    }
    
    // Clean up
    free_search_results(results);
    free_packet_dataset(dataset);
}

// Test discovery with custom operation set (faster, targeted search)
void test_targeted_mxt275_discovery(void) {
    // Create dataset and load test data
    packet_dataset_t* dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool success = load_packets_from_json(dataset, "data/mxt275_uart_checksum.jsonl");
    TEST_ASSERT(success);
    
    // Create custom operation set known to work with MXT275
    operation_t mxt275_operations[] = {
        OP_ADD,
        OP_XOR, 
        OP_ONES_COMPLEMENT,
        OP_CONST_ADD,
        OP_IDENTITY
    };
    
    search_config_t config = {
        .complexity = COMPLEXITY_INTERMEDIATE,
        .max_fields = 5,        // Increased to match working test
        .max_constants = 256,   // Increased to match working test
        .checksum_size = 1,
        .verbose = false,
        .early_exit = true,
        .max_solutions = 1,
        .progress_interval_ms = 250,
        .use_custom_operations = true,
        .custom_operations = mxt275_operations,
        .custom_operation_count = sizeof(mxt275_operations) / sizeof(mxt275_operations[0])
    };
    
    // Create search results
    search_results_t* results = create_search_results(10);
    TEST_ASSERT_NOT_NULL(results);
    
    // Create progress tracker
    progress_tracker_t tracker = {0};
    
    // Execute targeted search
    bool search_success = execute_checksum_search(dataset, &config, results, &tracker);
    TEST_ASSERT(search_success);
    
    // Should find solution faster with targeted operation set
    TEST_ASSERT(results->solution_count >= 1);
    
    if (results->solution_count > 0) {
        const checksum_solution_t* solution = &results->solutions[0];
        TEST_ASSERT(solution->validated);
        
        // Verify operations used are from our targeted set
        for (int i = 0; i < solution->operation_count; i++) {
            bool found_in_set = false;
            for (size_t j = 0; j < sizeof(mxt275_operations) / sizeof(mxt275_operations[0]); j++) {
                if (solution->operations[i] == mxt275_operations[j]) {
                    found_in_set = true;
                    break;
                }
            }
            TEST_ASSERT(found_in_set);
        }
    }
    
    // Clean up
    free_search_results(results);
    free_packet_dataset(dataset);
}

// Test dataset comparison: GMRS vs JSON loaded data
void test_dataset_comparison(void) {
    printf("🔍 Testing dataset comparison: GMRS vs JSON...\n");
    
    // Create GMRS dataset from file
    packet_dataset_t* gmrs_dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(gmrs_dataset);
    
    bool gmrs_success = load_packets_from_json(gmrs_dataset, "data/gmrs_test_dataset.jsonl");
    TEST_ASSERT(gmrs_success);
    
    // Create JSON dataset
    packet_dataset_t* json_dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(json_dataset);
    
    bool success = load_packets_from_json(json_dataset, "data/mxt275_uart_checksum.jsonl");
    TEST_ASSERT(success);
    
    printf("📊 GMRS dataset: %zu packets\n", gmrs_dataset->count);
    printf("📊 JSON dataset: %zu packets\n", json_dataset->count);
    
    // Compare first packet from each
    if (gmrs_dataset->count > 0 && json_dataset->count > 0) {
        const test_packet_t* gmrs_packet = &gmrs_dataset->packets[0];
        const test_packet_t* json_packet = &json_dataset->packets[0];
        
        printf("📦 GMRS packet 0: length=%zu, checksum=0x%02llX\n", 
               gmrs_packet->packet_length, (unsigned long long)gmrs_packet->expected_checksum);
        printf("📦 JSON packet 0: length=%zu, checksum=0x%02llX\n", 
               json_packet->packet_length, (unsigned long long)json_packet->expected_checksum);
               
        // Print raw data for comparison
        printf("🔍 GMRS data: ");
        for (size_t i = 0; i < gmrs_packet->packet_length; i++) {
            printf("%02x", gmrs_packet->packet_data[i]);
        }
        printf("\n🔍 JSON data: ");
        for (size_t i = 0; i < json_packet->packet_length; i++) {
            printf("%02x", json_packet->packet_data[i]);
        }
        printf("\n");
    }
    
    // Test search with GMRS dataset first (should succeed)
    search_config_t config = {
        .complexity = COMPLEXITY_INTERMEDIATE,
        .max_fields = 5,        // Increased from 4 to match working test
        .max_constants = 256,   // Increased from 16 to match working test  
        .checksum_size = 1,
        .verbose = false,
        .early_exit = true,
        .max_solutions = 1,
        .progress_interval_ms = 250,
        .use_custom_operations = true,
        .custom_operations = (operation_t[]){OP_IDENTITY, OP_ADD, OP_ONES_COMPLEMENT, OP_CONST_ADD, OP_XOR},
        .custom_operation_count = 5
    };
    
    search_results_t* gmrs_results = create_search_results(10);
    progress_tracker_t tracker1 = {0};
    
    printf("🎯 Testing search with GMRS dataset...\n");
    gmrs_success = execute_checksum_search(gmrs_dataset, &config, gmrs_results, &tracker1);
    printf("📈 GMRS search result: %s, solutions found: %zu\n", 
           gmrs_success ? "SUCCESS" : "FAILED", gmrs_results->solution_count);
    
    // Test search with JSON dataset  
    search_results_t* json_results = create_search_results(10);
    progress_tracker_t tracker2 = {0};
    
    printf("🎯 Testing search with JSON dataset...\n");
    bool json_success = execute_checksum_search(json_dataset, &config, json_results, &tracker2);
    printf("📈 JSON search result: %s, solutions found: %zu\n", 
           json_success ? "SUCCESS" : "FAILED", json_results->solution_count);
    
    // Clean up
    free_search_results(gmrs_results);
    free_search_results(json_results);
    free_packet_dataset(gmrs_dataset);
    free_packet_dataset(json_dataset);
}

// Test with malformed JSON (error handling)
void test_json_error_handling(void) {
    // Test with non-existent file
    packet_dataset_t* dataset = create_packet_dataset(10);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool success = load_packets_from_json(dataset, "data/nonexistent.jsonl");
    TEST_ASSERT(!success);  // Should fail
    
    free_packet_dataset(dataset);
    
    // Test with NULL path
    dataset = create_packet_dataset(10);
    TEST_ASSERT_NOT_NULL(dataset);
    
    success = load_packets_from_json(dataset, NULL);
    TEST_ASSERT(!success);  // Should fail
    
    free_packet_dataset(dataset);
}

int main(void) {
    TEST_SETUP();
    
    RUN_TEST(test_json_packet_loading);
    RUN_TEST(test_dataset_comparison);
    RUN_TEST(test_mxt275_algorithm_discovery);
    RUN_TEST(test_targeted_mxt275_discovery);
    RUN_TEST(test_json_error_handling);
    
    return TEST_SUMMARY();
}
