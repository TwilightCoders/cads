#include "packet_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Create a new packet dataset
packet_dataset_t* create_packet_dataset(size_t initial_capacity) {
    packet_dataset_t* dataset = malloc(sizeof(packet_dataset_t));
    if (!dataset) return NULL;
    
    dataset->packets = malloc(initial_capacity * sizeof(test_packet_t));
    if (!dataset->packets) {
        free(dataset);
        return NULL;
    }
    
    dataset->count = 0;
    dataset->capacity = initial_capacity;
    return dataset;
}

// Free packet dataset and all contained packets
void free_packet_dataset(packet_dataset_t* dataset) {
    if (!dataset) return;
    
    // Free individual packet data
    for (size_t i = 0; i < dataset->count; i++) {
        free(dataset->packets[i].packet_data);
    }
    
    free(dataset->packets);
    free(dataset);
}

// Expand dataset capacity if needed
static bool ensure_capacity(packet_dataset_t* dataset, size_t required_capacity) {
    if (dataset->capacity >= required_capacity) return true;
    
    size_t new_capacity = dataset->capacity * 2;
    if (new_capacity < required_capacity) new_capacity = required_capacity;
    
    test_packet_t* new_packets = realloc(dataset->packets, new_capacity * sizeof(test_packet_t));
    if (!new_packets) return false;
    
    dataset->packets = new_packets;
    dataset->capacity = new_capacity;
    return true;
}

// Convert hex string to bytes
static bool hex_string_to_bytes(const char* hex_str, uint8_t** bytes, size_t* length) {
    if (!hex_str || !bytes || !length) return false;
    
    // Remove spaces and validate hex characters
    size_t hex_len = strlen(hex_str);
    size_t byte_count = 0;
    
    // Count valid hex characters
    for (size_t i = 0; i < hex_len; i++) {
        if (isxdigit(hex_str[i])) {
            byte_count++;
        } else if (!isspace(hex_str[i])) {
            return false; // Invalid character
        }
    }
    
    if (byte_count % 2 != 0) return false; // Must be even number of hex digits
    
    *length = byte_count / 2;
    *bytes = malloc(*length);
    if (!*bytes) return false;
    
    // Convert hex pairs to bytes
    size_t byte_index = 0;
    uint8_t nibble_count = 0;
    uint8_t current_byte = 0;
    
    for (size_t i = 0; i < hex_len; i++) {
        if (isspace(hex_str[i])) continue;
        
        uint8_t nibble;
        if (hex_str[i] >= '0' && hex_str[i] <= '9') {
            nibble = hex_str[i] - '0';
        } else if (hex_str[i] >= 'A' && hex_str[i] <= 'F') {
            nibble = hex_str[i] - 'A' + 10;
        } else if (hex_str[i] >= 'a' && hex_str[i] <= 'f') {
            nibble = hex_str[i] - 'a' + 10;
        } else {
            free(*bytes);
            return false;
        }
        
        if (nibble_count == 0) {
            current_byte = nibble << 4;
            nibble_count = 1;
        } else {
            current_byte |= nibble;
            (*bytes)[byte_index++] = current_byte;
            nibble_count = 0;
        }
    }
    
    return true;
}

// Convert hex string to uint64_t checksum
static uint64_t hex_string_to_checksum(const char* hex_str, size_t checksum_size) {
    if (!hex_str || checksum_size == 0 || checksum_size > 8) return 0;
    
    uint64_t checksum = 0;
    size_t hex_len = strlen(hex_str);
    
    // Parse hex string
    for (size_t i = 0; i < hex_len && i < checksum_size * 2; i++) {
        if (isspace(hex_str[i])) continue;
        
        uint8_t nibble;
        if (hex_str[i] >= '0' && hex_str[i] <= '9') {
            nibble = hex_str[i] - '0';
        } else if (hex_str[i] >= 'A' && hex_str[i] <= 'F') {
            nibble = hex_str[i] - 'A' + 10;
        } else if (hex_str[i] >= 'a' && hex_str[i] <= 'f') {
            nibble = hex_str[i] - 'a' + 10;
        } else {
            continue; // Skip invalid characters
        }
        
        checksum = (checksum << 4) | nibble;
    }
    
    return checksum;
}

// Add packet from hex strings
bool add_packet_from_hex(packet_dataset_t* dataset, const char* hex_data, 
                        const char* hex_checksum, size_t checksum_size, 
                        const char* description) {
    if (!dataset || !hex_data || !hex_checksum || !description) return false;
    
    if (!ensure_capacity(dataset, dataset->count + 1)) return false;
    
    uint8_t* packet_data;
    size_t packet_length;
    
    // Convert hex data to bytes
    if (!hex_string_to_bytes(hex_data, &packet_data, &packet_length)) {
        return false;
    }
    
    // Convert hex checksum
    uint64_t checksum = hex_string_to_checksum(hex_checksum, checksum_size);
    
    // Create new packet entry
    test_packet_t* packet = &dataset->packets[dataset->count];
    packet->packet_data = packet_data;
    packet->packet_length = packet_length;
    packet->expected_checksum = checksum;
    packet->checksum_size = checksum_size;
    packet->description = strdup(description);
    
    dataset->count++;
    return true;
}

