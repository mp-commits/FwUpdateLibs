project(niram)

add_library(${PROJECT_NAME}
    STATIC
        no_init_ram.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        libs::crc
)

add_library(libs::niram ALIAS ${PROJECT_NAME})
