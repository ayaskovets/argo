#pragma once

#define TEST_T(test_suite_name, test_name, queue, ...) \
  TEST(test_suite_name, test_name) { templates::test_name<queue>(__VA_ARGS__); }

#define TEST_P_T(test_suite_name, test_name, queue, ...)                \
  TEST_P(test_suite_name, test_name) {                                  \
    templates::test_name<queue>(GetParam() __VA_OPT__(, ) __VA_ARGS__); \
  }
