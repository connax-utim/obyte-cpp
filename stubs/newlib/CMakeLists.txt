add_library(stub_newlib STATIC
    "heap.c"
    "locks.c"
    "poll.c"
    "pthread.c"
    "random.c"
    "reent_init.c"
    "select.c"
    "syscall_table.c"
    "syscalls.c"
    "termios.c"
    "time.c"
    "utime.c"
    "include/sys/reent.h")

target_include_directories(stub_newlib PUBLIC . include)
add_library(stub::newlib ALIAS stub_newlib)

target_link_libraries(stub_newlib "-u app_main")
target_link_libraries(stub_newlib stub::soc stub::heap stub::esp32)