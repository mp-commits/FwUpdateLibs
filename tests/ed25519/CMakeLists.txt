project(ed25519_tests)

include(add_catch2_test_suite)

add_catch2_test_suite(
    TEST_NAME
        ed25519_tests

    TEST_SOURCES
        ed25519_tests.cpp

    TEST_LINK_LIBRARIES
        libs::ed25519
)
