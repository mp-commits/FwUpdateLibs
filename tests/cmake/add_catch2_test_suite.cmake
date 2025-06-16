include(CMakeParseArguments)

function(add_catch2_test_suite)
    
    set(single_value_args TEST_NAME)
    set(multi_value_args
        TEST_SOURCES
        TEST_INCLUDE_PATHS
        TEST_LINK_LIBRARIES
    )

    cmake_parse_arguments(ARG "" "${single_value_args}" "${multi_value_args}" ${ARGN})

    add_executable(${ARG_TEST_NAME}
        ${ARG_TEST_SOURCES}
    )

    target_include_directories(${ARG_TEST_NAME}
        PRIVATE
            ${ARG_TEST_INCLUDE_PATHS}
    )

    target_link_libraries(${ARG_TEST_NAME}
        PRIVATE
            Catch2::Catch2WithMain
            ${ARG_TEST_LINK_LIBRARIES}
    )

    add_test(NAME ${ARG_TEST_NAME}
             COMMAND ${ARG_TEST_NAME})

endfunction(add_catch2_test_suite)
