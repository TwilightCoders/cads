#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "include/checksum_engine.h"
#include "src/core/packet_data.h"
#include "src/core/progress_tracker.h"
#include "src/utils/config.h"

// High precision timing
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main() {
    printf("🔬 CADS Performance Profile\n");
    printf("===========================\n");
    
    // Load the MXT275 dataset
    packet_dataset_t* dataset = create_packet_dataset(20);
    if (!dataset) {
        printf("❌ Failed to create dataset\n");
        return 1;
    }
    
    bool load_success = load_packets_from_json(dataset, "tests/data/gmrs_test_dataset.jsonl");
    if (!load_success) {
        printf("❌ Failed to load dataset from JSON\n");
        free_packet_dataset(dataset);
        return 1;
    }
    
    printf("📦 Dataset loaded: %zu packets\n", dataset->count);
    
    // Configure for a pure performance test (no solutions expected)
    operation_t performance_operations[] = {
        OP_ONES_COMPLEMENT, OP_CONST_ADD  // These shouldn't produce trivial solutions
    };
    
    config_t config = create_custom_operation_config(performance_operations, 2);
    config.max_fields = 6;         // Larger search space for profiling
    config.max_constants = 64;     // Reasonable constant range
    config.dataset = dataset;
    disable_early_exit(&config);   // Test full performance without early termination
    set_progress_interval(&config, 1000);  // Less frequent updates for cleaner profiling
    
    search_results_t* results = create_search_results(10);
    config.threads = 1; // single-threaded profile baseline
    
    double start_time = get_time_ms();
    
    printf("🏁 Starting search...\n");
    bool success = execute_weighted_checksum_search(&config, results, NULL);
    
    double end_time = get_time_ms();
    double elapsed = end_time - start_time;
    
    printf("\n📊 Profile Results:\n");
    printf("Success: %s\n", success ? "✅" : "❌");
    printf("Tests performed: %llu\n", (unsigned long long)results->tests_performed);
    printf("Solutions found: %zu\n", results->solution_count);
    printf("Total time: %.2f ms\n", elapsed);
    printf("Tests per second: %.0f\n", results->tests_performed / (elapsed / 1000.0));
    
    if (results->solution_count > 0) {
        printf("Rate: %.2f ms per test\n", elapsed / results->tests_performed);
    }
    
    free_search_results(results);
    free_packet_dataset(dataset);
    
    return 0;
}
