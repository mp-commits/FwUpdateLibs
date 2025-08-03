project(updateclient)

add_executable(${PROJECT_NAME}
    client.cpp
    updateclient.cpp
    udpsocket.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        argparse::argparse
        libs::hexfile
)

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)

project(testserver)

add_executable(${PROJECT_NAME}
    testserver.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        argparse::argparse
)

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)
