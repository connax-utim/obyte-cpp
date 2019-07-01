add_library(stub_heap STATIC
        "heap_caps.c"
        "heap_caps_init.c"
        "multi_heap.c"
        "include/esp_heap_caps.h")

target_include_directories(stub_heap PUBLIC . include)
add_library(stub::heap ALIAS stub_heap)

target_link_libraries(stub_heap "-u app_main")
target_link_libraries(stub_heap stub::soc)
