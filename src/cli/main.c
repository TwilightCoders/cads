#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/checksum_engine.h"
#include "../../include/cads_config_loader.h"
#include "../../include/algorithm_registry.h"
#include "../utils/search_display.h"
#include "../utils/hardware_benchmark.h"

void print_usage(const char* program_name) {
    printf("CADS - Checksum Algorithm Discovery System v1-beta\n");
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
    printf("  -t, --threading        Enable multi-threaded search\n");
    printf("  -T, --threads N        Number of threads (default: auto-detect)\n");
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
    
    printf("  # Multi-threaded analysis (faster):\n");
    printf("  %s -C examples/mxt275_discovery.cads -t\n\n", program_name);
    
    printf("Packet Data Format (JSON Lines):\n");
    printf("  {\"packet\": \"9c30010000000000\", \"checksum\": \"31\", \"description\": \"CH1\"}\n");
    printf("  {\"packet\": \"9c30020000000000\", \"checksum\": \"32\", \"description\": \"CH2\"}\n\n");
}

int main(int argc, char* argv[]) {
    
    // UNIFIED CONFIGURATION: Always use config_t internally
    config_t* config = create_cads_config_from_cli(argc, argv);
    if (!config) {
        // Help was requested or error occurred
        if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
            print_usage(argv[0]);
            return 0;
        }
        return 1;
    }
    
    // Show header only in verbose mode
    if (config->verbose) {
        printf("🔍 CADS - Checksum Algorithm Discovery System v1-beta\n");
        printf("===================================================\n\n");
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
    
    if (config->verbose) {
        printf("✅ Loaded %zu packets successfully\n\n", config->dataset->count);
    }
    
    // Create search results 
    search_results_t* results = create_search_results(50);
    if (!results) {
        fprintf(stderr, "❌ Error: Failed to create search results\n");
        free_cads_config(config);
        return 1;
    }
    
    // Run hardware benchmark only if we need to display search estimation
    hardware_benchmark_result_t benchmark = {0};
    bool need_estimation = config->verbose; // Only run benchmark if verbose mode is on
    
    if (need_estimation) {
        benchmark = run_hardware_benchmark();
    }
    
    // Execute checksum search (threaded or single-threaded)
    if (config->verbose) {
        printf("🚀 Starting checksum algorithm discovery...\n\n");
    }
    // Always use unified threaded engine (handles both single and multi-threaded execution)
    bool search_success = execute_checksum_search_threaded(config, results, &benchmark);
    
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
