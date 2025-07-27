#ifndef PACKET_DATA_H
#define PACKET_DATA_H

#include "../../include/cads_types.h"

// Packet data management functions
typedef struct {
    test_packet_t* packets;
    size_t count;
    size_t capacity;
} packet_dataset_t;

// Create and manage packet datasets
packet_dataset_t* create_packet_dataset(size_t initial_capacity);
void free_packet_dataset(packet_dataset_t* dataset);

// Add packets to dataset
bool add_packet_from_hex(packet_dataset_t* dataset, const char* hex_data, 
                        const char* hex_checksum, size_t checksum_size, 
                        const char* description);
bool add_packet_from_bytes(packet_dataset_t* dataset, const uint8_t* data, 
                          size_t data_length, uint64_t checksum, 
                          size_t checksum_size, const char* description);

// Load packets from various formats
bool load_packets_from_file(packet_dataset_t* dataset, const char* filename);
bool load_packets_from_csv(packet_dataset_t* dataset, const char* filename);
bool load_packets_from_json(packet_dataset_t* dataset, const char* filename);

// Create default test dataset (backwards compatibility)
packet_dataset_t* create_default_gmrs_dataset(void);

// Utility functions
uint64_t extract_checksum_from_packet(const uint8_t* full_packet, size_t packet_length, 
                                     size_t checksum_size, bool little_endian);
bool validate_packet_format(const test_packet_t* packet);

#endif // PACKET_DATA_H