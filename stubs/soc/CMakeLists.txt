add_library(stub_soc STATIC
        "src/memory_layout_utils.c"
        "src/lldesc.c"
        "src/hal/spi_hal.c"
        "src/hal/spi_hal_iram.c"
        "src/hal/spi_slave_hal.c"
        "src/hal/spi_slave_hal_iram.c"
        "src/soc_include_legacy_warn.c"
        "src/hal/spi_flash_hal.c"
        "src/hal/spi_flash_hal_iram.c"
        )

target_include_directories(stub_soc PUBLIC . include esp32/include)
add_library(stub::soc ALIAS stub_soc)
target_link_libraries(stub_soc stub::log)

target_link_libraries(stub_soc "-u app_main")
target_link_libraries(stub_soc stub::spi_flash)