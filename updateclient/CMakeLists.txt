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
        libs::fragmentstore
        libs::updateserver
)

if (WIN32)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        wsock32 
        ws2_32
)
endif()

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)

project(rollbackclient)

add_executable(${PROJECT_NAME}
    client.cpp
    rollbackclient.cpp
    udpsocket.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        argparse::argparse
        libs::hexfile
        libs::fragmentstore
        libs::updateserver
)

if (WIN32)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        wsock32 
        ws2_32
)
endif()

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)

project(testserver)

add_executable(${PROJECT_NAME}
    udpsocket.cpp
    testserver.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        argparse::argparse
        libs::updateserver
)

if (WIN32)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        wsock32 
        ws2_32
)
endif()

target_link_options(${PROJECT_NAME}
    PRIVATE
        -static
)
