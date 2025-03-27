cmake_minimum_required(VERSION 3.13)

# include the SDK build script
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

# set the location of the picotool command
set(picotool_DIR $ENV{PICOTOOL_PATH})

project(pdp1105-console-adapter)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(pdp1105-console-adapter
    src/AbsoluteLoader.cpp
    src/ActivityLED.cpp
    src/AuxPort.cpp
    src/BootstrapLoader.cpp
    src/crc32.c
    src/DiagMode.cpp
    src/FileSet.cpp
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

target_include_directories(pdp1105-console-adapter
    PRIVATE
        external/async_xmodem)

target_link_libraries(pdp1105-console-adapter 
    pico_stdlib
    hardware_pwm
)

pico_set_linker_script(pdp1105-console-adapter ${CMAKE_SOURCE_DIR}/src/pdp1105-console-adapter.ld)

# Use USB console only
pico_enable_stdio_usb(pdp1105-console-adapter ENABLED)
pico_enable_stdio_uart(pdp1105-console-adapter DISABLED)

# create map/bin/hex file etc.
pico_add_extra_outputs(pdp1105-console-adapter)
