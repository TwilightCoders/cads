#include "checksum_engine.h"
#include "../../include/algorithm_registry.h"
#include "../utils/field_combiner.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Recursive operation testing function
bool test_operation_sequence(const packet_dataset_t* dataset, 
                            const cads_config_file_t* config,
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
                            progress_tracker_t* tracker) {
    
    // Base case: we've built a complete operation sequence, test it
    if (current_depth >= max_depth) {
        (*tests_performed)++;
        
        // Update progress bar periodically (time-based)
        if (tracker && should_display_progress(tracker)) {
            update_progress(tracker, *tests_performed, results->solution_count);
            display_detailed_progress(tracker, "Testing");
        }
        
        // Test this operation sequence against all packets
        bool all_match = true;
        for (size_t packet_idx = 0; packet_idx < dataset->count; packet_idx++) {
            const test_packet_t* packet = &dataset->packets[packet_idx];
            
            // Skip packets with different checksum sizes
            if (packet->checksum_size != config->checksum_size) {
                all_match = false;
                break;
            }
            
            // Check if packet has enough fields
            for (int f = 0; f < field_count; f++) {
                if (field_permutation[f] >= packet->packet_length) {
                    all_match = false;
                    break;
                }
            }
            if (!all_match) break;
            
            // Apply operation sequence
            uint64_t calculated = extract_packet_field_value(packet->packet_data, 
                                                           packet->packet_length,
                                                           field_permutation[0], 
                                                           config->checksum_size);
            
            // Apply each operation in sequence
            for (int op_idx = 0; op_idx < max_depth && (op_idx + 1) < field_count; op_idx++) {
                uint64_t next_val = extract_packet_field_value(packet->packet_data,
                                                             packet->packet_length,
                                                             field_permutation[op_idx + 1],
                                                             config->checksum_size);
                calculated = execute_algorithm(operation_sequence[op_idx], calculated, next_val, constant);
            }
            
            // Mask and compare
            calculated = mask_checksum_to_size(calculated, config->checksum_size);
            uint64_t expected = mask_checksum_to_size(packet->expected_checksum, config->checksum_size);
            
            if (calculated != expected) {
                all_match = false;
                break;
            }
        }
        
        if (all_match) {
            // Found a solution!
            checksum_solution_t solution = {0};
            for (int f = 0; f < field_count; f++) {
                solution.field_indices[f] = field_permutation[f];
            }
            solution.field_count = field_count;
            for (int op = 0; op < max_depth && op < 4; op++) {
                solution.operations[op] = operation_sequence[op];
            }
            solution.operation_count = max_depth;
            solution.constant = constant;
            solution.checksum_size = config->checksum_size;
            solution.validated = true;
            
            if (add_solution(results, &solution)) {
                // Move cursor down from progress area and print solution (preserving progress display)
                printf("\n🎉 SOLUTION #%zu FOUND!\n", results->solution_count);
                printf("   Fields: ");
                for (int f = 0; f < field_count; f++) {
                    printf("%d ", field_permutation[f]);
                }
                printf("\n   Operations: ");
                for (int op = 0; op < max_depth; op++) {
                    // Find algorithm name
                    for (int a = 0; a < algorithm_count; a++) {
                        if (algorithms[a].op == operation_sequence[op]) {
                            printf("%s ", algorithms[a].name);
                            break;
                        }
                    }
                }
                printf("\n   Constant: 0x%02X\n\n", constant);
            }
            return true;  // Found a solution
        }
        return false;  // No solution with this sequence
    }
    
    // Recursive case: try each algorithm at this depth
    for (int alg_idx = 0; alg_idx < algorithm_count; alg_idx++) {
        operation_sequence[current_depth] = algorithms[alg_idx].op;
        
        if (test_operation_sequence(dataset, config, field_permutation, field_count,
                                  algorithms, algorithm_count, operation_sequence,
                                  current_depth + 1, max_depth, constant, results, tests_performed, tracker)) {
            // Early exit if solution found and early exit enabled
            if (config->early_exit) {
                return true;
            }
        }
    }
    
    return false;
}

