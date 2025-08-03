#include "progress_tracker.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

// Generic exponential moving average function
static double exponential_moving_average(double current_value, double previous_smoothed, double alpha, bool is_first_update) {
    if (is_first_update || previous_smoothed == 0.0) {
        return current_value;
    }
    if (current_value <= 0) {
        return previous_smoothed;  // Keep previous value if current is invalid
    }
    return alpha * current_value + (1.0 - alpha) * previous_smoothed;
}

// Initialize progress tracker
void init_progress_tracker(progress_tracker_t* tracker, uint64_t total_combinations, int interval_ms) {
    if (!tracker) return;
    
    tracker->total_combinations = total_combinations;
    tracker->completed_tests = 0;
    tracker->tests_at_last_update = 0;
    tracker->avg_tests_per_second = 0.0;
    tracker->smoothed_rate = 0.0;
    tracker->smoothed_eta = 0.0;
    gettimeofday(&tracker->start_time, NULL);
    tracker->last_update = tracker->start_time;
    tracker->last_progress_display = tracker->start_time;
    tracker->solutions_found = 0;
    tracker->progress_interval_ms = interval_ms;
}

// Update progress tracking
void update_progress(progress_tracker_t* tracker, uint64_t completed_tests, int solutions_found) {
    if (!tracker) return;
    
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    tracker->completed_tests = completed_tests;
    tracker->solutions_found = solutions_found;
    
    // Calculate time since last update in seconds (with microsecond precision)
    double time_since_last = (current_time.tv_sec - tracker->last_update.tv_sec) + 
                            (current_time.tv_usec - tracker->last_update.tv_usec) / 1000000.0;
    
    // Calculate instantaneous rate since last update
    double instantaneous_rate = 0.0;
    if (time_since_last > 0) {
        uint64_t tests_since_last = completed_tests - tracker->tests_at_last_update;
        instantaneous_rate = (double)tests_since_last / time_since_last;
    }
    
    // Calculate overall average rate
    double elapsed = (current_time.tv_sec - tracker->start_time.tv_sec) + 
                    (current_time.tv_usec - tracker->start_time.tv_usec) / 1000000.0;
    if (elapsed > 0) {
        tracker->avg_tests_per_second = (double)completed_tests / elapsed;
    }
    
    // Update smoothed rate using exponential moving average
    // Alpha = 0.2 gives good balance between responsiveness and stability
    const double alpha_rate = 0.2;
    bool is_first_rate_update = (tracker->smoothed_rate == 0.0);
    tracker->smoothed_rate = exponential_moving_average(instantaneous_rate, tracker->smoothed_rate, alpha_rate, is_first_rate_update);
    
    // Update smoothed ETA using more aggressive smoothing for stability
    // Alpha = 0.5 for ETA gives more stable estimates than rate smoothing
    const double alpha_eta = 0.5;
    
    // Calculate raw ETA using smoothed rate (more stable than instantaneous rate)
    if (tracker->smoothed_rate > 0 && tracker->completed_tests < tracker->total_combinations) {
        uint64_t remaining_tests = tracker->total_combinations - tracker->completed_tests;
        double raw_eta = (double)remaining_tests / tracker->smoothed_rate;
        bool is_first_eta_update = (tracker->smoothed_eta == 0.0);
        tracker->smoothed_eta = exponential_moving_average(raw_eta, tracker->smoothed_eta, alpha_eta, is_first_eta_update);
    } else if (tracker->completed_tests >= tracker->total_combinations) {
        // Set ETA to 0 when we've completed the work
        tracker->smoothed_eta = 0.0;
    }
    // If we can't calculate a new ETA, keep the previous smoothed value (don't reset to 0)
    
    // Update tracking variables
    tracker->tests_at_last_update = completed_tests;
    tracker->last_update = current_time;
}

// Finish progress tracking
void finish_progress(progress_tracker_t* tracker) {
    if (!tracker) return;
    
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    tracker->completed_tests = tracker->total_combinations;
    tracker->last_update = end_time;
    
    // Final calculation
    double elapsed = (end_time.tv_sec - tracker->start_time.tv_sec) + 
                    (end_time.tv_usec - tracker->start_time.tv_usec) / 1000000.0;
    if (elapsed > 0) {
        tracker->avg_tests_per_second = (double)tracker->total_combinations / elapsed;
    }
}