// Add packet from byte arrays
bool add_packet_from_bytes(packet_dataset_t* dataset, const uint8_t* data, 
                          size_t data_length, uint64_t checksum, 
                          size_t checksum_size, const char* description) {
    if (!dataset || !data || !description) return false;
    
    if (!ensure_capacity(dataset, dataset->count + 1)) return false;
    
    // Copy packet data
    uint8_t* packet_data = malloc(data_length);
    if (!packet_data) return false;
    memcpy(packet_data, data, data_length);
    
    // Create new packet entry
    test_packet_t* packet = &dataset->packets[dataset->count];
    packet->packet_data = packet_data;
    packet->packet_length = data_length;
    packet->expected_checksum = checksum;
    packet->checksum_size = checksum_size;
    packet->description = strdup(description);
    
    dataset->count++;
    return true;
}

// Load packets from CSV format
bool load_packets_from_csv(packet_dataset_t* dataset, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return false;
    
    char line[2048];
    bool header_skipped = false;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip header line
        if (!header_skipped) {
            header_skipped = true;
            continue;
        }
        
        // Parse CSV line: description,packet_data,expected_checksum
        char* description = strtok(line, ",");
        char* packet_data = strtok(NULL, ",");
        char* checksum_str = strtok(NULL, ",\n\r");
        
        if (description && packet_data && checksum_str) {
            // Remove quotes if present
            if (description[0] == '"') description++;
            if (description[strlen(description)-1] == '"') description[strlen(description)-1] = '\0';
            if (packet_data[0] == '"') packet_data++;
            if (packet_data[strlen(packet_data)-1] == '"') packet_data[strlen(packet_data)-1] = '\0';
            if (checksum_str[0] == '"') checksum_str++;
            if (checksum_str[strlen(checksum_str)-1] == '"') checksum_str[strlen(checksum_str)-1] = '\0';
            
            // Add packet with default 1-byte checksum
            add_packet_from_hex(dataset, packet_data, checksum_str, 1, description);
        }
    }
    
    fclose(file);
    return true;
}

// Create default GMRS dataset (backwards compatibility)
packet_dataset_t* create_default_gmrs_dataset(void) {
    packet_dataset_t* dataset = create_packet_dataset(16);
    if (!dataset) return NULL;
    
    // Original GMRS test packets from legacy code
    struct {
        const char* hex_data;
        const char* hex_checksum;
        const char* description;
    } gmrs_packets[] = {
        // Low channels (1-7) - simple pattern
        {"9c30010000000000", "31", "CH1"},
        {"9c30030000000000", "33", "CH3"},
        {"9c30040000000000", "34", "CH4"},
        {"9c30050000000000", "35", "CH5"},
        {"9c30060000000000", "36", "CH6"},
        {"9c30070000000000", "37", "CH7"},
        
        // High channels (15-22) - complex pattern
        {"9c300f00000100", "3e", "CH15"},
        {"9c301000000100", "21", "CH16"},
        {"9c301100000100", "20", "CH17"},
        {"9c301200000100", "23", "CH18"},
        {"9c301300000100", "22", "CH19"},
        {"9c301400000100", "25", "CH20"},
        {"9c301500000100", "24", "CH21"},
        {"9c301600000100", "27", "CH22"},
        
        // CTCSS channels - special cases
        {"9c30020109000000", "3a", "CH2+CTCSS09"},
        {"9c300f010a0100", "35", "CH15+CTCSS10"},
    };
    
    for (size_t i = 0; i < sizeof(gmrs_packets) / sizeof(gmrs_packets[0]); i++) {
        add_packet_from_hex(dataset, gmrs_packets[i].hex_data, 
                           gmrs_packets[i].hex_checksum, 1, 
                           gmrs_packets[i].description);
    }
    
    return dataset;
}

// Extract checksum from full packet
uint64_t extract_checksum_from_packet(const uint8_t* full_packet, size_t packet_length, 
                                     size_t checksum_size, bool little_endian) {
    if (!full_packet || packet_length < checksum_size) return 0;
    
    const uint8_t* checksum_bytes = full_packet + (packet_length - checksum_size);
    uint64_t checksum = 0;
    
    if (little_endian) {
        // Little-endian: least significant byte first
        for (int i = checksum_size - 1; i >= 0; i--) {
            checksum = (checksum << 8) | checksum_bytes[i];
        }
    } else {
        // Big-endian: most significant byte first
        for (size_t i = 0; i < checksum_size; i++) {
            checksum = (checksum << 8) | checksum_bytes[i];
        }
    }
    
    return checksum;
}

// Validate packet format
bool validate_packet_format(const test_packet_t* packet) {
    if (!packet || !packet->packet_data || packet->packet_length == 0) return false;
    if (packet->checksum_size == 0 || packet->checksum_size > 8) return false;
    if (!packet->description) return false;
    return true;
}