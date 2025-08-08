// Shared checksum engine helper implementations extracted from legacy recursive engine
// Provides result container management and basic field/masking helpers used by evaluator & threaded engine.

#include "checksum_engine.h"
#include <stdio.h>
// Optional precomputed field value cache (packet_index x field_index)
static uint64_t** g_field_cache = NULL;
static size_t g_field_cache_packets = 0;
static size_t g_field_cache_fields = 0;
static size_t g_field_cache_checksum_size = 0;

void clear_field_cache(void) {
    if (g_field_cache) {
        for (size_t i = 0; i < g_field_cache_packets; i++) {
            free(g_field_cache[i]);
        }
        free(g_field_cache);
    }
    g_field_cache = NULL;
    g_field_cache_packets = 0;
    g_field_cache_fields = 0;
    g_field_cache_checksum_size = 0;
}

bool build_field_cache(const packet_dataset_t* dataset, size_t checksum_size, size_t max_fields) {
    clear_field_cache();
    if (!dataset || dataset->count == 0) return false;
    // Determine min packet length to bound fields
    size_t min_len = dataset->packets[0].packet_length;
    for (size_t i=1;i<dataset->count;i++) if (dataset->packets[i].packet_length < min_len) min_len = dataset->packets[i].packet_length;
    if (max_fields > min_len) max_fields = min_len;
    g_field_cache_packets = dataset->count;
    g_field_cache_fields = max_fields;
    g_field_cache_checksum_size = checksum_size;
    g_field_cache = (uint64_t**)malloc(g_field_cache_packets * sizeof(uint64_t*));
    if (!g_field_cache) { clear_field_cache(); return false; }
    for (size_t p=0; p<g_field_cache_packets; p++) {
        g_field_cache[p] = (uint64_t*)malloc(g_field_cache_fields * sizeof(uint64_t));
        if (!g_field_cache[p]) { clear_field_cache(); return false; }
        for (size_t f=0; f<g_field_cache_fields; f++) {
            const test_packet_t* pkt = &dataset->packets[p];
            uint64_t value = 0;
            size_t bytes_to_extract = (checksum_size > 1) ? checksum_size : 1;
            for (size_t b=0; b<bytes_to_extract && (f + b) < pkt->packet_length; b++) {
                value = (value << 8) | pkt->packet_data[f + b];
            }
            g_field_cache[p][f] = value;
        }
    }
    return true;
}

// Deterministic solution ordering comparator
static int compare_solutions(const void* a, const void* b) {
    const checksum_solution_t* A = (const checksum_solution_t*)a;
    const checksum_solution_t* B = (const checksum_solution_t*)b;
    if (A->field_count != B->field_count) return A->field_count - B->field_count;
    if (A->operation_count != B->operation_count) return A->operation_count - B->operation_count;
    for (int i=0;i<A->field_count;i++) {
        if (A->field_indices[i] != B->field_indices[i]) return (int)A->field_indices[i] - (int)B->field_indices[i];
    }
    for (int i=0;i<A->operation_count;i++) {
        if (A->operations[i] != B->operations[i]) return (int)A->operations[i] - (int)B->operations[i];
    }
    if (A->constant != B->constant) return (A->constant < B->constant) ? -1 : 1;
    if (A->checksum_size != B->checksum_size) return (int)A->checksum_size - (int)B->checksum_size;
    return 0;
}

void sort_search_solutions(search_results_t* results) {
    if (!results || results->solution_count < 2) return;
    qsort(results->solutions, results->solution_count, sizeof(checksum_solution_t), compare_solutions);
}
#include <stdlib.h>
#include <string.h>

search_results_t* create_search_results(size_t initial_capacity) {
    search_results_t* results = malloc(sizeof(search_results_t));
    if (!results) return NULL;

    results->solutions = malloc(initial_capacity * sizeof(checksum_solution_t));
    if (!results->solutions) {
        free(results);
        return NULL;
    }

    results->solution_count = 0;
    results->solution_capacity = initial_capacity;
    results->tests_performed = 0;
    results->search_completed = false;
    results->early_exit_triggered = false;
    return results;
}

void free_search_results(search_results_t* results) {
    if (!results) return;
    free(results->solutions);
    free(results);
}

bool add_solution(search_results_t* results, const checksum_solution_t* solution) {
    if (!results || !solution) return false;
    if (results->solution_count >= results->solution_capacity) {
        size_t new_capacity = results->solution_capacity * 2;
        checksum_solution_t* new_solutions = realloc(results->solutions, new_capacity * sizeof(checksum_solution_t));
        if (!new_solutions) return false;
        results->solutions = new_solutions;
        results->solution_capacity = new_capacity;
    }
    memcpy(&results->solutions[results->solution_count], solution, sizeof(checksum_solution_t));
    results->solution_count++;
    return true;
}

bool should_continue_search(const search_results_t* results, const config_t* config) {
    if (!results || !config) return false;
    if (config->early_exit && results->solution_count > 0) return false;
    if (config->max_solutions > 0 && results->solution_count >= (size_t)config->max_solutions) return false;
    return true;
}

uint64_t extract_packet_field_value(const uint8_t* packet_data, size_t packet_length,
                                   uint8_t field_index, size_t checksum_size) {
    if (!packet_data || field_index >= packet_length) return 0;
    if (g_field_cache && checksum_size == g_field_cache_checksum_size && field_index < g_field_cache_fields) {
        // Caller must ensure packet_data corresponds to same ordering; we derive packet index by pointer arithmetic not stored here.
        // Fast path not available without packet index; fall back unless single-byte checksum and we can map via dataset iteration elsewhere.
        // (Simplification: only accelerate multi-use via sequence_evaluator direct extraction loop using original logic.)
    }
    uint64_t value = 0;
    size_t bytes_to_extract = (checksum_size > 1) ? checksum_size : 1;
    for (size_t i = 0; i < bytes_to_extract && (field_index + i) < packet_length; i++) {
        value = (value << 8) | packet_data[field_index + i];
    }
    return value;
}

uint64_t mask_checksum_to_size(uint64_t checksum, size_t checksum_size) {
    if (checksum_size >= 8) return checksum;
    uint64_t mask = (1ULL << (checksum_size * 8)) - 1;
    return checksum & mask;
}