// Calculate ETA in seconds using smoothed rate and ETA smoothing
double calculate_eta_seconds(const progress_tracker_t* tracker) {
    if (!tracker || tracker->smoothed_rate <= 0) return -1;
    
    // If we've completed more tests than estimated, ETA is 0
    if (tracker->completed_tests >= tracker->total_combinations) {
        return 0;
    }
    
    uint64_t remaining_tests = tracker->total_combinations - tracker->completed_tests;
    double raw_eta = (double)remaining_tests / tracker->smoothed_rate;
    
    // Return the smoothed ETA if we have one, otherwise return raw ETA
    return tracker->smoothed_eta > 0 ? tracker->smoothed_eta : raw_eta;
}

// Calculate elapsed time in seconds
double calculate_elapsed_seconds(const progress_tracker_t* tracker) {
    if (!tracker) return 0;
    return (tracker->last_update.tv_sec - tracker->start_time.tv_sec) + 
           (tracker->last_update.tv_usec - tracker->start_time.tv_usec) / 1000000.0;
}

// Calculate current tests per second
double calculate_tests_per_second(const progress_tracker_t* tracker) {
    if (!tracker) return 0;
    return tracker->avg_tests_per_second;
}

// Format duration as human readable string with two most significant units
void format_duration(double seconds, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    if (seconds < 0) {
        snprintf(buffer, buffer_size, "unknown");
        return;
    }
    
    int days = (int)(seconds / 86400);
    int hours = (int)((seconds - days * 86400) / 3600);
    int minutes = (int)((seconds - days * 86400 - hours * 3600) / 60);
    int secs = (int)(seconds - days * 86400 - hours * 3600 - minutes * 60);
    
    if (days > 0) {
        // Show days and hours
        snprintf(buffer, buffer_size, "%dd %dh", days, hours);
    } else if (hours > 0) {
        // Show hours and minutes
        snprintf(buffer, buffer_size, "%dh %dm", hours, minutes);
    } else if (minutes > 0) {
        // Show minutes and seconds
        snprintf(buffer, buffer_size, "%dm %ds", minutes, secs);
    } else {
        // Show just seconds
        snprintf(buffer, buffer_size, "%ds", secs);
    }
}

// Format large numbers with appropriate units
// Number formatting scale mapping
typedef struct {
    double threshold;
    double divisor;
    const char* suffix;
} number_scale_t;

static const number_scale_t number_scales[] = {
    {1000000000000.0, 1000000000000.0, "T"},
    {1000000000.0,    1000000000.0,    "B"},
    {1000000.0,       1000000.0,       "M"},
    {1000.0,          1000.0,          "K"},
    {0.0,             1.0,             ""}  // Base case
};

static const size_t num_scales = sizeof(number_scales) / sizeof(number_scales[0]);

void format_large_number(uint64_t number, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    double num = (double)number;
    
    for (size_t i = 0; i < num_scales; i++) {
        if (num >= number_scales[i].threshold) {
            if (strlen(number_scales[i].suffix) > 0) {
                snprintf(buffer, buffer_size, "%.1f%s", num / number_scales[i].divisor, number_scales[i].suffix);
            } else {
                snprintf(buffer, buffer_size, "%llu", (unsigned long long)number);
            }
            return;
        }
    }
}

void format_rate(double rate, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    for (size_t i = 0; i < num_scales; i++) {
        if (rate >= number_scales[i].threshold) {
            if (strlen(number_scales[i].suffix) > 0) {
                snprintf(buffer, buffer_size, "%.1f%s", rate / number_scales[i].divisor, number_scales[i].suffix);
            } else {
                snprintf(buffer, buffer_size, "%.1f", rate);
            }
            return;
        }
    }
}


