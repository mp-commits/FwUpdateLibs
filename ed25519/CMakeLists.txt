project(ed25519)

add_library(${PROJECT_NAME}
    STATIC 
        extra/ed25519_extra.c
        src/add_scalar.c
        src/fe.c
        src/ge.c
        src/key_exchange.c
        src/keypair.c
        src/sc.c
        src/seed.c
        src/sha512.c
        src/sign.c
        src/verify.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        src
        extra
)

add_library(libs::ed25519 ALIAS ${PROJECT_NAME})
