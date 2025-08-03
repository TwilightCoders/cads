#include "../../include/algorithm_registry.h"
#include "../algorithms/basic_ops.h"
#include "../algorithms/intermediate_ops.h"
#include "../algorithms/advanced_ops.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

// Forward declarations for algorithm functions
uint64_t basic_add_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_sub_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_xor_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_and_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_or_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_identity_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_not_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_lshift_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_rshift_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_mul_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_div_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_mod_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_negate_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_const_add_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_const_xor_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_const_sub_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_ones_complement_wrapper(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_twos_complement_wrapper(uint64_t a, uint64_t b, uint64_t constant);

// Global algorithm registry
static algorithm_registry_entry_t* g_algorithm_registry = NULL;
static int g_algorithm_count = 0;
static bool g_registry_initialized = false;

// Complexity level statistics
static const complexity_stats_t complexity_statistics[] = {
    {COMPLEXITY_BASIC, "Basic", 6, 500000.0, "Simple arithmetic and logical operations"},
    {COMPLEXITY_INTERMEDIATE, "Intermediate", 12, 100000.0, "Bit manipulation and constant operations"},
    {COMPLEXITY_ADVANCED, "Advanced", 11, 5000.0, "CRC variants and complex algorithms"},
    {COMPLEXITY_ALL, "All", 29, 50000.0, "Complete algorithm suite"}
};

// Master algorithm registry with all operations
static const algorithm_registry_entry_t master_registry[] = {
    // BASIC algorithms (6 total) - All 1 cycle
    {OP_ADD, COMPLEXITY_BASIC, "ADD", "Simple addition", false, basic_add_wrapper, 1},
    {OP_SUB, COMPLEXITY_BASIC, "SUB", "Subtraction", false, basic_sub_wrapper, 1},
    {OP_XOR, COMPLEXITY_BASIC, "XOR", "Exclusive OR", false, basic_xor_wrapper, 1},
    {OP_AND, COMPLEXITY_BASIC, "AND", "Bitwise AND", false, basic_and_wrapper, 1},
    {OP_OR, COMPLEXITY_BASIC, "OR", "Bitwise OR", false, basic_or_wrapper, 1},
    {OP_IDENTITY, COMPLEXITY_BASIC, "ID", "Pass-through", false, basic_identity_wrapper, 1},
    
    // INTERMEDIATE algorithms (12 total) - 1-30 cycles
    {OP_NOT, COMPLEXITY_INTERMEDIATE, "NOT", "Bitwise NOT", false, intermediate_not_wrapper, 1},
    {OP_LSHIFT, COMPLEXITY_INTERMEDIATE, "LSH", "Left shift", false, intermediate_lshift_wrapper, 1},
    {OP_RSHIFT, COMPLEXITY_INTERMEDIATE, "RSH", "Right shift", false, intermediate_rshift_wrapper, 1},
    {OP_MUL, COMPLEXITY_INTERMEDIATE, "MUL", "Multiplication", false, intermediate_mul_wrapper, 3},
    {OP_DIV, COMPLEXITY_INTERMEDIATE, "DIV", "Division", false, intermediate_div_wrapper, 30},
    {OP_MOD, COMPLEXITY_INTERMEDIATE, "MOD", "Modulo", false, intermediate_mod_wrapper, 30},
    {OP_NEGATE, COMPLEXITY_INTERMEDIATE, "NEG", "Two's complement negation", false, intermediate_negate_wrapper, 1},
    {OP_CONST_ADD, COMPLEXITY_INTERMEDIATE, "C+", "Add constant", true, intermediate_const_add_wrapper, 1},
    {OP_CONST_XOR, COMPLEXITY_INTERMEDIATE, "C^", "XOR with constant", true, intermediate_const_xor_wrapper, 1},
    {OP_CONST_SUB, COMPLEXITY_INTERMEDIATE, "C-", "Subtract constant", true, intermediate_const_sub_wrapper, 1},
    {OP_ONES_COMPLEMENT, COMPLEXITY_INTERMEDIATE, "1COMP", "One's complement sum", false, intermediate_ones_complement_wrapper, 1},
    {OP_TWOS_COMPLEMENT, COMPLEXITY_INTERMEDIATE, "2COMP", "Two's complement sum", false, intermediate_twos_complement_wrapper, 2},
    
    // ADVANCED algorithms (11 total) - 2-25 cycles
    {OP_ROTLEFT, COMPLEXITY_ADVANCED, "ROTL", "Rotate left", false, NULL, 2},
    {OP_ROTRIGHT, COMPLEXITY_ADVANCED, "ROTR", "Rotate right", false, NULL, 2},
    {OP_CRC8_CCITT, COMPLEXITY_ADVANCED, "CRC8C", "CRC-8 CCITT", false, NULL, 8},
    {OP_CRC8_DALLAS, COMPLEXITY_ADVANCED, "CRC8D", "CRC-8 Dallas/Maxim", false, NULL, 8},
    {OP_CRC8_SAE, COMPLEXITY_ADVANCED, "CRC8S", "CRC-8 SAE J1850", false, NULL, 8},
    {OP_FLETCHER8, COMPLEXITY_ADVANCED, "FLETCH", "Fletcher-8 checksum", false, NULL, 6},
    {OP_SWAP_NIBBLES, COMPLEXITY_ADVANCED, "SWAP", "Swap nibbles", false, NULL, 2},
    {OP_REVERSE_BITS, COMPLEXITY_ADVANCED, "REVB", "Reverse bits", false, NULL, 8},
    {OP_LOOKUP_TABLE, COMPLEXITY_ADVANCED, "LUT", "Lookup table", false, NULL, 3},
    {OP_POLY_CRC, COMPLEXITY_ADVANCED, "PCRC", "Polynomial CRC", true, NULL, 20},
    {OP_CHECKSUM_VARIANT, COMPLEXITY_ADVANCED, "CVAR", "Checksum variant", true, NULL, 5}
};

// Wrapper functions for basic operations
uint64_t basic_add_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return basic_add(a, b, constant);
}

