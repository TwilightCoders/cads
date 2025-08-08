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

// Calculate search space for a given set of operations
static uint64_t calculate_search_space(int num_assigned_ops, int total_algorithm_count, 
                                      int max_fields, int max_constants, uint64_t field_permutations) {
    uint64_t operation_sequences = 0;
    
    for (int complexity = 1; complexity <= max_fields; complexity++) {
        // N starting operations × total_ops^(remaining positions)
        uint64_t ops_for_complexity = num_assigned_ops;
        for (int j = 1; j < complexity + 1; j++) {
            ops_for_complexity *= total_algorithm_count;
        }
        operation_sequences += ops_for_complexity;
    }
    
    return field_permutations * operation_sequences * max_constants;
}

// Calculate total workload (search space × computational weight)
static double calculate_workload(const thread_partition_t* partition, int total_algorithm_count,
                                int max_fields, int max_constants, uint64_t field_permutations,
                                const algorithm_registry_entry_t* algorithms, int algorithm_count) {
    if (partition->num_assigned_operations == 0) return 0.0;
    
    // Calculate search space
    uint64_t search_space = calculate_search_space(partition->num_assigned_operations, total_algorithm_count,
                                                  max_fields, max_constants, field_permutations);
    
    // Calculate average computational weight of assigned operations
    double avg_weight = 0.0;
    for (int i = 0; i < partition->num_assigned_operations; i++) {
        // Find weight for this operation
        for (int j = 0; j < algorithm_count; j++) {
            if (algorithms[j].op == partition->assigned_operations[i]) {
                avg_weight += algorithms[j].computational_weight;
                break;
            }
        }
    }
    avg_weight /= partition->num_assigned_operations;
    
    // Workload = search_space × average_computational_weight
    return (double)search_space * avg_weight;
}

// Calculate individual operation workload (search space contribution × computational weight)
static double calculate_operation_workload(int computational_weight, int total_algorithm_count,
                                         int max_fields, int max_constants, uint64_t field_permutations) {
    // Each operation contributes to search space as: 1 starting operation × total_ops^(complexity-1)
    uint64_t operation_search_contribution = 0;
    
    for (int complexity = 1; complexity <= max_fields; complexity++) {
        uint64_t ops_for_complexity = 1; // This operation as starter
        for (int j = 1; j < complexity + 1; j++) {
            ops_for_complexity *= total_algorithm_count;
        }
        operation_search_contribution += ops_for_complexity;
    }
    
    uint64_t total_contribution = field_permutations * operation_search_contribution * max_constants;
    
    // Workload = search space contribution × computational weight  
    return (double)total_contribution * computational_weight;
}

