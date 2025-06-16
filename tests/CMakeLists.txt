project(tests)

message(STATUS "Building unit tests for ${PROJECT_NAME}")

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(example)
add_subdirectory(fragmentstore)
add_subdirectory(imitation_flash)
