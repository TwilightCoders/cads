#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include "../../include/cads_config_loader.h"
#include "../../include/cads_types.h"
#include "../../src/core/packet_data.h"
#include "../../src/utils/config.h"

// Forward declarations
static bool parse_packets_section(config_t* config, FILE* file);

static char* trim_whitespace(char* str) {
    char* end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static complexity_level_t parse_complexity_level(const char* str) {
    if (strcasecmp(str, "basic") == 0) return COMPLEXITY_BASIC;
    if (strcasecmp(str, "intermediate") == 0) return COMPLEXITY_INTERMEDIATE;
    if (strcasecmp(str, "advanced") == 0) return COMPLEXITY_ADVANCED;
    return COMPLEXITY_INTERMEDIATE;
}

static operation_t parse_operation(const char* str) {
    if (strcasecmp(str, "identity") == 0) return OP_IDENTITY;
    if (strcasecmp(str, "add") == 0) return OP_ADD;
    if (strcasecmp(str, "xor") == 0) return OP_XOR;
    if (strcasecmp(str, "ones_complement") == 0) return OP_ONES_COMPLEMENT;
    if (strcasecmp(str, "const_add") == 0) return OP_CONST_ADD;
    if (strcasecmp(str, "const_sub") == 0) return OP_CONST_SUB;
    // if (strcasecmp(str, "const_mul") == 0) return OP_CONST_MUL;
    if (strcasecmp(str, "const_xor") == 0) return OP_CONST_XOR;
    if (strcasecmp(str, "sub") == 0) return OP_SUB;
    if (strcasecmp(str, "mul") == 0) return OP_MUL;
    if (strcasecmp(str, "div") == 0) return OP_DIV;
    if (strcasecmp(str, "mod") == 0) return OP_MOD;
    return OP_IDENTITY;
}

static bool parse_bool(const char* str) {
    return (strcasecmp(str, "true") == 0 || strcasecmp(str, "yes") == 0 || 
            strcasecmp(str, "1") == 0 || strcasecmp(str, "on") == 0);
}

static bool parse_config_section(config_t* config, FILE* file) {
    char line[512];
    
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim_whitespace(line);
        
        if (strlen(trimmed) == 0 || trimmed[0] == '#') continue;
        if (strncmp(trimmed, "[packets]", 9) == 0) {
            // We hit packets section - parse it directly here since we already consumed the header
            return parse_packets_section(config, file);
        }
        
        char* equals = strchr(trimmed, '=');
        if (!equals) continue;
        
        *equals = '\0';
        char* key = trim_whitespace(trimmed);
        char* value = trim_whitespace(equals + 1);
        
        if (strcmp(key, "name") == 0) {
            config->name = strdup(value);
        } else if (strcmp(key, "description") == 0) {
            config->description = strdup(value);
        } else if (strcmp(key, "complexity") == 0) {
            config->complexity = parse_complexity_level(value);
        } else if (strcmp(key, "max_fields") == 0) {
            config->max_fields = atoi(value);
        } else if (strcmp(key, "max_constants") == 0) {
            config->max_constants = atoi(value);
        } else if (strcmp(key, "checksum_size") == 0) {
            config->checksum_size = atoi(value);
        } else if (strcmp(key, "early_exit") == 0) {
            config->early_exit = parse_bool(value);
        } else if (strcmp(key, "max_solutions") == 0) {
            config->max_solutions = atoi(value);
        } else if (strcmp(key, "progress_interval") == 0) {
            config->progress_interval = atoi(value);
        } else if (strcmp(key, "verbose") == 0) {
            config->verbose = parse_bool(value);
        } else if (strcmp(key, "threads") == 0) {
            config->threads = atoi(value);
        } else if (strcmp(key, "operations") == 0) {
            char* operations_str = strdup(value);
            char* token = strtok(operations_str, ",");
            int op_count = 0;
            operation_t ops[32];
            
            while (token != NULL && op_count < 32) {
                ops[op_count++] = parse_operation(trim_whitespace(token));
                token = strtok(NULL, ",");
            }
            
            if (op_count > 0) {
                config->custom_operations = malloc(sizeof(operation_t) * op_count);
                memcpy(config->custom_operations, ops, sizeof(operation_t) * op_count);
                config->custom_operation_count = op_count;
            }
            
            free(operations_str);
        }
    }
    
    return false;
}

