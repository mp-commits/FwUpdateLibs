project(crc)

add_library(${PROJECT_NAME}
    STATIC
        crc32.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
)

add_library(libs::crc ALIAS ${PROJECT_NAME})
