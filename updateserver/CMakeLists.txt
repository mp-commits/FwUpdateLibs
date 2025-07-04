project(updateserver)

add_library(${PROJECT_NAME}
    STATIC 
        server.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
)

add_library(libs::updateserver ALIAS ${PROJECT_NAME})
