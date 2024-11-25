cmake_minimum_required(VERSION 3.13)

add_library(rp2040_mcp4728_lib INTERFACE)
target_sources(rp2040_mcp4728_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/rp2040_mcp4728_lib.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rp2040_i2c_lib.cpp
)
target_include_directories(rp2040_mcp4728_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(rp2040_mcp4728_lib INTERFACE pico_stdlib hardware_i2c)

add_library(rp2040_mcp4728_cli_lib INTERFACE)
target_sources(rp2040_mcp4728_cli_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/rp2040_mcp4728_cli.cpp
)
target_include_directories(rp2040_mcp4728_cli_lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(rp2040_mcp4728_cli_lib INTERFACE
    rp2040_mcp4728_lib
)