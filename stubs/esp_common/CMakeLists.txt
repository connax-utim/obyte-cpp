add_library(stub_esp_common STATIC
        "src/dbg_stubs.c"
        "src/esp_err_to_name.c"
        "src/esp_timer.c"
        "src/ets_timer_legacy.c"
        "src/freertos_hooks.c"
        "src/ipc.c"
        "src/pm_locks.c"
        "src/stack_check.c")
target_include_directories(stub_esp_common PUBLIC . include)
add_library(stub::esp_common ALIAS stub_esp_common)

target_link_libraries(stub_esp_common "-u app_main")
target_link_libraries(stub_esp_common stub::soc stub::esp32)
