cmake_minimum_required(VERSION 3.13)

# Path to Pico SDK
set(PICO_SDK_PATH "C:/pico/pico-sdk")
include("${PICO_SDK_PATH}/pico_sdk_init.cmake")

# Project
project(GM_Clock C CXX ASM)
pico_sdk_init()

# C++20
set(CMAKE_CXX_STANDARD 20)

# Add your source
add_executable(GM_Clock
        main.cpp
)

# Link Pico standard libraries
target_link_libraries(GM_Clock pico_stdlib)

# Generate UF2, ELF, BIN files
pico_add_extra_outputs(GM_Clock)

# -------------------------------------------------
# Custom folder for UF2 files only
# -------------------------------------------------
# Change this to your desired absolute path
set(UF2_OUTPUT_DIR "D:/Projects/GM Clock/uf2_files")

# Create the folder if it doesn't exist
file(MAKE_DIRECTORY ${UF2_OUTPUT_DIR})

# Move only the UF2 files to the custom folder after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.uf2
        "D:/Projects/GM Clock/uf2_files"
)
