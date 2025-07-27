/* Unity Test Framework - Minimal Version for CADS
 * Based on Unity Test Framework (MIT License)
 * Simplified for our specific needs */

#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Test result tracking
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    const char* current_test;
} unity_state_t;

extern unity_state_t Unity;

// Macros for test setup
#define TEST_SETUP() unity_setup()
#define TEST_TEARDOWN() unity_teardown()
#define RUN_TEST(test) unity_run_test(test, #test)
#define TEST_SUMMARY() unity_summary()

// Assertion macros
#define TEST_ASSERT(condition) \
    unity_assert((condition), __LINE__, #condition)

#define TEST_ASSERT_EQUAL(expected, actual) \
    unity_assert_equal((expected), (actual), __LINE__, #expected, #actual)

#define TEST_ASSERT_EQUAL_HEX8(expected, actual) \
    unity_assert_equal_hex8((expected), (actual), __LINE__, #expected, #actual)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    unity_assert_equal_string((expected), (actual), __LINE__, #expected, #actual)

#define TEST_ASSERT_NOT_NULL(ptr) \
    unity_assert_not_null((ptr), __LINE__, #ptr)

#define TEST_ASSERT_NULL(ptr) \
    unity_assert_null((ptr), __LINE__, #ptr)

#define TEST_FAIL(message) \
    unity_fail(__LINE__, message)

// Function declarations
void unity_setup(void);
void unity_teardown(void);
void unity_run_test(void (*test_func)(void), const char* test_name);
int unity_summary(void);

void unity_assert(bool condition, int line, const char* expression);
void unity_assert_equal(int expected, int actual, int line, const char* exp_str, const char* act_str);
void unity_assert_equal_hex8(uint8_t expected, uint8_t actual, int line, const char* exp_str, const char* act_str);
void unity_assert_equal_string(const char* expected, const char* actual, int line, const char* exp_str, const char* act_str);
void unity_assert_not_null(const void* ptr, int line, const char* expression);
void unity_assert_null(const void* ptr, int line, const char* expression);
void unity_fail(int line, const char* message);

// Color output support
#define UNITY_COLOR_RED     "\033[31m"
#define UNITY_COLOR_GREEN   "\033[32m"
#define UNITY_COLOR_YELLOW  "\033[33m"
#define UNITY_COLOR_BLUE    "\033[34m"
#define UNITY_COLOR_RESET   "\033[0m"

#endif // UNITY_H