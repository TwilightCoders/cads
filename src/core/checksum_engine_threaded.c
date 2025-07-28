#include "checksum_engine.h"
#include "../../include/cads_config_loader.h"
#include "../../include/algorithm_registry.h"
#include "../utils/field_combiner.h"
#include "../utils/search_display.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Unified search engine - handles both single and multi-threaded execution
bool execute_checksum_search_threaded(const cads_config_file_t* config, 
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
    
    // Display threading info if verbose
    if (config->verbose) {
        if (actual_threads == 1) {
            printf("🔄 Single-threaded execution\n");
        } else {
            printf("🧵 Multi-threaded execution: %d threads\n", actual_threads);
        }
    }
    
    // Initialize algorithm registry for estimation
    if (!initialize_algorithm_registry()) {
        return false;
    }
    
    // Get algorithm count for estimation
    int algorithm_count;
    if (config->custom_operation_count > 0 && config->custom_operations) {
        algorithm_count = config->custom_operation_count;
    } else {
        const algorithm_registry_entry_t* algorithms = get_algorithms_by_complexity(config->complexity, &algorithm_count);
        (void)algorithms; // Suppress unused variable warning
    }
    
    // Display search space estimation with complexity emojis (only if verbose)
    if (config->verbose) {
        display_search_estimation_cads(config, algorithm_count, benchmark);
    }
    
    // Execute with unified threading architecture
    if (actual_threads == 1) {
        // Single-threaded execution (base case)
        progress_tracker_t tracker;
        return execute_checksum_search(config, results, &tracker);
    } else {
        // Multi-threaded execution
        // TODO: Implement true parallel work distribution
        if (config->verbose) {
            printf("⚠️  Note: Multi-threading work distribution in progress - using single-threaded execution\n\n");
        }
        progress_tracker_t tracker;
        return execute_checksum_search(config, results, &tracker);
    }
}
