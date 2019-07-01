add_library(stub_xtensa STATIC "debug_helpers.c"
        "debug_helpers_asm.S"
        "eri.c"
        "trax.c"
        "include/xtensa/hal.h")
target_include_directories(stub_xtensa PUBLIC . include esp32/include)
add_library(stub::xtensa ALIAS stub_xtensa)

target_link_libraries(stub_xtensa "-u app_main")
target_link_libraries(stub_xtensa stub::soc)