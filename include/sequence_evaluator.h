// Unified operation sequence evaluation for CADS
#ifndef SEQUENCE_EVALUATOR_H
#define SEQUENCE_EVALUATOR_H

#include "cads_types.h"
#include "cads_config_loader.h"
#include "algorithm_registry.h"
#include "../src/core/packet_data.h"

// Evaluate an operation sequence over all packets.
// Returns true if sequence matches all packets' expected checksums.
bool evaluate_operation_sequence(const packet_dataset_t* dataset,
                                 const config_t* config,
                                 const uint8_t* field_permutation,
                                 int field_count,
                                 const operation_t* operation_sequence,
                                 int operation_count,
                                 uint8_t constant);

#endif // SEQUENCE_EVALUATOR_H
