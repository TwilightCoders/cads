#include "basic_ops.h"

// Basic arithmetic and logical operations
uint8_t basic_add(uint8_t a, uint8_t b, uint8_t constant) {
    return (a + b) & 0xFF;
}

uint8_t basic_sub(uint8_t a, uint8_t b, uint8_t constant) {
    return (a - b) & 0xFF;
}

uint8_t basic_xor(uint8_t a, uint8_t b, uint8_t constant) {
    return a ^ b;
}

uint8_t basic_and(uint8_t a, uint8_t b, uint8_t constant) {
    return a & b;
}

uint8_t basic_or(uint8_t a, uint8_t b, uint8_t constant) {
    return a | b;
}

uint8_t basic_identity(uint8_t a, uint8_t b, uint8_t constant) {
    return a;
}

// Algorithm registry for basic operations
static const algorithm_info_t basic_algorithm_registry[] = {
    {OP_ADD, COMPLEXITY_BASIC, "ADD", "Simple addition", false},
    {OP_SUB, COMPLEXITY_BASIC, "SUB", "Subtraction", false},
    {OP_XOR, COMPLEXITY_BASIC, "XOR", "Exclusive OR", false},
    {OP_AND, COMPLEXITY_BASIC, "AND", "Bitwise AND", false},
    {OP_OR, COMPLEXITY_BASIC, "OR", "Bitwise OR", false},
    {OP_IDENTITY, COMPLEXITY_BASIC, "ID", "Pass-through", false}
};

const algorithm_info_t* get_basic_algorithms(int* count) {
    *count = sizeof(basic_algorithm_registry) / sizeof(basic_algorithm_registry[0]);
    return basic_algorithm_registry;
}