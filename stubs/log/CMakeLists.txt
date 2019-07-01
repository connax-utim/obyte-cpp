add_library(stub_log STATIC "log.c" "include/esp_log.h")

target_include_directories(stub_log PUBLIC . include)
add_library(stub::log ALIAS stub_log)

target_link_libraries(stub_log "-u app_main")
target_link_libraries(stub_log stub::soc stub::freertos stub::newlib)