static bool parse_packets_section(config_t* config, FILE* file) {
    char line[512];
    
    config->dataset = create_packet_dataset(100);
    if (!config->dataset) return false;
    
    
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim_whitespace(line);
        
        if (strlen(trimmed) == 0 || trimmed[0] == '#') continue;
        
        char packet_hex[256] = {0};
        char checksum_hex[64] = {0};
        char description[256] = {0};
        
        int parsed = sscanf(trimmed, "%255s %63s %255[^\n]", packet_hex, checksum_hex, description);
        if (parsed >= 2) {
            if (parsed == 2) {
                sprintf(description, "Packet %zu", config->dataset->count + 1);
            }
            
            size_t packet_len = strlen(packet_hex) / 2;
            uint8_t* packet_data = malloc(packet_len);
            
            for (size_t i = 0; i < packet_len; i++) {
                sscanf(&packet_hex[i * 2], "%2hhx", &packet_data[i]);
            }
            
            uint64_t checksum;
            sscanf(checksum_hex, "%llx", &checksum);
            
            add_packet_from_bytes(config->dataset, packet_data, packet_len, checksum, 1, description);
            free(packet_data);
        }
    }
    
    return config->dataset->count > 0;
}

config_t* load_cads_config(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Unable to open .cads config file: %s\n", filename);
        return NULL;
    }
    
    config_t* config = calloc(1, sizeof(config_t));
    if (!config) {
        fclose(file);
        return NULL;
    }
    
    config->complexity = COMPLEXITY_INTERMEDIATE;
    config->max_fields = 4;
    config->max_constants = 128;
    config->early_exit = false;
    config->max_solutions = 0;
    config->progress_interval = 250;
    config->verbose = false;
    config->threads = 1;
    
    char line[512];
    bool found_config = false;
    bool found_packets = false;
    
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = trim_whitespace(line);
        
        if (strncmp(trimmed, "[config]", 8) == 0) {
            found_config = true;
            if (parse_config_section(config, file)) {
                // parse_config_section returns true if it found and parsed [packets] too
                found_packets = true;
            } else {
                break;
            }
        } else if (strncmp(trimmed, "[packets]", 9) == 0) {
            found_packets = true;
            if (!parse_packets_section(config, file)) {
                fprintf(stderr, "Error: Failed to parse packets section\n");
                break;
            }
        }
    }
    
    fclose(file);
    
    
    if (!found_config && !found_packets) {
        fprintf(stderr, "Error: .cads file must contain at least [config] or [packets] section\n");
        free_cads_config(config);
        return NULL;
    }
    
    return config;
}

void free_cads_config(config_t* config) {
    if (!config) return;
    
    free(config->name);
    free(config->description);
    free(config->custom_operations);
    
    if (config->dataset) {
        free_packet_dataset(config->dataset);
    }
    
    free(config);
}

// Create default CADS configuration
config_t* create_default_cads_config(void) {
    config_t* config = calloc(1, sizeof(config_t));
    if (!config) return NULL;
    
    config->name = strdup("Default Configuration");
    config->description = strdup("Command-line configuration");
    config->complexity = COMPLEXITY_INTERMEDIATE;
    config->max_fields = 4;
    config->max_constants = 128;
    config->early_exit = false;
    config->max_solutions = 0;
    config->progress_interval = 250;
    config->verbose = false;
    config->threads = 1;
    config->custom_operations = NULL;
    config->custom_operation_count = 0;
    config->dataset = NULL;
    
    return config;
}

