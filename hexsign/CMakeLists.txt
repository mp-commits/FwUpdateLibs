project(hexsign)

include(FetchContent)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG        v3.2
)

FetchContent_MakeAvailable(argparse)

add_executable(${PROJECT_NAME}
    hexfile.cpp
    hexsign.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        argparse::argparse
        libs::ed25519
        libs::fragmentstore
)

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)
