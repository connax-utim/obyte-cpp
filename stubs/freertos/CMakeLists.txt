add_library(stub_freertos STATIC
    "croutine.c"
    "event_groups.c"
    "FreeRTOS-openocd.c"
    "list.c"
    "port.c"
    "portasm.S"
    "queue.c"
    "tasks.c"
    "timers.c"
    "xtensa_context.S"
    "xtensa_init.c"
    "xtensa_intr.c"
    "xtensa_intr_asm.S"
    "xtensa_overlay_os_hook.c"
    "xtensa_vector_defaults.S"
    "xtensa_vectors.S"
    "freertos/FreeRTOS.h")
target_include_directories(stub_freertos PUBLIC . freertos)
add_library(stub::freertos ALIAS stub_freertos)

target_link_libraries(stub_freertos "-u app_main")
target_link_libraries(stub_freertos stub::soc stub::newlib stub::xtensa stub::esp_rom stub::esp_common)
