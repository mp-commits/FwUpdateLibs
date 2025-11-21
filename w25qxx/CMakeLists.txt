project(w25qxx)

add_library(${PROJECT_NAME}
    STATIC
        ${FWUPDATELIBS_ROOT}/submodules/w25qxx/src/driver_w25qxx.c
        interface_w25qxx.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
        ${FWUPDATELIBS_ROOT}/submodules/w25qxx/src
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        libs::fragmentstore
)

add_library(libs::w25qxx ALIAS ${PROJECT_NAME})
