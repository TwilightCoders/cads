#ifndef ADVANCED_OPS_H
#define ADVANCED_OPS_H

#include "../../include/cads_types.h"

// Advanced algorithm implementations (11 operations)
uint64_t advanced_rotleft(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_rotright(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_crc8_ccitt(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_crc8_dallas(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_crc8_sae(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_fletcher8(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_swap_nibbles(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_reverse_bits(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_lookup_table(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_poly_crc(uint64_t a, uint64_t b, uint64_t constant);
uint64_t advanced_checksum_variant(uint64_t a, uint64_t b, uint64_t constant);

// Utility functions
uint64_t rotate_left(uint64_t value, uint8_t positions, uint8_t bit_width);
uint64_t rotate_right(uint64_t value, uint8_t positions, uint8_t bit_width);
uint64_t reverse_bits(uint64_t value, uint8_t bit_width);

// Registry function for advanced algorithms
const algorithm_info_t* get_advanced_algorithms(int* count);

// CRC lookup table access
extern const uint8_t crc8_ccitt_table[256];
extern const uint8_t sample_lookup_table[256];

#endif // ADVANCED_OPS_H