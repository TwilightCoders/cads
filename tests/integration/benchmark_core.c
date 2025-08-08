#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "../../include/checksum_engine.h"
#include "../../src/core/packet_data.h"
#include "../../src/core/progress_tracker.h"
#include "../../src/utils/config.h"

// High precision timing
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main() {
    printf("🚀 CADS Core Performance Benchmark\n");
    printf("===================================\n");
    
    // Load the dataset
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
    
    // Multiple test configurations to find peak performance
    struct {
        const char* name;
        int max_fields;
        int max_constants;
        operation_t operations[3];
        int op_count;
    } test_configs[] = {
        {"Ultra Fast", 3, 16, {OP_IDENTITY, OP_ADD}, 2},
        {"Fast", 4, 32, {OP_IDENTITY, OP_ADD}, 2}, 
        {"Medium", 5, 64, {OP_IDENTITY, OP_ADD}, 2},
        {"Peak Test", 6, 64, {OP_ONES_COMPLEMENT, OP_CONST_ADD}, 2},
        {"Complex", 4, 128, {OP_ADD, OP_XOR, OP_ONES_COMPLEMENT}, 3}
    };
    
    int num_configs = sizeof(test_configs) / sizeof(test_configs[0]);
    
    printf("\n🔥 Running %d performance configurations...\n\n", num_configs);
    
    double best_rate = 0.0;
    const char* best_config = "";
    
    for (int i = 0; i < num_configs; i++) {
        printf("⚡ Test %d: %s Configuration\n", i+1, test_configs[i].name);
        printf("   Fields: %d, Constants: %d, Operations: %d\n", 
               test_configs[i].max_fields, test_configs[i].max_constants, test_configs[i].op_count);
        
        // Create configuration
        config_t config = create_custom_operation_config(test_configs[i].operations, test_configs[i].op_count);
        config.max_fields = test_configs[i].max_fields;
        config.max_constants = test_configs[i].max_constants;
        config.dataset = dataset;
        disable_early_exit(&config);   // Find all solutions
        set_progress_interval(&config, 5000);  // Minimal progress updates
        
        search_results_t* results = create_search_results(10);
    // Single-threaded for fair perf isolation (engine unifies paths)
    config.threads = 1;
        
    // Measure just the core search
    double start_time = get_time_ms();
    bool success = execute_weighted_checksum_search(&config, results, NULL);
        double end_time = get_time_ms();
        
        double elapsed = end_time - start_time;
        double rate = results->tests_performed / (elapsed / 1000.0);
        
        printf("   Results: %llu tests in %.1fms = %.1fM tests/sec\n", 
               (unsigned long long)results->tests_performed, elapsed, rate / 1000000.0);
        
        if (rate > best_rate) {
            best_rate = rate;
            best_config = test_configs[i].name;
        }
        
        free_search_results(results);
        printf("\n");
    }
    
    printf("🏆 PEAK PERFORMANCE: %.1fM tests/sec (%s configuration)\n", 
           best_rate / 1000000.0, best_config);
    
    // Now run the specific configuration that should hit 32M+
    printf("\n🎯 Reproducing 32M+ tests/sec benchmark...\n");
    
    operation_t peak_ops[] = {OP_ONES_COMPLEMENT, OP_CONST_ADD};
    config_t peak_config = create_custom_operation_config(peak_ops, 2);
    peak_config.max_fields = 6;
    peak_config.max_constants = 64;
    peak_config.dataset = dataset;
    disable_early_exit(&peak_config);
    set_progress_interval(&peak_config, 10000);  // Very minimal updates
    
    search_results_t* peak_results = create_search_results(10);
    peak_config.threads = 1;
    
    double peak_start = get_time_ms();
    bool peak_success = execute_weighted_checksum_search(&peak_config, peak_results, NULL);
    double peak_end = get_time_ms();
    
    double peak_elapsed = peak_end - peak_start;
    double peak_rate = peak_results->tests_performed / (peak_elapsed / 1000.0);
    
    printf("📊 Peak Benchmark Results:\n");
    printf("   Tests: %llu\n", (unsigned long long)peak_results->tests_performed);
    printf("   Time: %.1fms\n", peak_elapsed);
    printf("   Rate: %.1fM tests/sec\n", peak_rate / 1000000.0);
    printf("   Search Space: 6 fields × 64 constants × 2 operations\n");
    
    if (peak_rate >= 30000000.0) {
        printf("✅ SUCCESS: Achieved 30M+ tests/sec target!\n");
    } else {
        printf("⚠️  Rate below 30M target. Factors:\n");
        printf("   - Progress tracking overhead\n");
        printf("   - Test harness overhead\n");
        printf("   - Solution discovery printing\n");
    }
    
    free_search_results(peak_results);
    free_packet_dataset(dataset);
    
    return 0;
}
