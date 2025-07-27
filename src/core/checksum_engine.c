#include "checksum_engine.h"
#include "../../include/algorithm_registry.h"
#include "../utils/field_combiner.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create search results structure
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

// Free search results
void free_search_results(search_results_t* results) {
    if (!results) return;
    free(results->solutions);
    free(results);
}

// Expand results capacity if needed
static bool ensure_results_capacity(search_results_t* results, size_t required_capacity) {
    if (results->solution_capacity >= required_capacity) return true;
    
    size_t new_capacity = results->solution_capacity * 2;
    if (new_capacity < required_capacity) new_capacity = required_capacity;
    
    checksum_solution_t* new_solutions = realloc(results->solutions, 
                                                new_capacity * sizeof(checksum_solution_t));
    if (!new_solutions) return false;
    
    results->solutions = new_solutions;
    results->solution_capacity = new_capacity;
    return true;
}

// Add solution to results
bool add_solution(search_results_t* results, const checksum_solution_t* solution) {
    if (!results || !solution) return false;
    
    if (!ensure_results_capacity(results, results->solution_count + 1)) {
        return false;
    }
    
    // Copy solution
    memcpy(&results->solutions[results->solution_count], solution, sizeof(checksum_solution_t));
    results->solution_count++;
    
    return true;
}

// Check if we should continue searching based on early exit conditions
bool should_continue_search(const search_results_t* results, const search_config_t* config) {
    if (!results || !config) return false;
    
    // Check early exit condition
    if (config->early_exit && results->solution_count > 0) {
        return false;
    }
    
    // Check max solutions limit
    if (config->max_solutions > 0 && results->solution_count >= config->max_solutions) {
        return false;
    }
    
    return true;
}

// Extract field value from packet data with proper multi-byte handling
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

// Mask checksum to specified size
uint64_t mask_checksum_to_size(uint64_t checksum, size_t checksum_size) {
    if (checksum_size == 0 || checksum_size > 8) return checksum;
    
    if (checksum_size == 8) return checksum;
    
    uint64_t mask = (1ULL << (checksum_size * 8)) - 1;
    return checksum & mask;
}

// Test a specific algorithm combination against all packets
bool test_algorithm_combination(const uint8_t* field_indices, uint8_t field_count,
                               const operation_t* operations, uint8_t operation_count,
                               uint64_t constant, size_t checksum_size,
                               const packet_dataset_t* dataset,
                               checksum_solution_t* solution) {
    if (!field_indices || !operations || !dataset || !solution || field_count == 0) {
        return false;
    }
    
    // Test this combination against all packets
    for (size_t packet_idx = 0; packet_idx < dataset->count; packet_idx++) {
        const test_packet_t* packet = &dataset->packets[packet_idx];
        
        // Skip packets with different checksum sizes
        if (packet->checksum_size != checksum_size) continue;
        
        // Calculate checksum using this algorithm combination
        uint64_t calculated_checksum = 0;
        
        if (field_count >= 1) {
            calculated_checksum = extract_packet_field_value(packet->packet_data, 
                                                           packet->packet_length,
                                                           field_indices[0], 
                                                           checksum_size);
            
            // Apply operations in sequence
            for (uint8_t op_idx = 0; op_idx < operation_count && (op_idx + 1) < field_count; op_idx++) {
                uint64_t second_value = extract_packet_field_value(packet->packet_data,
                                                                 packet->packet_length,
                                                                 field_indices[op_idx + 1],
                                                                 checksum_size);
                
                calculated_checksum = execute_algorithm(operations[op_idx], 
                                                       calculated_checksum, 
                                                       second_value, 
                                                       constant);
            }
        }
        
        // Mask to checksum size
        calculated_checksum = mask_checksum_to_size(calculated_checksum, checksum_size);
        
        // Compare with expected checksum
        uint64_t expected = mask_checksum_to_size(packet->expected_checksum, checksum_size);
        if (calculated_checksum != expected) {
            return false; // This combination doesn't work for this packet
        }
    }
    
    // If we get here, this combination works for all packets!
    // Fill in the solution structure
    memcpy(solution->field_indices, field_indices, field_count);
    solution->field_count = field_count;
    memcpy(solution->operations, operations, operation_count * sizeof(operation_t));
    solution->operation_count = operation_count;
    solution->constant = constant;
    solution->checksum_size = checksum_size;
    solution->validated = true;
    
    return true;
}

