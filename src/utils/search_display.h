#ifndef SEARCH_DISPLAY_H
#define SEARCH_DISPLAY_H

#include "../../include/cads_types.h"
#include "../../include/cads_config_loader.h"
#include "../core/packet_data.h"
#include "hardware_benchmark.h"
#include <pthread.h>
#include <time.h>

// Forward declare thread progress structure from threading engine
typedef struct {
    uint64_t tests_performed;
    double current_rate;
    time_t last_update;
    time_t start_time;  // For elapsed time calculation
    bool completed;     // Whether this thread has finished its assigned work
    int solutions_found; // Number of solutions this specific thread found
    pthread_mutex_t mutex;
} thread_progress_t;

// Display search space estimation with hardware-calibrated complexity emojis
void display_search_estimation(const packet_dataset_t* dataset, 
                              const config_t* config,
                              int algorithm_count,
                              const hardware_benchmark_result_t* benchmark);

// CADS-specific version that works with config_t
void display_search_estimation_cads(const config_t* config,
                                   int algorithm_count,
                                   const hardware_benchmark_result_t* benchmark);

// Multi-thread progress display
void display_per_thread_progress(thread_progress_t** all_progress, int num_threads, progress_tracker_t* tracker);

#endif // SEARCH_DISPLAY_H
