#include <stdio.h>
#include <stdlib.h>
#include "../../include/cads_types.h"
#include "../../include/algorithm_registry.h"

int main(int argc, char* argv[]) {
    printf("CADS - Checksum Algorithm Discovery System v4.0\n");
    printf("Modular architecture test build\n\n");
    
    // Initialize algorithm registry
    if (!initialize_algorithm_registry()) {
        fprintf(stderr, "Error: Failed to initialize algorithm registry\n");
        return 1;
    }
    
    // Test basic functionality
    int total_algorithms;
    const algorithm_registry_entry_t* all_algorithms = get_all_algorithms(&total_algorithms);
    
    printf("Algorithm Registry Test:\n");
    printf("Total algorithms loaded: %d\n\n", total_algorithms);
    
    // Test complexity levels
    const char* complexity_names[] = {"Basic", "Intermediate", "Advanced", "All"};
    for (int i = 0; i < NUM_COMPLEXITY_LEVELS; i++) {
        int count;
        const algorithm_registry_entry_t* algorithms = get_algorithms_by_complexity(i, &count);
        printf("%s algorithms: %d\n", complexity_names[i], count);
    }
    
    printf("\nBuild test successful!\n");
    
    // Cleanup
    cleanup_algorithm_registry();
    
    return 0;
}