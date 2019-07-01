add_library(stub_esp_rom STATIC "esp_rom.c")

target_include_directories(stub_esp_rom PUBLIC . include)
add_library(stub::esp_rom ALIAS stub_esp_rom)

target_link_libraries(stub_esp_rom "-u app_main")
#target_link_libraries(stub_esp_rom stub::soc)