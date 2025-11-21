project(ed25519)

add_library(${PROJECT_NAME}
    STATIC 
        extra/ed25519_extra.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/add_scalar.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/fe.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/ge.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/key_exchange.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/keypair.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/sc.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/seed.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/sha512.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/sign.c
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src/verify.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${FWUPDATELIBS_ROOT}/submodules/ed25519/src
        extra
)

add_library(libs::ed25519 ALIAS ${PROJECT_NAME})
