#ifndef ALGORITHM_REGISTRY_H
#define ALGORITHM_REGISTRY_H

#include "cads_types.h"

// Algorithm function pointer type for unified interface
typedef uint64_t (*algorithm_func_t)(uint64_t a, uint64_t b, uint64_t constant);

// Extended algorithm info with function pointer
typedef struct {
    operation_t op;
    complexity_level_t complexity;
    const char* name;
    const char* description;
    bool requires_constant;
    algorithm_func_t func;             // Function pointer for execution
    double avg_ops_per_second;         // Performance estimate
} algorithm_registry_entry_t;

// Registry management functions
bool initialize_algorithm_registry(void);
void cleanup_algorithm_registry(void);

// Get algorithms by complexity level
const algorithm_registry_entry_t* get_algorithms_by_complexity(complexity_level_t complexity, int* count);
const algorithm_registry_entry_t* get_all_algorithms(int* count);

// Get specific algorithm info
const algorithm_registry_entry_t* get_algorithm_by_operation(operation_t op);
const char* get_complexity_name(complexity_level_t complexity);

// Performance estimation
uint64_t estimate_total_combinations(const search_config_t* config, size_t packet_count);
double estimate_completion_time(const search_config_t* config, size_t packet_count);

// Algorithm execution wrapper
uint64_t execute_algorithm(operation_t op, uint64_t a, uint64_t b, uint64_t constant);

// Complexity level statistics
typedef struct {
    complexity_level_t level;
    const char* name;
    int algorithm_count;
    double avg_ops_per_second;
    const char* description;
} complexity_stats_t;

const complexity_stats_t* get_complexity_stats(int* count);

#endif // ALGORITHM_REGISTRY_H