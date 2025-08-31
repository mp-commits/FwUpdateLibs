project(imitation_flash)

add_library(${PROJECT_NAME}
    STATIC
        imitation_flash.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
)

add_library(testing::flash ALIAS ${PROJECT_NAME})

include(add_catch2_test_suite)

add_catch2_test_suite(
    TEST_NAME
        imitation_flash_tests

    TEST_SOURCES
        imitation_flash.c
        imitation_flash_test.cpp

    TEST_INCLUDE_PATHS
        include
)