// Enhanced partitioner that balances per-operation workload
partitioning_result_t* create_workload_balanced_partitions(
    const algorithm_registry_entry_t* algorithms, 
    int algorithm_count, 
    int num_threads,
    int max_fields,
    int max_constants,
    uint64_t field_permutations) {
    
    if (!algorithms || algorithm_count == 0 || num_threads <= 0) {
        return NULL;
    }
    
    // Calculate per-operation workloads
    typedef struct {
        operation_t op;
        int weight;
        double workload;
        int original_index;
    } operation_workload_t;
    
    operation_workload_t* op_workloads = malloc(algorithm_count * sizeof(operation_workload_t));
    if (!op_workloads) return NULL;
    
    for (int i = 0; i < algorithm_count; i++) {
        op_workloads[i].op = algorithms[i].op;
        op_workloads[i].weight = algorithms[i].computational_weight;
        op_workloads[i].workload = calculate_operation_workload(algorithms[i].computational_weight,
                                                               algorithm_count, max_fields, max_constants, field_permutations);
        op_workloads[i].original_index = i;
        
    }
    
    // Sort operations by workload (descending) for better bin packing
    for (int i = 0; i < algorithm_count - 1; i++) {
        for (int j = i + 1; j < algorithm_count; j++) {
            if (op_workloads[i].workload < op_workloads[j].workload) {
                operation_workload_t temp = op_workloads[i];
                op_workloads[i] = op_workloads[j];
                op_workloads[j] = temp;
            }
        }
    }
    
    // Allocate result structure
    partitioning_result_t* result = malloc(sizeof(partitioning_result_t));
    if (!result) {
        free(op_workloads);
        return NULL;
    }
    
    result->partitions = calloc(num_threads, sizeof(thread_partition_t));
    if (!result->partitions) {
        free(result);
        free(op_workloads);
        return NULL;
    }
    
    result->num_threads = num_threads;
    result->total_operations = algorithm_count;
    result->total_weight = 0;
    
    // Initialize thread partitions and track workload
    double thread_workloads[num_threads];
    for (int i = 0; i < num_threads; i++) {
        result->partitions[i].thread_id = i;
        result->partitions[i].num_assigned_operations = 0;
        result->partitions[i].total_weight = 0;
        thread_workloads[i] = 0.0;
    }
    
    // Calculate total weight
    for (int i = 0; i < algorithm_count; i++) {
        result->total_weight += algorithms[i].computational_weight;
    }
    
    // Assign each operation to thread with minimum current workload
    for (int i = 0; i < algorithm_count; i++) {
        // Find thread with minimum current workload
        int min_workload_thread = 0;
        double min_workload = thread_workloads[0];
        
        for (int t = 1; t < num_threads; t++) {
            if (thread_workloads[t] < min_workload) {
                min_workload = thread_workloads[t];
                min_workload_thread = t;
            }
        }
        
        // Assign operation to this thread
        thread_partition_t* partition = &result->partitions[min_workload_thread];
        if (partition->num_assigned_operations < MAX_OPERATIONS_PER_THREAD) {
            partition->assigned_operations[partition->num_assigned_operations] = op_workloads[i].op;
            partition->num_assigned_operations++;
            partition->total_weight += op_workloads[i].weight;
            thread_workloads[min_workload_thread] += op_workloads[i].workload;
        }
    }
    
    free(op_workloads);
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

// Enhanced print partition summary that shows workload calculations
void print_partition_summary_with_workload(const partitioning_result_t* result, 
                                          const algorithm_registry_entry_t* algorithms, int algorithm_count,
                                          int max_fields, int max_constants, uint64_t field_permutations) {
    if (!result) return;
    
    printf("🧵 Workload-Balanced Thread Partitioning Summary:\n");
    printf("   Total operations: %d, Total weight: %d, Threads: %d\n", 
           result->total_operations, result->total_weight, result->num_threads);
    printf("   Max fields: %d, Max constants: %d, Field permutations: %llu\n\n",
           max_fields, max_constants, (unsigned long long)field_permutations);
    
    for (int t = 0; t < result->num_threads; t++) {
        const thread_partition_t* partition = &result->partitions[t];
        
        // Calculate search space for this thread
        uint64_t search_space = calculate_search_space(partition->num_assigned_operations, result->total_operations,
                                                      max_fields, max_constants, field_permutations);
        
        // Calculate workload
        double workload = calculate_workload(partition, result->total_operations, max_fields, max_constants, 
                                           field_permutations, algorithms, algorithm_count);
        
        printf("   Thread %d: %d ops (weight: %d) → search space: %llu → workload: %.2e\n", 
               t, partition->num_assigned_operations, partition->total_weight, 
               (unsigned long long)search_space, workload);
        
        printf("     Operations: ");
        for (int i = 0; i < partition->num_assigned_operations; i++) {
            // Find the algorithm name for this operation
            for (int j = 0; j < algorithm_count; j++) {
                if (algorithms[j].op == partition->assigned_operations[i]) {
                    printf("%s", algorithms[j].name);
                    break;
                }
            }
            if (i < partition->num_assigned_operations - 1) printf(", ");
        }
        printf("\n\n");
    }
    
    // Calculate workload imbalance
    double max_workload = 0.0, min_workload = 1e20;
    for (int t = 0; t < result->num_threads; t++) {
        double workload = calculate_workload(&result->partitions[t], result->total_operations, max_fields, max_constants, 
                                           field_permutations, algorithms, algorithm_count);
        if (workload > max_workload) max_workload = workload;
        if (workload < min_workload) min_workload = workload;
    }
    
    double workload_imbalance = max_workload > 0 ? ((max_workload - min_workload) / max_workload) * 100.0 : 0.0;
    printf("   Workload imbalance: %.1f%% (max: %.2e, min: %.2e)\n\n", 
           workload_imbalance, max_workload, min_workload);
}

// Print partition summary for debugging (original version for compatibility)
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
            printf("Op%d", (int)partition->assigned_operations[i]);
            if (i < partition->num_assigned_operations - 1) printf(", ");
        }
        printf("\n");
        
        if (partition->total_weight > max_weight) max_weight = partition->total_weight;
        if (partition->total_weight < min_weight) min_weight = partition->total_weight;
    }
    
    double imbalance = max_weight > 0 ? ((double)(max_weight - min_weight) / max_weight) * 100.0 : 0.0;
    printf("\n   Load imbalance: %.1f%% (max: %d, min: %d)\n\n", imbalance, max_weight, min_weight);
}