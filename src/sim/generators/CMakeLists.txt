INCLUDE_DIRECTORIES(
        .
        ..
)


FILE(GLOB ODBSIM_GEN_RANDOM
        gen_random.c
)

ADD_LIBRARY(ckobdsim_random STATIC ${ODBSIM_GEN_RANDOM})

