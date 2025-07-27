#include <stdio.h>
#include "src/core/json_loader.h"

int main() {
    printf("Testing JSON loader compilation\n");
    packet_dataset_t* dataset = load_packets_from_json("test.jsonl");
    if (dataset) {
        printf("Loaded %zu packets\n", dataset->count);
        free_packet_dataset(dataset);
    }
    return 0;
}