uint64_t basic_sub_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return basic_sub(a, b, constant);
}

uint64_t basic_xor_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return basic_xor(a, b, constant);
}

uint64_t basic_and_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return basic_and(a, b, constant);
}

uint64_t basic_or_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return basic_or(a, b, constant);
}

uint64_t basic_identity_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return basic_identity(a, b, constant);
}

// Wrapper functions for intermediate operations
uint64_t intermediate_not_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_not(a, b, constant);
}

uint64_t intermediate_lshift_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_lshift(a, b, constant);
}

uint64_t intermediate_rshift_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_rshift(a, b, constant);
}

uint64_t intermediate_mul_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_mul(a, b, constant);
}

uint64_t intermediate_div_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_div(a, b, constant);
}

uint64_t intermediate_mod_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_mod(a, b, constant);
}

uint64_t intermediate_negate_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_negate(a, b, constant);
}

uint64_t intermediate_const_add_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_const_add(a, b, constant);
}

uint64_t intermediate_const_xor_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_const_xor(a, b, constant);
}

uint64_t intermediate_const_sub_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_const_sub(a, b, constant);
}

uint64_t intermediate_ones_complement_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_ones_complement(a, b, constant);
}

uint64_t intermediate_twos_complement_wrapper(uint64_t a, uint64_t b, uint64_t constant) {
    return intermediate_twos_complement(a, b, constant);
}

bool initialize_algorithm_registry(void) {
    if (g_registry_initialized) {
        return true;
    }
    
    g_algorithm_count = sizeof(master_registry) / sizeof(master_registry[0]);
    g_algorithm_registry = malloc(g_algorithm_count * sizeof(algorithm_registry_entry_t));
    
    if (!g_algorithm_registry) {
        return false;
    }
    
    memcpy(g_algorithm_registry, master_registry, 
           g_algorithm_count * sizeof(algorithm_registry_entry_t));
    
    g_registry_initialized = true;
    return true;
}

void cleanup_algorithm_registry(void) {
    if (g_algorithm_registry) {
        free(g_algorithm_registry);
        g_algorithm_registry = NULL;
    }
    g_algorithm_count = 0;
    g_registry_initialized = false;
}

const algorithm_registry_entry_t* get_algorithms_by_complexity(complexity_level_t complexity, int* count) {
    if (!g_registry_initialized) {
        *count = 0;
        return NULL;
    }
    
    static algorithm_registry_entry_t filtered_algorithms[NUM_OPS];
    int filtered_count = 0;
    
    for (int i = 0; i < g_algorithm_count; i++) {
        if (complexity == COMPLEXITY_ALL || g_algorithm_registry[i].complexity <= complexity) {
            filtered_algorithms[filtered_count++] = g_algorithm_registry[i];
        }
    }
    
    *count = filtered_count;
    return filtered_algorithms;
}

