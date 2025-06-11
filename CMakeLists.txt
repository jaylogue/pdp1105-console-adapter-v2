cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK build script
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

# Set the location of the picotool command
set(picotool_DIR $ENV{PICOTOOL_PATH})

# Declare console adapter project
project(pdp1105-console-adapter)

# Initialize the Pico SDK
pico_sdk_init()

# Build console adapter executable
add_executable(pdp1105-console-adapter
    src/AbsoluteLoader.cpp
    src/ActivityLED.cpp
    src/AuxPort.cpp
    src/BootstrapLoader.cpp
    src/crc32.c
    src/DiagMode.cpp
    src/FileLib.cpp
    src/HostPort.cpp
    src/LDADataSource.cpp
    src/LoadFileMode.cpp
    src/M93xxController.cpp
    src/main.cpp
    src/Menu.cpp
    src/MenuMode.cpp
    src/PaperTapeReader.cpp
    src/PTRProgressBar.cpp
    src/SCLPort.cpp
    src/Settings.cpp
    src/SimpleDataSource.cpp
    src/TerminalMode.cpp
    src/UploadFileMode.cpp
    src/Utils.cpp
    external/async_xmodem/xmodem_server.c
)

# Additional compilation options
target_compile_options(pdp1105-console-adapter
    PRIVATE
        -Wall
        -Wextra
        -Wimplicit-fallthrough
        -Wsign-conversion
        -Wno-missing-field-initializers
        -fno-strict-aliasing
        -fno-strict-overflow
        -fno-delete-null-pointer-checks)

# Additional include directories for building executable
target_include_directories(pdp1105-console-adapter
    PRIVATE
        external/async_xmodem)

# Additional Pico SDK libraries for linking executable
target_link_libraries(pdp1105-console-adapter 
    pico_stdlib
    hardware_pwm)

# Use custom application-specific linker script
pico_set_linker_script(pdp1105-console-adapter
    ${CMAKE_SOURCE_DIR}/src/pdp1105-console-adapter.ld)

# Add #define for target hardware revision if passed
# on the command line
if(DEFINED HW_REV)
    target_compile_definitions(pdp1105-console-adapter
        PRIVATE
            HW_REV=${HW_REV})
endif()

# Add #defines containing git commit id and branch
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
target_compile_definitions(pdp1105-console-adapter
    PRIVATE
        GIT_COMMIT_ID="${GIT_COMMIT_ID}"
        GIT_BRANCH="${GIT_BRANCH}")

# Use USB console only
pico_enable_stdio_usb(pdp1105-console-adapter ENABLED)
pico_enable_stdio_uart(pdp1105-console-adapter DISABLED)

# Create map/bin/hex file etc.
pico_add_extra_outputs(pdp1105-console-adapter)
