#include "search_display.h"
#include "hardware_benchmark.h"
#include "../core/progress_tracker.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

// Universal unit mapping for scalar => unit formatting
typedef struct {
    double threshold;
    double divisor;
    const char* suffix;
    bool show_remainder;
    const char* format_str;  // Format string for primary value (e.g., "%.1f%s", "%d%s")
} unit_mapping_t;

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Number unit mapping for large number formatting
static const unit_mapping_t format_number_units[] = {
    {1000000000000.0, 1000000000000.0, "T", false, "%.1f%s"},
    {1000000000.0,    1000000000.0,    "B", false, "%.1f%s"},
    {1000000.0,       1000000.0,       "M", false, "%.1f%s"},
    {1000.0,          1000.0,          "K", false, "%.1f%s"},
    {0.0,             1.0,             "", false, "%.0f%s"}  // Base case
};

// Time unit mapping for duration formatting (seconds to years)
static const unit_mapping_t format_time_units[] = {
    {31536000.0, 31536000.0, "y", true, "%d%s"},    // Years - show months remainder
    {2592000.0,  2592000.0,  "mo", true, "%d%s"},   // Months - show days remainder
    {604800.0,   604800.0,   "w", true, "%d%s"},    // Weeks - show days remainder
    {86400.0,    86400.0,    "d", true, "%d%s"},    // Days - show hours remainder
    {3600.0,     3600.0,     "h", true, "%d%s"},    // Hours - show minutes remainder
    {60.0,       60.0,       "m", true, "%d%s"},    // Minutes - show seconds remainder  
    {0.0,        1.0,        "s", false, "%.0f%s"}  // Seconds - no remainder
};

// Universal formatter that works with any unit mapping
static int format_with_units(double value, char* buffer, size_t buffer_size, 
                             const unit_mapping_t* units, size_t num_units) {
    if (!buffer || buffer_size == 0 || !units) {
        return -1;
    }
    
    for (size_t i = 0; i < num_units; i++) {
        if (value < units[i].threshold) {
            continue;
        }
        
        int primary = (int)(value / units[i].divisor);
        
        if (units[i].show_remainder && i + 1 < num_units) {
            // Calculate remainder for next unit
            double remainder_value = value - (primary * units[i].divisor);
            int remainder = (int)(remainder_value / units[i + 1].divisor);
            
            if (remainder > 0) {
                return snprintf(buffer, buffer_size, "%d%s %d%s", 
                               primary, units[i].suffix, 
                               remainder, units[i + 1].suffix);
            } else {
                return snprintf(buffer, buffer_size, "%d%s", primary, units[i].suffix);
            }
        } else {
            // Use the format string from the mapping
            if (units[i].suffix[0] != '\0') {  // Compile-time check instead of strlen()
                double formatted_value = value / units[i].divisor;
                return snprintf(buffer, buffer_size, units[i].format_str, formatted_value, units[i].suffix);
            } else {
                // Special case for base unit with no suffix
                return snprintf(buffer, buffer_size, "%.0f", value);
            }
        }
    }
    
    // Fallback - should never reach here with properly ordered units
    return snprintf(buffer, buffer_size, "%.0f", value);
}


