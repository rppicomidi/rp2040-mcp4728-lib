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
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(cli-example C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. rp2040-mcp2748-lib)

add_executable(cli-example
    cli-example.cpp
    ${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/src/embedded_cli.c
)

pico_set_program_name(cli-example "cli-example")
pico_set_program_version(cli-example "0.1")

if (DEFINED ENV{NUM_MCP4728} AND (NOT NUM_MCP4728))
    set(NUM_MCP4728 $ENV{NUM_MCP4728})
endif()
if (NUM_MCP4728)
    target_compile_definitions(cli-example PUBLIC
        RP2040_MCP4728_EXAMPLES_NUM_DACS=${NUM_MCP4728})
endif()

if (DEFINED ENV{MCP4728_I2C} AND (NOT MCP4728_I2C))
    set(MCP4728_I2C $ENV{MCP4728_I2C})
endif()
if (MCP4728_I2C)
    target_compile_definitions(cli-example PUBLIC
        RP2040_MCP4728_EXAMPLES_I2C=${MCP4728_I2C})
endif()

if (DEFINED ENV{MCP4728_BAUD} AND (NOT MCP4728_BAUD))
    set(MCP4728_BAUD $ENV{MCP4728_BAUD})
endif()
if (MCP4728_BAUD)
    target_compile_definitions(cli-example PUBLIC
    RP2040_MCP4728_EXAMPLES_BAUD=${MCP4728_BAUD})
endif()

if (DEFINED ENV{MCP4728_I2C_SDA} AND (NOT MCP4728_I2C_SDA))
    set(MCP4728_I2C_SDA $ENV{MCP4728_I2C_SDA})
endif()
if (MCP4728_I2C_SDA)
    target_compile_definitions(cli-example PUBLIC
        RP2040_MCP4728_EXAMPLES_I2C_SDA_GPIO=${MCP4728_I2C_SDA})
endif()
if (DEFINED ENV{MCP4728_I2C_SCL} AND (NOT MCP4728_I2C_SCL))
    set(MCP4728_I2C_SCL $ENV{MCP4728_I2C_SDA})
endif()
if (MCP4728_I2C_SCL)
    target_compile_definitions(cli-example PUBLIC
        RP2040_MCP4728_EXAMPLES_I2C_SCL_GPIO=${MCP4728_I2C_SCL})
endif()

math(EXPR last_idx "${NUM_MCP4728}-1")

foreach(dac RANGE 0 ${last_idx})
    math(EXPR addr "0x60 + ${dac}")
    if (MCP4728_ADDR${dac})
        set(addr ${MCP4728_ADDR${dac}})
    elseif(DEFINED ENV{MCP4728_ADDR${dac}})
        set(addr $ENV{MCP4728_ADDR${dac}})
    else()
        continue()
    endif()
    target_compile_definitions(cli-example PUBLIC
    RP2040_MCP4728_EXAMPLES_DAC_ADDR${dac}=${addr})
endforeach()

foreach(dac RANGE 0 ${last_idx})
    if (MCP4728_LDAC${dac})
        set(ldac ${MCP4728_LDAC${dac}})
    elseif(DEFINED ENV{MCP4728_LDAC${dac}})
        set(ldac $ENV{MCP4728_LDAC${dac}})
    else()
        continue()
    endif()
    target_compile_definitions(cli-example PUBLIC
    RP2040_MCP4728_EXAMPLES_LDAC${dac}=${ldac})
endforeach()

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(cli-example 1)
pico_enable_stdio_usb(cli-example 1)

target_link_libraries(cli-example
        pico_stdlib
        rp2040_mcp4728_lib
        rp2040_mcp4728_cli_lib
)

target_include_directories(cli-example PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/embedded-cli/lib/include
)

pico_add_extra_outputs(cli-example)