// Main exhaustive recursive search function
bool execute_checksum_search(const cads_config_file_t* config,
                            search_results_t* results,
                            progress_tracker_t* tracker) {
    if (!config || !config->dataset || !results) return false;
    
    const packet_dataset_t* dataset = config->dataset;
    
    // Initialize algorithm registry
    if (!initialize_algorithm_registry()) {
        return false;
    }
    
    // Build operations array - either custom or from complexity level
    int algorithm_count;
    algorithm_registry_entry_t* algorithms = NULL;
    
    if (config->custom_operation_count > 0 && config->custom_operations) {
        // Use custom operations
        algorithm_count = config->custom_operation_count;
        printf("🎯 Using CUSTOM operation set (%d operations)\n", algorithm_count);
    } else {
        // Use pre-canned complexity level
        const algorithm_registry_entry_t* complexity_algorithms = get_algorithms_by_complexity(config->complexity, &algorithm_count);
        if (!complexity_algorithms || algorithm_count == 0) {
            cleanup_algorithm_registry();
            return false;
        }
        printf("📊 Using %s complexity level (%d operations)\n", 
                get_complexity_name(config->complexity), algorithm_count);
    }
    
    // Allocate and populate operations array
    algorithms = malloc(algorithm_count * sizeof(algorithm_registry_entry_t));
    if (!algorithms) {
        cleanup_algorithm_registry();
        return false;
    }
    
    if (config->custom_operation_count > 0 && config->custom_operations) {
        // Populate from custom operations
        for (int i = 0; i < algorithm_count; i++) {
            const algorithm_registry_entry_t* entry = get_algorithm_by_operation(config->custom_operations[i]);
            if (entry) {
                algorithms[i] = *entry;
            } else {
                printf("⚠️  Warning: Custom operation %d not found in registry\n", config->custom_operations[i]);
                free(algorithms);
                cleanup_algorithm_registry();
                return false;
            }
        }
    } else {
        // Populate from complexity level
        const algorithm_registry_entry_t* complexity_algorithms = get_algorithms_by_complexity(config->complexity, &algorithm_count);
        memcpy(algorithms, complexity_algorithms, algorithm_count * sizeof(algorithm_registry_entry_t));
    }
    
    // Calculate packet length (use shortest packet to be safe)
    size_t min_packet_length = dataset->packets[0].packet_length;
    for (size_t i = 1; i < dataset->count; i++) {
        if (dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = dataset->packets[i].packet_length;
        }
    }

    printf("🔍 Starting recursive exhaustive checksum analysis...\n");
    printf("Dataset: %zu packets, Min packet length: %zu bytes\n", dataset->count, min_packet_length);
    printf("Complexity: %s, Algorithms: %d\n", get_complexity_name(config->complexity), algorithm_count);
    printf("Max fields: %d, Max constants: %d\n", config->max_fields, config->max_constants);
    printf("Max operation depth: %d\n\n", config->max_fields - 1);
    
    // Initialize progress tracker
    if (tracker) {
        // Simple factorial-based search space estimate
        // Approximate: P(packet_length, max_fields) × operations^max_fields × constants
        uint64_t permutations = 1;
        for (int i = 0; i < config->max_fields && i < (int)min_packet_length; i++) {
            permutations *= (min_packet_length - i);
        }
        
        uint64_t operation_sequences = 1;
        for (int i = 0; i < config->max_fields - 1; i++) {
            operation_sequences *= algorithm_count;
        }
        
        uint64_t estimated_tests = permutations * operation_sequences * config->max_constants;
        init_progress_tracker(tracker, estimated_tests, config->progress_interval);
    }
    
    uint64_t tests_performed = 0;
    bool search_interrupted = false;
    
    // Test different complexity levels (1 to max_fields)
    for (int complexity_level = 1; complexity_level <= config->max_fields && !search_interrupted; complexity_level++) {
        
        // Generate all field combinations (bit masks)
        uint64_t max_mask = (1ULL << min_packet_length) - 1;
        for (uint64_t field_mask = 1; field_mask <= max_mask && !search_interrupted; field_mask++) {
            // Extract field indices from mask
            uint8_t fields[CADS_MAX_FIELDS];
            int field_count = 0;
            for (size_t i = 0; i < min_packet_length && field_count < CADS_MAX_FIELDS; i++) {
                if (field_mask & (1ULL << i)) {
                    fields[field_count++] = i;
                }
            }
            
            // Skip if wrong complexity level or too many fields
            if (field_count != complexity_level || field_count > config->max_fields) {
                continue;
            }
            
            // Generate all permutations of these fields
            uint8_t permutations[24][CADS_MAX_FIELDS]; // Support up to 4! = 24
            uint32_t perm_count = 0;
            generate_all_permutations(fields, field_count, permutations, &perm_count);
            
            // Test each permutation with different operation sequences
            for (uint32_t p = 0; p < perm_count && !search_interrupted; p++) {
                // Test different constants
                for (int const_val = 0; const_val < config->max_constants && !search_interrupted; const_val++) {
                    uint8_t constant = (uint8_t)const_val;
                    
                    // Use recursive function to test all operation sequences up to max depth
                    int max_operation_depth = field_count > 1 ? field_count - 1 : 0;
                    if (max_operation_depth > 0) {
                        operation_t operation_sequence[CADS_MAX_FIELDS];
                        
                        if (test_operation_sequence(dataset, config, permutations[p], field_count,
                                                  algorithms, algorithm_count, operation_sequence,
                                                  0, max_operation_depth, constant, results, &tests_performed, tracker)) {
                            // Found a solution
                            if (config->early_exit) {
                                search_interrupted = true;
                                results->early_exit_triggered = true;
                                break;
                            }
                        }
                    }
                    
                    // Update progress occasionally (time-based)
                    if (tracker && should_display_progress(tracker)) {
                        update_progress(tracker, tests_performed, results->solution_count);
                        display_detailed_progress(tracker, "Recursive");
                    }
                }
            }
        }
    }
    
    // Final updates
    results->tests_performed = tests_performed;
    results->search_completed = !search_interrupted;
    
    if (tracker) {
        update_progress(tracker, tests_performed, results->solution_count);
        finish_progress(tracker);
        printf("\n");
        display_final_summary(tracker);
    }
    
    // Cleanup allocated algorithms array
    free(algorithms);
    cleanup_algorithm_registry();
    return true;
}