// Display search space estimation with hardware-calibrated complexity emojis
void display_search_estimation(const packet_dataset_t* dataset, 
                              const config_t* config,
                              int algorithm_count,
                              const hardware_benchmark_result_t* benchmark) {
    
    if (!dataset || !config) return;
    
    // Calculate minimum packet length
    size_t min_packet_length = dataset->packets[0].packet_length;
    for (size_t i = 1; i < dataset->count; i++) {
        if (dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = dataset->packets[i].packet_length;
        }
    }
    
    // Calculate search space estimation (same logic as recursive engine)
    uint64_t permutations = 1;
    for (int i = 0; i < config->max_fields && i < (int)min_packet_length; i++) {
        permutations *= (min_packet_length - i);
    }
    
    // FIXED: Calculate for ALL complexity levels like original, not just max_fields
    uint64_t operation_sequences = 0;
    
    // Sum operation sequences across all complexity levels (1 to max_fields)
    for (int complexity = 1; complexity <= config->max_fields; complexity++) {
        uint64_t ops_for_complexity = 1;
        // Allow field_count+1 operations for complex patterns
        for (int i = 0; i < complexity + 1; i++) {
            ops_for_complexity *= algorithm_count;
        }
        operation_sequences += ops_for_complexity;
    }
    
    uint64_t estimated_tests = permutations * operation_sequences * config->max_constants;
    
    // Format the number with hardware-calibrated complexity emojis
    char formatted_tests_buffer[16];
    char emoji_buffer[64];
    char time_estimate_buffer[64];
    double baseline_tests_per_second = benchmark && benchmark->valid ? benchmark->tests_per_second : 15000000.0; // Default fallback
    (void)format_with_units((double)estimated_tests, formatted_tests_buffer, sizeof(formatted_tests_buffer), format_number_units, ARRAY_SIZE(format_number_units));
    
    printf("🔢 Search Space Estimation:\n");
    printf("   Field permutations: %llu\n", (unsigned long long)permutations);
    printf("   Operation sequences: %llu\n", (unsigned long long)operation_sequences);
    printf("   Constants to test: %d\n", config->max_constants);
    
    if (benchmark && benchmark->valid) {
        get_time_based_complexity_emojis(estimated_tests, baseline_tests_per_second, emoji_buffer, time_estimate_buffer);
        printf("   📊 Total domain size: %s tests %s %s\n", formatted_tests_buffer, emoji_buffer, time_estimate_buffer);
        printf("   💻 Hardware baseline: %.1fM tests/sec\n\n", baseline_tests_per_second / 1000000.0);
    } else {
        printf("   📊 Total domain size: %s tests %s\n\n", formatted_tests_buffer, emoji_buffer);
    }
}

// CADS-specific version that works with config_t
void display_search_estimation_cads(const config_t* config,
                                   int algorithm_count,
                                   const hardware_benchmark_result_t* benchmark) {
    
    if (!config || !config->dataset) return;
    
    const packet_dataset_t* dataset = config->dataset;
    
    // Calculate minimum packet length
    size_t min_packet_length = dataset->packets[0].packet_length;
    for (size_t i = 1; i < dataset->count; i++) {
        if (dataset->packets[i].packet_length < min_packet_length) {
            min_packet_length = dataset->packets[i].packet_length;
        }
    }
    
    // Calculate search space estimation
    uint64_t permutations = 1;
    for (int i = 0; i < config->max_fields && i < (int)min_packet_length; i++) {
        permutations *= (min_packet_length - i);
    }
    
    // FIXED: Calculate for ALL complexity levels like original, not just max_fields
    uint64_t operation_sequences = 0;
    
    // Sum operation sequences across all complexity levels (1 to max_fields)
    for (int complexity = 1; complexity <= config->max_fields; complexity++) {
        uint64_t ops_for_complexity = 1;
        // Allow field_count+1 operations for complex patterns
        for (int i = 0; i < complexity + 1; i++) {
            ops_for_complexity *= algorithm_count;
        }
        operation_sequences += ops_for_complexity;
    }
    
    uint64_t estimated_tests = permutations * operation_sequences * config->max_constants;
    
    // Format the number with hardware-calibrated complexity emojis
    char formatted_tests_buffer[16];
    char emoji_buffer[64];
    char time_estimate_buffer[64];
    double baseline_tests_per_second = benchmark && benchmark->valid ? benchmark->tests_per_second : 15000000.0; // Default fallback
    (void)format_with_units((double)estimated_tests, formatted_tests_buffer, sizeof(formatted_tests_buffer), format_number_units, ARRAY_SIZE(format_number_units));
    
    printf("🔢 Search Space Estimation:\n");
    printf("   Field permutations: %llu\n", (unsigned long long)permutations);
    printf("   Operation sequences: %llu\n", (unsigned long long)operation_sequences);
    printf("   Constants to test: %d\n", config->max_constants);
    
    if (benchmark && benchmark->valid) {
        get_time_based_complexity_emojis(estimated_tests, baseline_tests_per_second, emoji_buffer, time_estimate_buffer);
        printf("   📊 Total domain size: %s tests %s %s\n", formatted_tests_buffer, emoji_buffer, time_estimate_buffer);
        printf("   💻 Hardware baseline: %.1fM tests/sec\n\n", baseline_tests_per_second / 1000000.0);
    } else {
        printf("   📊 Total domain size: %s tests %s\n\n", formatted_tests_buffer, emoji_buffer);
    }
}




