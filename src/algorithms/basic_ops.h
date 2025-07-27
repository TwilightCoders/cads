#ifndef BASIC_OPS_H
#define BASIC_OPS_H

#include "../../include/cads_types.h"

// Basic algorithm implementations (6 operations)
// These operate on individual values, but can be chained for multi-byte checksums
uint64_t basic_add(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_sub(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_xor(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_and(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_or(uint64_t a, uint64_t b, uint64_t constant);
uint64_t basic_identity(uint64_t a, uint64_t b, uint64_t constant);

// Registry function for basic algorithms
const algorithm_info_t* get_basic_algorithms(int* count);

#endif // BASIC_OPS_H