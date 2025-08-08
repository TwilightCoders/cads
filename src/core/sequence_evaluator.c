#include "checksum_engine.h"
#include "../../include/sequence_evaluator.h"
#include "../../include/algorithm_registry.h"

// Core unified evaluation logic extracted from previous duplicated implementations.
bool evaluate_operation_sequence(const packet_dataset_t* dataset,
                                 const config_t* config,
                                 const uint8_t* field_permutation,
                                 int field_count,
                                 const operation_t* operation_sequence,
                                 int operation_count,
                                 uint8_t constant) {
    if (!dataset || !config || !field_permutation || !operation_sequence) return false;
    for (size_t packet_idx = 0; packet_idx < dataset->count; packet_idx++) {
        const test_packet_t* packet = &dataset->packets[packet_idx];
        if (packet->checksum_size != config->checksum_size) return false; // mismatch invalidates sequence
        // Field bounds check
        for (int f = 0; f < field_count; f++) {
            if (field_permutation[f] >= packet->packet_length) return false;
        }
        uint64_t calculated = extract_packet_field_value(packet->packet_data,
                                                         packet->packet_length,
                                                         field_permutation[0],
                                                         config->checksum_size);
        int field_idx = 1;
        for (int op_idx = 0; op_idx < operation_count; op_idx++) {
            operation_t op = operation_sequence[op_idx];
            if (op == OP_ONES_COMPLEMENT) {
                calculated = execute_algorithm(op, calculated, 0, 0);
            } else if (op == OP_CONST_ADD || op == OP_CONST_SUB || op == OP_CONST_XOR || op == OP_POLY_CRC || op == OP_CHECKSUM_VARIANT) {
                calculated = execute_algorithm(op, calculated, 0, constant);
            } else if (field_idx < field_count) {
                uint64_t next_val = extract_packet_field_value(packet->packet_data,
                                                               packet->packet_length,
                                                               field_permutation[field_idx],
                                                               config->checksum_size);
                calculated = execute_algorithm(op, calculated, next_val, 0);
                field_idx++;
            } else {
                break; // No more fields available
            }
        }
        calculated = mask_checksum_to_size(calculated, config->checksum_size);
        uint64_t expected = mask_checksum_to_size(packet->expected_checksum, config->checksum_size);
        if (calculated != expected) return false;
    }
    return true;
}