// Implementation of other required functions from the header
uint64_t extract_packet_field_value(const uint8_t* packet_data, size_t packet_length,
                                   uint8_t field_index, size_t checksum_size) {
    if (!packet_data || field_index >= packet_length) return 0;
    
    // For multi-byte checksums, we might need to extract multiple bytes
    uint64_t value = 0;
    size_t bytes_to_extract = (checksum_size > 1) ? checksum_size : 1;
    
    // Extract up to checksum_size bytes starting from field_index
    for (size_t i = 0; i < bytes_to_extract && (field_index + i) < packet_length; i++) {
        value = (value << 8) | packet_data[field_index + i];
    }
    
    return value;
}

uint64_t mask_checksum_to_size(uint64_t checksum, size_t checksum_size) {
    if (checksum_size >= 8) return checksum;
    
    uint64_t mask = (1ULL << (checksum_size * 8)) - 1;
    return checksum & mask;
}

search_results_t* create_search_results(size_t initial_capacity) {
    search_results_t* results = malloc(sizeof(search_results_t));
    if (!results) return NULL;
    
    results->solutions = malloc(initial_capacity * sizeof(checksum_solution_t));
    if (!results->solutions) {
        free(results);
        return NULL;
    }
    
    results->solution_count = 0;
    results->solution_capacity = initial_capacity;
    results->tests_performed = 0;
    results->search_completed = false;
    results->early_exit_triggered = false;
    
    return results;
}

void free_search_results(search_results_t* results) {
    if (!results) return;
    free(results->solutions);
    free(results);
}

bool add_solution(search_results_t* results, const checksum_solution_t* solution) {
    if (!results || !solution) return false;
    
    // Expand capacity if needed
    if (results->solution_count >= results->solution_capacity) {
        size_t new_capacity = results->solution_capacity * 2;
        checksum_solution_t* new_solutions = realloc(results->solutions, 
                                                    new_capacity * sizeof(checksum_solution_t));
        if (!new_solutions) return false;
        
        results->solutions = new_solutions;
        results->solution_capacity = new_capacity;
    }
    
    // Copy solution
    memcpy(&results->solutions[results->solution_count], solution, sizeof(checksum_solution_t));
    results->solution_count++;
    
    return true;
}

bool should_continue_search(const search_results_t* results, const cads_config_file_t* config) {
    if (!results || !config) return false;
    
    // Check early exit condition
    if (config->early_exit && results->solution_count > 0) {
        return false;
    }
    
    // Check max solutions limit
    if (config->max_solutions > 0 && results->solution_count >= (size_t)config->max_solutions) {
        return false;
    }
    
    return true;
}
