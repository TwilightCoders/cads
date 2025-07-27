#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include "../../include/cads_types.h"

// CLI argument parsing
typedef struct {
    bool help;
    bool version;
    bool list_algorithms;
    bool estimate_time;
    complexity_level_t complexity;
    int max_fields;
    int max_constants;
    size_t checksum_size;
    bool verbose;
    bool little_endian;                // Checksum byte order
    bool early_exit;                   // Exit after finding first solution
    int max_solutions;                 // Maximum solutions to find (0 = unlimited)
    const char* input_file;            // Input packet file
    const char* output_file;           // JSON output file
    const char* resume_file;           // Checkpoint file
    int progress_interval;
    int threads;                       // Number of parallel threads (0 = auto-detect)
} cli_args_t;

// Parse command line arguments
bool parse_arguments(int argc, char* argv[], cli_args_t* args);

// Display help and usage information
void print_help(const char* program_name);
void print_version(void);
void print_algorithms(void);
void print_time_estimate(const cli_args_t* args);

// Validate argument combinations
bool validate_arguments(const cli_args_t* args);

// Convert CLI args to search config
search_config_t* create_search_config(const cli_args_t* args);

#endif // ARG_PARSER_H