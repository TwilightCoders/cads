#ifndef HARDWARE_BENCHMARK_H
#define HARDWARE_BENCHMARK_H

#include <stdint.h>
#include <stdbool.h>

// Hardware benchmark result structure
typedef struct {
    uint64_t tests_performed;      // Number of tests completed in benchmark
    uint64_t duration_us;          // Actual benchmark duration in microseconds  
    double tests_per_second;       // Calculated baseline performance
    bool valid;                    // Whether benchmark completed successfully
} hardware_benchmark_result_t;

// Run a 1-second hardware benchmark to establish baseline performance
hardware_benchmark_result_t run_hardware_benchmark(void);

// Calculate time-based complexity emojis using hardware baseline
const char* get_time_based_complexity_emojis(uint64_t estimated_tests, 
                                            double baseline_tests_per_second,
                                            char* emoji_buffer,
                                            char* time_estimate_buffer);

#endif // HARDWARE_BENCHMARK_H