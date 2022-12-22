cmake_minimum_required(VERSION 3.13.1)

set(MODEL_FOLDER source)
set(EI_SDK_FOLDER source/edge-impulse-sdk)
set(MICROPHONE_FOLDER source/microphone)

include(pico_sdk_import.cmake)


project(app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/FatFs_SPI build)



add_library(pico_analog_microphone INTERFACE)
target_sources(pico_analog_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/source/mic/analog_microphone.cpp
)
target_include_directories(pico_analog_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/source/mic/include
)
target_link_libraries(pico_analog_microphone INTERFACE pico_stdlib hardware_adc hardware_dma)




add_executable(app
  source/main.cpp 
  source/hw_config.c
 )

add_subdirectory(source)

target_link_libraries(app PUBLIC pico_stdlib)
target_link_libraries(app PUBLIC pico_analog_microphone)
target_link_libraries(app PUBLIC FatFs_SPI)



# enable usb output, disable uart output
pico_enable_stdio_usb(app 1)
pico_enable_stdio_uart(app 0)


# add all sources to the project
target_sources(app PRIVATE ${SOURCE_FILES})



pico_add_extra_outputs(app)

