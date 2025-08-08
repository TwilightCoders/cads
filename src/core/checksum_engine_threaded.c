#include "checksum_engine.h"
#include "../../include/cads_config_loader.h"
#include "../../include/algorithm_registry.h"
#include "../utils/field_combiner.h"
#include "../utils/search_display.h"
#include "thread_partitioner.h"
#include "../../include/sequence_evaluator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Forward declarations
typedef struct weighted_thread_context_s weighted_thread_context_t;

// Function declarations
void display_per_thread_progress(thread_progress_t** all_progress, int num_threads, progress_tracker_t* tracker);
void print_found_solutions(const search_results_t* results, const algorithm_registry_entry_t* algorithms, int algorithm_count);

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



// Weighted thread context for operation partitioning
struct weighted_thread_context_s {
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
    thread_progress_t* thread_progress;  // Per-thread progress tracking
    thread_progress_t** all_thread_progress;  // Array of all thread progress for unified view
    int total_threads;
};

// Enhanced progress monitoring thread with per-thread support  
void* progress_monitor_thread(void* arg) {
    weighted_thread_context_t* ctx = (weighted_thread_context_t*)arg;
    
    while (true) {
        pthread_mutex_lock(ctx->progress_mutex);
        uint64_t current_tests = *(ctx->total_tests);
        size_t current_solutions = ctx->results->solution_count;
        bool interrupted = *(ctx->search_interrupted);
        pthread_mutex_unlock(ctx->progress_mutex);
        
        // Check if we should stop due to solution found and early exit enabled
        if (current_solutions > 0 && ctx->config->early_exit) {
            pthread_mutex_lock(ctx->progress_mutex);
            *(ctx->search_interrupted) = true;
            pthread_mutex_unlock(ctx->progress_mutex);
            break;
        }
        
        // Check if search is complete (all tests done)
        if (current_tests >= ctx->tracker->total_combinations) {
            pthread_mutex_lock(ctx->progress_mutex);
            *(ctx->search_interrupted) = true;
            pthread_mutex_unlock(ctx->progress_mutex);
            break;
        }
        
        // Update progress tracker
        update_progress(ctx->tracker, current_tests, current_solutions);
        if (should_display_progress(ctx->tracker)) {
            // Check if user wants per-thread progress (could be a config option)
            // For now, default to unified progress
            if (ctx->config->verbose && ctx->total_threads > 1) {
                display_per_thread_progress(ctx->all_thread_progress, ctx->total_threads, ctx->tracker);
            } else {
                display_detailed_progress(ctx->tracker, "Parallel");
            }
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

// Print solutions found during threaded search (called after all threads stopped)
void print_found_solutions(const search_results_t* results, const algorithm_registry_entry_t* algorithms, int algorithm_count) {
    if (!results || results->solution_count == 0) {
        return;
    }
    
    // Clear any remaining progress display before printing solutions
    printf("\n");
    
    for (size_t sol_idx = 0; sol_idx < results->solution_count; sol_idx++) {
        const checksum_solution_t* solution = &results->solutions[sol_idx];
        
        printf("🎉 SOLUTION #%zu FOUND!\n", sol_idx + 1);
        printf("   Fields: ");
        for (int f = 0; f < solution->field_count; f++) {
            printf("%d ", solution->field_indices[f]);
        }
        printf("\n   Operations: ");
        for (int op = 0; op < solution->operation_count; op++) {
            for (int a = 0; a < algorithm_count; a++) {
                if (algorithms[a].op == solution->operations[op]) {
                    printf("%s ", algorithms[a].name);
                    break;
                }
            }
        }
        printf("\n   Constant: 0x%02X\n\n", (unsigned int)solution->constant);
    }
}

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
        
        // Debug: Log when we test the known working solution pattern
        if (max_depth == 4 && field_count == 4 && constant == 0x18 &&
            field_permutation[0] == 3 && field_permutation[1] == 2 && field_permutation[2] == 4 && field_permutation[3] == 5 &&
            operation_sequence[0] == OP_CONST_ADD && operation_sequence[1] == OP_CONST_ADD && 
            operation_sequence[2] == OP_XOR && operation_sequence[3] == OP_XOR) {
            printf("DEBUG: TESTING KNOWN SOLUTION PATTERN!\n");
            printf("Fields: [%d,%d,%d,%d], Ops: [%d,%d,%d,%d], Constant: 0x%02X\n",
                   field_permutation[0], field_permutation[1], field_permutation[2], field_permutation[3],
                   (int)operation_sequence[0], (int)operation_sequence[1], (int)operation_sequence[2], (int)operation_sequence[3], constant);
        }
        
        
    bool all_match = evaluate_operation_sequence(dataset, config, field_permutation, field_count, operation_sequence, max_depth, constant);
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
                // Solution found - don't print here, let main thread handle it after stopping all threads
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
    time_t last_update = time(NULL);
    
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
                        // Mark thread as completed when exiting due to interruption
                        time_t interrupt_time = time(NULL);
                        pthread_mutex_lock(&ctx->thread_progress->mutex);
                        double total_elapsed = interrupt_time - ctx->thread_progress->start_time;
                        if (total_elapsed > 0) {
                            ctx->thread_progress->current_rate = (double)ctx->thread_progress->tests_performed / total_elapsed;
                        }
                        ctx->thread_progress->completed = true;
                        ctx->thread_progress->last_update = interrupt_time;
                        pthread_mutex_unlock(&ctx->thread_progress->mutex);
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
                        
                        // Track solutions found by this thread
                        if (found) {
                            pthread_mutex_lock(&ctx->thread_progress->mutex);
                            ctx->thread_progress->solutions_found++;
                            pthread_mutex_unlock(&ctx->thread_progress->mutex);
                        }
                        
                        // Check for early exit
                        if (found && ctx->config->early_exit) {
                            pthread_mutex_lock(ctx->progress_mutex);
                            *(ctx->search_interrupted) = true;
                            pthread_mutex_unlock(ctx->progress_mutex);
                            
                            // Mark thread as completed when exiting due to solution found
                            time_t solution_time = time(NULL);
                            pthread_mutex_lock(&ctx->thread_progress->mutex);
                            double total_elapsed = solution_time - ctx->thread_progress->start_time;
                            if (total_elapsed > 0) {
                                ctx->thread_progress->current_rate = (double)ctx->thread_progress->tests_performed / total_elapsed;
                            }
                            ctx->thread_progress->completed = true;
                            ctx->thread_progress->last_update = solution_time;
                            pthread_mutex_unlock(&ctx->thread_progress->mutex);
                            break;
                        }
                        
                        // Update progress periodically based on time (more efficient)
                        time_t current_time = time(NULL);
                        if (current_time - last_update >= (ctx->config->progress_interval / 1000)) {
                            // Update global progress  
                            pthread_mutex_lock(ctx->progress_mutex);
                            *(ctx->total_tests) += local_tests;
                            pthread_mutex_unlock(ctx->progress_mutex);
                            
                            // Update per-thread progress with rate calculation
                            pthread_mutex_lock(&ctx->thread_progress->mutex);
                            ctx->thread_progress->tests_performed += local_tests;
                            // Calculate overall rate since thread start (more accurate than incremental rate)
                            double total_elapsed = current_time - ctx->thread_progress->start_time;
                            if (total_elapsed > 0) {
                                ctx->thread_progress->current_rate = (double)ctx->thread_progress->tests_performed / total_elapsed;
                            }
                            ctx->thread_progress->last_update = current_time;
                            pthread_mutex_unlock(&ctx->thread_progress->mutex);
                            
                            local_tests = 0;
                            last_update = current_time;
                        }
                    }
                }
            }
        }
    }
    
    // Add remaining local tests and mark thread as completed
    pthread_mutex_lock(ctx->progress_mutex);
    *(ctx->total_tests) += local_tests;
    pthread_mutex_unlock(ctx->progress_mutex);
    
    // Update per-thread progress and mark as completed
    time_t final_time = time(NULL);
    pthread_mutex_lock(&ctx->thread_progress->mutex);
    ctx->thread_progress->tests_performed += local_tests;
    // Calculate final overall rate
    double total_elapsed = final_time - ctx->thread_progress->start_time;
    if (total_elapsed > 0) {
        ctx->thread_progress->current_rate = (double)ctx->thread_progress->tests_performed / total_elapsed;
    }
    ctx->thread_progress->last_update = final_time;
    ctx->thread_progress->completed = true;  // Mark thread as completed
    pthread_mutex_unlock(&ctx->thread_progress->mutex);
    
    return NULL;
}

