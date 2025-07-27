#ifndef INTERMEDIATE_OPS_H
#define INTERMEDIATE_OPS_H

#include "../../include/cads_types.h"

// Intermediate algorithm implementations (12 operations)
uint8_t intermediate_not(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_lshift(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_rshift(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_mul(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_div(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_mod(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_negate(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_const_add(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_const_xor(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_const_sub(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_ones_complement(uint8_t a, uint8_t b, uint8_t constant);
uint8_t intermediate_twos_complement(uint8_t a, uint8_t b, uint8_t constant);

// Registry function for intermediate algorithms
const algorithm_info_t* get_intermediate_algorithms(int* count);

#endif // INTERMEDIATE_OPS_H