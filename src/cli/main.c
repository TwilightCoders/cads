#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/checksum_engine.h"
#include "../../include/cads_config_loader.h"

void print_usage(const char* program_name) {
    printf("CADS - Checksum Algorithm Discovery System v4.0\n");
    printf("Reverse engineering radio communication protocols through exhaustive checksum analysis.\n\n");
    
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    
    printf("Options:\n");
    printf("  -i, --input FILE       Input packet data file (JSON Lines format)\n");
    printf("  -C, --config FILE      Load configuration and data from .cads file\n");
    printf("  -c, --complexity LEVEL Complexity level: basic, intermediate, advanced (default: intermediate)\n");
    printf("  -f, --max-fields N     Maximum fields to test (default: 4)\n");
    printf("  -k, --max-constants N  Maximum constant values (default: 128)\n");
    printf("  -e, --early-exit       Exit after finding first solution\n");
    printf("  -m, --max-solutions N  Maximum solutions to find (default: unlimited)\n");
    printf("  -p, --progress-ms N    Progress update interval in ms (default: 250)\n");
    printf("  -v, --verbose          Verbose output\n");
    printf("  -h, --help             Show this help message\n\n");
    
    printf("Examples:\n");
    printf("  # Use .cads config file (recommended):\n");
    printf("  %s -C examples/mxt275_discovery.cads\n\n", program_name);
    
    printf("  # Discover MXT275 radio checksum:\n");
    printf("  %s -i tests/data/mxt275_uart_checksum.jsonl -c intermediate -f 5 -e\n\n", program_name);
    
    printf("  # Discover GMRS radio checksum (fast):\n");
    printf("  %s -i tests/data/gmrs_test_dataset.jsonl -c basic -f 3 -e\n\n", program_name);
    
    printf("  # Thorough analysis (find all solutions):\n");
    printf("  %s -i tests/data/gmrs_test_dataset.jsonl -c advanced -f 6 -k 256\n\n", program_name);
    
    printf("Packet Data Format (JSON Lines):\n");
    printf("  {\"packet\": \"9c30010000000000\", \"checksum\": \"31\", \"description\": \"CH1\"}\n");
    printf("  {\"packet\": \"9c30020000000000\", \"checksum\": \"32\", \"description\": \"CH2\"}\n\n");
}

int main(int argc, char* argv[]) {
    printf("🔍 CADS - Checksum Algorithm Discovery System v4.0\n");
    printf("===================================================\n\n");
    
    // UNIFIED CONFIGURATION: Always use cads_config_file_t internally
    cads_config_file_t* config = create_cads_config_from_cli(argc, argv);
    if (!config) {
        // Help was requested or error occurred
        if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
            print_usage(argv[0]);
            return 0;
        }
        return 1;
    }
    
    // Display configuration info
    if (config->verbose) {
        printf("📋 Configuration: %s\n", config->name ? config->name : "Unnamed");
        if (config->description) {
            printf("   Description: %s\n", config->description);
        }
        printf("   Complexity: %s\n", 
               config->complexity == COMPLEXITY_BASIC ? "Basic" :
               config->complexity == COMPLEXITY_INTERMEDIATE ? "Intermediate" : "Advanced");
        printf("   Max fields: %d\n", config->max_fields);
        printf("   Max constants: %d\n", config->max_constants);
        printf("   Early exit: %s\n", config->early_exit ? "Yes" : "No");
        printf("   Progress interval: %dms\n\n", config->progress_interval);
    }
    
    // Verify we have packet data
    if (!config->dataset || config->dataset->count == 0) {
        fprintf(stderr, "❌ Error: No packet data loaded\n");
        free_cads_config(config);
        return 1;
    }
    
    printf("✅ Loaded %zu packets successfully\n\n", config->dataset->count);
    
    // Convert unified config to search config
    search_config_t search_config = create_search_config_from_cads(config);
    
    // Create search results and progress tracker
    search_results_t* results = create_search_results(50);
    if (!results) {
        fprintf(stderr, "❌ Error: Failed to create search results\n");
        free_cads_config(config);
        return 1;
    }
    
    progress_tracker_t tracker;
    
    // Execute checksum search
    printf("🚀 Starting checksum algorithm discovery...\n\n");
    bool search_success = execute_checksum_search(config->dataset, &search_config, results, &tracker);
    
    if (!search_success) {
        fprintf(stderr, "❌ Error: Checksum search failed\n");
        free_search_results(results);
        free_cads_config(config);
        return 1;
    }
    
    // Display results summary
    printf("\n🎯 DISCOVERY RESULTS SUMMARY\n");
    printf("============================\n");
    printf("Tests performed: %llu\n", (unsigned long long)results->tests_performed);
    printf("Solutions found: %zu\n", results->solution_count);
    printf("Search completed: %s\n", results->search_completed ? "Yes" : "Interrupted");
    
    if (results->solution_count > 0) {
        printf("\n🏆 DISCOVERED ALGORITHMS:\n");
        for (size_t i = 0; i < results->solution_count; i++) {
            const checksum_solution_t* solution = &results->solutions[i];
            printf("\n   Solution #%zu:\n", i + 1);
            printf("     Fields: ");
            for (int f = 0; f < solution->field_count; f++) {
                printf("%d ", solution->field_indices[f]);
            }
            printf("\n     Operations: %d total\n", solution->operation_count);
            printf("     Constant: 0x%02llX\n", (unsigned long long)solution->constant);
            printf("     Checksum size: %zu bytes\n", solution->checksum_size);
            printf("     Validated: %s\n", solution->validated ? "✅" : "❌");
        }
        printf("\n✅ SUCCESS: Algorithm(s) discovered successfully!\n");
    } else {
        printf("\n⚠️  No algorithms discovered with current parameters.\n");
        printf("💡 Try increasing complexity level or field count\n");
    }
    
    // Cleanup
    free_search_results(results);
    free_cads_config(config);
    
    return results->solution_count > 0 ? 0 : 1;
}