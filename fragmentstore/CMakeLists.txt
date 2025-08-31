project(fragmentstore)

add_library(${PROJECT_NAME}
    STATIC 
        command.c
        fragmentstore.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
)

add_library(libs::fragmentstore ALIAS ${PROJECT_NAME})
