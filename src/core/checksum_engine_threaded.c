#include "checksum_engine.h"
#include "../../include/cads_config_loader.h"
#include "../../include/algorithm_registry.h"
#include "../utils/field_combiner.h"
#include "../utils/search_display.h"
#include "thread_partitioner.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Recursive operation testing function declaration
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

// Progress monitor context for compatibility with existing progress monitoring
typedef struct {
    progress_tracker_t* tracker;
    pthread_mutex_t* progress_mutex;
    uint64_t* total_tests;
    search_results_t* results;
    bool* search_interrupted;
} progress_monitor_context_t;

// Thread-safe context structure
typedef struct {
    const config_t* config;
    const packet_dataset_t* dataset;
    const algorithm_registry_entry_t* algorithms;
    int algorithm_count;
    search_results_t* results;
    progress_tracker_t* tracker;
    pthread_mutex_t* results_mutex;
    pthread_mutex_t* progress_mutex;
    uint64_t* total_tests;
    bool* search_interrupted;
} thread_context_t;

// Progress monitoring thread function
void* progress_monitor_thread(void* arg) {
    thread_context_t* ctx = (thread_context_t*)arg;
    
    while (true) {
        pthread_mutex_lock(ctx->progress_mutex);
        uint64_t current_tests = *(ctx->total_tests);
        size_t current_solutions = ctx->results->solution_count;
        bool interrupted = *(ctx->search_interrupted);
        pthread_mutex_unlock(ctx->progress_mutex);
        
        // Update progress tracker
        update_progress(ctx->tracker, current_tests, current_solutions);
        if (should_display_progress(ctx->tracker)) {
            display_detailed_progress(ctx->tracker, "Parallel");
        }
        
        // Exit if search is complete or interrupted
        if (interrupted || current_tests >= ctx->tracker->total_combinations) {
            break;
        }
        
        // Sleep for progress interval
        usleep(ctx->config->progress_interval * 1000); // Convert ms to microseconds
    }
    
    return NULL;
}

// Weighted thread context for operation partitioning
typedef struct {
    int thread_id;
    const config_t* config;
    const packet_dataset_t* dataset;
    const algorithm_registry_entry_t* algorithms;
    int algorithm_count;
    operation_t* assigned_operations;  // Operations this thread should explore
    int num_assigned_operations;
    search_results_t* results;
    progress_tracker_t* tracker;
    pthread_mutex_t* results_mutex;
    pthread_mutex_t* progress_mutex;
    uint64_t* total_tests;
    bool* search_interrupted;
} weighted_thread_context_t;

// Custom recursive function that forces the first operation but explores all combinations after
bool test_constrained_operation_sequence(const packet_dataset_t* dataset, 
                                        const config_t* config,
                                        const uint8_t* field_permutation,
                                        int field_count,
                                        const algorithm_registry_entry_t* algorithms,
                                        int algorithm_count,
                                        operation_t* operation_sequence,
                                        operation_t starting_operation,
                                        int current_depth,
                                        int max_depth,
                                        uint8_t constant,
                                        search_results_t* results,
                                        uint64_t* tests_performed) {
    
    // Base case: we've built a complete operation sequence, test it
    if (current_depth >= max_depth) {
        (*tests_performed)++;
        
        // Test this operation sequence against all packets (same logic as original)
        bool all_match = true;
        for (size_t packet_idx = 0; packet_idx < dataset->count; packet_idx++) {
            const test_packet_t* packet = &dataset->packets[packet_idx];
            
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
            
            // Apply operation sequence correctly for patterns like MXT275
            // Start with first field value
            uint64_t calculated = extract_packet_field_value(packet->packet_data, 
                                                           packet->packet_length,
                                                           field_permutation[0], 
                                                           config->checksum_size);
            
            // Apply operations step by step
            int field_idx = 1; // Next field to consume
            for (int op_idx = 0; op_idx < max_depth; op_idx++) {
                operation_t op = operation_sequence[op_idx];
                
                if (op == OP_ONES_COMPLEMENT) {
                    // Unary operation - no field needed
                    calculated = execute_algorithm(op, calculated, 0, 0);
                } else if (op == OP_CONST_ADD) {
                    // Use constant
                    calculated = execute_algorithm(op, calculated, 0, constant);
                } else if (field_idx < field_count) {
                    // Binary operation - consume next field
                    uint64_t next_val = extract_packet_field_value(packet->packet_data,
                                                                 packet->packet_length,
                                                                 field_permutation[field_idx],
                                                                 config->checksum_size);
                    calculated = execute_algorithm(op, calculated, next_val, 0);
                    field_idx++;
                } else {
                    // No more fields available for binary operation - skip
                    break;
                }
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
                printf("\n🎉 SOLUTION #%zu FOUND!\n", results->solution_count);
                printf("   Fields: ");
                for (int f = 0; f < field_count; f++) {
                    printf("%d ", field_permutation[f]);
                }
                printf("\n   Operations: ");
                for (int op = 0; op < max_depth; op++) {
                    for (int a = 0; a < algorithm_count; a++) {
                        if (algorithms[a].op == operation_sequence[op]) {
                            printf("%s ", algorithms[a].name);
                            break;
                        }
                    }
                }
                printf("\n   Constant: 0x%02X\n\n", constant);
            }
            return true;
        }
        return false;
    }
    
    // Recursive case: fill the next position
    for (int alg_idx = 0; alg_idx < algorithm_count; alg_idx++) {
        // At depth 0, only allow the starting operation
        if (current_depth == 0 && algorithms[alg_idx].op != starting_operation) {
            continue;
        }
        
        operation_sequence[current_depth] = algorithms[alg_idx].op;
        
        
        if (test_constrained_operation_sequence(dataset, config, field_permutation, field_count,
                                              algorithms, algorithm_count, operation_sequence,
                                              starting_operation, current_depth + 1, max_depth, 
                                              constant, results, tests_performed)) {
            if (config->early_exit) {
                return true;
            }
        }
    }
    
    return false;
}

