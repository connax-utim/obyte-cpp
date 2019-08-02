cmake_minimum_required(VERSION 3.13)
project(obyte_cpp)

SET (BOOST_ROOT "/usr/local/boost_1_70_0")
SET (BOOST_INCLUDEDIR "/usr/local/boost_1_70_0/boost")
SET (BOOST_LIBRARYDIR "/usr/local/boost_1_70_0/libs")
FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS thread system)
FIND_PACKAGE(OpenSSL)

#if("${TARGET}" STREQUAL "esp32")
#    # Include for ESP-IDF build system functions
#    include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
#    # Create idf::esp32 and idf::freertos static libraries
#    idf_build_process(esp32
#            # try and trim the build; additional components
#            # will be included as needed based on dependency tree
#            #
#            # although esptool_py does not generate static library,
#            # processing the component is needed for flashing related
#            # targets and file generation
#            COMPONENTS esp32 freertos esptool_py xtenza soc
#            SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig
#            BUILD_DIR ${CMAKE_BINARY_DIR})
#else()
#    # Create stubs for esp32 and freertos, stub::esp32 and stub::freertos
#    add_subdirectory(stubs/esp32)
#    add_subdirectory(stubs/freertos)
#    add_subdirectory(stubs/spi_flash)
#    add_subdirectory(stubs/xtensa)
#    add_subdirectory(stubs/soc)
#    add_subdirectory(stubs/log)
#    add_subdirectory(stubs/newlib)
#    add_subdirectory(stubs/heap)
#    add_subdirectory(stubs/esp_rom)
#    add_subdirectory(stubs/esp_common)
#endif()

include_directories(src)
include_directories(src/lib)
include_directories(src/lib/micro-ecc)
include_directories(src/lib/utility)

#set(elf_file ${CMAKE_PROJECT_NAME}.elf)

add_executable(obyte_cpp
        src/lib/micro-ecc/types.h
        src/lib/micro-ecc/uECC.c
        src/lib/micro-ecc/uECC.h
        src/lib/micro-ecc/uECC_vli.h
        src/lib/utility/EndianUtil.hpp
        src/lib/utility/LimbUtil.hpp
        src/lib/utility/ProgMemUtil.hpp
        src/lib/utility/RotateUtil.hpp
        src/lib/AES.hpp
        src/lib/AES128.cpp
        src/lib/AESCommon.cpp
        src/lib/AESEsp32.cpp
        src/lib/AuthenticatedCipher.cpp
        src/lib/AuthenticatedCipher.hpp
        src/lib/Base64.cpp
        src/lib/Base64.hpp
        src/lib/BlockCipher.cpp
        src/lib/BlockCipher.hpp
        src/lib/byte_order.c
        src/lib/byte_order.h
        src/lib/Cipher.cpp
        src/lib/Cipher.hpp
        src/lib/Crypto.cpp
        src/lib/Crypto.hpp
        src/lib/GCM.cpp
        src/lib/GCM.hpp
        src/lib/GF128.cpp
        src/lib/GF128.hpp
        src/lib/GHASH.cpp
        src/lib/GHASH.hpp
        src/lib/Hash2.cpp
        src/lib/Hash2.hpp
        src/lib/json.hpp
        src/lib/ripemd-160.c
        src/lib/ripemd-160.h
        src/lib/root_certificates.hpp
        src/lib/SHA256.cpp
        src/lib/SHA256.hpp
        src/lib/websocketclient.cpp
        src/lib/websocketclient.hpp
        src/byteduino.cpp
        src/byteduino.hpp
        src/communication.cpp
        src/communication.hpp
        src/cosigning.cpp
        src/cosigning.hpp
        src/definitions.hpp
        src/hashing.cpp
        src/hashing.hpp
        src/hub_challenge.cpp
        src/hub_challenge.hpp
        src/key_rotation.cpp
        src/key_rotation.hpp
        src/messenger.cpp
        src/messenger.hpp
        src/pairing.cpp
        src/pairing.hpp
        src/payment.cpp
        src/payment.hpp
        src/random_gen.cpp
        src/random_gen.hpp
        src/signature.cpp
        src/signature.hpp
        src/structures.hpp
        src/wallet.cpp
        src/wallet.hpp
        CMakeLists.txt src/main.cpp)

#target_link_libraries(obyte_cpp Boost::system Boost::thread)

#if("${TARGET}" STREQUAL "esp32")
#    target_link_libraries(${elf_file} idf::esp32 idf::freertos idf::spi_flash Boost::system Boost::thread)
#    # Attach additional targets to the executable file for flashing,
#    # linker script generation, partition_table generation, etc.
#    idf_build_executable(${elf_file})
#else()
#    target_link_libraries(${elf_file}
#            stub::freertos
#            stub::newlib
#            stub::esp32
#            stub::spi_flash
#            stub::xtensa
#            stub::soc
#            stub::log
#            stub::heap
#            stub::esp_rom
#            stub::esp_common
#            Boost::system
#            Boost::thread)
#endif()

target_link_libraries(obyte_cpp Boost::system Boost::thread OpenSSL::SSL OpenSSL::Crypto)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)