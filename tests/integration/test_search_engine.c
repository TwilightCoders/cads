/* Integration test for search engine with custom operations */

#include "../unity.h"
#include "../../include/checksum_engine.h"
#include "../../src/core/packet_data.h"
#include "../../src/core/progress_tracker.h"

void setUp(void) {
    // No setup needed - search engine initializes its own registry
}

void tearDown(void) {
    // No teardown needed
}

// Test custom operation selection
void test_custom_operation_selection(void) {
    packet_dataset_t* dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool load_success = load_packets_from_json(dataset, "data/gmrs_test_dataset.jsonl");
    TEST_ASSERT(load_success);
    
    // Define custom operations for Forj algorithm
    operation_t forj_operations[] = {
        OP_IDENTITY, OP_ADD, OP_ONES_COMPLEMENT, OP_CONST_ADD, OP_XOR
    };
    
    search_config_t config = {
        .complexity = COMPLEXITY_INTERMEDIATE,
        .max_fields = 5,
        .max_constants = 256,
        .checksum_size = 1,
        .verbose = false,
        .early_exit = true,
        .max_solutions = 1,
        .progress_interval = 10000,  // Shorter interval to see progress bar
        .custom_operations = forj_operations,
        .custom_operation_count = 5,
        .use_custom_operations = true
    };
    
    search_results_t* results = create_search_results(10);
    TEST_ASSERT_NOT_NULL(results);
    
    progress_tracker_t tracker;
    
    // Execute search - should find at least one solution quickly
    bool success = execute_checksum_search(dataset, &config, results, &tracker);
    TEST_ASSERT(success);
    TEST_ASSERT(results->solution_count > 0);
    TEST_ASSERT(results->tests_performed > 0);
    
    // Verify the solution is valid
    checksum_solution_t* solution = &results->solutions[0];
    TEST_ASSERT(solution->validated);
    TEST_ASSERT(solution->field_count > 0);
    TEST_ASSERT(solution->operation_count > 0);
    
    free_search_results(results);
    free_packet_dataset(dataset);
}

// Test search engine without custom operations (standard complexity)
void test_standard_complexity_search(void) {
    packet_dataset_t* dataset = create_packet_dataset(20);
    TEST_ASSERT_NOT_NULL(dataset);
    
    bool load_success = load_packets_from_json(dataset, "data/gmrs_test_dataset.jsonl");
    TEST_ASSERT(load_success);
    
    search_config_t config = {
        .complexity = COMPLEXITY_BASIC,  // Use basic operations only
        .max_fields = 3,                 // Limit search space
        .max_constants = 10,             // Small constant range
        .checksum_size = 1,
        .verbose = false,
        .early_exit = true,
        .max_solutions = 1,
        .progress_interval = 100000,
        .use_custom_operations = false   // Use standard complexity
    };
    
    search_results_t* results = create_search_results(10);
    TEST_ASSERT_NOT_NULL(results);
    
    progress_tracker_t tracker;
    
    // Execute search - should work with basic operations
    bool success = execute_checksum_search(dataset, &config, results, &tracker);
    TEST_ASSERT(success);
    
    // May or may not find solutions with basic ops, but should not crash
    TEST_ASSERT(results->tests_performed > 0);
    
    free_search_results(results);
    free_packet_dataset(dataset);
}

// Test search results validation
void test_search_results_validation(void) {
    search_results_t* results = create_search_results(5);
    TEST_ASSERT_NOT_NULL(results);
    TEST_ASSERT_EQUAL(0, results->solution_count);
    TEST_ASSERT_EQUAL(5, results->solution_capacity);
    TEST_ASSERT(!results->search_completed);
    TEST_ASSERT(!results->early_exit_triggered);
    
    // Test adding a solution
    checksum_solution_t solution = {0};
    solution.field_indices[0] = 2;
    solution.field_indices[1] = 5;
    solution.field_count = 2;
    solution.operations[0] = OP_XOR;
    solution.operation_count = 1;
    solution.constant = 0x42;
    solution.checksum_size = 1;
    solution.validated = true;
    
    bool added = add_solution(results, &solution);
    TEST_ASSERT(added);
    TEST_ASSERT_EQUAL(1, results->solution_count);
    
    // Verify solution was copied correctly
    checksum_solution_t* stored = &results->solutions[0];
    TEST_ASSERT_EQUAL(2, stored->field_indices[0]);
    TEST_ASSERT_EQUAL(5, stored->field_indices[1]);
    TEST_ASSERT_EQUAL(2, stored->field_count);
    TEST_ASSERT_EQUAL(OP_XOR, stored->operations[0]);
    TEST_ASSERT_EQUAL(0x42, stored->constant);
    TEST_ASSERT(stored->validated);
    
    free_search_results(results);
}

// Test early exit conditions
void test_early_exit_conditions(void) {
    search_config_t config = {
        .early_exit = true,
        .max_solutions = 1
    };
    
    search_results_t results = {0};
    results.solution_count = 0;
    
    // Should continue when no solutions found
    TEST_ASSERT(should_continue_search(&results, &config));
    
    // Should stop when solution found with early exit
    results.solution_count = 1;
    TEST_ASSERT(!should_continue_search(&results, &config));
    
    // Test max solutions limit
    config.early_exit = false;
    config.max_solutions = 3;
    results.solution_count = 2;
    TEST_ASSERT(should_continue_search(&results, &config));
    
    results.solution_count = 3;
    TEST_ASSERT(!should_continue_search(&results, &config));
}

int main(void) {
    TEST_SETUP();
    
    RUN_TEST(test_custom_operation_selection);
    RUN_TEST(test_standard_complexity_search);
    RUN_TEST(test_search_results_validation);
    RUN_TEST(test_early_exit_conditions);
    
    return TEST_SUMMARY();
}
