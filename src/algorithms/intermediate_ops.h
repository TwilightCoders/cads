#ifndef INTERMEDIATE_OPS_H
#define INTERMEDIATE_OPS_H

#include "../../include/cads_types.h"

// Intermediate algorithm implementations (12 operations)
uint64_t intermediate_not(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_lshift(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_rshift(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_mul(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_div(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_mod(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_negate(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_const_add(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_const_xor(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_const_sub(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_ones_complement(uint64_t a, uint64_t b, uint64_t constant);
uint64_t intermediate_twos_complement(uint64_t a, uint64_t b, uint64_t constant);

// Registry function for intermediate algorithms
const algorithm_info_t* get_intermediate_algorithms(int* count);

#endif // INTERMEDIATE_OPS_H