// Validate a solution against the dataset
bool validate_solution(const checksum_solution_t* solution, const packet_dataset_t* dataset) {
    if (!solution || !dataset) return false;
    
    return test_algorithm_combination(solution->field_indices, 
                                    solution->field_count,
                                    solution->operations,
                                    solution->operation_count,
                                    solution->constant,
                                    solution->checksum_size,
                                    dataset,
                                    &(checksum_solution_t){0}); // Temporary solution for validation
}

// Estimate total search space
uint64_t estimate_search_space(const packet_dataset_t* dataset, const search_config_t* config) {
    if (!dataset || !config || dataset->count == 0) return 0;
    
    // Get algorithm count for this complexity level
    int algorithm_count;
    get_algorithms_by_complexity(config->complexity, &algorithm_count);
    
    // Estimate field combinations
    size_t avg_packet_length = 0;
    for (size_t i = 0; i < dataset->count; i++) {
        avg_packet_length += dataset->packets[i].packet_length;
    }
    avg_packet_length /= dataset->count;
    
    // Calculate combinations for different field counts
    uint64_t total_combinations = 0;
    for (int fields = 1; fields <= config->max_fields && fields <= avg_packet_length; fields++) {
        // Approximate C(n,k) * k! for field combinations and permutations
        uint64_t field_combinations = 1;
        for (int i = 0; i < fields; i++) {
            field_combinations *= (avg_packet_length - i);
            field_combinations /= (i + 1);
        }
        
        // Add permutations and algorithm/constant combinations
        total_combinations += field_combinations * algorithm_count * config->max_constants;
    }
    
    return total_combinations;
}

// Recursive operation testing function
bool test_operation_sequence(const packet_dataset_t* dataset, 
                            const search_config_t* config,
                            const uint8_t* field_permutation,
                            int field_count,
                            const algorithm_registry_entry_t* algorithms,
                            int algorithm_count,
                            operation_t* operation_sequence,
                            int current_depth,
                            int max_depth,
                            uint8_t constant,
                            search_results_t* results) {
    
    // Base case: we've built a complete operation sequence, test it
    if (current_depth >= max_depth) {
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
                printf("🎉 SOLUTION #%zu FOUND!\n", results->solution_count);
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
                                  current_depth + 1, max_depth, constant, results)) {
            // Early exit if solution found and early exit enabled
            if (config->early_exit) {
                return true;
            }
        }
    }
    
    return false;
}

