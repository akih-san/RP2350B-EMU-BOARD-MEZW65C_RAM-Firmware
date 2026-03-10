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
set(PICO_BOARD weact_studio_rp2350b_core CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(mezw65c C CXX ASM)

add_compile_options(-O0)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(mezw65c mezw65c.c
		io_dos.c
        fatfs/ff.c
        iodev/iodev.c
        iodev/cpu_ctl.c
        iodev/mem_ctl.c
        iodev/spi_sd.c
        iodev/pio_dev.c
        iodev/urt_dev.c
)

pico_set_program_name(mezw65c "mezw65c")
pico_set_program_version(mezw65c "0.1")

# Generate PIO header
pico_generate_pio_header(mezw65c ${CMAKE_CURRENT_LIST_DIR}/w65_irq.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(mezw65c 0)
pico_enable_stdio_usb(mezw65c 1)

# Add the standard library to the build
target_link_libraries(mezw65c
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(mezw65c PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/fatfs/
        ${CMAKE_CURRENT_SOURCE_DIR}/iodev/include/
)

# Add any user requested libraries
target_link_libraries(mezw65c 
        hardware_spi
        hardware_pio
        hardware_pwm
        )

pico_add_extra_outputs(mezw65c)

