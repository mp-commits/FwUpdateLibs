project(fragment_store_tests)

include(add_catch2_test_suite)

add_catch2_test_suite(
    TEST_NAME
        fragment_store_tests

    TEST_SOURCES
        fragmentstore_test.cpp
        ${FWUPDATELIBS_ROOT}/fragmentstore/fragmentstore.c

    TEST_INCLUDE_PATHS
        ${FWUPDATELIBS_ROOT}/fragmentstore/include
    
    TEST_LINK_LIBRARIES
        testing::flash
)
