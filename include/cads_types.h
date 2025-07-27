#ifndef CADS_TYPES_H
#define CADS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>

// Configuration limits and defaults
#define CADS_MAX_PACKET_SIZE 1024     // Maximum supported packet size
#define CADS_MAX_FIELDS 16            // Maximum number of fields in a packet
#define CADS_MAX_PERMUTATIONS 24      // Limit permutations for performance
#define CADS_MAX_CONSTANTS 256        // All possible byte values
#define CADS_DEFAULT_CHECKSUM_SIZE 1  // Default checksum size in bytes
#define CADS_MAX_CHECKSUM_SIZE 8      // Maximum checksum size (uint64_t)

// Test packet structure for validation - now variable length
typedef struct {
    uint8_t* packet_data;              // Packet data without checksum
    size_t packet_length;              // Length of packet data (excluding checksum)
    uint64_t expected_checksum;        // Expected checksum (up to 64-bit)
    size_t checksum_size;              // Checksum size in bytes (1-8)
    const char* description;           // Human-readable description
} test_packet_t;

// Operation types categorized by complexity
typedef enum {
    // BASIC operations (6 total)
    OP_ADD = 0, OP_SUB, OP_XOR, OP_AND, OP_OR, OP_IDENTITY,
    
    // INTERMEDIATE operations (12 total)  
    OP_NOT, OP_LSHIFT, OP_RSHIFT, OP_MUL, OP_DIV, OP_MOD, 
    OP_NEGATE, OP_CONST_ADD, OP_CONST_XOR, OP_CONST_SUB,
    OP_ONES_COMPLEMENT, OP_TWOS_COMPLEMENT,
    
    // ADVANCED operations (11 total)
    OP_ROTLEFT, OP_ROTRIGHT, OP_CRC8_CCITT, OP_CRC8_DALLAS, 
    OP_CRC8_SAE, OP_FLETCHER8, OP_SWAP_NIBBLES, OP_REVERSE_BITS,
    OP_LOOKUP_TABLE, OP_POLY_CRC, OP_CHECKSUM_VARIANT,
    
    NUM_OPS
} operation_t;

// Complexity levels for algorithm categorization
typedef enum {
    COMPLEXITY_BASIC = 0,
    COMPLEXITY_INTERMEDIATE,
    COMPLEXITY_ADVANCED,
    COMPLEXITY_ALL,
    NUM_COMPLEXITY_LEVELS
} complexity_level_t;

// Algorithm metadata for registry
typedef struct {
    operation_t op;
    complexity_level_t complexity;
    const char* name;
    const char* description;
    bool requires_constant;
} algorithm_info_t;

// Progress tracking structure
typedef struct {
    uint64_t total_combinations;
    uint64_t completed_tests;
    uint64_t tests_at_last_update;
    double avg_tests_per_second;
    double smoothed_rate;               // Exponential moving average of rate
    struct timeval start_time;
    struct timeval last_update;
    struct timeval last_progress_display;       // Last time progress was displayed
    int solutions_found;
    int progress_interval_ms;           // Progress update interval in milliseconds
} progress_tracker_t;

// Search configuration
typedef struct {
    complexity_level_t complexity;
    int max_fields;                    // Maximum number of fields to combine
    int max_constants;                 // Maximum constant value to test
    size_t checksum_size;              // Expected checksum size in bytes (1-8)
    bool verbose;
    bool early_exit;                   // Exit after finding first solution
    int max_solutions;                 // Maximum solutions to find (0 = unlimited)
    const char* output_file;
    const char* resume_file;
    int progress_interval_ms;
    const char* input_file;            // Input file with test packets
    
    // Custom operation selection (for targeted testing)
    operation_t* custom_operations;    // Array of specific operations to test
    int custom_operation_count;        // Number of operations in custom_operations
    bool use_custom_operations;        // If true, only test custom_operations instead of complexity level
} search_config_t;

// Solution result structure
typedef struct {
    uint8_t field_indices[CADS_MAX_FIELDS];
    int field_count;
    operation_t operations[4];         // Up to 4 operations in sequence
    int operation_count;
    uint64_t constant;                 // Support larger constants for multi-byte checksums
    size_t checksum_size;              // Size of the checksum this solution produces
    bool validated;
} checksum_solution_t;

// Search results container
typedef struct {
    checksum_solution_t* solutions;    // Array of solutions found
    size_t solution_count;             // Number of solutions found
    size_t solution_capacity;          // Capacity of solutions array
    uint64_t tests_performed;          // Total tests performed during search
    bool search_completed;             // Whether search finished normally
    bool early_exit_triggered;        // Whether early exit was triggered
} search_results_t;

// Expression tree node for complex operations (future use)
typedef struct expr_node {
    bool is_field;
    uint8_t field_index;
    operation_t op;
    uint8_t constant;
    struct expr_node* left;
    struct expr_node* right;
} expr_node_t;

#endif // CADS_TYPES_H