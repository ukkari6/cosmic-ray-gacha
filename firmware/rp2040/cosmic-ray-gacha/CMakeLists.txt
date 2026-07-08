# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(cosmic-ray-gacha C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(cosmic-ray-gacha cosmic-ray-gacha.c )

pico_set_program_name(cosmic-ray-gacha "cosmic-ray-gacha")
pico_set_program_version(cosmic-ray-gacha "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(cosmic-ray-gacha 1)
pico_enable_stdio_usb(cosmic-ray-gacha 0)

# Add the standard library to the build
target_link_libraries(cosmic-ray-gacha
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(cosmic-ray-gacha PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(cosmic-ray-gacha)

#最適化レベル0（-O0）Debugビルドにしてデータを省略させないようにする
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
target_compile_options(cosmic-ray-gacha PRIVATE -O0)