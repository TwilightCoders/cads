#include "intermediate_ops.h"

// Intermediate complexity operations
uint8_t intermediate_not(uint8_t a, uint8_t b, uint8_t constant) {
    return ~a & 0xFF;
}

uint8_t intermediate_lshift(uint8_t a, uint8_t b, uint8_t constant) {
    return (a << (b & 0x7)) & 0xFF;
}

uint8_t intermediate_rshift(uint8_t a, uint8_t b, uint8_t constant) {
    return (a >> (b & 0x7)) & 0xFF;
}

uint8_t intermediate_mul(uint8_t a, uint8_t b, uint8_t constant) {
    return (a * (b ? b : 1)) & 0xFF;
}

uint8_t intermediate_div(uint8_t a, uint8_t b, uint8_t constant) {
    return b ? ((a / b) & 0xFF) : 0;
}

uint8_t intermediate_mod(uint8_t a, uint8_t b, uint8_t constant) {
    return b ? ((a % b) & 0xFF) : 0;
}

uint8_t intermediate_negate(uint8_t a, uint8_t b, uint8_t constant) {
    return (-a) & 0xFF;
}

uint8_t intermediate_const_add(uint8_t a, uint8_t b, uint8_t constant) {
    return (a + constant) & 0xFF;
}

uint8_t intermediate_const_xor(uint8_t a, uint8_t b, uint8_t constant) {
    return (a ^ constant) & 0xFF;
}

uint8_t intermediate_const_sub(uint8_t a, uint8_t b, uint8_t constant) {
    return (a - constant) & 0xFF;
}

uint8_t intermediate_ones_complement(uint8_t a, uint8_t b, uint8_t constant) {
    return (~(a + b)) & 0xFF;
}

uint8_t intermediate_twos_complement(uint8_t a, uint8_t b, uint8_t constant) {
    return ((~(a + b)) + 1) & 0xFF;
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