# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================


cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_TOOLCHAIN_PATH "${USERHOME}/.pico-sdk/toolchain/${toolchainVersion}")


set(CMAKE_BUILD_TYPE Debug)

set(PROJECT_NAME nixie_clock)
set(OUTPUT_NAME nixie_clock)

set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Point to the repo's submodules for the PicoSDK and FreeRTOS
set(PICO_SDK_PATH "../pico-sdk/")
set(FREERTOS_KERNEL_PATH "FreeRTOS-Kernel")
set(FREERTOS_KERNEL_PORT_RELATIVE_PATH "portable/ThirdParty/Community-Supported-Ports/GCC/RP2350_ARM_NTZ")

set(FREERTOS_PORT_PATH ${FREERTOS_KERNEL_PATH}/${FREERTOS_KERNEL_PORT_RELATIVE_PATH})
message("USING FreeRTOS PATH=${FREERTOS_KERNEL_PATH}")
message("USING FreeRTOS PORT PATH=${FREERTOS_PORT_PATH}")

# Import Pico SDK and FreeRTOS CMake configs (this is a nameless evil, do not look upon it)
include(pico_sdk_import.cmake)
include(${FREERTOS_PORT_PATH}/library.cmake)

project(${PROJECT_NAME}
        LANGUAGES C CXX ASM)

pico_sdk_init()

add_executable(${OUTPUT_NAME}
    src/main.cpp
    # src/pico_led.c
    src/apa102.cpp
    src/clock.cpp
    src/wifi.cpp
    src/network_time.cpp
    src/base64.c
    src/push_button.cpp
    src/push_buttons.cpp
    src/http_client.cpp
    src/buffer_pool.cpp
    src/time_zone.cpp
    src/url_parser.c
)

pico_generate_pio_header(${OUTPUT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/apa102.pio)

target_include_directories(${OUTPUT_NAME} PUBLIC 
    include/ 
    ${FREERTOS_KERNEL_PATH}/include 
    ${PICO_SDK_PATH}/lib/mbedtls/include
    ${PICO_SDK_PATH}/lib/lwip/src/apps/altcp_tls
)

# Rage
set_source_files_properties(
    ${PICO_SDK_PATH}/lib/lwip/src/apps/altcp_tls/altcp_tls_mbedtls.c
    PROPERTIES COMPILE_OPTIONS "-Wunused-result"
)

set_source_files_properties(
    ${PICO_SDK_PATH}/lib/lwip/src/apps/http/altcp_proxyconnect.c
    PROPERTIES COMPILE_OPTIONS "-Wno-incompatible-pointer-types"
)

# This makes printf() work over the USB serial port
pico_enable_stdio_usb(${OUTPUT_NAME} 1)


target_link_libraries(${OUTPUT_NAME}
    pico_stdlib
    pico_runtime
    pico_lwip_sntp
    hardware_pio
    hardware_dma
    hardware_adc
    pico_aon_timer
    pico_stdio_usb
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_http
    pico_lwip_mbedtls
    pico_mbedtls
    # pico_altcp_tls_mbedtls
    FreeRTOS-Kernel-Heap4)


add_compile_options(-save-temps=obj -fverbose-asm)
target_compile_options(FreeRTOS-Kernel-Heap4 INTERFACE -save-temps=obj -fverbose-asm)
target_compile_definitions(FreeRTOS-Kernel INTERFACE PICO_RP2350=1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${OUTPUT_NAME})
