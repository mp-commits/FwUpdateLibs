cmake_minimum_required(VERSION 3.14)
project(FwUpdateLibs)

set(FWUPDATELIBS_ROOT ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(ed25519)
add_subdirectory(fragmentstore)
add_subdirectory(updateserver)

if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")

include(FetchContent)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG        v3.2
)

FetchContent_MakeAvailable(argparse)

add_subdirectory(hexfile)
add_subdirectory(hexsign)
add_subdirectory(updateclient)
set(BUILD_TESTS 1)
endif()

if (BUILD_TESTS)
    message(STATUS "Building unit tests for ${PROJECT_NAME}")
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "NOT building unit tests for ${PROJECT_NAME}")
endif()
