project(keyfile)

add_library(${PROJECT_NAME}
    STATIC
        base64.cpp
        openSSH_key.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

add_library(libs::keyfile ALIAS ${PROJECT_NAME})
