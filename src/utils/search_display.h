#ifndef SEARCH_DISPLAY_H
#define SEARCH_DISPLAY_H

#include "../../include/cads_types.h"
#include "../../include/cads_config_loader.h"
#include "../core/packet_data.h"
#include "hardware_benchmark.h"

// Display search space estimation with hardware-calibrated complexity emojis
void display_search_estimation(const packet_dataset_t* dataset, 
                              const config_t* config,
                              int algorithm_count,
                              const hardware_benchmark_result_t* benchmark);

// CADS-specific version that works with config_t
void display_search_estimation_cads(const config_t* config,
                                   int algorithm_count,
                                   const hardware_benchmark_result_t* benchmark);

#endif // SEARCH_DISPLAY_H
