project(hexsign)

add_executable(${PROJECT_NAME}
    base64.cpp
    hexsign.cpp
    openSSH_key.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        argparse::argparse
        libs::ed25519
        libs::fragmentstore
        libs::hexfile
)

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)
