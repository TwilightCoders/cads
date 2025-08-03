#include "search_display.h"
#include "hardware_benchmark.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

// Forward declaration
static const char* format_number_with_emojis(uint64_t tests, char* emoji_buffer, 
                                            double baseline_tests_per_second);

// Display search space estimation with hardware-calibrated complexity emojis
void display_search_estimation(const packet_dataset_t* dataset, 
                              const config_t* config,
                              int algorithm_count,
                              const hardware_benchmark_result_t* benchmark) {
    
    if (!dataset || !config) return;
    
    // Calculate minimum packet length
    size_t min_packet_length = dataset->packets[0].packet_length;
    for (size_t i = 1; i < dataset->count; i++) {
        if (dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = dataset->packets[i].packet_length;
        }
    }
    
    // Calculate search space estimation (same logic as recursive engine)
    uint64_t permutations = 1;
    for (int i = 0; i < config->max_fields && i < (int)min_packet_length; i++) {
        permutations *= (min_packet_length - i);
    }
    
    // FIXED: Calculate for ALL complexity levels like original, not just max_fields
    uint64_t operation_sequences = 0;
    
    // Sum operation sequences across all complexity levels (1 to max_fields)
    for (int complexity = 1; complexity <= config->max_fields; complexity++) {
        uint64_t ops_for_complexity = 1;
        // Allow field_count+1 operations for complex patterns
        for (int i = 0; i < complexity + 1; i++) {
            ops_for_complexity *= algorithm_count;
        }
        operation_sequences += ops_for_complexity;
    }
    
    uint64_t estimated_tests = permutations * operation_sequences * config->max_constants;
    
    // Format the number with hardware-calibrated complexity emojis
    char emoji_buffer[64];
    char time_estimate_buffer[64];
    double baseline_tests_per_second = benchmark && benchmark->valid ? benchmark->tests_per_second : 15000000.0; // Default fallback
    const char* formatted_tests = format_number_with_emojis(estimated_tests, emoji_buffer, baseline_tests_per_second);
    
    printf("🔢 Search Space Estimation:\n");
    printf("   Field permutations: %llu\n", (unsigned long long)permutations);
    printf("   Operation sequences: %llu\n", (unsigned long long)operation_sequences);
    printf("   Constants to test: %d\n", config->max_constants);
    
    if (benchmark && benchmark->valid) {
        get_time_based_complexity_emojis(estimated_tests, baseline_tests_per_second, emoji_buffer, time_estimate_buffer);
        printf("   📊 Total domain size: %s tests %s %s\n", formatted_tests, emoji_buffer, time_estimate_buffer);
        printf("   💻 Hardware baseline: %.1fM tests/sec\n\n", baseline_tests_per_second / 1000000.0);
    } else {
        printf("   📊 Total domain size: %s tests %s\n\n", formatted_tests, emoji_buffer);
    }
}

// CADS-specific version that works with config_t
void display_search_estimation_cads(const config_t* config,
                                   int algorithm_count,
                                   const hardware_benchmark_result_t* benchmark) {
    
    if (!config || !config->dataset) return;
    
    const packet_dataset_t* dataset = config->dataset;
    
    // Calculate minimum packet length
    size_t min_packet_length = dataset->packets[0].packet_length;
    for (size_t i = 1; i < dataset->count; i++) {
        if (dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = dataset->packets[i].packet_length;
        }
    }
    
    // Calculate search space estimation
    uint64_t permutations = 1;
    for (int i = 0; i < config->max_fields && i < (int)min_packet_length; i++) {
        permutations *= (min_packet_length - i);
    }
    
    // FIXED: Calculate for ALL complexity levels like original, not just max_fields
    uint64_t operation_sequences = 0;
    
    // Sum operation sequences across all complexity levels (1 to max_fields)
    for (int complexity = 1; complexity <= config->max_fields; complexity++) {
        uint64_t ops_for_complexity = 1;
        // Allow field_count+1 operations for complex patterns
        for (int i = 0; i < complexity + 1; i++) {
            ops_for_complexity *= algorithm_count;
        }
        operation_sequences += ops_for_complexity;
    }
    
    uint64_t estimated_tests = permutations * operation_sequences * config->max_constants;
    
    // Format the number with hardware-calibrated complexity emojis
    char emoji_buffer[64];
    char time_estimate_buffer[64];
    double baseline_tests_per_second = benchmark && benchmark->valid ? benchmark->tests_per_second : 15000000.0; // Default fallback
    const char* formatted_tests = format_number_with_emojis(estimated_tests, emoji_buffer, baseline_tests_per_second);
    
    printf("🔢 Search Space Estimation:\n");
    printf("   Field permutations: %llu\n", (unsigned long long)permutations);
    printf("   Operation sequences: %llu\n", (unsigned long long)operation_sequences);
    printf("   Constants to test: %d\n", config->max_constants);
    
    if (benchmark && benchmark->valid) {
        get_time_based_complexity_emojis(estimated_tests, baseline_tests_per_second, emoji_buffer, time_estimate_buffer);
        printf("   📊 Total domain size: %s tests %s %s\n", formatted_tests, emoji_buffer, time_estimate_buffer);
        printf("   💻 Hardware baseline: %.1fM tests/sec\n\n", baseline_tests_per_second / 1000000.0);
    } else {
        printf("   📊 Total domain size: %s tests %s\n\n", formatted_tests, emoji_buffer);
    }
}

// Format number with suffix (emojis now handled by get_time_based_complexity_emojis)
static const char* format_number_with_emojis(uint64_t tests, char* emoji_buffer, 
                                            double baseline_tests_per_second) {
    (void)baseline_tests_per_second; // Suppress unused parameter warning
    static char number_buffer[32];
    
    // Format number with suffix only (emojis handled elsewhere)
    if (tests >= 1000000000000ULL) {
        snprintf(number_buffer, sizeof(number_buffer), "%.1fT", tests / 1000000000000.0);
    } else if (tests >= 1000000000ULL) {
        snprintf(number_buffer, sizeof(number_buffer), "%.1fB", tests / 1000000000.0);
    } else if (tests >= 1000000ULL) {
        snprintf(number_buffer, sizeof(number_buffer), "%.1fM", tests / 1000000.0);
    } else if (tests >= 1000ULL) {
        snprintf(number_buffer, sizeof(number_buffer), "%.1fK", tests / 1000.0);
    } else {
        snprintf(number_buffer, sizeof(number_buffer), "%llu", (unsigned long long)tests);
    }
    
    // Clear emoji buffer (not used in this function anymore)
    emoji_buffer[0] = '\0';
    
    return number_buffer;
}
