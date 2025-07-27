#ifndef ADVANCED_OPS_H
#define ADVANCED_OPS_H

#include "../../include/cads_types.h"

// Advanced algorithm implementations (11 operations)
uint8_t advanced_rotleft(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_rotright(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_crc8_ccitt(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_crc8_dallas(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_crc8_sae(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_fletcher8(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_swap_nibbles(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_reverse_bits(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_lookup_table(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_poly_crc(uint8_t a, uint8_t b, uint8_t constant);
uint8_t advanced_checksum_variant(uint8_t a, uint8_t b, uint8_t constant);

// Utility functions
uint8_t rotate_left(uint8_t value, uint8_t positions);
uint8_t rotate_right(uint8_t value, uint8_t positions);
uint8_t reverse_bits(uint8_t value);

// Registry function for advanced algorithms
const algorithm_info_t* get_advanced_algorithms(int* count);

// CRC lookup table access
extern const uint8_t crc8_ccitt_table[256];
extern const uint8_t sample_lookup_table[256];

#endif // ADVANCED_OPS_H