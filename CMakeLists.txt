cmake_minimum_required(VERSION 3.13)

# Path to your local Pico SDK
set(PICO_SDK_PATH "C:/pico/pico-sdk")

# Project setup
set(PICO_PLATFORM rp2350-arm-s)
set(PICO_BOARD pico2_w)

include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")


project(GM_Clock C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# C / C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Add main program
add_executable(GM_Clock
        main.cpp
        lwipopts.h
        wifi_setup.cpp
        wifi_setup.h
        time_grab.cpp
        time_grab.h
        LED_control.cpp
        LED_control.h
)

# Link libraries
target_link_libraries(GM_Clock
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        pico_util
        hardware_powman
        hardware_spi
)

target_include_directories(GM_Clock PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}  # this lets it find lwipopts.h
        ${PICO_SDK_PATH}/src/common/pico_util/include
)

# Enable USB serial output, disable UART
pico_enable_stdio_usb(GM_Clock 1)
pico_enable_stdio_uart(GM_Clock 0)

# Generate UF2, BIN, and ELF
pico_add_extra_outputs(GM_Clock)


# -------------------------------------------------
# Optional: custom UF2 output directory + timestamp
# -------------------------------------------------
set(UF2_OUTPUT_DIR "D:/Projects/GM Clock/uf2_files")
file(MAKE_DIRECTORY ${UF2_OUTPUT_DIR})
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d_%H%M")
set(UF2_FILENAME "GM_Clock_${BUILD_TIMESTAMP}.uf2")

add_custom_command(
        TARGET GM_Clock POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/GM_Clock.uf2"
        "${UF2_OUTPUT_DIR}/${UF2_FILENAME}"
        COMMENT "Copying UF2 file to ${UF2_OUTPUT_DIR}/${UF2_FILENAME}"
)
