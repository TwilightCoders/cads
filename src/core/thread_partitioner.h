#ifndef THREAD_PARTITIONER_H
#define THREAD_PARTITIONER_H

#include "../../include/cads_types.h"
#include "../../include/algorithm_registry.h"
#include <limits.h>

#define MAX_OPERATIONS_PER_THREAD 32

typedef struct {
    int thread_id;
    operation_t assigned_operations[MAX_OPERATIONS_PER_THREAD];
    int num_assigned_operations;
    int total_weight;
} thread_partition_t;

typedef struct {
    thread_partition_t* partitions;
    int num_threads;
    int total_operations;
    int total_weight;
} partitioning_result_t;

// Function declarations
partitioning_result_t* create_weighted_partitions(
    const algorithm_registry_entry_t* algorithms, 
    int algorithm_count, 
    int num_threads
);

void free_partitioning_result(partitioning_result_t* result);

void print_partition_summary(const partitioning_result_t* result);

// Bin packing algorithm - distribute operations to minimize max thread weight
bool balance_thread_weights(partitioning_result_t* result);

#endif