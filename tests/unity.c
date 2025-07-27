/* Unity Test Framework Implementation - Minimal Version for CADS */

#include "unity.h"

unity_state_t Unity = {0};

void unity_setup(void) {
    Unity.tests_run = 0;
    Unity.tests_passed = 0;
    Unity.tests_failed = 0;
    Unity.current_test = NULL;
    
    printf("🧪 CADS Test Suite\n");
    printf("==================\n\n");
}

void unity_teardown(void) {
    // Nothing to clean up in minimal version
}

void unity_run_test(void (*test_func)(void), const char* test_name) {
    Unity.current_test = test_name;
    Unity.tests_run++;
    
    printf("Running %s... ", test_name);
    fflush(stdout);
    
    int failed_before = Unity.tests_failed;
    test_func();
    
    if (Unity.tests_failed == failed_before) {
        Unity.tests_passed++;
        printf(UNITY_COLOR_GREEN "✅ PASS" UNITY_COLOR_RESET "\n");
    } else {
        printf(UNITY_COLOR_RED "❌ FAIL" UNITY_COLOR_RESET "\n");
    }
}

int unity_summary(void) {
    printf("\n==================\n");
    printf("🏁 Test Results\n");
    printf("==================\n");
    printf("Tests run: %d\n", Unity.tests_run);
    printf("Passed: " UNITY_COLOR_GREEN "%d" UNITY_COLOR_RESET "\n", Unity.tests_passed);
    printf("Failed: " UNITY_COLOR_RED "%d" UNITY_COLOR_RESET "\n", Unity.tests_failed);
    
    if (Unity.tests_failed == 0) {
        printf("\n" UNITY_COLOR_GREEN "🎉 ALL TESTS PASSED!" UNITY_COLOR_RESET "\n");
        return 0;
    } else {
        printf("\n" UNITY_COLOR_RED "⚠️  SOME TESTS FAILED" UNITY_COLOR_RESET "\n");
        return 1;
    }
}

void unity_assert(bool condition, int line, const char* expression) {
    if (!condition) {
        Unity.tests_failed++;
        printf("\n" UNITY_COLOR_RED "ASSERTION FAILED" UNITY_COLOR_RESET " at line %d: %s\n", line, expression);
    }
}

void unity_assert_equal(int expected, int actual, int line, const char* exp_str, const char* act_str) {
    if (expected != actual) {
        Unity.tests_failed++;
        printf("\n" UNITY_COLOR_RED "ASSERTION FAILED" UNITY_COLOR_RESET " at line %d:\n", line);
        printf("  Expected: %s = %d\n", exp_str, expected);
        printf("  Actual:   %s = %d\n", act_str, actual);
    }
}

void unity_assert_equal_hex8(uint8_t expected, uint8_t actual, int line, const char* exp_str, const char* act_str) {
    if (expected != actual) {
        Unity.tests_failed++;
        printf("\n" UNITY_COLOR_RED "ASSERTION FAILED" UNITY_COLOR_RESET " at line %d:\n", line);
        printf("  Expected: %s = 0x%02X\n", exp_str, expected);
        printf("  Actual:   %s = 0x%02X\n", act_str, actual);
    }
}

void unity_assert_equal_string(const char* expected, const char* actual, int line, const char* exp_str, const char* act_str) {
    if (strcmp(expected, actual) != 0) {
        Unity.tests_failed++;
        printf("\n" UNITY_COLOR_RED "ASSERTION FAILED" UNITY_COLOR_RESET " at line %d:\n", line);
        printf("  Expected: %s = \"%s\"\n", exp_str, expected);
        printf("  Actual:   %s = \"%s\"\n", act_str, actual);
    }
}

void unity_assert_not_null(const void* ptr, int line, const char* expression) {
    if (ptr == NULL) {
        Unity.tests_failed++;
        printf("\n" UNITY_COLOR_RED "ASSERTION FAILED" UNITY_COLOR_RESET " at line %d: %s should not be NULL\n", line, expression);
    }
}

void unity_assert_null(const void* ptr, int line, const char* expression) {
    if (ptr != NULL) {
        Unity.tests_failed++;
        printf("\n" UNITY_COLOR_RED "ASSERTION FAILED" UNITY_COLOR_RESET " at line %d: %s should be NULL\n", line, expression);
    }
}

void unity_fail(int line, const char* message) {
    Unity.tests_failed++;
    printf("\n" UNITY_COLOR_RED "TEST FAILED" UNITY_COLOR_RESET " at line %d: %s\n", line, message);
}