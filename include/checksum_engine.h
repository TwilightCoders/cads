#ifndef CHECKSUM_ENGINE_H
#define CHECKSUM_ENGINE_H

#include "cads_types.h"
#include "cads_config_loader.h"
#include "../src/core/packet_data.h"
#include "../src/core/progress_tracker.h"
#include "../src/utils/hardware_benchmark.h"

// Search results management
search_results_t* create_search_results(size_t initial_capacity);
void free_search_results(search_results_t* results);
bool add_solution(search_results_t* results, const checksum_solution_t* solution);

// Early exit logic
bool should_continue_search(const search_results_t* results, const cads_config_file_t* config);

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
uint64_t estimate_search_space(const packet_dataset_t* dataset, const cads_config_file_t* config);

// Main search functions
bool execute_checksum_search(const cads_config_file_t* config,
                            search_results_t* results,
                            progress_tracker_t* tracker);

// Threaded search function using unified CADS config
bool execute_checksum_search_threaded(const cads_config_file_t* config, 
                                     search_results_t* results,
                                     const hardware_benchmark_result_t* benchmark);

#endif // CHECKSUM_ENGINE_H
