#ifndef PROGRESS_TRACKER_H
#define PROGRESS_TRACKER_H

#include "../../include/cads_types.h"

// Progress display functions
void init_progress_tracker(progress_tracker_t* tracker, uint64_t total_combinations, int interval_sec);
void update_progress(progress_tracker_t* tracker, uint64_t completed_tests, int solutions_found);
void finish_progress(progress_tracker_t* tracker);

// Time calculation utilities
double calculate_eta_seconds(const progress_tracker_t* tracker);
double calculate_elapsed_seconds(const progress_tracker_t* tracker);
double calculate_tests_per_second(const progress_tracker_t* tracker);

// Progress display formatting
void display_progress_bar(const progress_tracker_t* tracker);
void display_detailed_progress(const progress_tracker_t* tracker, const char* current_operation);
void display_final_summary(const progress_tracker_t* tracker);
void clear_progress_line(void);

// Time formatting utilities
void format_duration(double seconds, char* buffer, size_t buffer_size);
void format_large_number(uint64_t number, char* buffer, size_t buffer_size);

// Progress bar characters and styling
#define PROGRESS_BAR_WIDTH 50
#define PROGRESS_CHAR_FILLED "█"
#define PROGRESS_CHAR_PARTIAL "▓"
#define PROGRESS_CHAR_EMPTY "░"

// ANSI color codes for terminal output
#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_CYAN "\033[36m"
#define COLOR_BOLD "\033[1m"

#endif // PROGRESS_TRACKER_H