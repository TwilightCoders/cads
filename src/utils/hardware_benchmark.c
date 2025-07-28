#include "hardware_benchmark.h"
#include "../../include/algorithm_registry.h"
#include "../core/packet_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

// Standard benchmark packet for consistent testing across all machines
static const uint8_t BENCHMARK_PACKET[] = {0x9c, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
// static const size_t BENCHMARK_PACKET_SIZE = 8; // Removed unused constant
static const uint64_t BENCHMARK_EXPECTED_CHECKSUM = 0x42; // Dummy value

// Quick hardware benchmark - runs for ~1 second to measure baseline performance
hardware_benchmark_result_t run_hardware_benchmark(void) {
    hardware_benchmark_result_t result = {0};
    
    printf("⚡ Initializing search engine [");
    fflush(stdout);
    
    // Initialize algorithm registry for benchmark
    if (!initialize_algorithm_registry()) {
        printf(" FAILED!\n");
        return result;
    }
    
    // Get basic operations for consistent benchmarking
    int algorithm_count;
    const algorithm_registry_entry_t* algorithms = get_algorithms_by_complexity(COMPLEXITY_BASIC, &algorithm_count);
    if (!algorithms || algorithm_count == 0) {
        printf(" FAILED!\n");
        cleanup_algorithm_registry();
        return result;
    }
    
    // Run benchmark test for ~5 seconds with realistic progress bar
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    
    uint64_t tests_performed = 0;
    uint64_t benchmark_duration_us = 5000000; // 5 seconds in microseconds
    int progress = 0;
    // int last_progress_shown = -1; // Removed unused variable
    
    // Simulate realistic loading with random progress updates
    do {
        // Do actual benchmark work that mirrors real search engine operations
        for (int field1 = 0; field1 < 4; field1++) {
            for (int field2 = field1 + 1; field2 < 4; field2++) {
                for (int alg_idx = 0; alg_idx < algorithm_count; alg_idx++) {
                    for (int constant = 0; constant < 16; constant++) {
                        // Simulate real field extraction with bounds checking and masking
                        uint64_t val1 = 0, val2 = 0;
                        if (field1 < 8) val1 = BENCHMARK_PACKET[field1];
                        if (field2 < 8) val2 = BENCHMARK_PACKET[field2];
                        
                        // Simulate algorithm registry lookup (real overhead)
                        const algorithm_registry_entry_t* entry = &algorithms[alg_idx];
                        if (!entry->func) continue;
                        
                        // Apply actual algorithm with real function call overhead
                        uint64_t calculated = entry->func(val1, val2, constant);
                        
                        // Simulate real checksum size masking
                        calculated = calculated & 0xFF; // 1-byte checksum
                        
                        // Simulate packet validation loop (real search does this for all packets)
                        volatile bool all_match = true;
                        for (int pkt = 0; pkt < 4; pkt++) {
                            // Simulate extracting fields from multiple packets
                            uint64_t pkt_val1 = BENCHMARK_PACKET[pkt % 8];
                            uint64_t pkt_val2 = BENCHMARK_PACKET[(pkt + 1) % 8];
                            uint64_t pkt_calc = entry->func(pkt_val1, pkt_val2, constant) & 0xFF;
                            if (pkt_calc != BENCHMARK_EXPECTED_CHECKSUM) {
                                all_match = false;
                            }
                        }
                        (void)all_match; // Suppress unused warning
                        
                        tests_performed++;
                    }
                }
            }
        }
        
        gettimeofday(&current_time, NULL);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                        (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
        
        // Update progress bar based on time, not test count
        static double last_update_time = 0;
        if (elapsed > last_update_time + 0.2) { // Update every ~200ms
            // Random progress increment between 3-15 to make it visible
            int random_increment = 3 + (rand() % 13);
            progress += random_increment;
            if (progress > 100) progress = 100;
            
            // Show progress bar every update
            int bars = progress / 5; // 20 bars total (100/5)
            printf("\r⚡ Initializing search engine [");
            for (int i = 0; i < 20; i++) {
                if (i < bars) printf("█");
                else printf("░");
            }
            printf("] %d%%", progress);
            fflush(stdout);
            
            last_update_time = elapsed;
            
            // Exit immediately when we hit 100% - no lingering!
            if (progress >= 100) break;
        }
        
    } while (((uint64_t)(current_time.tv_sec - start_time.tv_sec) * 1000000 + 
             (uint64_t)(current_time.tv_usec - start_time.tv_usec)) < benchmark_duration_us);
    
    // Ensure we end at 100%
    printf("\r⚡ Initializing search engine [████████████████████] 100%% ✅\n");
    
    // Calculate final results
    gettimeofday(&current_time, NULL);
    uint64_t actual_duration_us = (current_time.tv_sec - start_time.tv_sec) * 1000000 + 
                                  (current_time.tv_usec - start_time.tv_usec);
    
    result.tests_performed = tests_performed;
    result.duration_us = actual_duration_us;
    // Apply calibration factor to match real search engine performance (empirically determined)
    double raw_performance = (double)tests_performed * 1000000.0 / (double)actual_duration_us;
    result.tests_per_second = raw_performance * 0.15; // Calibration factor ~15% of benchmark
    result.valid = true;
    
    // Debug output to verify benchmark results
    printf("   Hardware baseline: %.1fM tests/sec (%llu tests in %.2fs)\n\n", 
           result.tests_per_second / 1000000.0, 
           (unsigned long long)tests_performed,
           actual_duration_us / 1000000.0);
    
    cleanup_algorithm_registry();
    
    return result;
}

// Calculate time-based complexity emojis using hardware baseline
const char* get_time_based_complexity_emojis(uint64_t estimated_tests, 
                                            double baseline_tests_per_second,
                                            char* emoji_buffer,
                                            char* time_estimate_buffer) {
    
    // Calculate estimated completion time in seconds
    double estimated_seconds = (double)estimated_tests / baseline_tests_per_second;
    
    // Format time estimate
    if (estimated_seconds < 60) {
        snprintf(time_estimate_buffer, 64, "~%.0fs", estimated_seconds);
    } else if (estimated_seconds < 3600) {
        snprintf(time_estimate_buffer, 64, "~%.0fm", estimated_seconds / 60.0);
    } else if (estimated_seconds < 86400) {
        snprintf(time_estimate_buffer, 64, "~%.1fh", estimated_seconds / 3600.0);
    } else {
        snprintf(time_estimate_buffer, 64, "~%.1fd", estimated_seconds / 86400.0);
    }
    
    // Time-based emoji scale (data-driven approach)
    typedef struct {
        double max_seconds;
        const char* emoji;
    } time_emoji_scale_t;
    
    static const time_emoji_scale_t time_scales[] = {
        {60,        "🐰"},    // < 1 minute
        {300,       "✨"},    // 1-5 minutes  
        {900,       "⏰"},    // 5-15 minutes
        {3600,      "⏳"},    // 15-60 minutes
        {10800,     "🕐"},    // 1-3 hours
        {28800,     "😴"},    // 3-8 hours
        {86400,     "🌙"},    // 8-24 hours
        {259200,    "😳"},    // 1-3 days
        {604800,    "😱"},    // 3-7 days (~1 week)
        {2419200,   "🥵"},    // 1-4 weeks
        {7776000,   "😨"},    // 1-3 months  
        {31536000,  "🥱"},    // 3-12 months
        {94608000,  "😵‍💫"},  // 1-3 years
        {315360000, "🥴"},    // 3-10 years
        {3153600000,"💀"},    // 10-100 years
        {0,         "🤯"}     // 100+ years (exploding heads with power-of-2)
    };
    
    // Find appropriate emoji scale
    const char* emoji_type = "🤯";
    int emoji_count = 1;
    
    for (size_t i = 0; i < sizeof(time_scales) / sizeof(time_scales[0]); i++) {
        if (time_scales[i].max_seconds == 0 || estimated_seconds < time_scales[i].max_seconds) {
            emoji_type = time_scales[i].emoji;
            break;
        }
    }
    
    // For 100+ years, calculate exploding head count using powers of 2
    if (estimated_seconds >= 3153600000) {
        double centuries = estimated_seconds / 3153600000.0;
        // Use log2 to calculate power-of-2 scaling: 100yr=1🤯, 200yr=2🤯, 400yr=3🤯, etc.
        emoji_count = 1 + (int)(log2(centuries));
        if (emoji_count > 10) emoji_count = 10; // Cap at 10 emojis
    }
    
    // Build emoji string
    emoji_buffer[0] = '\0';
    for (int i = 0; i < emoji_count && i < 10; i++) {
        strcat(emoji_buffer, emoji_type);
    }
    
    return emoji_buffer;
}