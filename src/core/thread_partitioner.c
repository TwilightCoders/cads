#include "thread_partitioner.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Create weighted partitions using bin packing algorithm
partitioning_result_t* create_weighted_partitions(
    const algorithm_registry_entry_t* algorithms, 
    int algorithm_count, 
    int num_threads) {
    
    if (!algorithms || algorithm_count == 0 || num_threads <= 0) {
        return NULL;
    }
    
    // Allocate result structure
    partitioning_result_t* result = malloc(sizeof(partitioning_result_t));
    if (!result) return NULL;
    
    result->partitions = calloc(num_threads, sizeof(thread_partition_t));
    if (!result->partitions) {
        free(result);
        return NULL;
    }
    
    result->num_threads = num_threads;
    result->total_operations = algorithm_count;
    result->total_weight = 0;
    
    // Initialize thread partitions
    for (int i = 0; i < num_threads; i++) {
        result->partitions[i].thread_id = i;
        result->partitions[i].num_assigned_operations = 0;
        result->partitions[i].total_weight = 0;
    }
    
    // Calculate total weight and sort operations by weight (descending)
    typedef struct {
        operation_t op;
        int weight;
        int original_index;
    } weighted_op_t;
    
    weighted_op_t* weighted_ops = malloc(algorithm_count * sizeof(weighted_op_t));
    if (!weighted_ops) {
        free_partitioning_result(result);
        return NULL;
    }
    
    for (int i = 0; i < algorithm_count; i++) {
        weighted_ops[i].op = algorithms[i].op;
        weighted_ops[i].weight = algorithms[i].computational_weight;  // Use registry weight directly
        weighted_ops[i].original_index = i;
        result->total_weight += weighted_ops[i].weight;
    }
    
    // Sort operations by weight (descending) for better bin packing
    for (int i = 0; i < algorithm_count - 1; i++) {
        for (int j = i + 1; j < algorithm_count; j++) {
            if (weighted_ops[i].weight < weighted_ops[j].weight) {
                weighted_op_t temp = weighted_ops[i];
                weighted_ops[i] = weighted_ops[j];
                weighted_ops[j] = temp;
            }
        }
    }
    
    // First Fit Decreasing bin packing algorithm
    // Assign each operation to the thread with the lowest current weight
    for (int i = 0; i < algorithm_count; i++) {
        // Find thread with minimum weight
        int min_weight_thread = 0;
        int min_weight = result->partitions[0].total_weight;
        
        for (int t = 1; t < num_threads; t++) {
            if (result->partitions[t].total_weight < min_weight) {
                min_weight = result->partitions[t].total_weight;
                min_weight_thread = t;
            }
        }
        
        // Assign operation to this thread
        thread_partition_t* partition = &result->partitions[min_weight_thread];
        if (partition->num_assigned_operations < MAX_OPERATIONS_PER_THREAD) {
            partition->assigned_operations[partition->num_assigned_operations] = weighted_ops[i].op;
            partition->num_assigned_operations++;
            partition->total_weight += weighted_ops[i].weight;
        }
    }
    
    free(weighted_ops);
    return result;
}

// Free partitioning result
void free_partitioning_result(partitioning_result_t* result) {
    if (result) {
        if (result->partitions) {
            free(result->partitions);
        }
        free(result);
    }
}

// Print partition summary for debugging
void print_partition_summary(const partitioning_result_t* result) {
    if (!result) return;
    
    printf("🧵 Thread Partitioning Summary:\n");
    printf("   Total operations: %d, Total weight: %d, Threads: %d\n", 
           result->total_operations, result->total_weight, result->num_threads);
    printf("   Target weight per thread: %d\n\n", result->total_weight / result->num_threads);
    
    int max_weight = 0, min_weight = INT_MAX;
    
    for (int t = 0; t < result->num_threads; t++) {
        const thread_partition_t* partition = &result->partitions[t];
        printf("   Thread %d (weight: %d, ops: %d): ", 
               t, partition->total_weight, partition->num_assigned_operations);
        
        for (int i = 0; i < partition->num_assigned_operations; i++) {
            // Find the algorithm entry for this operation
            for (int j = 0; j < result->total_operations; j++) {
                // We need access to the algorithms array here - will need to pass it in
                printf("Op%d", (int)partition->assigned_operations[i]);
                break;
            }
            if (i < partition->num_assigned_operations - 1) printf(", ");
        }
        printf("\n");
        
        if (partition->total_weight > max_weight) max_weight = partition->total_weight;
        if (partition->total_weight < min_weight) min_weight = partition->total_weight;
    }
    
    double imbalance = max_weight > 0 ? ((double)(max_weight - min_weight) / max_weight) * 100.0 : 0.0;
    printf("\n   Load imbalance: %.1f%% (max: %d, min: %d)\n\n", imbalance, max_weight, min_weight);
}