const algorithm_registry_entry_t* get_all_algorithms(int* count) {
    if (!g_registry_initialized) {
        *count = 0;
        return NULL;
    }
    
    *count = g_algorithm_count;
    return g_algorithm_registry;
}

const algorithm_registry_entry_t* get_algorithm_by_operation(operation_t op) {
    if (!g_registry_initialized) {
        return NULL;
    }
    
    for (int i = 0; i < g_algorithm_count; i++) {
        if (g_algorithm_registry[i].op == op) {
            return &g_algorithm_registry[i];
        }
    }
    
    return NULL;
}

const char* get_complexity_name(complexity_level_t complexity) {
    switch (complexity) {
        case COMPLEXITY_BASIC: return "Basic";
        case COMPLEXITY_INTERMEDIATE: return "Intermediate";
        case COMPLEXITY_ADVANCED: return "Advanced";
        case COMPLEXITY_ALL: return "All";
        default: return "Unknown";
    }
}

uint64_t estimate_total_combinations(const config_t* config, size_t packet_count) {
    if (packet_count == 0) return 0;
    
    int algorithm_count;
    get_algorithms_by_complexity(config->complexity, &algorithm_count);
    
    // Estimate field combinations (simplified)
    uint64_t field_combinations = 1;
    for (int fields = 1; fields <= config->max_fields; fields++) {
        // C(packet_length, fields) * fields! (permutations)
        field_combinations += (packet_count * fields * 2); // Simplified estimate
    }
    
    return field_combinations * algorithm_count * config->max_constants;
}

double estimate_completion_time(const config_t* config, size_t packet_count) {
    uint64_t total_combinations = estimate_total_combinations(config, packet_count);
    
    // Get average operations per second for this complexity level
    double avg_ops_per_sec = 50000.0; // Default
    for (int i = 0; i < 4; i++) {
        if (complexity_statistics[i].level == config->complexity) {
            avg_ops_per_sec = complexity_statistics[i].avg_ops_per_second;
            break;
        }
    }
    
    return (double)total_combinations / avg_ops_per_sec;
}

uint64_t execute_algorithm(operation_t op, uint64_t a, uint64_t b, uint64_t constant) {
    const algorithm_registry_entry_t* entry = get_algorithm_by_operation(op);
    if (!entry || !entry->func) {
        return 0; // Error or not implemented
    }
    
    return entry->func(a, b, constant);
}

const complexity_stats_t* get_complexity_stats(int* count) {
    *count = sizeof(complexity_statistics) / sizeof(complexity_statistics[0]);
    return complexity_statistics;
}

// Profile actual operation performance to validate weights
void profile_algorithm_performance(void) {
    printf("🔬 Profiling algorithm performance...\n");
    
    const int ITERATIONS = 1000000;
    uint64_t test_values[] = {0x12345678, 0xABCDEF00, 0x55AA55AA, 0xFF00FF00};
    const int NUM_TESTS = sizeof(test_values) / sizeof(test_values[0]);
    
    for (int i = 0; i < g_algorithm_count; i++) {
        const algorithm_registry_entry_t* entry = &g_algorithm_registry[i];
        
        struct timeval start, end;
        gettimeofday(&start, NULL);
        
        // Run operation many times to get timing
        volatile uint64_t result = 0; // volatile prevents optimization
        for (int iter = 0; iter < ITERATIONS; iter++) {
            for (int t = 0; t < NUM_TESTS; t++) {
                result += execute_algorithm(entry->op, test_values[t], test_values[(t+1) % NUM_TESTS], 0xD0);
            }
        }
        
        gettimeofday(&end, NULL);
        
        double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 + 
                           (end.tv_usec - start.tv_usec) / 1000.0;
        double ops_per_ms = (ITERATIONS * NUM_TESTS) / elapsed_ms;
        
        printf("   %s: %.2f M ops/sec (weight: %d, result: %llu)\n", 
               entry->description, ops_per_ms / 1000.0, 
               entry->computational_weight, (unsigned long long)result);
    }
    printf("\n");
}
