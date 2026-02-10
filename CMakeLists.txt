cmake_minimum_required(VERSION 3.13)

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

include(pico_sdk_import.cmake)

project(health-heart C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(pico-tflmicro)

# ADICIONADO: lib/ssd1306.c na lista de compilação
add_executable(health-heart 
    health-heart.c 
    tinyml.cpp
    lib/ssd1306.c 
)

pico_set_program_name(health-heart "health-heart")
pico_set_program_version(health-heart "0.1")

pico_enable_stdio_uart(health-heart 0)
pico_enable_stdio_usb(health-heart 1)

# ADICIONADO: hardware_i2c
target_link_libraries(health-heart 
    pico_stdlib
    hardware_adc
    hardware_i2c
    pico-tflmicro
)

# ADICIONADO: Incluir a pasta 'lib' para achar os .h
target_include_directories(health-heart PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/lib 
    ${CMAKE_CURRENT_LIST_DIR}/pico-tflmicro/src
)

pico_add_extra_outputs(health-heart)