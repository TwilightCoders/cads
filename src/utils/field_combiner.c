#include "field_combiner.h"
#include <stdlib.h>
#include <string.h>

// Create field combination generator
field_combination_generator_t* create_field_generator(size_t packet_length, uint8_t max_fields) {
    if (packet_length == 0 || max_fields == 0 || max_fields > packet_length) return NULL;
    
    field_combination_generator_t* generator = malloc(sizeof(field_combination_generator_t));
    if (!generator) return NULL;
    
    generator->fields = malloc(max_fields * sizeof(uint8_t));
    if (!generator->fields) {
        free(generator);
        return NULL;
    }
    
    generator->field_count = 0;
    generator->packet_length = packet_length;
    generator->current_mask = 0;
    generator->max_fields = max_fields;
    
    return generator;
}

// Free field generator
void free_field_generator(field_combination_generator_t* generator) {
    if (!generator) return;
    free(generator->fields);
    free(generator);
}

// Generate next field combination
bool next_field_combination(field_combination_generator_t* generator) {
    if (!generator) return false;
    
    // Increment to next valid combination
    do {
        generator->current_mask++;
        
        // Check if we've exhausted all combinations
        if (generator->current_mask >= (1 << generator->packet_length)) {
            return false;
        }
        
        // Count set bits and extract field indices
        generator->field_count = 0;
        for (size_t i = 0; i < generator->packet_length; i++) {
            if (generator->current_mask & (1 << i)) {
                if (generator->field_count < generator->max_fields) {
                    generator->fields[generator->field_count] = i;
                    generator->field_count++;
                } else {
                    // Too many fields, continue to next combination
                    generator->field_count = generator->max_fields + 1;
                    break;
                }
            }
        }
        
    } while (generator->field_count == 0 || generator->field_count > generator->max_fields);
    
    return true;
}

// Reset field generator
void reset_field_generator(field_combination_generator_t* generator) {
    if (!generator) return;
    generator->current_mask = 0;
    generator->field_count = 0;
}

// Calculate factorial for permutation count
static uint32_t factorial(uint8_t n) {
    if (n <= 1) return 1;
    uint32_t result = 1;
    for (uint8_t i = 2; i <= n; i++) {
        result *= i;
        // Prevent overflow for large factorials
        if (result > 1000000) return 1000000; // Cap at 1M permutations
    }
    return result;
}

// Calculate total permutations
uint32_t calculate_total_permutations(uint8_t field_count) {
    return factorial(field_count);
}

// Create permutation generator
permutation_generator_t* create_permutation_generator(const uint8_t* fields, uint8_t field_count) {
    if (!fields || field_count == 0 || field_count > CADS_MAX_FIELDS) return NULL;
    
    permutation_generator_t* generator = malloc(sizeof(permutation_generator_t));
    if (!generator) return NULL;
    
    // Copy initial permutation
    memcpy(generator->permutation, fields, field_count);
    generator->field_count = field_count;
    generator->current_index = 0;
    generator->total_permutations = calculate_total_permutations(field_count);
    
    return generator;
}

// Free permutation generator
void free_permutation_generator(permutation_generator_t* generator) {
    if (generator) free(generator);
}

// Swap two elements
static void swap_elements(uint8_t* a, uint8_t* b) {
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
}

// Generate next permutation using Heap's algorithm
bool next_permutation(permutation_generator_t* generator) {
    if (!generator || generator->current_index >= generator->total_permutations) {
        return false;
    }
    
    generator->current_index++;
    
    // For the first permutation, just return the current state
    if (generator->current_index == 1) {
        return true;
    }
    
    // Use Heap's algorithm for efficient permutation generation
    // This is a simplified version for small field counts
    static uint8_t c[CADS_MAX_FIELDS] = {0};
    static uint8_t i = 0;
    
    if (generator->current_index == 2) {
        // Initialize c array and i
        memset(c, 0, sizeof(c));
        i = 0;
    }
    
    while (i < generator->field_count) {
        if (c[i] < i) {
            if (i % 2 == 0) {
                swap_elements(&generator->permutation[0], &generator->permutation[i]);
            } else {
                swap_elements(&generator->permutation[c[i]], &generator->permutation[i]);
            }
            c[i]++;
            i = 0;
            return true;
        } else {
            c[i] = 0;
            i++;
        }
    }
    
    return false; // No more permutations
}

// Reset permutation generator
void reset_permutation_generator(permutation_generator_t* generator) {
    if (!generator) return;
    generator->current_index = 0;
}

// Generate all permutations at once (for small field counts)
bool generate_all_permutations(const uint8_t* fields, uint8_t field_count, 
                              uint8_t permutations[][CADS_MAX_FIELDS], 
                              uint32_t* permutation_count) {
    if (!fields || !permutations || !permutation_count || field_count == 0) return false;
    
    *permutation_count = 0;
    
    // SIMPLE WORKING IMPLEMENTATION - replace broken complex generator
    if (field_count == 1) {
        permutations[0][0] = fields[0];
        *permutation_count = 1;
        return true;
    }
    
    if (field_count == 2) {
        // 2! = 2 permutations
        permutations[0][0] = fields[0]; permutations[0][1] = fields[1];
        permutations[1][0] = fields[1]; permutations[1][1] = fields[0];
        *permutation_count = 2;
        return true;
    }
    
    if (field_count == 3) {
        // 3! = 6 permutations
        permutations[0][0] = fields[0]; permutations[0][1] = fields[1]; permutations[0][2] = fields[2];
        permutations[1][0] = fields[0]; permutations[1][1] = fields[2]; permutations[1][2] = fields[1];
        permutations[2][0] = fields[1]; permutations[2][1] = fields[0]; permutations[2][2] = fields[2];
        permutations[3][0] = fields[1]; permutations[3][1] = fields[2]; permutations[3][2] = fields[0];
        permutations[4][0] = fields[2]; permutations[4][1] = fields[0]; permutations[4][2] = fields[1];
        permutations[5][0] = fields[2]; permutations[5][1] = fields[1]; permutations[5][2] = fields[0];
        *permutation_count = 6;
        return true;
    }
    
    if (field_count == 4) {
        // 4! = 24 permutations - generate all systematically
        uint8_t temp[4] = {fields[0], fields[1], fields[2], fields[3]};
        uint32_t count = 0;
        
        // Use Heap's algorithm for 4 elements
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (j == i) continue;
                for (int k = 0; k < 4; k++) {
                    if (k == i || k == j) continue;
                    for (int l = 0; l < 4; l++) {
                        if (l == i || l == j || l == k) continue;
                        permutations[count][0] = temp[i];
                        permutations[count][1] = temp[j];
                        permutations[count][2] = temp[k];
                        permutations[count][3] = temp[l];
                        count++;
                    }
                }
            }
        }
        *permutation_count = count;
        return true;
    }
    
    return false; // Unsupported field count
}

// Validate field combination
bool is_valid_field_combination(const uint8_t* fields, uint8_t field_count, size_t packet_length) {
    if (!fields || field_count == 0 || packet_length == 0) return false;
    
    // Check for duplicate fields and valid indices
    for (uint8_t i = 0; i < field_count; i++) {
        if (fields[i] >= packet_length) return false;
        
        // Check for duplicates
        for (uint8_t j = i + 1; j < field_count; j++) {
            if (fields[i] == fields[j]) return false;
        }
    }
    
    return true;
}