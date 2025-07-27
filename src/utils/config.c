#include "config.h"
#include <stdio.h>
#include <string.h>

// Configuration creation functions
search_config_t create_default_search_config(void) {
    search_config_t config = {
        .complexity = COMPLEXITY_INTERMEDIATE,
        .max_fields = 4,
        .max_constants = 256,
        .checksum_size = 1,
        .verbose = false,
        .early_exit = false,
        .max_solutions = 0,           // Unlimited
        .output_file = NULL,
        .resume_file = NULL,
        .progress_interval_ms = 500,  // 500ms default
        .input_file = NULL,
        .custom_operations = NULL,
        .custom_operation_count = 0,
        .use_custom_operations = false
    };
    return config;
}

search_config_t create_basic_search_config(int max_fields, int max_constants) {
    search_config_t config = create_default_search_config();
    config.complexity = COMPLEXITY_BASIC;
    config.max_fields = max_fields;
    config.max_constants = max_constants;
    config.early_exit = true;         // Usually want early exit for basic searches
    config.max_solutions = 1;
    config.progress_interval_ms = 250; // Faster updates for smaller searches
    return config;
}

search_config_t create_fast_search_config(void) {
    search_config_t config = create_default_search_config();
    config.complexity = COMPLEXITY_BASIC;
    config.max_fields = 3;
    config.max_constants = 16;
    config.early_exit = true;
    config.max_solutions = 1;
    config.progress_interval_ms = 250;
    return config;
}

search_config_t create_thorough_search_config(void) {
    search_config_t config = create_default_search_config();
    config.complexity = COMPLEXITY_ADVANCED;
    config.max_fields = 6;
    config.max_constants = 256;
    config.early_exit = false;        // Find all solutions
    config.max_solutions = 0;         // Unlimited
    config.progress_interval_ms = 1000; // Less frequent updates for long searches
    return config;
}

search_config_t create_custom_operation_config(operation_t* operations, int operation_count) {
    search_config_t config = create_default_search_config();
    config.custom_operations = operations;
    config.custom_operation_count = operation_count;
    config.use_custom_operations = true;
    config.early_exit = true;
    config.max_solutions = 1;
    config.progress_interval_ms = 250;
    return config;
}

// Configuration modification helpers
void set_progress_interval(search_config_t* config, int interval_ms) {
    if (config && interval_ms > 0) {
        config->progress_interval_ms = interval_ms;
    }
}

void enable_early_exit(search_config_t* config, int max_solutions) {
    if (config) {
        config->early_exit = true;
        config->max_solutions = max_solutions > 0 ? max_solutions : 1;
    }
}

void disable_early_exit(search_config_t* config) {
    if (config) {
        config->early_exit = false;
        config->max_solutions = 0;  // Unlimited
    }
}

void set_complexity_level(search_config_t* config, complexity_level_t complexity) {
    if (config) {
        config->complexity = complexity;
        config->use_custom_operations = false;  // Disable custom ops when setting complexity
    }
}

void set_custom_operations(search_config_t* config, operation_t* operations, int count) {
    if (config && operations && count > 0) {
        config->custom_operations = operations;
        config->custom_operation_count = count;
        config->use_custom_operations = true;
    }
}

// Configuration validation
bool validate_search_config(const search_config_t* config) {
    if (!config) return false;
    
    if (config->max_fields < 1 || config->max_fields > CADS_MAX_FIELDS) return false;
    if (config->max_constants < 1 || config->max_constants > CADS_MAX_CONSTANTS) return false;
    if (config->checksum_size < 1 || config->checksum_size > CADS_MAX_CHECKSUM_SIZE) return false;
    if (config->progress_interval_ms < 10) return false;  // Minimum 10ms
    
    if (config->use_custom_operations) {
        if (!config->custom_operations || config->custom_operation_count < 1) return false;
    }
    
    return true;
}

void print_search_config(const search_config_t* config) {
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
    printf("   Progress Interval: %dms\n", config->progress_interval_ms);
    printf("   Custom Operations: %s", config->use_custom_operations ? "Yes" : "No");
    if (config->use_custom_operations) {
        printf(" (%d operations)", config->custom_operation_count);
    }
    printf("\n");
}