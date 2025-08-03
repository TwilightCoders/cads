#include "intermediate_ops.h"

// Intermediate complexity operations
uint64_t intermediate_not(uint64_t a, uint64_t b, uint64_t constant) {
    (void)b;
    (void)constant;
    return ~a;
}

uint64_t intermediate_lshift(uint64_t a, uint64_t b, uint64_t constant) {
    (void)constant;
    return a << (b & 0x3F); // Limit shift to 63 bits for 64-bit values
}

uint64_t intermediate_rshift(uint64_t a, uint64_t b, uint64_t constant) {
    (void)constant;
    return a >> (b & 0x3F);
}

uint64_t intermediate_mul(uint64_t a, uint64_t b, uint64_t constant) {
    (void)constant;
    return a * (b ? b : 1);
}

uint64_t intermediate_div(uint64_t a, uint64_t b, uint64_t constant) {
    (void)constant;
    return b ? (a / b) : 0;
}

uint64_t intermediate_mod(uint64_t a, uint64_t b, uint64_t constant) {
    (void)constant;
    return b ? (a % b) : 0;
}

uint64_t intermediate_negate(uint64_t a, uint64_t b, uint64_t constant) {
    (void)b;
    (void)constant;
    return (~a) + 1; // Two's complement negation
}

uint64_t intermediate_const_add(uint64_t a, uint64_t b, uint64_t constant) {
    (void)b;  // Ignore b parameter like original
    return a + constant;
}

uint64_t intermediate_const_xor(uint64_t a, uint64_t b, uint64_t constant) {
    (void)b;
    return a ^ constant;
}

uint64_t intermediate_const_sub(uint64_t a, uint64_t b, uint64_t constant) {
    (void)b;
    return a - constant;
}

uint64_t intermediate_ones_complement(uint64_t a, uint64_t b, uint64_t constant) {
    (void)constant;
    return ~(a + b);
}

uint64_t intermediate_twos_complement(uint64_t a, uint64_t b, uint64_t constant) {
    (void)constant;
    return (~(a + b)) + 1;
}

// Algorithm registry for intermediate operations
static const algorithm_info_t intermediate_algorithm_registry[] = {
    {OP_NOT, COMPLEXITY_INTERMEDIATE, "NOT", "Bitwise NOT", false},
    {OP_LSHIFT, COMPLEXITY_INTERMEDIATE, "LSH", "Left shift", false},
    {OP_RSHIFT, COMPLEXITY_INTERMEDIATE, "RSH", "Right shift", false},
    {OP_MUL, COMPLEXITY_INTERMEDIATE, "MUL", "Multiplication", false},
    {OP_DIV, COMPLEXITY_INTERMEDIATE, "DIV", "Division", false},
    {OP_MOD, COMPLEXITY_INTERMEDIATE, "MOD", "Modulo", false},
    {OP_NEGATE, COMPLEXITY_INTERMEDIATE, "NEG", "Two's complement negation", false},
    {OP_CONST_ADD, COMPLEXITY_INTERMEDIATE, "C+", "Add constant", true},
    {OP_CONST_XOR, COMPLEXITY_INTERMEDIATE, "C^", "XOR with constant", true},
    {OP_CONST_SUB, COMPLEXITY_INTERMEDIATE, "C-", "Subtract constant", true},
    {OP_ONES_COMPLEMENT, COMPLEXITY_INTERMEDIATE, "1COMP", "One's complement sum", false},
    {OP_TWOS_COMPLEMENT, COMPLEXITY_INTERMEDIATE, "2COMP", "Two's complement sum", false}
};

const algorithm_info_t* get_intermediate_algorithms(int* count) {
    *count = sizeof(intermediate_algorithm_registry) / sizeof(intermediate_algorithm_registry[0]);
    return intermediate_algorithm_registry;
}