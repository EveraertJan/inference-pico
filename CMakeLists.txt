cmake_minimum_required(VERSION 3.12)

set(MODEL_FOLDER source)
set(EI_SDK_FOLDER source/edge-impulse-sdk)

include(pico_sdk_import.cmake)


project(app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()



add_executable(app
  source/main.cpp
  source/hw_config.c
 )

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI fatfs)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/ST7735_TFT_PICO st7735)

add_library(pico_analog_microphone INTERFACE)
target_sources(pico_analog_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lib/mic/analog_microphone.cpp
)
target_include_directories(pico_analog_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lib/mic/include
)
target_link_libraries(pico_analog_microphone INTERFACE pico_stdlib hardware_adc hardware_dma FatFs_SPI st7735)


target_link_libraries(app PUBLIC pico_stdlib)
target_link_libraries(app PUBLIC pico_analog_microphone)
target_link_libraries(app PUBLIC FatFs_SPI)
target_link_libraries(app PUBLIC st7735)


pico_set_program_name(app "recorder")
pico_set_program_version(app "0.1")

# enable usb output, disable uart output
pico_enable_stdio_usb(app 1)
pico_enable_stdio_uart(app 1)


# add all sources to the project
target_sources(app PRIVATE ${SOURCE_FILES})



pico_add_extra_outputs(app)

