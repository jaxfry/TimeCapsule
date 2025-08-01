cmake_minimum_required(VERSION 3.13...3.27)
include(pico_sdk_import.cmake)

project(TimeCapsule C CXX ASM)
pico_sdk_init()

add_executable(TimeCapsule
    main.c
    rv3028/rv3028.c
    fs_manager.c
)

# Add FatFS library
add_library(fatfs INTERFACE)
target_include_directories(fatfs INTERFACE ${CMAKE_CURRENT_LIST_DIR}/fatfs)


target_include_directories(TimeCapsule PRIVATE rv3028)

pico_enable_usb_device(TimeCapsule "TimeCapsule" "JaxFry")

target_link_libraries(TimeCapsule
    pico_stdlib
    hardware_flash
    hardware_i2c
    fatfs
)

pico_add_extra_outputs(TimeCapsule)
