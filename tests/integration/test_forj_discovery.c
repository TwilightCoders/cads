/* Integration test for Forj algorithm discovery capability */

#include "../unity.h"
#include "../../include/checksum_engine.h"
#include "../../src/core/packet_data.h"
#include "../../src/core/progress_tracker.h"
#include "../../src/utils/config.h"

void setUp(void) {
    // No setup needed - search engine initializes its own registry
}

void tearDown(void) {
    // No teardown needed - search engine cleans up its own registry
}

// Test that the search engine can discover the Forj algorithm
void test_forj_algorithm_discovery(void) {
    // Create test dataset
    packet_dataset_t* dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool load_success = load_packets_from_json(dataset, "data/gmrs_test_dataset.jsonl");
    TEST_ASSERT(load_success);
    
    // Create focused search configuration for Forj algorithm discovery
    config_t config = create_default_search_config();  // Uses INTERMEDIATE complexity
    config.max_fields = 4;        // Reduced for faster testing (still finds Forj algorithm)
    config.max_constants = 32;    // Reduced search space - focuses on smaller constants
    config.dataset = dataset;
    enable_early_exit(&config, 1);
    set_progress_interval(&config, 250);
    
    // Create search results
    search_results_t* results = create_search_results(10);
    TEST_ASSERT_NOT_NULL(results);
    
    // Create progress tracker
    progress_tracker_t tracker = {0};
    
    // Execute search - this should discover the Forj algorithm
    bool search_success = execute_checksum_search(&config, results, &tracker);
    TEST_ASSERT(search_success);
    
    // Verify we found at least one solution
    TEST_ASSERT(results->solution_count >= 1);
    
    // Verify the first solution works for all packets
    if (results->solution_count > 0) {
        const checksum_solution_t* solution = &results->solutions[0];
        TEST_ASSERT(solution->validated);
        TEST_ASSERT_EQUAL(1, solution->checksum_size);
        
        // Solution should use multiple fields and operations
        TEST_ASSERT(solution->field_count >= 3);
        TEST_ASSERT(solution->operation_count >= 3);
    }
    
    // Clean up
    free_search_results(results);
    free_packet_dataset(dataset);
}

// Test discovery with custom operation set (faster)
void test_forj_discovery_with_custom_operations(void) {
    // Create test dataset
    packet_dataset_t* dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool load_success = load_packets_from_json(dataset, "data/gmrs_test_dataset.jsonl");
    TEST_ASSERT(load_success);
    
    // Create custom operation set focused on Forj algorithm
    operation_t forj_operations[] = {
        OP_ADD,
        OP_XOR, 
        OP_ONES_COMPLEMENT,
        OP_CONST_ADD,
        OP_IDENTITY
    };
    
    config_t config = create_custom_operation_config(forj_operations, 
                                                           sizeof(forj_operations) / sizeof(forj_operations[0]));
    config.max_fields = 4;        // Reduced for faster testing
    config.max_constants = 16;    // Much smaller search space with custom operations
    config.dataset = dataset;
    
    // Create search results
    search_results_t* results = create_search_results(10);
    TEST_ASSERT_NOT_NULL(results);
    
    // Create progress tracker
    progress_tracker_t tracker = {0};
    
    // Execute search with custom operations
    bool search_success = execute_checksum_search(&config, results, &tracker);
    TEST_ASSERT(search_success);
    
    // Should find solution faster with focused operation set
    TEST_ASSERT(results->solution_count >= 1);
    
    if (results->solution_count > 0) {
        const checksum_solution_t* solution = &results->solutions[0];
        TEST_ASSERT(solution->validated);
        
        // Verify operations used are from our custom set
        for (int i = 0; i < solution->operation_count; i++) {
            bool found_in_custom_set = false;
            for (size_t j = 0; j < sizeof(forj_operations) / sizeof(forj_operations[0]); j++) {
                if (solution->operations[i] == forj_operations[j]) {
                    found_in_custom_set = true;
                    break;
                }
            }
            TEST_ASSERT(found_in_custom_set);
        }
    }
    
    // Clean up
    free_search_results(results);
    free_packet_dataset(dataset);
}

int main(void) {
    TEST_SETUP();
    
    RUN_TEST(test_forj_algorithm_discovery);
    RUN_TEST(test_forj_discovery_with_custom_operations);
    
    return TEST_SUMMARY();
}
