#ifndef CHECKSUM_ENGINE_H
#define CHECKSUM_ENGINE_H

#include "../../include/cads_types.h"
#include "packet_data.h"
#include "progress_tracker.h"

// Search result structure
typedef struct {
    checksum_solution_t* solutions;    // Array of found solutions
    size_t solution_count;             // Number of solutions found
    size_t solution_capacity;          // Capacity of solutions array
    uint64_t tests_performed;          // Total tests performed
    bool search_completed;             // Whether search completed fully
    bool early_exit_triggered;        // Whether early exit was triggered
} search_results_t;

// Search engine functions
search_results_t* create_search_results(size_t initial_capacity);
void free_search_results(search_results_t* results);

// Main search function
bool execute_checksum_search(const packet_dataset_t* dataset, 
                            const search_config_t* config,
                            search_results_t* results,
                            progress_tracker_t* tracker);

// Solution management
bool add_solution(search_results_t* results, const checksum_solution_t* solution);
bool validate_solution(const checksum_solution_t* solution, const packet_dataset_t* dataset);

// Search strategy functions
bool should_continue_search(const search_results_t* results, const search_config_t* config);
uint64_t estimate_search_space(const packet_dataset_t* dataset, const search_config_t* config);

// Algorithm testing
bool test_algorithm_combination(const uint8_t* field_indices, uint8_t field_count,
                               const operation_t* operations, uint8_t operation_count,
                               uint64_t constant, size_t checksum_size,
                               const packet_dataset_t* dataset,
                               checksum_solution_t* solution);

// Multi-byte checksum handling
uint64_t extract_packet_field_value(const uint8_t* packet_data, size_t packet_length,
                                   uint8_t field_index, size_t checksum_size);
uint64_t mask_checksum_to_size(uint64_t checksum, size_t checksum_size);

// Progress callbacks
typedef void (*progress_callback_t)(const progress_tracker_t* tracker, void* user_data);
void set_progress_callback(progress_callback_t callback, void* user_data);

#endif // CHECKSUM_ENGINE_H