// Create CADS config from command line arguments
config_t* create_cads_config_from_cli(int argc, char* argv[]) {
    config_t* config = create_default_cads_config();
    if (!config) return NULL;
    
    const char* input_file = "tests/data/gmrs_test_dataset.jsonl";
    const char* cads_config_file = NULL;
    
    static struct option long_options[] = {
        {"input", required_argument, 0, 'i'},
        {"config", required_argument, 0, 'C'},
        {"complexity", required_argument, 0, 'c'},
        {"max-fields", required_argument, 0, 'f'},
        {"max-constants", required_argument, 0, 'k'},
        {"early-exit", no_argument, 0, 'e'},
        {"max-solutions", required_argument, 0, 'm'},
        {"progress-ms", required_argument, 0, 'p'},
        {"verbose", no_argument, 0, 'v'},
        {"threads", required_argument, 0, 't'},
        {"threading", no_argument, 0, 'T'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    optind = 1; // Reset getopt
    while ((c = getopt_long(argc, argv, "i:C:c:f:k:em:p:vt:Th", long_options, NULL)) != -1) {
        switch (c) {
            case 'i':
                input_file = optarg;
                break;
            case 'C':
                cads_config_file = optarg;
                break;
            case 'c':
                config->complexity = parse_complexity_level(optarg);
                break;
            case 'f':
                config->max_fields = atoi(optarg);
                break;
            case 'k':
                config->max_constants = atoi(optarg);
                break;
            case 'e':
                config->early_exit = true;
                config->max_solutions = 1;
                break;
            case 'm':
                config->max_solutions = atoi(optarg);
                break;
            case 'p':
                config->progress_interval = atoi(optarg);
                break;
            case 'v':
                config->verbose = true;
                break;
            case 't':
                config->threads = atoi(optarg);
                break;
            case 'T':
                config->threads = 0; // Auto-detect thread count
                break;
            case 'h':
                free_cads_config(config);
                return NULL; // Signal help requested
            case '?':
                free_cads_config(config);
                return NULL; // Signal error
        }
    }
    
    // If .cads file specified, load it but allow CLI args to override
    if (cads_config_file) {
        // Store CLI values before loading file
        int cli_threads = config->threads;
        bool cli_verbose = config->verbose;
        complexity_level_t cli_complexity = config->complexity;
        int cli_max_fields = config->max_fields;
        int cli_max_constants = config->max_constants;
        bool cli_early_exit = config->early_exit;
        int cli_max_solutions = config->max_solutions;
        int cli_progress_interval = config->progress_interval;
        
        // Track which CLI options were explicitly provided
        bool provided_threads = false;
        bool provided_verbose = false;
        bool provided_complexity = false;
        bool provided_max_fields = false;
        bool provided_max_constants = false;
        bool provided_early_exit = false;
        bool provided_max_solutions = false;
        bool provided_progress_interval = false;
        
        // Re-scan to detect which args were provided
        optind = 1;
        int temp_c;
        while ((temp_c = getopt_long(argc, argv, "i:C:c:f:k:em:p:vt:Th", long_options, NULL)) != -1) {
            switch (temp_c) {
                case 'c': provided_complexity = true; break;
                case 'f': provided_max_fields = true; break;
                case 'k': provided_max_constants = true; break;
                case 'e': provided_early_exit = true; break;
                case 'm': provided_max_solutions = true; break;
                case 'p': provided_progress_interval = true; break;
                case 'v': provided_verbose = true; break;
                case 't': provided_threads = true; break;
                case 'T': provided_threads = true; break;
            }
        }
        
        // Load file config
        config_t* file_config = load_cads_config(cads_config_file);
        if (!file_config) {
            free_cads_config(config);
            return NULL;
        }
        
        // Apply CLI overrides to file config
        if (provided_threads) file_config->threads = cli_threads;
        if (provided_verbose) file_config->verbose = cli_verbose;
        if (provided_complexity) file_config->complexity = cli_complexity;
        if (provided_max_fields) file_config->max_fields = cli_max_fields;
        if (provided_max_constants) file_config->max_constants = cli_max_constants;
        if (provided_early_exit) {
            file_config->early_exit = cli_early_exit;
            file_config->max_solutions = cli_max_solutions; // -e sets max_solutions = 1
        }
        if (provided_max_solutions) file_config->max_solutions = cli_max_solutions;
        if (provided_progress_interval) file_config->progress_interval = cli_progress_interval;
        
        free_cads_config(config);
        return file_config;
    }
    
    // Load packet data from JSON file
    if (!load_packets_into_cads_config(config, input_file)) {
        free_cads_config(config);
        return NULL;
    }
    
    return config;
}

// Load packets from JSON file into CADS config
bool load_packets_into_cads_config(config_t* config, const char* json_file) {
    if (!config || !json_file) return false;
    
    config->dataset = create_packet_dataset(100);
    if (!config->dataset) return false;
    
    return load_packets_from_json(config->dataset, json_file);
}