// Helper function to recursively test all sequences starting with a specific operation
bool test_starting_operation_sequences(const packet_dataset_t* dataset, 
                                     const config_t* config,
                                     const uint8_t* field_permutation,
                                     int field_count,
                                     const algorithm_registry_entry_t* algorithms,
                                     int algorithm_count,
                                     operation_t* operation_sequence,
                                     operation_t starting_operation,
                                     int max_depth,
                                     uint8_t constant,
                                     search_results_t* results,
                                     uint64_t* tests_performed) {
    
    return test_constrained_operation_sequence(dataset, config, field_permutation, field_count,
                                             algorithms, algorithm_count, operation_sequence,
                                             starting_operation, 0, max_depth, constant, 
                                             results, tests_performed);
}

// Weighted worker thread - explores only assigned operations via recursive search  
void* weighted_worker_thread(void* arg) {
    weighted_thread_context_t* ctx = (weighted_thread_context_t*)arg;
    uint64_t local_tests = 0;
    
    // If no operations assigned, exit immediately
    if (ctx->num_assigned_operations == 0) {
        return NULL;
    }
    
    // Find minimum packet length for field generation
    size_t min_packet_length = SIZE_MAX;
    for (size_t i = 0; i < ctx->dataset->count; i++) {
        if (ctx->dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = ctx->dataset->packets[i].packet_length;
        }
    }
    
    // Iterate through all complexity levels (same logic as single-threaded version)
    for (int complexity_level = 1; complexity_level <= ctx->config->max_fields; complexity_level++) {
        
        // Generate all field combinations using bit masks (same as single-threaded)
        uint64_t max_mask = (1ULL << min_packet_length) - 1;
        for (uint64_t field_mask = 1; field_mask <= max_mask; field_mask++) {
            // Extract field indices from mask 
            uint8_t fields[CADS_MAX_FIELDS];
            int field_count = 0;
            for (size_t i = 0; i < min_packet_length && field_count < CADS_MAX_FIELDS; i++) {
                if (field_mask & (1ULL << i)) {
                    fields[field_count++] = i;
                }
            }
            
            // Skip if wrong complexity level or too many fields
            if (field_count != complexity_level || field_count > ctx->config->max_fields) {
                continue;
            }
            
            // Generate all permutations of these fields
            uint8_t permutations[24][CADS_MAX_FIELDS];
            uint32_t perm_count = 0;
            generate_all_permutations(fields, field_count, permutations, &perm_count);
            
            // For each field permutation
            for (uint32_t perm_idx = 0; perm_idx < perm_count; perm_idx++) {
                // For each constant value
                for (int constant = 0; constant < ctx->config->max_constants; constant++) {
                    // Check if search should be interrupted
                    pthread_mutex_lock(ctx->progress_mutex);
                    bool interrupted = *(ctx->search_interrupted);
                    pthread_mutex_unlock(ctx->progress_mutex);
                    
                    if (interrupted) {
                        break;
                    }
                    
                    // For each assigned operation (start the recursive branch with this operation)
                    for (int op_idx = 0; op_idx < ctx->num_assigned_operations; op_idx++) {
                        operation_t start_operation = ctx->assigned_operations[op_idx];
                        
                        // Initialize operation sequence with the assigned starting operation
                        operation_t operation_sequence[CADS_MAX_FIELDS];
                        operation_sequence[0] = start_operation;
                        
                        // Use same max_operation_depth logic as single-threaded version
                        int max_operation_depth = field_count + 1;  // Allow extra operations for unary ops
                        
                        // Test all possible completions of sequences starting with our operation
                        operation_t test_sequence[CADS_MAX_FIELDS];
                        test_sequence[0] = start_operation;
                        
                        bool found = test_starting_operation_sequences(ctx->dataset, ctx->config,
                                                                     permutations[perm_idx], field_count,
                                                                     ctx->algorithms, ctx->algorithm_count,
                                                                     test_sequence, start_operation, max_operation_depth,
                                                                     constant, ctx->results, &local_tests);
                        
                        // Check for early exit
                        if (found && ctx->config->early_exit) {
                            pthread_mutex_lock(ctx->progress_mutex);
                            *(ctx->search_interrupted) = true;
                            pthread_mutex_unlock(ctx->progress_mutex);
                            break;
                        }
                        
                        // Update total tests periodically (thread-safe)
                        if (local_tests % 1000 == 0) {
                            pthread_mutex_lock(ctx->progress_mutex);
                            *(ctx->total_tests) += local_tests;
                            local_tests = 0;
                            pthread_mutex_unlock(ctx->progress_mutex);
                        }
                    }
                }
            }
        }
    }
    
    // Add remaining local tests
    if (local_tests > 0) {
        pthread_mutex_lock(ctx->progress_mutex);
        *(ctx->total_tests) += local_tests;
        pthread_mutex_unlock(ctx->progress_mutex);
    }
    
    return NULL;
}

