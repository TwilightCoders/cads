#ifndef CONFIG_H
#define CONFIG_H

#include "../../include/cads_types.h"
#include "../../include/cads_config_loader.h"

// Configuration helper functions
cads_config_file_t create_default_search_config(void);
cads_config_file_t create_basic_search_config(int max_fields, int max_constants);
cads_config_file_t create_fast_search_config(void);
cads_config_file_t create_thorough_search_config(void);
cads_config_file_t create_custom_operation_config(operation_t* operations, int operation_count);

// Configuration modification helpers
void set_progress_interval(cads_config_file_t* config, int interval_ms);
void enable_early_exit(cads_config_file_t* config, int max_solutions);
void disable_early_exit(cads_config_file_t* config);
void set_complexity_level(cads_config_file_t* config, complexity_level_t complexity);
void set_custom_operations(cads_config_file_t* config, operation_t* operations, int count);

// Configuration validation
bool validate_search_config(const cads_config_file_t* config);
void print_search_config(const cads_config_file_t* config);

#endif // CONFIG_H