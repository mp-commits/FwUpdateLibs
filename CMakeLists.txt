cmake_minimum_required(VERSION 3.14)
project(FwUpdateLibs)

set(FWUPDATELIBS_ROOT ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(ed25519)
add_subdirectory(fragmentstore)
add_subdirectory(updateserver)

set(BUILD_TESTS 1)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
