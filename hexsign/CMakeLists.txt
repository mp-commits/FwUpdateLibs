project(hexsign)

add_executable(${PROJECT_NAME}
    hexsign.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        argparse::argparse
        libs::ed25519
        libs::fragmentstore
        libs::hexfile
        libs::keyfile
)

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)