// Weighted checksum search - handles both single and multi-threaded execution
bool execute_weighted_checksum_search(const config_t* config, 
                                     search_results_t* results,
                                     const hardware_benchmark_result_t* benchmark __attribute__((unused))) {
    
    if (!config || !results || !config->dataset || config->dataset->count == 0) {
        return false;
    }
    
    // Initialize algorithm registry
    if (!initialize_algorithm_registry()) {
        return false;
    }

    // Build field cache (best-effort; ignore failure) for faster extraction on large datasets
    build_field_cache(config->dataset, config->checksum_size, config->max_fields);
    
    // Build operations array first to know how many operations we have
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
    
    // Normalize thread count: cap at operation count and use at least 1 thread  
    int actual_threads;
    if (config->threads > 1) {
        actual_threads = config->threads;
    } else if (config->threads == 0) {
        actual_threads = sysconf(_SC_NPROCESSORS_ONLN); // Auto-detect
    } else {
        actual_threads = 1; // Single-threaded mode (threads == 1)
    }
    
    // Cap threads at available operations - no point having more threads than operations
    if (actual_threads > algorithm_count) {
        actual_threads = algorithm_count;
        if (config->verbose) {
            printf("🔧 Capping threads to %d (number of operations available)\n", actual_threads);
        }
    }
    
    // Single-threaded still uses the optimized weighted algorithm, just with 1 thread
    if (actual_threads == 1 && config->verbose) {
        printf("🔄 Single-threaded execution (optimized)\n");
    }
    
    // Multi-threaded weighted execution
    if (config->verbose && actual_threads > 1) {
        printf("🧵 Weighted multi-threaded execution: %d threads\n", actual_threads);
    }
    
    
    // Calculate estimated work first (needed for workload balancing)
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
    
    // Create workload-balanced partitions that consider both search space and computational weight
    partitioning_result_t* partitions = create_workload_balanced_partitions(algorithms, algorithm_count, actual_threads,
                                                                           config->max_fields, config->max_constants, permutations);
    if (!partitions) {
        free(algorithms);
        cleanup_algorithm_registry();
        return false;
    }
    
    if (config->verbose) {
        print_partition_summary_with_workload(partitions, algorithms, algorithm_count,
                                             config->max_fields, config->max_constants, permutations);
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
    
    // Calculate per-thread work estimates using actual combinatorial formula
    uint64_t* thread_estimates = malloc(actual_threads * sizeof(uint64_t));
    if (thread_estimates) {
        for (int i = 0; i < actual_threads; i++) {
            if (i < partitions->num_threads) {
                // Calculate precise search space for constrained recursive search
                // Thread starts with N assigned operations, then branches to all operations
                int thread_assigned_ops = partitions->partitions[i].num_assigned_operations;
                
                uint64_t thread_operation_sequences = 0;
                for (int complexity = 1; complexity <= config->max_fields; complexity++) {
                    // N starting operations × total_ops^(remaining positions)
                    uint64_t ops_for_complexity = thread_assigned_ops;
                    for (int j = 1; j < complexity + 1; j++) {
                        ops_for_complexity *= algorithm_count;
                    }
                    thread_operation_sequences += ops_for_complexity;
                }
                
                thread_estimates[i] = permutations * thread_operation_sequences * config->max_constants;
            } else {
                thread_estimates[i] = estimated_tests / actual_threads; // Fallback
            }
        }
        set_thread_estimates(&tracker, thread_estimates, actual_threads);
    }
    
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
    
    // Initialize per-thread progress tracking
    thread_progress_t* thread_progress = malloc(actual_threads * sizeof(thread_progress_t));
    thread_progress_t** all_thread_progress = malloc(actual_threads * sizeof(thread_progress_t*));
    if (!thread_progress || !all_thread_progress) {
        free(contexts);
        free(threads);
        free_partitioning_result(partitions);
        free(algorithms);
        cleanup_algorithm_registry();
        return false;
    }
    
    time_t search_start_time = time(NULL);
    for (int i = 0; i < actual_threads; i++) {
        thread_progress[i].tests_performed = 0;
        thread_progress[i].current_rate = 0.0;
        thread_progress[i].last_update = search_start_time;
        thread_progress[i].start_time = search_start_time;
        thread_progress[i].completed = false;
        thread_progress[i].solutions_found = 0;
        pthread_mutex_init(&thread_progress[i].mutex, NULL);
        all_thread_progress[i] = &thread_progress[i];
    }
    
    // Start progress monitoring thread
    pthread_t progress_thread;
    bool progress_thread_created = false;
    weighted_thread_context_t progress_thread_ctx = {
        .thread_id = -1,  // Monitor thread
        .config = config,
        .dataset = config->dataset,
        .algorithms = algorithms,
        .algorithm_count = algorithm_count,
        .results = results,
        .tracker = &tracker,
        .results_mutex = &results_mutex,
        .progress_mutex = &progress_mutex,
        .total_tests = &total_tests,
        .search_interrupted = &search_interrupted,
        .all_thread_progress = all_thread_progress,
        .total_threads = actual_threads
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
            .search_interrupted = &search_interrupted,
            .thread_progress = &thread_progress[i],
            .all_thread_progress = all_thread_progress,
            .total_threads = actual_threads
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
    
    // Final progress update to show correct solution count and completion state
    update_progress(&tracker, total_tests, results->solution_count);
    
    // Stop progress monitoring first
    if (progress_thread_created) {
        search_interrupted = true;
        pthread_join(progress_thread, NULL);
    }
    
    // Show final progress with completion state (no ETA, just elapsed time)  
    if (config->verbose && actual_threads > 1) {
        // Mark all threads as complete for final display
        time_t current_time = time(NULL);
        for (int i = 0; i < actual_threads; i++) {
            pthread_mutex_lock(&thread_progress[i].mutex);
            thread_progress[i].last_update = current_time;
            thread_progress[i].completed = true;  // Ensure all threads show as completed
            pthread_mutex_unlock(&thread_progress[i].mutex);
        }
        display_per_thread_progress(all_thread_progress, actual_threads, &tracker);
    }
    
    // Print any solutions found (now that all threads have stopped)
    if (results->solution_count > 0) {
    // Deterministic ordering
    sort_search_solutions(results);
        print_found_solutions(results, algorithms, algorithm_count);
    }
    
    // Cleanup
    for (int i = 0; i < actual_threads; i++) {
        pthread_mutex_destroy(&thread_progress[i].mutex);
    }
    free(thread_progress);
    free(all_thread_progress);
    free(contexts);
    free(threads);
    if (tracker.thread_estimates) {
        free(tracker.thread_estimates);
    }
    free_partitioning_result(partitions);
    pthread_mutex_destroy(&results_mutex);
    pthread_mutex_destroy(&progress_mutex);
    free(algorithms);
    cleanup_algorithm_registry();
    clear_field_cache();
    
    return true;
}

