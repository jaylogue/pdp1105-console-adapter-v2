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
    src/main.cpp
    src/TerminalMode.cpp
    src/MenuMode.cpp
    src/DiagMode.cpp
    src/HostPort.cpp
    src/SCLPort.cpp
    src/AuxPort.cpp
    src/PaperTapeReader.cpp
    src/LoadFileMode.cpp
    src/UploadFileMode.cpp
    src/M93xxController.cpp
    src/LDADataSource.cpp
    src/SimpleDataSource.cpp
    src/BootstrapLoader.cpp
    src/AbsoluteLoader.cpp
    src/FileSet.cpp
    src/ActivityLED.cpp
    src/Utils.cpp
    src/crc32.c
    external/async_xmodem/xmodem_server.c
)

target_include_directories(pdp1105-console-adapter
    PRIVATE
        external/async_xmodem)

target_link_libraries(pdp1105-console-adapter 
    pico_stdlib
    hardware_pwm
)

# Use USB console only
pico_enable_stdio_usb(pdp1105-console-adapter ENABLED)
pico_enable_stdio_uart(pdp1105-console-adapter DISABLED)

# create map/bin/hex file etc.
pico_add_extra_outputs(pdp1105-console-adapter)
