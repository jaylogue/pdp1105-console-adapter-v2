cmake_minimum_required(VERSION 3.13)

# include the SDK build script
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

# set the location of the picotool command
set(picotool_DIR $ENV{PICOTOOL_PATH})

# Limit firmware size to half of overall flash size; 
# remainder will be used for internal file storage.
set(PICO_FLASH_SIZE_BYTES "1 * 1024 * 1024")

project(pdp1105-console-adapter)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(pdp1105-console-adapter
    main.cpp
    TerminalMode.cpp
    MenuMode.cpp
    LoadFileMode.cpp
    UploadFileMode.cpp
)

target_link_libraries(pdp1105-console-adapter 
    pico_stdlib
    hardware_pwm
)

# Use USB console only
pico_enable_stdio_usb(pdp1105-console-adapter ENABLED)
pico_enable_stdio_uart(pdp1105-console-adapter DISABLED)

# create map/bin/hex file etc.
pico_add_extra_outputs(pdp1105-console-adapter)
