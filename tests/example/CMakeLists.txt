project(example)

include(add_catch2_test_suite)

add_catch2_test_suite(
    TEST_NAME
        example_test

    TEST_SOURCES
        example_test.cpp

    TEST_INCLUDE_PATHS
        include
)