// Main exhaustive search function based on ultimate_checksum_cracker.c
bool execute_checksum_search(const packet_dataset_t* dataset, 
                            const search_config_t* config,
                            search_results_t* results,
                            progress_tracker_t* tracker) {
    if (!dataset || !config || !results) return false;
    
    // Initialize algorithm registry
    if (!initialize_algorithm_registry()) {
        return false;
    }
    
    // Get algorithms for this complexity level
    int algorithm_count;
    const algorithm_registry_entry_t* algorithms = get_algorithms_by_complexity(config->complexity, &algorithm_count);
    if (!algorithms || algorithm_count == 0) {
        cleanup_algorithm_registry();
        return false;
    }
    
    // Calculate packet length (use shortest packet to be safe)
    size_t min_packet_length = dataset->packets[0].packet_length;
    for (size_t i = 1; i < dataset->count; i++) {
        if (dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = dataset->packets[i].packet_length;
        }
    }
    
    // Estimate search space more accurately
    uint64_t max_field_combinations = (1ULL << min_packet_length) - 1; // 2^n - 1
    uint64_t total_combinations = max_field_combinations * algorithm_count * algorithm_count * 
                                 algorithm_count * config->max_constants;
    
    if (tracker) {
        init_progress_tracker(tracker, total_combinations, config->progress_interval);
    }
    
    uint64_t tests_performed = 0;
    bool search_interrupted = false;
    
    printf("🔍 Starting exhaustive checksum analysis...\n");
    printf("Dataset: %zu packets, Min packet length: %zu bytes\n", dataset->count, min_packet_length);
    printf("Complexity: %s, Algorithms: %d\n", get_complexity_name(config->complexity), algorithm_count);
    printf("Max fields: %d, Max constants: %d\n", config->max_fields, config->max_constants);
    printf("Estimated tests: %.1fB\n\n", total_combinations / 1000000000.0);
    
    // Test different complexity levels (1 to max_fields)
    for (int complexity_level = 1; complexity_level <= config->max_fields && !search_interrupted; complexity_level++) {
        printf("Testing complexity level %d (up to %d fields)...\n", complexity_level, complexity_level);
        
        // Generate all field combinations (bit masks)
        uint64_t max_mask = (1ULL << min_packet_length) - 1;
        for (uint64_t field_mask = 1; field_mask <= max_mask && !search_interrupted; field_mask++) {
            // Extract field indices from mask
            uint8_t fields[CADS_MAX_FIELDS];
            int field_count = 0;
            for (int i = 0; i < min_packet_length && field_count < CADS_MAX_FIELDS; i++) {
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
            
            // Test each permutation
            for (int p = 0; p < perm_count && !search_interrupted; p++) {
                // Test different operation sequences (up to 4 operations for now)
                for (int op1_idx = 0; op1_idx < algorithm_count && !search_interrupted; op1_idx++) {
                    for (int op2_idx = 0; op2_idx < (field_count > 1 ? algorithm_count : 1) && !search_interrupted; op2_idx++) {
                        for (int op3_idx = 0; op3_idx < (field_count > 2 ? algorithm_count : 1) && !search_interrupted; op3_idx++) {
                            for (int const_val = 0; const_val < config->max_constants && !search_interrupted; const_val++) {
                                uint8_t constant = (uint8_t)const_val;
                                
                                // Test this combination against all packets
                                bool all_match = true;
                                for (size_t packet_idx = 0; packet_idx < dataset->count; packet_idx++) {
                                    const test_packet_t* packet = &dataset->packets[packet_idx];
                                    
                                    // Skip packets with different checksum sizes
                                    if (packet->checksum_size != config->checksum_size) {
                                        all_match = false;
                                        break;
                                    }
                                    
                                    // Check if packet has enough fields
                                    bool has_all_fields = true;
                                    for (int f = 0; f < field_count; f++) {
                                        if (permutations[p][f] >= packet->packet_length) {
                                            has_all_fields = false;
                                            break;
                                        }
                                    }
                                    if (!has_all_fields) {
                                        all_match = false;
                                        break;
                                    }
                                    
                                    // Apply operations in sequence (extended for complex algorithms)
                                    uint64_t calculated = 0;
                                    
                                    if (field_count >= 1) {
                                        // Start with first field
                                        calculated = extract_packet_field_value(packet->packet_data, 
                                                                               packet->packet_length,
                                                                               permutations[p][0], 
                                                                               config->checksum_size);
                                        
                                        if (field_count >= 2) {
                                            uint64_t val2 = extract_packet_field_value(packet->packet_data,
                                                                                     packet->packet_length,
                                                                                     permutations[p][1],
                                                                                     config->checksum_size);
                                            calculated = execute_algorithm(algorithms[op1_idx].op, calculated, val2, constant);
                                            
                                            if (field_count >= 3) {
                                                uint64_t val3 = extract_packet_field_value(packet->packet_data,
                                                                                         packet->packet_length,
                                                                                         permutations[p][2],
                                                                                         config->checksum_size);
                                                calculated = execute_algorithm(algorithms[op2_idx].op, calculated, val3, constant);
                                                
                                                if (field_count >= 4) {
                                                    uint64_t val4 = extract_packet_field_value(packet->packet_data,
                                                                                             packet->packet_length,
                                                                                             permutations[p][3],
                                                                                             config->checksum_size);
                                                    calculated = execute_algorithm(algorithms[op3_idx].op, calculated, val4, constant);
                                                }
                                            }
                                        }
                                    }
                                    
                                    // Mask to checksum size
                                    calculated = mask_checksum_to_size(calculated, config->checksum_size);
                                    uint64_t expected = mask_checksum_to_size(packet->expected_checksum, config->checksum_size);
                                    
                                    if (calculated != expected) {
                                        all_match = false;
                                        break;
                                    }
                                }
                                
                                tests_performed++;
                                
                                if (all_match) {
                                    // Found a solution!
                                    checksum_solution_t solution = {0};
                                    
                                    // Fill solution details
                                    for (int f = 0; f < field_count; f++) {
                                        solution.field_indices[f] = permutations[p][f];
                                    }
                                    solution.field_count = field_count;
                                    solution.operations[0] = algorithms[op1_idx].op;
                                    if (field_count > 1) solution.operations[1] = algorithms[op2_idx].op;
                                    if (field_count > 2) solution.operations[2] = algorithms[op3_idx].op;
                                    // Note: operations[4] and [5] would need more fields in solution struct
                                    solution.operation_count = field_count > 1 ? field_count - 1 : 0;
                                    solution.constant = constant;
                                    solution.checksum_size = config->checksum_size;
                                    solution.validated = true;
                                    
                                    if (add_solution(results, &solution)) {
                                        printf("🎉 SOLUTION #%zu FOUND!\n", results->solution_count);
                                        printf("   Fields: ");
                                        for (int f = 0; f < field_count; f++) {
                                            printf("%d ", permutations[p][f]);
                                        }
                                        printf("\n   Operations: %s", algorithms[op1_idx].name);
                                        if (field_count > 1) printf(" %s", algorithms[op2_idx].name);
                                        if (field_count > 2) printf(" %s", algorithms[op3_idx].name);
                                        printf("\n   Constant: 0x%02X\n", constant);
                                        
                                        // Show algorithm steps
                                        printf("   Algorithm Steps:\n");
                                        if (field_count == 1) {
                                            printf("     result = field[%d]\n", permutations[p][0]);
                                        } else if (field_count == 2) {
                                            printf("     result = field[%d] %s field[%d] (constant=0x%02X)\n", 
                                                   permutations[p][0], algorithms[op1_idx].name, permutations[p][1], constant);
                                        } else if (field_count == 3) {
                                            printf("     step1 = field[%d] %s field[%d] (constant=0x%02X)\n", 
                                                   permutations[p][0], algorithms[op1_idx].name, permutations[p][1], constant);
                                            printf("     result = step1 %s field[%d]\n", 
                                                   algorithms[op2_idx].name, permutations[p][2]);
                                        } else if (field_count == 4) {
                                            printf("     step1 = field[%d] %s field[%d] (constant=0x%02X)\n", 
                                                   permutations[p][0], algorithms[op1_idx].name, permutations[p][1], constant);
                                            printf("     step2 = step1 %s field[%d]\n", 
                                                   algorithms[op2_idx].name, permutations[p][2]);
                                            printf("     result = step2 %s field[%d]\n", 
                                                   algorithms[op3_idx].name, permutations[p][3]);
                                        }
                                        printf("\n");
                                    }
                                    
                                    // Check early exit conditions
                                    if (!should_continue_search(results, config)) {
                                        search_interrupted = true;
                                        results->early_exit_triggered = config->early_exit || 
                                                                      (config->max_solutions > 0 && 
                                                                       results->solution_count >= config->max_solutions);
                                    }
                                }
                                
                                // Update progress
                                if (tracker && tests_performed % config->progress_interval == 0) {
                                    update_progress(tracker, tests_performed, results->solution_count);
                                    if (config->verbose) {
                                        display_detailed_progress(tracker, algorithms[op1_idx].name);
                                    } else {
                                        // Always show basic progress bar
                                        display_progress_bar(tracker);
                                    }
                                }
                            }
                        }
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
    
    cleanup_algorithm_registry();
    return true;
}