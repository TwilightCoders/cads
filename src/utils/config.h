#ifndef CONFIG_H
#define CONFIG_H

#include "../../include/cads_types.h"
#include "../../include/cads_config_loader.h"

// Configuration helper functions
config_t create_default_search_config(void);
config_t create_basic_search_config(int max_fields, int max_constants);
config_t create_fast_search_config(void);
config_t create_thorough_search_config(void);
config_t create_custom_operation_config(operation_t* operations, int operation_count);

// Configuration modification helpers
void set_progress_interval(config_t* config, int interval_ms);
void enable_early_exit(config_t* config, int max_solutions);
void disable_early_exit(config_t* config);
void set_complexity_level(config_t* config, complexity_level_t complexity);
void set_custom_operations(config_t* config, operation_t* operations, int count);

// Configuration validation
bool validate_search_config(const config_t* config);
void print_search_config(const config_t* config);

#endif // CONFIG_H
