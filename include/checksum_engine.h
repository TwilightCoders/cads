#ifndef CHECKSUM_ENGINE_H
#define CHECKSUM_ENGINE_H

#include "cads_types.h"
#include "cads_config_loader.h"
#include "algorithm_registry.h"
#include "../src/core/packet_data.h"
#include "../src/core/progress_tracker.h"
#include "../src/utils/hardware_benchmark.h"

// Search results management
search_results_t* create_search_results(size_t initial_capacity);
void free_search_results(search_results_t* results);
bool add_solution(search_results_t* results, const checksum_solution_t* solution);

// Early exit logic
bool should_continue_search(const search_results_t* results, const config_t* config);

// Core checksum field and mask operations
uint64_t extract_packet_field_value(const uint8_t* packet_data, size_t packet_length,
                                   uint8_t field_index, size_t checksum_size);
uint64_t mask_checksum_to_size(uint64_t checksum, size_t checksum_size);

// Algorithm testing
bool test_algorithm_combination(const uint8_t* field_indices, uint8_t field_count,
                               const operation_t* operations, uint8_t operation_count,
                               uint64_t constant, size_t checksum_size,
                               const packet_dataset_t* dataset,
                               checksum_solution_t* solution);

// Solution validation
bool validate_solution(const checksum_solution_t* solution, const packet_dataset_t* dataset);

// Search space estimation
uint64_t estimate_search_space(const packet_dataset_t* dataset, const config_t* config);

// Main search functions
bool execute_checksum_search(const config_t* config,
                            search_results_t* results,
                            progress_tracker_t* tracker);

// Weighted threaded search with operation partitioning (handles both single and multi-threaded)
bool execute_weighted_checksum_search(const config_t* config, 
                                     search_results_t* results,
                                     const hardware_benchmark_result_t* benchmark);

// Recursive operation testing
bool test_operation_sequence(const packet_dataset_t* dataset, 
                            const config_t* config,
                            const uint8_t* field_permutation,
                            int field_count,
                            const algorithm_registry_entry_t* algorithms,
                            int algorithm_count,
                            operation_t* operation_sequence,
                            int current_depth,
                            int max_depth,
                            uint8_t constant,
                            search_results_t* results,
                            uint64_t* tests_performed,
                            progress_tracker_t* tracker);

#endif // CHECKSUM_ENGINE_H
