project(updateserver_tests)

include(add_catch2_test_suite)

add_catch2_test_suite(
    TEST_NAME
        updateserver_tests

    TEST_SOURCES
        updateserver_test.cpp
        ${FWUPDATELIBS_ROOT}/updateserver/server.c

    TEST_INCLUDE_PATHS
        ${FWUPDATELIBS_ROOT}/updateserver/include
)

add_catch2_test_suite(
    TEST_NAME
        transfer_tests

    TEST_SOURCES
        transfer_test.cpp
        ${FWUPDATELIBS_ROOT}/updateserver/transfer.c

    TEST_INCLUDE_PATHS
        ${FWUPDATELIBS_ROOT}/updateserver/include
)
