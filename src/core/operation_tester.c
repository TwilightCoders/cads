#include "checksum_engine.h"
#include "../../include/algorithm_registry.h"
#include "../utils/field_combiner.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../include/sequence_evaluator.h"

// Recursive operation testing function
bool test_operation_sequence(const packet_dataset_t* dataset, 
                            const config_t* config,
                            const uint8_t* field_permutation,
                            int field_count,
                            const algorithm_registry_entry_t* algorithms,
                            int algorithm_count,
                            operation_t* operation_sequence,
                            int current_depth,
                            int max_depth,
                            uint8_t constant,
                            search_results_t* results,
                            uint64_t* tests_performed,
                            progress_tracker_t* tracker) {
    
    // Base case: we've built a complete operation sequence, test it
    if (current_depth >= max_depth) {
        (*tests_performed)++;
        
        // Update progress bar periodically (time-based) - only in single-threaded mode
        // In multi-threaded mode, progress is handled centrally to avoid display conflicts
        if (tracker && should_display_progress(tracker)) {
            update_progress(tracker, *tests_performed, results->solution_count);
            // Only display progress in single-threaded mode
            // Multi-threaded mode has dedicated progress monitor thread
            if (config->threads <= 1) {
                display_detailed_progress(tracker, "Testing");
            }
        }
        
    bool all_match = evaluate_operation_sequence(dataset, config, field_permutation, field_count, operation_sequence, max_depth, constant);
    if (all_match) {
            // Found a solution!
            checksum_solution_t solution = {0};
            for (int f = 0; f < field_count; f++) {
                solution.field_indices[f] = field_permutation[f];
            }
            solution.field_count = field_count;
            for (int op = 0; op < max_depth && op < 4; op++) {
                solution.operations[op] = operation_sequence[op];
            }
            solution.operation_count = max_depth;
            solution.constant = constant;
            solution.checksum_size = config->checksum_size;
            solution.validated = true;
            
            if (add_solution(results, &solution)) {
                // Move cursor down from progress area and print solution (preserving progress display)
                printf("\n🎉 SOLUTION #%zu FOUND!\n", results->solution_count);
                printf("   Fields: ");
                for (int f = 0; f < field_count; f++) {
                    printf("%d ", field_permutation[f]);
                }
                printf("\n   Operations: ");
                for (int op = 0; op < max_depth; op++) {
                    // Find algorithm name
                    for (int a = 0; a < algorithm_count; a++) {
                        if (algorithms[a].op == operation_sequence[op]) {
                            printf("%s ", algorithms[a].name);
                            break;
                        }
                    }
                }
                printf("\n   Constant: 0x%02X\n\n", constant);
            }
            return true;  // Found a solution
        }
        return false;  // No solution with this sequence
    }
    
    // Recursive case: try each algorithm at this depth
    for (int alg_idx = 0; alg_idx < algorithm_count; alg_idx++) {
        operation_sequence[current_depth] = algorithms[alg_idx].op;
        
        if (test_operation_sequence(dataset, config, field_permutation, field_count,
                                  algorithms, algorithm_count, operation_sequence,
                                  current_depth + 1, max_depth, constant, results, tests_performed, tracker)) {
            // Early exit if solution found and early exit enabled
            if (config->early_exit) {
                return true;
            }
        }
    }
    
    return false;
}
