/* Unit tests for field combination generation */

#include "../unity.h"
#include "../../src/utils/field_combiner.h"

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No teardown needed
}

// Test field generator creation
void test_field_generator_creation(void) {
    field_combination_generator_t* generator = create_field_generator(8, 2);
    TEST_ASSERT_NOT_NULL(generator);
    
    free_field_generator(generator);
}

// Test field combination generation
void test_field_combinations(void) {
    field_combination_generator_t* generator = create_field_generator(4, 2);
    TEST_ASSERT_NOT_NULL(generator);
    
    int count = 0;
    bool found_0_1 = false;
    bool found_2_3 = false;
    
    while (next_field_combination(generator)) {
        count++;
        
        // Check specific combinations
        if (generator->field_count == 2) {
            if (generator->fields[0] == 0 && generator->fields[1] == 1) found_0_1 = true;
            if (generator->fields[0] == 2 && generator->fields[1] == 3) found_2_3 = true;
        }
        
        // Validate field indices are within range
        for (size_t i = 0; i < generator->field_count; i++) {
            TEST_ASSERT(generator->fields[i] < 4);  // Should be 0-3 for 4-byte packet
        }
    }
    
    TEST_ASSERT(count > 0);  // Should generate some combinations
    TEST_ASSERT(found_0_1);  // Should include [0,1] combination
    TEST_ASSERT(found_2_3);  // Should include [2,3] combination
    
    free_field_generator(generator);
}

// Test permutation generation
void test_permutation_generation(void) {
    uint8_t fields[] = {1, 3};
    uint8_t permutations[24][CADS_MAX_FIELDS];
    uint32_t perm_count = 0;
    
    generate_all_permutations(fields, 2, permutations, &perm_count);
    
    TEST_ASSERT_EQUAL(2, perm_count);  // 2! = 2 permutations
    
    // Check that we get both [1,3] and [3,1]
    bool found_1_3 = false;
    bool found_3_1 = false;
    
    for (uint32_t i = 0; i < perm_count; i++) {
        if (permutations[i][0] == 1 && permutations[i][1] == 3) found_1_3 = true;
        if (permutations[i][0] == 3 && permutations[i][1] == 1) found_3_1 = true;
    }
    
    TEST_ASSERT(found_1_3);
    TEST_ASSERT(found_3_1);
}

// Test edge cases
void test_edge_cases(void) {
    // Test with max fields = packet length
    field_combination_generator_t* generator = create_field_generator(3, 3);
    TEST_ASSERT_NOT_NULL(generator);
    
    bool found_full_combo = false;
    while (next_field_combination(generator)) {
        if (generator->field_count == 3) {
            // Should be able to select all fields [0,1,2]
            if (generator->fields[0] == 0 && generator->fields[1] == 1 && generator->fields[2] == 2) {
                found_full_combo = true;
            }
        }
    }
    
    TEST_ASSERT(found_full_combo);
    free_field_generator(generator);
    
    // Test with 1 field max
    generator = create_field_generator(4, 1);
    TEST_ASSERT_NOT_NULL(generator);
    
    int single_field_count = 0;
    while (next_field_combination(generator)) {
        TEST_ASSERT_EQUAL(1, generator->field_count);  // Should only generate single fields
        single_field_count++;
    }
    
    TEST_ASSERT_EQUAL(4, single_field_count);  // Should generate 4 single-field combinations
    free_field_generator(generator);
}

int main(void) {
    TEST_SETUP();
    
    RUN_TEST(test_field_generator_creation);
    RUN_TEST(test_field_combinations);
    RUN_TEST(test_permutation_generation);
    RUN_TEST(test_edge_cases);
    
    return TEST_SUMMARY();
}