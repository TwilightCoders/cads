#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Configuration creation functions
config_t create_default_search_config(void) {
    config_t config = {
        .name = NULL,
        .description = NULL,
        .complexity = COMPLEXITY_INTERMEDIATE,
        .max_fields = 4,
        .max_constants = 256,
        .checksum_size = 1,
        .early_exit = false,
        .max_solutions = 0,           // Unlimited
        .progress_interval = 500,     // 500ms default
        .verbose = false,
        .custom_operations = NULL,
        .custom_operation_count = 0,
        .dataset = NULL,
        .threads = 1
    };
    return config;
}

config_t create_basic_search_config(int max_fields, int max_constants) {
    config_t config = create_default_search_config();
    config.complexity = COMPLEXITY_BASIC;
    config.max_fields = max_fields;
    config.max_constants = max_constants;
    config.early_exit = true;         // Usually want early exit for basic searches
    config.max_solutions = 1;
    config.progress_interval = 250; // Faster updates for smaller searches
    return config;
}

config_t create_fast_search_config(void) {
    config_t config = create_default_search_config();
    config.complexity = COMPLEXITY_BASIC;
    config.max_fields = 3;
    config.max_constants = 16;
    config.early_exit = true;
    config.max_solutions = 1;
    config.progress_interval = 250;
    return config;
}

config_t create_thorough_search_config(void) {
    config_t config = create_default_search_config();
    config.complexity = COMPLEXITY_ADVANCED;
    config.max_fields = 6;
    config.max_constants = 256;
    config.early_exit = false;        // Find all solutions
    config.max_solutions = 0;         // Unlimited
    config.progress_interval = 1000; // Less frequent updates for long searches
    return config;
}

config_t create_custom_operation_config(operation_t* operations, int operation_count) {
    config_t config = create_default_search_config();
    config.custom_operations = operations;
    config.custom_operation_count = operation_count;
    config.early_exit = true;
    config.max_solutions = 1;
    config.progress_interval = 250;
    return config;
}

// Configuration modification helpers
void set_progress_interval(config_t* config, int interval_ms) {
    if (config && interval_ms > 0) {
        config->progress_interval = interval_ms;
    }
}

void enable_early_exit(config_t* config, int max_solutions) {
    if (config) {
        config->early_exit = true;
        config->max_solutions = max_solutions > 0 ? max_solutions : 1;
    }
}

void disable_early_exit(config_t* config) {
    if (config) {
        config->early_exit = false;
        config->max_solutions = 0;  // Unlimited
    }
}

void set_complexity_level(config_t* config, complexity_level_t complexity) {
    if (config) {
        config->complexity = complexity;
        // Clear custom operations when setting complexity
        config->custom_operations = NULL;
        config->custom_operation_count = 0;
    }
}

void set_custom_operations(config_t* config, operation_t* operations, int count) {
    if (config && operations && count > 0) {
        config->custom_operations = operations;
        config->custom_operation_count = count;
    }
}

// Configuration validation
bool validate_search_config(const config_t* config) {
    if (!config) return false;
    
    if (config->max_fields < 1 || config->max_fields > CADS_MAX_FIELDS) return false;
    if (config->max_constants < 1 || config->max_constants > CADS_MAX_CONSTANTS) return false;
    if (config->checksum_size < 1 || config->checksum_size > CADS_MAX_CHECKSUM_SIZE) return false;
    if (config->progress_interval < 10) return false;  // Minimum 10ms
    
    if (config->custom_operation_count > 0) {
        if (!config->custom_operations) return false;
    }
    
    return true;
}

void print_search_config(const config_t* config) {
    if (!config) {
        printf("❌ NULL configuration\n");
        return;
    }
    
    printf("🔧 Search Configuration:\n");
    printf("   Complexity: %s\n", 
           config->complexity == COMPLEXITY_BASIC ? "Basic" :
           config->complexity == COMPLEXITY_INTERMEDIATE ? "Intermediate" : 
           config->complexity == COMPLEXITY_ADVANCED ? "Advanced" : "Unknown");
    printf("   Max Fields: %d\n", config->max_fields);
    printf("   Max Constants: %d\n", config->max_constants);
    printf("   Checksum Size: %zu bytes\n", config->checksum_size);
    printf("   Early Exit: %s", config->early_exit ? "Yes" : "No");
    if (config->early_exit && config->max_solutions > 0) {
        printf(" (max %d solutions)", config->max_solutions);
    }
    printf("\n");
    printf("   Progress Interval: %dms\n", config->progress_interval);
    printf("   Custom Operations: %s", config->custom_operation_count > 0 ? "Yes" : "No");
    if (config->custom_operation_count > 0) {
        printf(" (%d operations)", config->custom_operation_count);
    }
    printf("\n");
}
