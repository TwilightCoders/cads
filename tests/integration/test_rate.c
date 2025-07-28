#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "../../include/checksum_engine.h"
#include "../../src/core/packet_data.h"
#include "../../src/core/progress_tracker.h"
#include "../../src/utils/config.h"

int main() {
    printf("🔬 Rate Calculation Test\n");
    printf("========================\n");
    
    packet_dataset_t* dataset = create_packet_dataset(20);
    if (!dataset) {
        printf("❌ Failed to create dataset\n");
        return 1;
    }
    
    bool load_success = load_packets_from_json(dataset, "data/gmrs_test_dataset.jsonl");
    if (!load_success) {
        printf("❌ Failed to load dataset from JSON\n");
        free_packet_dataset(dataset);
        return 1;
    }
    
    operation_t ops[] = {OP_IDENTITY, OP_ADD};
    
    config_t config = create_custom_operation_config(ops, 2);
    config.max_fields = 2;
    config.max_constants = 4;
    config.dataset = dataset;
    disable_early_exit(&config);  // Test wants to find all solutions
    set_progress_interval(&config, 250);
    
    search_results_t* results = create_search_results(10);
    progress_tracker_t tracker;
    
    printf("🏁 Starting rate test...\n");
    bool success = execute_checksum_search(&config, results, &tracker);
    
    printf("\n📊 Rate Test Results:\n");
    printf("Success: %s\n", success ? "✅" : "❌");
    printf("Tests performed: %llu\n", (unsigned long long)results->tests_performed);
    printf("Solutions found: %zu\n", results->solution_count);
    
    free_search_results(results);
    free_packet_dataset(dataset);
    
    return 0;
}
