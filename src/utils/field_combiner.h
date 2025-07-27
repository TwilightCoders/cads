#ifndef FIELD_COMBINER_H
#define FIELD_COMBINER_H

#include "../../include/cads_types.h"

// Field combination generator
typedef struct {
    uint8_t* fields;              // Array of field indices
    size_t field_count;           // Number of fields in combination
    size_t packet_length;         // Total packet length
    uint8_t current_mask;         // Current combination mask
    uint8_t max_fields;           // Maximum fields to combine
} field_combination_generator_t;

// Permutation generator
typedef struct {
    uint8_t permutation[CADS_MAX_FIELDS];  // Current permutation
    uint8_t field_count;                   // Number of fields to permute
    uint32_t current_index;                // Current permutation index
    uint32_t total_permutations;           // Total number of permutations
} permutation_generator_t;

// Field combination functions
field_combination_generator_t* create_field_generator(size_t packet_length, uint8_t max_fields);
void free_field_generator(field_combination_generator_t* generator);
bool next_field_combination(field_combination_generator_t* generator);
void reset_field_generator(field_combination_generator_t* generator);

// Permutation functions
permutation_generator_t* create_permutation_generator(const uint8_t* fields, uint8_t field_count);
void free_permutation_generator(permutation_generator_t* generator);
bool next_permutation(permutation_generator_t* generator);
void reset_permutation_generator(permutation_generator_t* generator);

// Utility functions
uint32_t calculate_total_permutations(uint8_t field_count);
bool generate_all_permutations(const uint8_t* fields, uint8_t field_count, 
                              uint8_t permutations[][CADS_MAX_FIELDS], 
                              uint32_t* permutation_count);

// Field validation
bool is_valid_field_combination(const uint8_t* fields, uint8_t field_count, size_t packet_length);

#endif // FIELD_COMBINER_H