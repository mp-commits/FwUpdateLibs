project(hexfile)

add_library(${PROJECT_NAME}
    STATIC 
        ${PROJECT_NAME}.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        .
)

add_library(libs::hexfile ALIAS ${PROJECT_NAME})
