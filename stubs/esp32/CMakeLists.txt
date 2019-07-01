add_library(stub_esp32 STATIC
        "brownout.c"
        "cache_err_int.c"
        "cache_sram_mmu.c"
        "clk.c"
        "cpu_start.c"
        "crosscore_int.c"
        "dport_access.c"
        "dport_panic_highint_hdl.S"
        "esp_adapter.c"
        "esp_timer_esp32.c"
        "esp_himem.c"
        "gdbstub.c"
        "hw_random.c"
        "int_wdt.c"
        "intr_alloc.c"
        "panic.c"
        "pm_esp32.c"
        "pm_trace.c"
        "reset_reason.c"
        "sleep_modes.c"
        "spiram.c"
        "spiram_psram.c"
        "system_api.c"
        "task_wdt.c")

target_include_directories(stub_esp32 PUBLIC . include)
add_library(stub::esp32 ALIAS stub_esp32)

target_link_libraries(stub_esp32 "-u app_main")
target_link_libraries(stub_esp32 stub::soc)