// Display detailed progress information
void display_detailed_progress(const progress_tracker_t* tracker, const char* current_operation) {
    if (!tracker) return;
    
    char completed_str[32], total_str[32], rate_str[32];
    char elapsed_str[64], eta_str[64];
    
    format_large_number(tracker->completed_tests, completed_str, sizeof(completed_str));
    format_large_number(tracker->total_combinations, total_str, sizeof(total_str));
    
    // Format rate using programmatic mapping
    format_rate(tracker->smoothed_rate, rate_str, sizeof(rate_str));
    
    double elapsed = calculate_elapsed_seconds(tracker);
    format_duration(elapsed, elapsed_str, sizeof(elapsed_str));
    
    double eta = calculate_eta_seconds(tracker);
    format_duration(eta, eta_str, sizeof(eta_str));
    
    double percentage = 0.0;
    if (tracker->total_combinations > 0) {
        percentage = (double)tracker->completed_tests / tracker->total_combinations * 100.0;
    }
    
    // Build the progress line with padding to ensure clean overwrites
    char progress_line[256];
    int written = snprintf(progress_line, sizeof(progress_line),
                          COLOR_CYAN "Progress: " COLOR_RESET "%s/%s (%.1f%%) | " 
                          COLOR_YELLOW "Rate: " COLOR_RESET "%s tests/sec | "
                          COLOR_BLUE "Elapsed: " COLOR_RESET "%s | "
                          COLOR_GREEN "ETA: " COLOR_RESET "%s | "
                          COLOR_BOLD "Solutions: %d" COLOR_RESET,
                          completed_str, total_str, percentage,
                          rate_str, elapsed_str, eta_str, tracker->solutions_found);
    
    if (current_operation && written < (int)sizeof(progress_line) - 10) {
        written += snprintf(progress_line + written, sizeof(progress_line) - written, " | %s", current_operation);
    }
    
    // Pad with spaces to ensure we overwrite any previous longer text (120 chars total)
    while (written < 120 && written < (int)sizeof(progress_line) - 1) {
        progress_line[written++] = ' ';
    }
    progress_line[written] = '\0';
    
    // Use ANSI escape to move up one line if this isn't the first update
    static bool first_update = true;
    if (!first_update) {
        printf("\033[A"); // Move cursor up one line
    }
    first_update = false;
    
    printf("\r%s\n", progress_line);
    // Print a blank second line for the cursor to rest on
    for (int i = 0; i < 80; i++) printf(" ");
    printf("\r");
    fflush(stdout);
}

// Clear the current progress line
void clear_progress_line(void) {
    printf("\033[A"); // Move up to progress line
    printf("\r");
    for (int i = 0; i < 120; i++) printf(" ");
    printf("\n\r");
    for (int i = 0; i < 80; i++) printf(" ");
    printf("\r");
    fflush(stdout);
}

// Check if enough time has passed to display progress update
bool should_display_progress(progress_tracker_t* tracker) {
    if (!tracker) return false;
    
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    // Calculate milliseconds since last progress display
    long time_diff_ms = (current_time.tv_sec - tracker->last_progress_display.tv_sec) * 1000 + 
                       (current_time.tv_usec - tracker->last_progress_display.tv_usec) / 1000;
    
    if (time_diff_ms >= tracker->progress_interval_ms) {
        tracker->last_progress_display = current_time;
        return true;
    }
    
    return false;
}

// Display final summary
void display_final_summary(const progress_tracker_t* tracker) {
    if (!tracker) return;
    
    printf("\n\n" COLOR_BOLD "═══════════════════════════════════════════════════════════════" COLOR_RESET "\n");
    printf(COLOR_BOLD "                         ANALYSIS COMPLETE                        " COLOR_RESET "\n");
    printf(COLOR_BOLD "═══════════════════════════════════════════════════════════════" COLOR_RESET "\n\n");
    
    char total_str[32], rate_str[32], elapsed_str[64];
    format_large_number(tracker->completed_tests, total_str, sizeof(total_str));
    // Use smoothed rate which represents actual throughput, not misleading average
    format_rate(tracker->smoothed_rate, rate_str, sizeof(rate_str));
    // double corrected_rate = tracker->avg_tests_per_second * (tracker->progress_interval_ms / 1000.0);
    // format_large_number((uint64_t)corrected_rate, rate_str, sizeof(rate_str));
    
    double elapsed = calculate_elapsed_seconds(tracker);
    format_duration(elapsed, elapsed_str, sizeof(elapsed_str));
    
    printf(COLOR_CYAN "Tests Performed: " COLOR_RESET COLOR_BOLD "%s" COLOR_RESET "\n", total_str);
    printf(COLOR_YELLOW "Average Rate: " COLOR_RESET COLOR_BOLD "%s tests/sec" COLOR_RESET "\n", rate_str);
    printf(COLOR_BLUE "Total Time: " COLOR_RESET COLOR_BOLD "%s" COLOR_RESET "\n", elapsed_str);
    printf(COLOR_GREEN "Solutions Found: " COLOR_RESET COLOR_BOLD "%d" COLOR_RESET "\n", tracker->solutions_found);
    
    if (tracker->solutions_found > 0) {
        printf("\n" COLOR_GREEN "✅ Success! " COLOR_RESET "Found working checksum algorithm(s).\n");
    } else {
        printf("\n" COLOR_YELLOW "⚠️  No solutions found " COLOR_RESET "with current parameters.\n");
        printf("Consider:\n");
        printf("• Increasing complexity level (--complexity intermediate or advanced)\n");
        printf("• Increasing max fields (--max-fields N)\n");
        printf("• Checking packet data format\n");
    }
    
    printf("\n");
}
