#ifndef CADS_CONFIG_LOADER_H
#define CADS_CONFIG_LOADER_H

#include "cads_types.h"
#include "../src/core/packet_data.h"

typedef struct {
    char* name;
    char* description;
    complexity_level_t complexity;
    int max_fields;
    int max_constants;
    size_t checksum_size;
    bool early_exit;
    int max_solutions;
    int progress_interval;
    bool verbose;
    operation_t* custom_operations;
    int custom_operation_count;
    packet_dataset_t* dataset;
    int threads;
} config_t;

// Core configuration functions
config_t* create_default_cads_config(void);
config_t* load_cads_config(const char* filename);
config_t* create_cads_config_from_cli(int argc, char* argv[]);
void free_cads_config(config_t* config);

// Configuration utilities
void merge_cli_args_into_cads_config(config_t* config, int argc, char* argv[]);
bool load_packets_into_cads_config(config_t* config, const char* json_file);

#endif
