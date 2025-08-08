#include "../unity.h"
#include "../../include/checksum_engine.h"
#include "../../src/utils/config.h"
#include "../../src/core/packet_data.h"

static void collect_solutions(config_t base, int threads, search_results_t** out_results) {
    base.threads = threads;
    search_results_t* results = create_search_results(32);
    TEST_ASSERT_NOT_NULL(results);
    bool ok = execute_weighted_checksum_search(&base, results, NULL);
    TEST_ASSERT(ok);
    sort_search_solutions(results);
    *out_results = results;
}

void test_thread_equivalence_small_domain(void) {
    packet_dataset_t* dataset = create_packet_dataset(8);
    TEST_ASSERT_NOT_NULL(dataset);
    bool load_ok = load_packets_from_json(dataset, "data/gmrs_test_dataset.jsonl");
    TEST_ASSERT(load_ok);

    operation_t ops[] = {OP_ADD, OP_XOR, OP_CONST_ADD, OP_IDENTITY};
    config_t cfg = create_custom_operation_config(ops, 4);
    cfg.dataset = dataset;
    cfg.max_fields = 3; // keep domain small for speed
    cfg.max_constants = 8;
    disable_early_exit(&cfg);

    search_results_t *single=NULL, *multi=NULL;
    collect_solutions(cfg, 1, &single);
    collect_solutions(cfg, 2, &multi);

    TEST_ASSERT_EQUAL(single->solution_count, multi->solution_count);
    size_t n = single->solution_count;
    for (size_t i=0;i<n;i++) {
        checksum_solution_t *A=&single->solutions[i];
        checksum_solution_t *B=&multi->solutions[i];
        TEST_ASSERT_EQUAL(A->field_count, B->field_count);
        for (int f=0; f<A->field_count; f++) TEST_ASSERT_EQUAL(A->field_indices[f], B->field_indices[f]);
        TEST_ASSERT_EQUAL(A->operation_count, B->operation_count);
        for (int o=0;o<A->operation_count;o++) TEST_ASSERT_EQUAL(A->operations[o], B->operations[o]);
        TEST_ASSERT_EQUAL(A->constant, B->constant);
    }

    free_search_results(single);
    free_search_results(multi);
    free_packet_dataset(dataset);
}

int main(void) {
    TEST_SETUP();
    RUN_TEST(test_thread_equivalence_small_domain);
    return TEST_SUMMARY();
}