// Progress bar token flags for conditional display
typedef enum {
    PROGRESS_SHOW_RATE     = 1 << 0,
    PROGRESS_SHOW_ELAPSED  = 1 << 1,
    PROGRESS_SHOW_ETA      = 1 << 2,
    PROGRESS_SHOW_SOLUTIONS = 1 << 3
} progress_token_flags_t;

// Progress bar builder - creates a formatted progress bar with optional tokens
static void build_progress_bar(const char* label, uint64_t completed, uint64_t total, double rate, 
                              double elapsed_seconds, const char* eta_str, int solutions, 
                              const char* color_code, int bar_width, bool indent, 
                              progress_token_flags_t show_flags) {
    // Create progress bar
    double progress_pct = 0.0;
    if (total > 0) {
        progress_pct = (double)completed / total * 100.0;
        if (progress_pct > 100.0) progress_pct = 100.0;
    }
    
    int filled_bars = (int)(progress_pct / 100.0 * bar_width);
    
    // Format numbers using helper functions
    char completed_str[16], total_str[16], rate_str[16], elapsed_str[16];
    (void)format_with_units((double)completed, completed_str, sizeof(completed_str), format_number_units, ARRAY_SIZE(format_number_units));
    (void)format_with_units((double)total, total_str, sizeof(total_str), format_number_units, ARRAY_SIZE(format_number_units));
    (void)format_with_units(rate, rate_str, sizeof(rate_str), format_number_units, ARRAY_SIZE(format_number_units));
    (void)format_with_units(elapsed_seconds, elapsed_str, sizeof(elapsed_str), format_time_units, ARRAY_SIZE(format_time_units));
    
    // Print the progress bar (with or without indentation)
    const char* indent_str = indent ? "       " : "";  // 5 spaces for thread indentation
    printf("%s%s%s:%s [", indent_str, color_code, label, COLOR_RESET);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled_bars) {
            printf("█");
        } else {
            printf("░");
        }
    }
    
    // Print progress stats with fixed-width formatting for tabular alignment
    // Max widths: number=6, percentage=8, rate=8, time=8, solutions=3
    // Order: Progress | Rate | Solutions | ETA | Time
    printf("] %6s/%6s %8.1f%% |", completed_str, total_str, progress_pct);
    
    if (show_flags & PROGRESS_SHOW_RATE) {
        printf(" %sRate:%s %6s/s |", COLOR_YELLOW, COLOR_RESET, rate_str);
    }
    
    if (show_flags & PROGRESS_SHOW_SOLUTIONS) {
        printf(" %sSolutions:%s %3d |", COLOR_GREEN, COLOR_RESET, solutions);
    }
    
    if (show_flags & PROGRESS_SHOW_ETA) {
        printf(" %sETA:%s %8s |", COLOR_GREEN, COLOR_RESET, eta_str);
    }
    
    if (show_flags & PROGRESS_SHOW_ELAPSED) {
        printf(" %sTime:%s %8s", COLOR_CYAN, COLOR_RESET, elapsed_str);
    }
    
    printf("\n");
}