// Weighted checksum search - handles both single and multi-threaded execution
bool execute_weighted_checksum_search(const config_t* config, 
                                     search_results_t* results,
                                     const hardware_benchmark_result_t* benchmark) {
    
    if (!config || !results || !config->dataset || config->dataset->count == 0) {
        return false;
    }
    
    // Normalize thread count: always use at least 1 thread  
    int actual_threads;
    if (config->threads > 1) {
        actual_threads = config->threads;
    } else if (config->threads == 0) {
        actual_threads = sysconf(_SC_NPROCESSORS_ONLN); // Auto-detect
    } else {
        actual_threads = 1; // Single-threaded mode (threads == 1)
    }
    
    // For single-threaded, use existing recursive engine
    if (actual_threads == 1) {
        if (config->verbose) {
            printf("🔄 Single-threaded execution\n");
        }
        progress_tracker_t tracker;
        return execute_checksum_search(config, results, &tracker);
    }
    
    // Multi-threaded weighted execution
    if (config->verbose) {
        printf("🧵 Weighted multi-threaded execution: %d threads\n", actual_threads);
    }
    
    // Initialize algorithm registry
    if (!initialize_algorithm_registry()) {
        return false;
    }
    
    // Build operations array
    int algorithm_count;
    algorithm_registry_entry_t* algorithms = malloc(32 * sizeof(algorithm_registry_entry_t));
    if (!algorithms) {
        cleanup_algorithm_registry();
        return false;
    }
    
    
    if (config->custom_operation_count > 0 && config->custom_operations) {
        algorithm_count = config->custom_operation_count;
        for (int i = 0; i < algorithm_count; i++) {
            const algorithm_registry_entry_t* entry = get_algorithm_by_operation(config->custom_operations[i]);
            if (entry) {
                algorithms[i] = *entry;
            } else {
                free(algorithms);
                cleanup_algorithm_registry();
                return false;
            }
        }
    } else {
        const algorithm_registry_entry_t* complexity_algorithms = get_algorithms_by_complexity(config->complexity, &algorithm_count);
        memcpy(algorithms, complexity_algorithms, algorithm_count * sizeof(algorithm_registry_entry_t));
    }
    
    // Create weighted partitions
    partitioning_result_t* partitions = create_weighted_partitions(algorithms, algorithm_count, actual_threads);
    if (!partitions) {
        free(algorithms);
        cleanup_algorithm_registry();
        return false;
    }
    
    if (config->verbose) {
        print_partition_summary(partitions);
    }
    
    // Calculate estimated work
    size_t min_packet_length = SIZE_MAX;
    for (size_t i = 0; i < config->dataset->count; i++) {
        if (config->dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = config->dataset->packets[i].packet_length;
        }
    }
    
    uint64_t permutations = 1;
    for (int i = 0; i < config->max_fields && i < (int)min_packet_length; i++) {
        permutations *= (min_packet_length - i);
    }
    
    // Calculate operation sequences for ALL complexity levels (same as single-threaded)
    uint64_t operation_sequences = 0;
    for (int complexity = 1; complexity <= config->max_fields; complexity++) {
        uint64_t ops_for_complexity = 1;
        // Allow field_count+1 operations for complex patterns
        for (int i = 0; i < complexity + 1; i++) {
            ops_for_complexity *= algorithm_count;
        }
        operation_sequences += ops_for_complexity;
    }
    
    uint64_t estimated_tests = permutations * operation_sequences * config->max_constants;
    
    // Initialize progress tracker
    progress_tracker_t tracker;
    init_progress_tracker(&tracker, estimated_tests, config->progress_interval);
    
    if (config->verbose) {
        printf("🔍 Starting weighted parallel exhaustive checksum analysis...\n");
        printf("Dataset: %zu packets, Min packet length: %zu bytes\n", config->dataset->count, min_packet_length);
        printf("Threads: %d, Algorithms: %d\n", actual_threads, algorithm_count);
        printf("Max fields: %d, Max constants: %d\n\n", config->max_fields, config->max_constants);
    }
    
    // Threading infrastructure
    pthread_t* threads = malloc(actual_threads * sizeof(pthread_t));
    if (!threads) {
        free_partitioning_result(partitions);
        free(algorithms);
        cleanup_algorithm_registry();
        return false;
    }
    
    weighted_thread_context_t* contexts = malloc(actual_threads * sizeof(weighted_thread_context_t));
    if (!contexts) {
        free(threads);
        free_partitioning_result(partitions);
        free(algorithms);
        cleanup_algorithm_registry();
        return false;
    }
    
    pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t progress_mutex = PTHREAD_MUTEX_INITIALIZER;
    uint64_t total_tests = 0;
    bool search_interrupted = false;
    
    // Start progress monitoring thread
    pthread_t progress_thread;
    bool progress_thread_created = false;
    thread_context_t progress_thread_ctx = {
        .config = config,
        .dataset = config->dataset,
        .algorithms = algorithms,
        .algorithm_count = algorithm_count,
        .results = results,
        .tracker = &tracker,
        .results_mutex = &results_mutex,
        .progress_mutex = &progress_mutex,
        .total_tests = &total_tests,
        .search_interrupted = &search_interrupted
    };
    
    if (pthread_create(&progress_thread, NULL, progress_monitor_thread, &progress_thread_ctx) == 0) {
        progress_thread_created = true;
    }
    
    // Launch worker threads
    for (int i = 0; i < actual_threads; i++) {
        contexts[i] = (weighted_thread_context_t) {
            .thread_id = i,
            .config = config,
            .dataset = config->dataset,
            .algorithms = algorithms,
            .algorithm_count = algorithm_count,
            .assigned_operations = partitions->partitions[i].assigned_operations,
            .num_assigned_operations = partitions->partitions[i].num_assigned_operations,
            .results = results,
            .tracker = &tracker,
            .results_mutex = &results_mutex,
            .progress_mutex = &progress_mutex,
            .total_tests = &total_tests,
            .search_interrupted = &search_interrupted
        };
        
        if (pthread_create(&threads[i], NULL, weighted_worker_thread, &contexts[i]) != 0) {
            // Failed to create thread - wait for already created threads
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            
            free(contexts);
            free(threads);
            free_partitioning_result(partitions);
            free(algorithms);
            cleanup_algorithm_registry();
            return false;
        }
    }
    
    // Wait for all worker threads to complete
    for (int i = 0; i < actual_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Stop progress monitoring
    if (progress_thread_created) {
        search_interrupted = true;
        pthread_join(progress_thread, NULL);
    }
    
    // Final progress update
    update_progress(&tracker, total_tests, results->solution_count);
    if (config->verbose || actual_threads == 1) {
        display_detailed_progress(&tracker, (actual_threads > 1) ? "Parallel" : "Single");
    }
    
    // Cleanup
    free(contexts);
    free(threads);
    free_partitioning_result(partitions);
    pthread_mutex_destroy(&results_mutex);
    pthread_mutex_destroy(&progress_mutex);
    free(algorithms);
    cleanup_algorithm_registry();
    
    return true;
}