// Multi-thread progress display with individual thread progress bars
void display_per_thread_progress(thread_progress_t** all_progress, int num_threads, progress_tracker_t* tracker) {
    static bool first_display = true;
    static int last_num_threads = 0;
    
    // Calculate cursor movement for multi-line display (threads + overall = num_threads + 1 lines)
    if (first_display || num_threads != last_num_threads) {
        // Clear any existing progress lines
        if (!first_display) {
            for (int i = 0; i < last_num_threads + 1; i++) {
                printf("\033[A\033[2K"); // Move up one line and clear it
            }
        }
        first_display = false;
        last_num_threads = num_threads;
    } else {
        // Move cursor up to start of progress area (num_threads + 1 lines)
        for (int i = 0; i < num_threads + 1; i++) {
            printf("\033[A\033[2K"); // Move up one line and clear it
        }
    }
    
    // Display per-thread progress bars first
    for (int i = 0; i < num_threads; i++) {
        thread_progress_t* progress = all_progress[i];
        
        // Lock and read thread progress
        pthread_mutex_lock(&progress->mutex);
        uint64_t thread_tests = progress->tests_performed;
        double thread_rate = progress->current_rate;
        time_t thread_start_time = progress->start_time;
        time_t thread_last_update = progress->last_update;
        bool thread_completed = progress->completed;
        int thread_solutions = progress->solutions_found;
        pthread_mutex_unlock(&progress->mutex);
        
        // Calculate thread's estimated portion and elapsed time
        uint64_t thread_estimated = tracker->total_combinations / num_threads;
        double elapsed_seconds = difftime(time(NULL), thread_start_time);
        
        // Calculate thread ETA
        char thread_eta_str[16];
        bool thread_complete = thread_completed || (thread_tests >= thread_estimated);
        
        // Detect stalled threads - no progress update for more than 3 seconds (and not completed)
        time_t current_time = time(NULL);
        bool thread_stalled = !thread_completed && (current_time - thread_last_update > 3);
        
        if (thread_complete) {
            snprintf(thread_eta_str, sizeof(thread_eta_str), "Done");
        } else if (thread_stalled) {
            snprintf(thread_eta_str, sizeof(thread_eta_str), "Stalled");
        } else if (thread_rate > 0) {
            double remaining_thread_tests = thread_estimated - thread_tests;
            if (remaining_thread_tests > 0) {
                double thread_eta_seconds = remaining_thread_tests / thread_rate;
                if (thread_eta_seconds >= 31536000.0 * 100) { // >= 100 years
                    snprintf(thread_eta_str, sizeof(thread_eta_str), "∞");
                } else {
                    (void)format_with_units(thread_eta_seconds, thread_eta_str, sizeof(thread_eta_str), format_time_units, ARRAY_SIZE(format_time_units));
                }
            } else {
                snprintf(thread_eta_str, sizeof(thread_eta_str), "0s");
            }
        } else {
            snprintf(thread_eta_str, sizeof(thread_eta_str), "∞");
        }
        
        // Create thread label
        char thread_label[32];
        snprintf(thread_label, sizeof(thread_label), "Thread %d", i);
        
        // Use progress bar builder for this thread (20 chars wide, indented)
        // For threads: show Rate + (ETA or Elapsed) + Solutions, but not both time types
        progress_token_flags_t thread_flags = PROGRESS_SHOW_RATE | PROGRESS_SHOW_SOLUTIONS;
        if (thread_complete) {
            thread_flags |= PROGRESS_SHOW_ELAPSED;  // Show elapsed time when done
        } else {
            thread_flags |= PROGRESS_SHOW_ETA;      // Show ETA while running
        }
        
        build_progress_bar(thread_label, thread_tests, thread_estimated, thread_rate, 
                          elapsed_seconds, thread_eta_str, thread_solutions, COLOR_BLUE, 20, true, thread_flags);
    }
    
    // Display total progress bar using builder
    double overall_elapsed = difftime(time(NULL), tracker->start_time.tv_sec);
    
    // Calculate overall ETA
    char overall_eta_str[32];
    bool overall_complete = (tracker->completed_tests >= tracker->total_combinations);
    if (overall_complete) {
        snprintf(overall_eta_str, sizeof(overall_eta_str), "Done");
    } else if (tracker->smoothed_rate > 0) {
        double remaining_tests = tracker->total_combinations - tracker->completed_tests;
        double eta_seconds = remaining_tests / tracker->smoothed_rate;
        if (eta_seconds >= 31536000.0 * 100) { // >= 100 years
            snprintf(overall_eta_str, sizeof(overall_eta_str), "∞");
        } else {
            (void)format_with_units(eta_seconds, overall_eta_str, sizeof(overall_eta_str), format_time_units, ARRAY_SIZE(format_time_units));
        }
    } else {
        snprintf(overall_eta_str, sizeof(overall_eta_str), "∞");
    }
    
    // Use progress bar builder for total progress (30 chars wide, not indented)
    // For overall: show all tokens (comprehensive view)
    progress_token_flags_t overall_flags = PROGRESS_SHOW_RATE | PROGRESS_SHOW_ELAPSED | 
                                         PROGRESS_SHOW_ETA | PROGRESS_SHOW_SOLUTIONS;
    
    build_progress_bar("Total", tracker->completed_tests, tracker->total_combinations, 
                      tracker->smoothed_rate, overall_elapsed, overall_eta_str, 
                      tracker->solutions_found, COLOR_CYAN, 30, false, overall_flags);
}
